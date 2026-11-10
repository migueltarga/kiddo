#!/usr/bin/env python3
"""Estimate word starts in existing narration WAVs without using TTS credits.

The sidecar format is {"method":"estimated", "duration_ms":N,
"starts_ms":[...]}. Each start corresponds to one whitespace-separated word
in the page's visible text, after image tags are removed. A future aligner can
replace these values with exact starts without changing the device protocol.
"""

import json
import math
from pathlib import Path
import re
import wave


ROOT = Path(__file__).resolve().parents[1] / 'stories'
IMAGE_TAG = re.compile(r'\[img\].*?\[/img\]', re.DOTALL)


def page_words(text):
    return re.sub(r'\s+', ' ', IMAGE_TAG.sub('', text)).strip().split()


def estimate_starts(wav_path, words):
    with wave.open(str(wav_path), 'rb') as audio:
        if audio.getnchannels() != 1 or audio.getsampwidth() != 1:
            raise ValueError(f'{wav_path}: expected 8-bit mono PCM')
        rate = audio.getframerate()
        data = audio.readframes(audio.getnframes())
    duration_ms = round(len(data) * 1000 / rate)
    frame_size = max(1, rate // 100)  # 10 ms
    energy = []
    for offset in range(0, len(data), frame_size):
        frame = data[offset:offset + frame_size]
        energy.append(math.sqrt(sum((sample - 128) ** 2 for sample in frame) / len(frame)))
    threshold = max(3.0, max(energy, default=0) * 0.08)
    active = [value >= threshold for value in energy]
    # Join syllables separated by less than 70 ms; longer pauses stay visible.
    last_active = None
    for i, is_active in enumerate(active):
        if not is_active:
            continue
        if last_active is not None and i - last_active <= 7:
            for gap in range(last_active + 1, i):
                active[gap] = True
        last_active = i
    voiced_frames = [i for i, value in enumerate(active) if value]
    if not voiced_frames:
        voiced_frames = list(range(len(energy)))

    weights = [max(1.0, sum(ch.isalpha() for ch in word) ** 0.7) for word in words]
    total = sum(weights)
    cumulative = 0.0
    starts = []
    for weight in weights:
        index = min(len(voiced_frames) - 1, int(cumulative / total * len(voiced_frames)))
        starts.append(round(voiced_frames[index] * frame_size * 1000 / rate))
        cumulative += weight
    return {'method': 'estimated', 'duration_ms': duration_ms, 'starts_ms': starts}


def create_timing(wav_path, text, timing_path):
    words = page_words(text)
    timing = estimate_starts(wav_path, words)
    timing_path.write_text(json.dumps(timing, separators=(',', ':')) + '\n', encoding='utf-8')
    return len(words)


def main():
    count = 0
    for path in sorted(ROOT.glob('*.json')):
        if path.name == 'index.json':
            continue
        story = json.loads(path.read_text(encoding='utf-8'))
        changed = False
        for node in story.get('nodes', {}).values():
            source = node.get('narration', '')
            if not source.startswith('audio/') or not source.endswith('.wav'):
                continue
            wav_path = ROOT / source
            if not wav_path.is_file():
                raise FileNotFoundError(wav_path)
            timing_source = source[:-4] + '.words.json'
            create_timing(wav_path, node.get('text', ''), ROOT / timing_source)
            node['narration_timing'] = timing_source
            count += 1
            changed = True
        if changed:
            path.write_text(json.dumps(story, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'Wrote timing files for {count} narrated pages')


if __name__ == '__main__':
    main()
