#!/usr/bin/env python3
"""
Validate the story catalog and story files.

Checks that every catalog entry points to a story file the device will
accept, and that each story's node graph is consistent: the start node and
every choice / inventory target exist, items are defined, inventory-choice
nodes have both outcomes, scenes, figures, props and sounds are ones the
device knows, and every node is reachable. See docs/STORY_FORMAT.md for the format.

Usage:
    python3 scripts/validate_stories.py [stories_dir]

Exits with status 1 if any problem is found.
"""

import json
import os
import re
import sys
import wave
from urllib.parse import urlparse

# Mirrors remote_catalog::isValidFileName() on the device
MAX_FILE_NAME_LEN = 30

# Mirror kScenes, kFigures and kProps in src/ui/components/story_scene.cpp (plus the
# "bedroom" and "home" aliases) and sound_from_name() in src/audio.cpp
SCENES = {'bedroom', 'bedroom_night', 'bedroom_day', 'home', 'living_room', 'kitchen', 'pool', 'classroom',
          'library', 'playground', 'school', 'road', 'garden', 'forest', 'cave', 'lake', 'mountain', 'sky', 'beach'}
FIGURES = {'zoe', 'ted', 'girl', 'girl2', 'boy', 'mom', 'dad', 'grandpa', 'grandma', 'teacher',
           'dog', 'cat', 'frog', 'owl', 'bear', 'squirrel', 'sheep'}
PROPS = {'float', 'ball', 'sandcastle', 'book', 'blocks', 'pancakes', 'cake', 'food', 'paints', 'chest', 'door'}
SOUNDS = {'click', 'boing', 'chime', 'success', 'oops', 'page', 'fanfare'}
LED_COLORS = {'red', 'green', 'blue', 'yellow', 'purple', 'cyan', 'white'}
LED_EFFECTS = {'off', 'rainbow'} | LED_COLORS | {
    f'{pattern}:{color}' for pattern in ('pulse', 'blink') for color in LED_COLORS
}


def check_file_name(name):
    if not name.endswith('.json') or name == 'index.json':
        return 'must be a .json story file other than index.json'
    if '/' in name or '\\' in name or name.startswith('.'):
        return 'must be a plain file name'
    if len(name) > MAX_FILE_NAME_LEN:
        return f'longer than {MAX_FILE_NAME_LEN} characters (SPIFFS limit)'
    return None


