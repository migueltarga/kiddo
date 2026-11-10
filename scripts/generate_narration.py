#!/usr/bin/env python3
"""Generate page recordings for Portuguese stories using ElevenLabs.

The API key is read from a hidden prompt (or ELEVENLABS_API_KEY), never saved.
Run with --dry-run to inspect the pages and character count without calling the API.
"""

import argparse
import getpass
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from generate_word_timings import create_timing


ROOT = Path(__file__).resolve().parents[1]
STORIES = ROOT / 'stories'
IMAGE_TAG = re.compile(r'\[img\].*?\[/img\]', re.DOTALL)
SAFE_NAME = re.compile(r'^[a-zA-Z0-9_-]+$')


def spoken_text(raw):
    return re.sub(r'\s+', ' ', IMAGE_TAG.sub('', raw)).strip()


def generate_audio(key, voice_id, text, mp3_path):
    payload = {
        'text': text,
        'model_id': 'eleven_v4',
        'language_code': 'pt',
    }
    request = urllib.request.Request(
        f'https://api.elevenlabs.io/v1/text-to-speech/{voice_id}?output_format=mp3_44100_128',
        data=json.dumps(payload, ensure_ascii=False).encode('utf-8'),
        headers={
            'xi-api-key': key,
            'Content-Type': 'application/json',
            'Accept': 'audio/mpeg',
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            mp3_path.write_bytes(response.read())
    except urllib.error.HTTPError as error:
        detail = error.read().decode('utf-8', 'replace')[:500]
        raise RuntimeError(f'ElevenLabs returned {error.code}: {detail}') from error


def convert_for_device(mp3_path, wav_path):
    with tempfile.NamedTemporaryFile(suffix='.wav', dir=wav_path.parent, delete=False) as temp:
        temp_path = Path(temp.name)
    try:
        subprocess.run([
            'ffmpeg', '-hide_banner', '-loglevel', 'error', '-y',
            '-i', str(mp3_path), '-ac', '1', '-ar', '16000',
            '-c:a', 'pcm_u8', str(temp_path),
        ], check=True)
        temp_path.replace(wav_path)
    finally:
        temp_path.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--voice-id', help='ElevenLabs voice ID (required to generate)')
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('stories', nargs='*', help='Portuguese story JSON files; defaults to all')
    args = parser.parse_args()
    if not args.dry_run and not args.voice_id:
        parser.error('--voice-id is required unless --dry-run is used')

    paths = [Path(name) for name in args.stories] if args.stories else sorted(STORIES.glob('*.json'))
    jobs = []
    for path in paths:
        story = json.loads(path.read_text(encoding='utf-8'))
        if story.get('lang') not in ('pt', 'pt-br'):
            # Catalog language is authoritative for stories without a lang field.
            catalog = json.loads((STORIES / 'index.json').read_text(encoding='utf-8'))
            if not any(entry.get('file') == path.name and entry.get('lang') == 'pt-br'
                       for entry in catalog.get('stories', [])):
                continue
        for node_key, node in story.get('nodes', {}).items():
            if not SAFE_NAME.fullmatch(node_key):
                raise ValueError(f'Unsafe node key: {node_key}')
            text = spoken_text(node.get('text', ''))
            if not text:
                continue
            relative = Path('audio') / path.stem / f'{node_key}.wav'
            jobs.append((path, story, node_key, text, relative))

    print(f'{len(jobs)} pages, {sum(len(job[3]) for job in jobs)} characters')
    if args.dry_run:
        return
    key = os.environ.get('ELEVENLABS_API_KEY') or getpass.getpass('ElevenLabs key: ')
    for number, (path, story, node_key, text, relative) in enumerate(jobs, 1):
        wav_path = STORIES / relative
        wav_path.parent.mkdir(parents=True, exist_ok=True)
        if not wav_path.exists():
            with tempfile.TemporaryDirectory() as temp_dir:
                mp3_path = Path(temp_dir) / 'voice.mp3'
                generate_audio(key, args.voice_id, text, mp3_path)
                convert_for_device(mp3_path, wav_path)
        story['nodes'][node_key]['narration'] = relative.as_posix()
        timing_relative = relative.with_suffix('.words.json')
        create_timing(wav_path, story['nodes'][node_key].get('text', ''), STORIES / timing_relative)
        story['nodes'][node_key]['narration_timing'] = timing_relative.as_posix()
        path.write_text(json.dumps(story, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(f'[{number}/{len(jobs)}] {path.name}: {node_key} ({wav_path.stat().st_size} bytes)', flush=True)


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