def check_story(story, root=None):
    issues = []
    nodes = story.get('nodes', {})
    items = {i.get('id') for i in story.get('item_definitions', [])}

    for field in ('id', 'start'):
        if not story.get(field):
            issues.append(f'missing "{field}"')
    if story.get('start') and story['start'] not in nodes:
        issues.append(f'start node "{story["start"]}" does not exist')

    def check_item(where, item):
        if item and story.get('has_inventory') and item not in items:
            issues.append(f'{where}: item "{item}" is not in item_definitions')

    def check_names(where, field, names, known):
        if not isinstance(names, list):
            issues.append(f'{where}: "{field}" must be a list')
            return
        for name in names:
            if name not in known:
                issues.append(f'{where}: unknown {field[:-1] if field != "cast" else "figure"} "{name}" '
                              f'(one of {", ".join(sorted(known))})')

    for item in story.get('initial_inventory', []):
        check_item('initial_inventory', item.get('id'))
    if 'cast' in story:
        check_names('story', 'cast', story['cast'], FIGURES)
    for name, figure in story.get('characters', {}).items():
        if figure not in FIGURES:
            issues.append(f'characters: "{name}" is an unknown figure "{figure}" (one of {", ".join(sorted(FIGURES))})')

    for key, node in nodes.items():
        check_item(key, node.get('gives_item'))
        if node.get('scene') and node['scene'] not in SCENES:
            issues.append(f'{key}: unknown scene "{node["scene"]}" (one of {", ".join(sorted(SCENES))})')
        if 'cast' in node:
            check_names(key, 'cast', node['cast'], FIGURES)
        if 'props' in node:
            check_names(key, 'props', node['props'], PROPS)
        if node.get('sound') and node['sound'] not in SOUNDS:
            issues.append(f'{key}: unknown sound "{node["sound"]}" (one of {", ".join(sorted(SOUNDS))})')
        narration = node.get('narration')
        if narration is not None:
            if not isinstance(narration, str) or not narration or len(narration) > 240:
                issues.append(f'{key}: narration must be a path or URL of 1 to 240 characters')
            elif narration.startswith('/'):
                if '..' in narration or not narration.lower().endswith('.wav'):
                    issues.append(f'{key}: local narration must be an absolute .wav path without ".."')
            else:
                parsed = urlparse(narration)
                if parsed.scheme in ('http', 'https') and parsed.netloc and parsed.path.lower().endswith('.wav'):
                    pass
                elif parsed.scheme or narration.startswith('.') or '\\' in narration or '..' in narration or not narration.lower().endswith('.wav'):
                    issues.append(f'{key}: narration must be a safe relative .wav path, HTTP(S) URL, or local path')
                elif root and not os.path.isfile(os.path.join(root, narration)):
                    issues.append(f'{key}: narration file "{narration}" does not exist')
        timing = node.get('narration_timing')
        if timing is not None:
            if not narration:
                issues.append(f'{key}: narration_timing needs narration')
            elif not isinstance(timing, str) or not timing.endswith('.words.json') or timing.startswith(('/', '.')) or '..' in timing or '\\' in timing:
                issues.append(f'{key}: narration_timing must be a safe relative .words.json path')
            elif root:
                timing_path = os.path.join(root, timing)
                if not os.path.isfile(timing_path):
                    issues.append(f'{key}: timing file "{timing}" does not exist')
                else:
                    data = json.load(open(timing_path, encoding='utf-8'))
                    starts = data.get('starts_ms')
                    visible = re.sub(r'\[img\].*?\[/img\]', '', node.get('text', ''), flags=re.DOTALL)
                    words = visible.split()
                    if (not isinstance(starts, list) or len(starts) != len(words) or
                            any(not isinstance(x, int) or x < 0 for x in starts) or
                            starts != sorted(starts)):
                        issues.append(f'{key}: timing file needs one ordered start per visible word')
                    elif narration.startswith('audio/'):
                        with wave.open(os.path.join(root, narration), 'rb') as wav:
                            duration_ms = wav.getnframes() * 1000 // wav.getframerate()
                        if starts and starts[-1] > duration_ms:
                            issues.append(f'{key}: timing exceeds recording duration')
        if 'led' in node and node['led'] not in LED_EFFECTS:
            issues.append(f'{key}: unknown led effect "{node["led"]}"')
        for choice in node.get('choices', []):
            if choice.get('next') not in nodes:
                issues.append(f'{key}: choice leads to missing node "{choice.get("next")}"')
            check_item(key, choice.get('required_item'))
            check_item(key, choice.get('gives_item'))
        if node.get('inventory_choice'):
            check_item(key, node.get('correct_item'))
            for field in ('success_next', 'failure_next'):
                if node.get(field) not in nodes:
                    issues.append(f'{key}: {field} leads to missing node "{node.get(field)}"')
        elif not node.get('choices') and not node.get('end'):
            issues.append(f'{key}: has no choices but is not marked "end"')

    reachable = set()
    pending = [story.get('start')]
    while pending:
        key = pending.pop()
        if key not in nodes or key in reachable:
            continue
        reachable.add(key)
        node = nodes[key]
        pending += [c.get('next') for c in node.get('choices', [])]
        pending += [node.get('success_next'), node.get('failure_next')]
    for key in sorted(set(nodes) - reachable):
        issues.append(f'{key}: unreachable from start')

    return issues


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else 'stories'
    failed = False

    with open(os.path.join(root, 'index.json')) as f:
        catalog = json.load(f)

    for entry in catalog.get('stories', []):
        name = entry.get('file', '')
        problem = check_file_name(name)
        path = os.path.join(root, name)
        if problem:
            print(f'index.json: "{name}" {problem}')
            failed = True
            continue
        if not os.path.exists(path):
            print(f'index.json: "{name}" does not exist')
            failed = True
            continue
        if not entry.get('lang'):
            print(f'index.json: "{name}" has no lang')
            failed = True

        with open(path) as f:
            issues = check_story(json.load(f), root)
        for issue in issues:
            print(f'{name}: {issue}')
        failed |= bool(issues)
        if not issues:
            print(f'{name}: ok')

    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
