#!/usr/bin/env python3
"""Draw the story scenes, figures, props and small moving bits as LVGL images.

Each scene is a 220x100 RGB565 banner shown at the top of a story page that
has no picture of its own. Figures (Zoe, Ted, family, friends, animals),
props (a float, a sandcastle, a cake...) and the small moving bits (hearts,
notes, fish...) are ARGB8888 so they can stand on any scene. The app picks
and places them, see src/ui/components/story_scene.cpp, which expects the
ground near y=92 on the right half, where the figures stand.

Needs Pillow and macOS's SF Rounded font (system python3 has both):

    python3 scripts/gen_scenes.py            # writes src/ui/images/{scene,figure,prop,fx}_*.c
    python3 scripts/gen_scenes.py --preview  # also writes scenes.png with all of them
"""

import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 220, 100
SS = 4  # supersampling
FONT = "/System/Library/Fonts/SFNSRounded.ttf"  # chalkboard writing


def s(v):
    """Scale a final-pixel measure to the supersampled canvas."""
    return round(v * SS)


def S(*pts):
    return [(s(x), s(y)) for x, y in pts]


def box(x1, y1, x2, y2):
    return [s(x1), s(y1), s(x2), s(y2)]


def rgb(h):
    return ((h >> 16) & 255, (h >> 8) & 255, h & 255)


def lerp(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def gradient(img, top, bottom, y1=0, y2=H):
    draw = ImageDraw.Draw(img)
    for y in range(s(y1), s(y2)):
        t = (y - s(y1)) / max(1, s(y2) - s(y1) - 1)
        draw.line([(0, y), (img.width, y)], fill=lerp(rgb(top), rgb(bottom), t))


def glow(img, cx, cy, radius, color, strength):
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(layer).ellipse(box(cx - radius, cy - radius, cx + radius, cy + radius), fill=rgb(color) + (strength,))
    img.alpha_composite(layer.filter(ImageFilter.GaussianBlur(s(radius) / 2)))


def wave(img, base, amp, freq, phase, color, x1=0, x2=W):
    """Fill below a gentle sine line."""
    pts = [(s(x1), s(H))]
    for x in range(x1, x2 + 1, 2):
        pts.append((s(x), s(base - amp * math.sin(x / W * math.pi * freq + phase))))
    pts.append((s(x2), s(H)))
    ImageDraw.Draw(img).polygon(pts, fill=rgb(color))


def sparkle(draw, x, y, size, color):
    k = size * 0.28
    pts = [(x, y - size), (x + k, y - k), (x + size, y), (x + k, y + k),
           (x, y + size), (x - k, y + k), (x - size, y), (x - k, y - k)]
    draw.polygon(S(*pts), fill=color)


def dots(img, n, area, color, seed, sizes=(0.5, 0.7, 0.9)):
    rng = random.Random(seed)
    draw = ImageDraw.Draw(img)
    x1, y1, x2, y2 = area
    for _ in range(n):
        x, y, r = rng.uniform(x1, x2), rng.uniform(y1, y2), rng.choice(sizes)
        draw.ellipse(box(x - r, y - r, x + r, y + r), fill=rgb(color) + (rng.randint(150, 255),))


def crescent(img, cx, cy, r, color):
    mask = Image.new("L", img.size, 0)
    d = ImageDraw.Draw(mask)
    d.ellipse(box(cx - r, cy - r, cx + r, cy + r), fill=255)
    d.ellipse(box(cx - r * 0.45, cy - r * 1.3, cx + r * 1.6, cy + r * 0.6), fill=0)
    img.paste(Image.new("RGBA", img.size, rgb(color) + (255,)), (0, 0), mask)


def cloud(draw, x, y, w, color):
    h = w * 0.42
    draw.ellipse(box(x, y + h * 0.35, x + w, y + h), fill=color)
    draw.ellipse(box(x + w * 0.18, y + h * 0.05, x + w * 0.55, y + h * 0.85), fill=color)
    draw.ellipse(box(x + w * 0.42, y - h * 0.15, x + w * 0.85, y + h * 0.8), fill=color)


def sun(img, cx, cy, r):
    glow(img, cx, cy, r * 2.4, 0xFFE9A0, 140)
    ImageDraw.Draw(img).ellipse(box(cx - r, cy - r, cx + r, cy + r), fill=rgb(0xFFD45C))


def round_tree(draw, x, ground, h, trunk, leaves):
    draw.rectangle(box(x - 1.2, ground - h * 0.5, x + 1.2, ground), fill=rgb(trunk))
    r = h * 0.33
    draw.ellipse(box(x - r, ground - h, x + r, ground - h + 2 * r), fill=rgb(leaves))


def pine(draw, x, ground, h, color):
    w = h * 0.42
    for i in range(3):
        top = ground - h + i * h * 0.24
        draw.polygon(S((x, top), (x + w * (0.55 + i * 0.22), top + h * 0.42), (x - w * (0.55 + i * 0.22), top + h * 0.42)), fill=rgb(color))
    draw.rectangle(box(x - 1, ground - h * 0.1, x + 1, ground), fill=rgb(color))


def flower(draw, x, y, color):
    draw.line(S((x, y), (x, y + 8)), fill=rgb(0x3E8E3A), width=s(1))
    for a in range(0, 360, 72):
        px, py = x + 2.2 * math.cos(math.radians(a)), y + 2.2 * math.sin(math.radians(a))
        draw.ellipse(box(px - 1.6, py - 1.6, px + 1.6, py + 1.6), fill=rgb(color))
    draw.ellipse(box(x - 1.2, y - 1.2, x + 1.2, y + 1.2), fill=rgb(0xFFD45C))


def door(draw, x1, y1, x2, y2, color, glass=True):
    """Front door with panels, a little window and a golden knob."""
    dark = lerp(rgb(color), (0, 0, 0), 0.2)
    draw.rounded_rectangle(box(x1 - 2.5, y1 - 2.5, x2 + 2.5, y2), radius=s(2), fill=rgb(0xF4E6CC))
    draw.rectangle(box(x1, y1, x2, y2), fill=rgb(color))
    mid = (y1 + y2) / 2
    top = y1 + 4
    if glass:
        draw.rounded_rectangle(box(x1 + 4, y1 + 4, x2 - 4, y1 + 13), radius=s(3), fill=rgb(0xCDEEFF))
        top = y1 + 16
    draw.rounded_rectangle(box(x1 + 4, top, x2 - 4, mid + 2), radius=s(1.5), outline=dark, width=s(1))
    draw.rounded_rectangle(box(x1 + 4, mid + 6, x2 - 4, y2 - 4), radius=s(1.5), outline=dark, width=s(1))
    draw.ellipse(box(x2 - 7, mid + 2, x2 - 4, mid + 5), fill=rgb(0xFFD45C))


def books(draw, x1, x2, shelf, rng, tallest=16):
    """A row of book spines standing on a shelf."""
    colors = [0xE5534B, 0xF28C28, 0xFFD45C, 0x4C9A6A, 0x3F86D6, 0x8E6BD8, 0xE86FB0, 0x3FA7A0]
    x = x1
    while x < x2 - 3:
        w = rng.uniform(3, 5.5)
        h = rng.uniform(tallest * 0.65, tallest)
        if rng.random() < 0.12:  # a gap now and then
            x += w
            continue
        color = rgb(rng.choice(colors))
        draw.rectangle(box(x, shelf - h, min(x + w, x2), shelf), fill=color)
        draw.rectangle(box(x, shelf - h + 2, min(x + w, x2), shelf - h + 3), fill=lerp(color, (255, 255, 255), 0.45))
        x += w + 0.4


def checker(img, x1, y1, x2, y2, size, a, b):
    draw = ImageDraw.Draw(img)
    for row, y in enumerate(range(y1, y2, size)):
        for col, x in enumerate(range(x1, x2, size)):
            draw.rectangle(box(x, y, x + size, y + size), fill=rgb(a if (row + col) % 2 == 0 else b))


def bunting(draw, x1, x2, y, colors):
    draw.line(S((x1, y), ((x1 + x2) / 2, y + 4), (x2, y)), fill=rgb(0xB08A5E), width=s(0.8))
    n = int((x2 - x1) / 11)
    for i in range(n):
        x = x1 + 4 + i * 11
        sag = y + 4 * math.sin(math.pi * (x - x1) / (x2 - x1))
        draw.polygon(S((x, sag), (x + 8, sag), (x + 4, sag + 8)), fill=rgb(colors[i % len(colors)]))


def chalk_text(img, text, x, y, size, color):
    font = ImageFont.truetype(FONT, s(size))
    try:
        font.set_variation_by_name("Bold")
    except (OSError, ValueError):
        pass
    ImageDraw.Draw(img).text((s(x), s(y)), text, font=font, fill=rgb(color))


# ---------------- Scenes ----------------

def window(img, x1, y1, x2, y2, night):
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle(box(x1 - 3, y1 - 3, x2 + 3, y2 + 3), radius=s(3), fill=rgb(0xF4E6CC))
    sky_top, sky_bottom = (0x1D2150, 0x3B3577) if night else (0x7FCBF0, 0xCDEEFF)
    pane = Image.new("RGBA", (s(x2 - x1), s(y2 - y1)))
    for y in range(pane.height):
        ImageDraw.Draw(pane).line([(0, y), (pane.width, y)], fill=lerp(rgb(sky_top), rgb(sky_bottom), y / pane.height))
    img.alpha_composite(pane, (s(x1), s(y1)))
    if night:
        crescent(img, x1 + (x2 - x1) * 0.68, y1 + 12, 6, 0xFFF3D6)
        for px, py in [(x1 + 8, y1 + 8), (x1 + 16, y1 + 22), (x1 + 30, y1 + 30)]:
            sparkle(draw, px, py, 1.8, rgb(0xFFF3D6))
    else:
        cloud(draw, x1 + 6, y1 + 10, 22, rgb(0xFFFFFF))
    cx, cy = (x1 + x2) / 2, (y1 + y2) / 2
    draw.rectangle(box(cx - 1, y1, cx + 1, y2), fill=rgb(0xF4E6CC))
    draw.rectangle(box(x1, cy - 1, x2, cy + 1), fill=rgb(0xF4E6CC))


def floor(img, top, color, line):
    draw = ImageDraw.Draw(img)
    draw.rectangle(box(0, top, W, H), fill=rgb(color))
    for x in range(-20, W + 20, 26):
        draw.line(S((x, top), (x - 10, H)), fill=rgb(line), width=s(0.8))
    draw.rectangle(box(0, top - 2, W, top), fill=rgb(line))


def bedroom(img, night):
    """Bed under the window, a bedside lamp, bunting on the wall."""
    if night:
        gradient(img, 0x3A3470, 0x2B2760)
        curtain, wood, blanket, sheet = 0x8B5E9C, 0xC9A27A, 0x6E5AA8, 0xD9D3F0
    else:
        gradient(img, 0xF8EBD0, 0xF1DDB6)
        curtain, wood, blanket, sheet = 0xF2A35E, 0xB08A5E, 0x5DA9E9, 0xFFFFFF
    window(img, 24, 12, 70, 48, night)
    draw = ImageDraw.Draw(img)
    draw.polygon(S((16, 6), (28, 6), (22, 56), (14, 56)), fill=rgb(curtain))
    draw.polygon(S((66, 6), (78, 6), (80, 56), (72, 56)), fill=rgb(curtain))
    draw.rectangle(box(12, 4, 82, 7), fill=rgb(wood))
    bunting(draw, 112, 214, 6, [0xE5534B, 0xFFD45C, 0x4C9A6A, 0x3F86D6, 0xE86FB0] if not night else [0x8E6BD8, 0xFFD38A, 0x6FA8DC])
    floor(img, 82, 0x5A4366 if night else 0xC89A6B, 0x4A3756 if night else 0xB4875A)
    draw = ImageDraw.Draw(img)
    # Bed
    draw.rounded_rectangle(box(4, 52, 14, 92), radius=s(3), fill=rgb(wood))
    draw.rounded_rectangle(box(90, 64, 97, 92), radius=s(2), fill=rgb(wood))
    draw.rectangle(box(10, 64, 94, 80), fill=rgb(sheet))
    draw.ellipse(box(12, 58, 34, 70), fill=rgb(0xFFFFFF if not night else 0xE8E4F8))
    draw.rounded_rectangle(box(32, 62, 95, 82), radius=s(3), fill=rgb(blanket))
    for x in range(38, 92, 10):
        for y in (68, 76):
            sparkle(draw, x + (y % 3), y, 1.6, lerp(rgb(blanket), (255, 255, 255), 0.5))
    # Bedside table and lamp
    draw.rounded_rectangle(box(102, 66, 122, 84), radius=s(2), fill=rgb(wood))
    draw.line(S((104, 75), (120, 75)), fill=lerp(rgb(wood), (0, 0, 0), 0.2), width=s(0.8))
    if night:
        glow(img, 112, 52, 18, 0xFFD38A, 120)
        draw = ImageDraw.Draw(img)
    draw.rectangle(box(111, 54, 113, 66), fill=rgb(0xC9A27A))
    draw.polygon(S((104, 56), (120, 56), (117, 45), (107, 45)), fill=rgb(0xFFD38A if night else 0xF6A85A))


def scene_bedroom_night(img):
    bedroom(img, True)


def scene_bedroom_day(img):
    bedroom(img, False)


def scene_living_room(img):
    """Front door, sofa, picture on the wall."""
    gradient(img, 0xF6E2C4, 0xEFD3AC)
    draw = ImageDraw.Draw(img)
    for x in range(0, W, 14):
        draw.rectangle(box(x, 0, x + 6, 80), fill=rgb(0xF1D8B4))
    door(draw, 10, 20, 42, 80, 0xB0703A)
    floor(img, 80, 0xC89A6B, 0xB4875A)
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle(box(6, 82, 46, 87), radius=s(1.5), fill=rgb(0x8B5E3C))
    # Sofa
    teal, dark = rgb(0x3FA7A0), rgb(0x2E8580)
    draw.rounded_rectangle(box(56, 48, 128, 74), radius=s(5), fill=teal)
    draw.rounded_rectangle(box(52, 60, 62, 86), radius=s(3), fill=dark)
    draw.rounded_rectangle(box(122, 60, 132, 86), radius=s(3), fill=dark)
    draw.rounded_rectangle(box(60, 70, 124, 84), radius=s(3), fill=lerp(teal, (255, 255, 255), 0.12))
    draw.line(S((92, 71), (92, 83)), fill=dark, width=s(0.8))
    draw.rounded_rectangle(box(66, 54, 80, 68), radius=s(3), fill=rgb(0xF28C28))
    draw.rounded_rectangle(box(104, 54, 118, 68), radius=s(3), fill=rgb(0xFFD45C))
    for x in (58, 124):
        draw.rectangle(box(x, 86, x + 2, 90), fill=rgb(0x6B4A30))
    # Picture over the sofa: a sunny hill
    draw.rounded_rectangle(box(78, 12, 106, 36), radius=s(2), fill=rgb(0xB08A5E))
    draw.rectangle(box(81, 15, 103, 33), fill=rgb(0xCDEEFF))
    draw.ellipse(box(94, 17, 100, 23), fill=rgb(0xFFD45C))
    draw.pieslice(box(78, 24, 108, 44), 180, 360, fill=rgb(0x6DBE4F))
    draw.rectangle(box(78, 33, 106, 36), fill=rgb(0xB08A5E))
    draw.ellipse(box(124, 86, 214, 98), fill=rgb(0xE5534B))
    draw.ellipse(box(132, 88, 206, 96), fill=rgb(0xF0786F))


def scene_kitchen(img):
    """Cupboards and counter on the left, the table where things are put, a window."""
    gradient(img, 0xFFF4DE, 0xFBE9C8)
    checker(img, 0, 28, 56, 56, 7, 0xFFFFFF, 0xE3F1F6)
    draw = ImageDraw.Draw(img)
    orange, dark = rgb(0xF2A35E), rgb(0xD98A4A)
    draw.rectangle(box(0, 2, 56, 28), fill=orange)
    draw.line(S((28, 4), (28, 26)), fill=dark, width=s(0.8))
    for x in (24, 32):
        draw.rounded_rectangle(box(x - 0.8, 18, x + 0.8, 24), radius=s(0.8), fill=rgb(0xFFF4DE))
    # Pot steaming on the counter
    draw.rounded_rectangle(box(26, 42, 44, 55), radius=s(3), fill=rgb(0x9AA6B2))
    draw.rectangle(box(24, 41, 46, 43), fill=rgb(0x7D8996))
    for x in (31, 37):
        draw.arc(box(x - 2, 30, x + 2, 36), 90, 270, fill=rgb(0xC8D3DC), width=s(0.8))
        draw.arc(box(x - 2, 34, x + 2, 40), 270, 90, fill=rgb(0xC8D3DC), width=s(0.8))
    draw.rectangle(box(0, 55, 58, 59), fill=rgb(0xE6D3B3))
    draw.rectangle(box(0, 59, 56, 86), fill=dark)
    draw.line(S((28, 61), (28, 84)), fill=lerp(dark, (0, 0, 0), 0.15), width=s(0.8))
    for x in (24, 32):
        draw.rounded_rectangle(box(x - 0.8, 64, x + 0.8, 70), radius=s(0.8), fill=rgb(0xFFF4DE))
    window(img, 74, 10, 112, 38, False)
    checker(img, 0, 86, W, H, 7, 0xF6E7C8, 0xE2C79A)
    draw = ImageDraw.Draw(img)
    # Table with a checked cloth
    draw.rectangle(box(66, 70, 69, 90), fill=rgb(0x9C6A3E))
    draw.rectangle(box(117, 70, 120, 90), fill=rgb(0x9C6A3E))
    draw.rectangle(box(60, 62, 126, 72), fill=rgb(0xE5534B))
    for x in range(60, 126, 6):
        draw.rectangle(box(x, 62, x + 3, 72), fill=rgb(0xF0827B))
    draw.rectangle(box(60, 66, 126, 68), fill=rgb(0xF6B0AA))


def scene_pool(img):
    """Pool on the left with a ladder, deck where the figures stand."""
    gradient(img, 0x7FCBF0, 0xD4F1FF, 0, 40)
    sun(img, 196, 16, 9)
    draw = ImageDraw.Draw(img)
    cloud(draw, 40, 8, 30, rgb(0xFFFFFF))
    cloud(draw, 120, 16, 22, rgb(0xFFFFFF))
    # Hedge
    draw.rectangle(box(0, 34, W, 50), fill=rgb(0x4C9A4A))
    for x in range(-4, W + 8, 10):
        draw.ellipse(box(x, 28 + (x % 3), x + 13, 42), fill=rgb(0x5DAE5A))
    # Deck
    draw.rectangle(box(0, 48, W, H), fill=rgb(0xF4E3C3))
    for y in range(52, H, 9):
        draw.line(S((0, y), (W, y)), fill=rgb(0xE6D0A8), width=s(0.6))
    # Pool
    draw.rounded_rectangle(box(4, 54, 126, 94), radius=s(5), fill=rgb(0xFFFFFF))
    water = Image.new("RGBA", (s(116), s(34)))
    for y in range(water.height):
        ImageDraw.Draw(water).line([(0, y), (water.width, y)], fill=lerp(rgb(0x6FD3F5), rgb(0x2E9FD6), y / water.height))
    mask = Image.new("L", water.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, water.width - 1, water.height - 1], radius=s(4), fill=255)
    img.paste(water, (s(7), s(57)), mask)
    draw = ImageDraw.Draw(img)
    rng = random.Random(5)
    for _ in range(9):
        x, y = rng.uniform(12, 110), rng.uniform(62, 88)
        draw.arc(box(x, y - 2, x + rng.uniform(6, 12), y + 2), 200, 340, fill=rgb(0xC6F0FF), width=s(0.9))
    # Ladder
    for x in (110, 118):
        draw.line(S((x, 50), (x, 44), (x + 3, 42), (x + 3, 50)), fill=rgb(0xB8C4CE), width=s(1.4))
        draw.line(S((x, 50), (x, 66)), fill=rgb(0xB8C4CE), width=s(1.4))
    for y in (58, 63):
        draw.line(S((110, y), (118, y)), fill=rgb(0xB8C4CE), width=s(1.1))


def scene_classroom(img):
    """Chalkboard, clock, a pupil's desk where things are put."""
    gradient(img, 0xF6E7C8, 0xF1DDB6)
    draw = ImageDraw.Draw(img)
    draw.rectangle(box(0, 60, W, 82), fill=rgb(0xE9CFA3))
    draw.rectangle(box(0, 59, W, 61), fill=rgb(0xD2B285))
    draw.rounded_rectangle(box(12, 8, 124, 54), radius=s(2), fill=rgb(0xB0703A))
    draw.rectangle(box(15, 11, 121, 51), fill=rgb(0x2F5D4A))
    chalk_text(img, "ABC", 22, 13, 15, 0xF4F1E6)
    chalk_text(img, "1+2=3", 64, 28, 11, 0xFFE9A0)
    draw = ImageDraw.Draw(img)
    sun_x, sun_y = 104, 18
    draw.ellipse(box(sun_x - 4, sun_y - 4, sun_x + 4, sun_y + 4), outline=rgb(0xFFD45C), width=s(0.9))
    for a in range(0, 360, 45):
        c, sn = math.cos(math.radians(a)), math.sin(math.radians(a))
        draw.line(S((sun_x + 6 * c, sun_y + 6 * sn), (sun_x + 8.5 * c, sun_y + 8.5 * sn)), fill=rgb(0xFFD45C), width=s(0.8))
    draw.rectangle(box(14, 54, 122, 57), fill=rgb(0x9C6A3E))
    # Clock
    draw.ellipse(box(142, 10, 162, 30), fill=rgb(0xFFFFFF), outline=rgb(0xE5534B), width=s(2))
    draw.line(S((152, 20), (152, 14)), fill=rgb(0x3B2F25), width=s(1))
    draw.line(S((152, 20), (157, 22)), fill=rgb(0x3B2F25), width=s(1))
    bunting(draw, 170, 216, 6, [0x3F86D6, 0xFFD45C, 0xE5534B, 0x4C9A6A])
    floor(img, 82, 0xC89A6B, 0xB4875A)
    draw = ImageDraw.Draw(img)
    # Desk
    draw.rectangle(box(26, 74, 29, 92), fill=rgb(0x8B6A4E))
    draw.rectangle(box(83, 74, 86, 92), fill=rgb(0x8B6A4E))
    draw.rounded_rectangle(box(20, 70, 92, 75), radius=s(1.5), fill=rgb(0xF2A35E))


def scene_library(img):
    """Tall shelves full of books, a round window."""
    gradient(img, 0xF1DDB6, 0xEACB9C)
    draw = ImageDraw.Draw(img)
    wood, dark = rgb(0x9C6A3E), rgb(0x7A5030)
    draw.rectangle(box(4, 4, 124, 86), fill=dark)
    draw.rectangle(box(7, 7, 121, 86), fill=rgb(0x6B4A30))
    rng = random.Random(21)
    for shelf in (28, 52, 76):
        books(draw, 9, 119, shelf, rng, 18)
        draw.rectangle(box(7, shelf, 121, shelf + 3), fill=wood)
    draw.rectangle(box(62, 7, 65, 86), fill=wood)
    # Round window and a little plant
    draw.ellipse(box(146, 8, 178, 40), fill=rgb(0xF4E6CC))
    draw.ellipse(box(149, 11, 175, 37), fill=rgb(0xA8DCF5))
    cloud(draw, 152, 20, 16, rgb(0xFFFFFF))
    draw.rectangle(box(161, 11, 163, 37), fill=rgb(0xF4E6CC))
    floor(img, 86, 0xC89A6B, 0xB4875A)
    draw = ImageDraw.Draw(img)
    draw.ellipse(box(122, 88, 214, 98), fill=rgb(0x3F86D6))
    draw.ellipse(box(130, 90, 206, 96), fill=rgb(0x5DA9E9))


def scene_playground(img):
    """Slide and swings on the grass."""
    gradient(img, 0x86CDF2, 0xD4F1FF)
    sun(img, 186, 18, 10)
    draw = ImageDraw.Draw(img)
    cloud(draw, 100, 10, 30, rgb(0xFFFFFF))
    wave(img, 70, 3, 1.4, 0.4, 0x8FD07A)
    draw = ImageDraw.Draw(img)
    draw.rectangle(box(0, 76, W, H), fill=rgb(0x7CC56A))
    draw.ellipse(box(4, 82, 96, 98), fill=rgb(0xF2D49B))
    # Slide: ladder, platform, chute
    for x in (12, 22):
        draw.line(S((x, 86), (x + 2, 36)), fill=rgb(0x3F86D6), width=s(1.6))
    for y in range(44, 86, 8):
        draw.line(S((12.4, y), (22.4, y)), fill=rgb(0x3F86D6), width=s(1.1))
    draw.rectangle(box(12, 34, 38, 38), fill=rgb(0xFFD45C))
    draw.polygon(S((34, 34), (40, 34), (82, 82), (82, 88), (72, 88)), fill=rgb(0xE5534B))
    draw.line(S((37, 34), (80, 84)), fill=rgb(0xF0827B), width=s(1))
    # Swings
    for x1, x2 in [(92, 102), (128, 138)]:
        draw.line(S((x1, 90), ((x1 + x2) / 2, 26), (x2, 90)), fill=rgb(0x9C6A3E), width=s(2))
    draw.line(S((97, 27), (133, 27)), fill=rgb(0x9C6A3E), width=s(2.4))
    for x, color in [(106, 0xF28C28), (122, 0x8E6BD8)]:
        draw.line(S((x - 4, 28), (x - 4, 64)), fill=rgb(0x8A8A8A), width=s(0.7))
        draw.line(S((x + 4, 28), (x + 4, 64)), fill=rgb(0x8A8A8A), width=s(0.7))
        draw.rounded_rectangle(box(x - 6, 63, x + 6, 66), radius=s(1), fill=rgb(color))


def scene_school(img):
    """School front: brick building with a bell, a blue door and a gate."""
    gradient(img, 0x86CDF2, 0xD4F1FF)
    draw = ImageDraw.Draw(img)
    cloud(draw, 150, 10, 30, rgb(0xFFFFFF))
    draw.rectangle(box(0, 70, W, 86), fill=rgb(0x8FD07A))
    brick, roof = rgb(0xE07856), rgb(0xB0473A)
    draw.rectangle(box(10, 34, 126, 84), fill=brick)
    for y in range(38, 84, 6):
        draw.line(S((10, y), (126, y)), fill=lerp(brick, (255, 255, 255), 0.12), width=s(0.5))
    draw.polygon(S((4, 36), (68, 12), (132, 36)), fill=roof)
    # Bell
    draw.rectangle(box(62, 6, 74, 18), fill=rgb(0xF4E6CC))
    draw.polygon(S((60, 7), (68, 0), (76, 7)), fill=roof)
    draw.pieslice(box(64, 9, 72, 17), 180, 360, fill=rgb(0xFFD45C))
    draw.rectangle(box(64, 13, 72, 15), fill=rgb(0xFFD45C))
    # Windows and door
    for x in (18, 38, 86, 106):
        draw.rounded_rectangle(box(x, 44, x + 13, 58), radius=s(1.5), fill=rgb(0xF4E6CC))
        draw.rectangle(box(x + 1.5, 45.5, x + 11.5, 56.5), fill=rgb(0xA8DCF5))
        draw.line(S((x + 6.5, 45), (x + 6.5, 57)), fill=rgb(0xF4E6CC), width=s(1))
    draw.rounded_rectangle(box(54, 52, 82, 84), radius=s(3), fill=rgb(0xF4E6CC))
    draw.rectangle(box(57, 55, 79, 84), fill=rgb(0x3F86D6))
    draw.line(S((68, 55), (68, 84)), fill=rgb(0x2F6AB0), width=s(1))
    for x in (65, 71):
        draw.ellipse(box(x - 1.2, 68, x + 1.2, 70.4), fill=rgb(0xFFD45C))
    draw.rectangle(box(0, 84, W, H), fill=rgb(0xD9D2C3))
    draw.rectangle(box(0, 84, W, 86), fill=rgb(0xC4BCAA))
    # Gate: posts and bars in front, open in the middle
    draw = ImageDraw.Draw(img)
    for x in list(range(4, 52, 6)) + list(range(88, 136, 6)):
        draw.rectangle(box(x, 66, x + 1.6, 86), fill=rgb(0x3B4A6B))
    for x1, x2 in [(2, 52), (86, 136)]:
        draw.rectangle(box(x1, 68, x2, 70), fill=rgb(0x3B4A6B))
        draw.rectangle(box(x1, 78, x2, 80), fill=rgb(0x3B4A6B))


def scene_road(img):
    """Hills, a road and the family car."""
    gradient(img, 0x86CDF2, 0xD4F1FF)
    sun(img, 30, 18, 9)
    draw = ImageDraw.Draw(img)
    cloud(draw, 120, 10, 34, rgb(0xFFFFFF))
    wave(img, 52, 8, 1.2, 0.3, 0x9BD77A)
    draw = ImageDraw.Draw(img)
    for x, h in [(160, 26), (186, 20), (206, 28)]:
        round_tree(draw, x, 60, h, 0x8B5E3C, 0x4C9A6A)
    draw.rectangle(box(0, 60, W, 66), fill=rgb(0x7CC56A))
    draw.rectangle(box(0, 64, W, 84), fill=rgb(0x5A5F6A))
    for x in range(4, W, 22):
        draw.rectangle(box(x, 73, x + 11, 75), fill=rgb(0xFFFFFF))
    draw.rectangle(box(0, 84, W, H), fill=rgb(0xD9D2C3))
    draw.rectangle(box(0, 84, W, 86), fill=rgb(0xC4BCAA))
    # Car
    red = rgb(0xE5534B)
    draw.rounded_rectangle(box(12, 58, 86, 76), radius=s(6), fill=red)
    draw.polygon(S((26, 59), (34, 44), (64, 44), (74, 59)), fill=red)
    draw.polygon(S((31, 58), (37, 47), (48, 47), (48, 58)), fill=rgb(0xCDEEFF))
    draw.polygon(S((51, 58), (51, 47), (62, 47), (69, 58)), fill=rgb(0xCDEEFF))
    draw.rectangle(box(80, 62, 86, 66), fill=rgb(0xFFE9A0))
    draw.rectangle(box(12, 64, 15, 68), fill=rgb(0xF6A85A))
    for x in (28, 70):
        draw.ellipse(box(x - 7, 69, x + 7, 83), fill=rgb(0x2F2F36))
        draw.ellipse(box(x - 3, 73, x + 3, 79), fill=rgb(0xB8C4CE))


def scene_garden(img):
    gradient(img, 0x86CDF2, 0xD4F1FF)
    sun(img, 28, 22, 10)
    draw = ImageDraw.Draw(img)
    cloud(draw, 80, 12, 34, rgb(0xFFFFFF))
    cloud(draw, 150, 22, 26, rgb(0xFFFFFF))
    wave(img, 66, 6, 1.4, 0.4, 0x9BD77A)
    draw = ImageDraw.Draw(img)
    # Fence
    for x in range(4, 112, 12):
        draw.polygon(S((x, 82), (x, 58), (x + 4, 54), (x + 8, 58), (x + 8, 82)), fill=rgb(0xFFF8EA))
    draw.rectangle(box(0, 62, 114, 65), fill=rgb(0xF1E4CC))
    draw.rectangle(box(0, 73, 114, 76), fill=rgb(0xF1E4CC))
    wave(img, 82, 2, 2, 1.0, 0x6DBE4F)
    draw = ImageDraw.Draw(img)
    for i, x in enumerate(range(8, 112, 15)):
        flower(draw, x, 80 + (i % 2) * 3, [0xE5534B, 0xF28C28, 0xE86FB0, 0xFFFFFF][i % 4])


def scene_forest(img):
    gradient(img, 0x1D3B3C, 0x3E6E5C)
    glow(img, 120, 70, 50, 0x9FD9A8, 60)
    draw = ImageDraw.Draw(img)
    for x, h in [(8, 56), (30, 70), (54, 52), (176, 64), (200, 72), (220, 50)]:
        pine(draw, x, 84, h, 0x16302C)
    for x, h in [(70, 44), (96, 38), (150, 46)]:
        pine(draw, x, 84, h, 0x22443B)
    wave(img, 82, 3, 1.5, 0.2, 0x2E5A3A)
    draw = ImageDraw.Draw(img)
    # Path into the trees and two mushrooms
    draw.polygon(S((96, 100), (128, 100), (116, 80), (106, 80)), fill=rgb(0x5E7A4C))
    for x, y in [(20, 90), (34, 93)]:
        draw.rectangle(box(x - 1.5, y - 4, x + 1.5, y + 2), fill=rgb(0xF4E6CC))
        draw.pieslice(box(x - 6, y - 10, x + 6, y + 2), 180, 360, fill=rgb(0xE5534B))
        draw.ellipse(box(x - 2.5, y - 8, x - 0.5, y - 6), fill=rgb(0xFFFFFF))
        draw.ellipse(box(x + 1.5, y - 6.5, x + 3.5, y - 4.5), fill=rgb(0xFFFFFF))


def scene_cave(img):
    gradient(img, 0x2A2244, 0x1A1530)
    draw = ImageDraw.Draw(img)
    # Stalactites along the top
    rng = random.Random(3)
    x = -4
    while x < W:
        w, h = rng.uniform(10, 18), rng.uniform(8, 20)
        draw.polygon(S((x, 0), (x + w, 0), (x + w / 2, h)), fill=rgb(0x3B3158))
        x += w * 0.8
    wave(img, 84, 3, 1.2, 1.3, 0x3B3158)
    # Crystal clusters
    for cx, base, color in [(26, 86, 0x6FE3F0), (54, 88, 0xF58FD0), (90, 86, 0xA98BF5)]:
        glow(img, cx, base - 12, 16, color, 110)
        draw = ImageDraw.Draw(img)
        for dx, h, lean in [(-6, 16, -3), (0, 24, 0), (6, 14, 3)]:
            x0 = cx + dx
            draw.polygon(S((x0 - 3, base), (x0 + 3, base), (x0 + 3 + lean, base - h + 4), (x0 + lean, base - h), (x0 - 3 + lean, base - h + 4)), fill=rgb(color))
            draw.polygon(S((x0, base), (x0 + 3, base), (x0 + 3 + lean, base - h + 4), (x0 + lean, base - h)), fill=lerp(rgb(color), (255, 255, 255), 0.35))


def scene_lake(img):
    gradient(img, 0x6B5BA6, 0xF4B183, 0, 56)
    glow(img, 150, 54, 18, 0xFFD38A, 150)
    draw = ImageDraw.Draw(img)
    draw.ellipse(box(142, 46, 158, 62), fill=rgb(0xFFC97A))
    wave(img, 54, 5, 1.3, 0.8, 0x5B4E8A)
    gradient(img, 0x4E6FA8, 0x3A578C, 56, H)
    draw = ImageDraw.Draw(img)
    for y, x1, x2 in [(60, 136, 164), (65, 128, 172), (70, 140, 160)]:
        draw.line(S((x1, y), (x2, y)), fill=rgb(0xF4C08A), width=s(1))
    # Wooden bridge over the water
    wood, dark = rgb(0xB0703A), rgb(0x7A5030)
    draw.arc(box(14, 58, 104, 92), 200, 340, fill=dark, width=s(5))
    for i in range(11):
        t = i / 10
        x = 22 + t * 74
        y = 75 - 13 * math.sin(math.pi * t)
        draw.rectangle(box(x - 3.2, y - 2, x + 3.2, y + 1.5), fill=wood)
    for i in range(0, 11, 2):
        t = i / 10
        x = 22 + t * 74
        y = 75 - 13 * math.sin(math.pi * t)
        draw.line(S((x, y - 1), (x, y - 9)), fill=dark, width=s(1.2))
    draw.line(S(*[(22 + i / 20 * 74, 66 - 13 * math.sin(math.pi * i / 20)) for i in range(21)]), fill=dark, width=s(1.2))
    wave(img, 84, 2, 2.2, 0.3, 0x4F8A4B)
    draw = ImageDraw.Draw(img)
    for x in [4, 10, 112, 118]:
        draw.line(S((x, 86), (x + 2, 68)), fill=rgb(0x3E7A3A), width=s(1.2))
        draw.ellipse(box(x + 0.5, 66, x + 3.5, 73), fill=rgb(0x8B5E3C))


def scene_mountain(img):
    gradient(img, 0x8CC8EE, 0xE2F4FF)
    draw = ImageDraw.Draw(img)
    cloud(draw, 14, 14, 30, rgb(0xFFFFFF))
    for peak_x, peak_y, half, color in [(70, 14, 62, 0x8494BE), (160, 24, 56, 0x6E7FAE)]:
        draw.polygon(S((peak_x - half, 84), (peak_x, peak_y), (peak_x + half, 84)), fill=rgb(color))
        k = 0.28
        draw.polygon(S((peak_x - half * k, peak_y + (84 - peak_y) * k), (peak_x, peak_y), (peak_x + half * k, peak_y + (84 - peak_y) * k),
                       (peak_x + half * 0.1, peak_y + (84 - peak_y) * k * 0.8), (peak_x - half * 0.08, peak_y + (84 - peak_y) * k * 1.05)), fill=rgb(0xFFFFFF))
    cloud(draw, 120, 34, 40, rgb(0xFFFFFF))
    wave(img, 82, 4, 1.6, 0.9, 0x86A86A)
    draw = ImageDraw.Draw(img)
    for x, y in [(30, 92), (90, 94)]:
        draw.ellipse(box(x - 6, y - 4, x + 6, y + 3), fill=rgb(0x9C9C9C))


def scene_sky(img):
    gradient(img, 0x141A3D, 0x3B3577)
    dots(img, 40, (4, 4, W - 4, 72), 0xFFF6DC, 11)
    glow(img, 60, 30, 26, 0xFFECBE, 70)
    crescent(img, 60, 30, 14, 0xFFF3D6)
    draw = ImageDraw.Draw(img)
    for x, y, size in [(110, 16, 3.5), (186, 30, 3), (24, 60, 2.5)]:
        sparkle(draw, x, y, size, rgb(0xFFECB4))
    wave(img, 80, 6, 1.2, 0.5, 0x2B2560)
    wave(img, 88, 4, 1.8, 2.0, 0x1E1A45)


def scene_beach(img):
    gradient(img, 0x86CDF2, 0xD6F1FB, 0, 52)
    sun(img, 186, 18, 10)
    draw = ImageDraw.Draw(img)
    cloud(draw, 40, 12, 30, rgb(0xFFFFFF))
    gradient(img, 0x3FA7D6, 0x2F86C0, 50, H)
    draw = ImageDraw.Draw(img)
    for y in (56, 64, 71):
        for x in range(4, W, 28):
            draw.arc(box(x + (y % 3) * 6, y - 3, x + 10 + (y % 3) * 6, y + 3), 200, 340, fill=rgb(0xD6F1FB), width=s(1))
    wave(img, 76, 3, 2.4, 0.4, 0xF2D49B)
    draw = ImageDraw.Draw(img)
    # Starfish and a shell
    cx, cy = 34, 90
    draw.polygon(S(*[(cx + (5 if i % 2 == 0 else 2) * math.cos(math.radians(i * 36 - 90)),
                      cy + (5 if i % 2 == 0 else 2) * math.sin(math.radians(i * 36 - 90))) for i in range(10)]), fill=rgb(0xF28C28))
    draw.pieslice(box(70, 84, 82, 96), 180, 360, fill=rgb(0xF5A9C8))


SCENES = {
    "bedroom_night": scene_bedroom_night,
    "bedroom_day": scene_bedroom_day,
    "living_room": scene_living_room,
    "kitchen": scene_kitchen,
    "pool": scene_pool,
    "classroom": scene_classroom,
    "library": scene_library,
    "playground": scene_playground,
    "school": scene_school,
    "road": scene_road,
    "garden": scene_garden,
    "forest": scene_forest,
    "cave": scene_cave,
    "lake": scene_lake,
    "mountain": scene_mountain,
    "sky": scene_sky,
    "beach": scene_beach,
}


# ---------------- Figures ----------------

INK = (59, 47, 37, 255)


def outlined(fig, color=INK, width=1):
    """Cartoon outline: the figure's shape, grown a little, drawn behind it."""
    alpha = fig.getchannel("A").point(lambda a: 255 if a > 40 else 0)
    grown = alpha.filter(ImageFilter.MaxFilter(s(width) * 2 + 1))
    out = Image.new("RGBA", fig.size, (0, 0, 0, 0))
    out.paste(Image.new("RGBA", fig.size, color), (0, 0), grown)
    out.alpha_composite(fig)
    return out


def canvas(w, h):
    img = Image.new("RGBA", (s(w), s(h)), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def figure_zoe():
    img, d = canvas(36, 52)
    skin, hair, dress = rgb(0xF6CFAE), rgb(0x6B3F26), rgb(0xF28C28)
    # Pigtails behind the head
    for x in (6, 30):
        d.ellipse(box(x - 5, 12, x + 5, 23), fill=hair)
        d.ellipse(box(x - 1.8, 11, x + 1.8, 14.5), fill=rgb(0xE5534B))
    # Arms, dress, legs
    d.line(S((12, 31), (8, 40)), fill=skin, width=s(3.2))
    d.line(S((24, 31), (28, 40)), fill=skin, width=s(3.2))
    d.rectangle(box(13, 43, 16, 48), fill=skin)
    d.rectangle(box(20, 43, 23, 48), fill=skin)
    d.rounded_rectangle(box(11.5, 47, 17, 50.5), radius=s(1.5), fill=rgb(0x8B2E2E))
    d.rounded_rectangle(box(19, 47, 24.5, 50.5), radius=s(1.5), fill=rgb(0x8B2E2E))
    d.polygon(S((12.5, 28), (23.5, 28), (28, 45), (8, 45)), fill=dress)
    d.polygon(S((15, 28), (21, 28), (18, 31.5)), fill=rgb(0xFFF8EA))
    # Head
    d.ellipse(box(8, 7, 28, 28), fill=hair)
    d.ellipse(box(9.5, 9.5, 26.5, 27), fill=skin)
    d.chord(box(8.5, 6.5, 27.5, 21), 180, 360, fill=hair)
    d.ellipse(box(13.2, 17.6, 15.2, 19.8), fill=rgb(0x3B2F25))
    d.ellipse(box(20.8, 17.6, 22.8, 19.8), fill=rgb(0x3B2F25))
    d.ellipse(box(10.8, 20.5, 14, 23), fill=rgb(0xF4A3A0))
    d.ellipse(box(22, 20.5, 25.2, 23), fill=rgb(0xF4A3A0))
    d.arc(box(15.5, 20, 20.5, 24.5), 20, 160, fill=rgb(0x8B2E2E), width=s(0.9))
    return outlined(img)


def figure_ted():
    img, d = canvas(28, 32)
    fur, light = rgb(0xA8703F), rgb(0xE2BC8F)
    for x in (7, 21):
        d.ellipse(box(x - 4.5, 2, x + 4.5, 11), fill=fur)
        d.ellipse(box(x - 2.2, 4.3, x + 2.2, 8.7), fill=light)
    d.ellipse(box(3, 19, 9, 26), fill=fur)
    d.ellipse(box(19, 19, 25, 26), fill=fur)
    d.ellipse(box(6.5, 16, 21.5, 30), fill=fur)
    d.ellipse(box(9.5, 19.5, 18.5, 28), fill=light)
    d.ellipse(box(6, 26.5, 12, 31), fill=light)
    d.ellipse(box(16, 26.5, 22, 31), fill=light)
    d.ellipse(box(5, 3.5, 23, 20), fill=fur)
    d.ellipse(box(9.5, 11, 18.5, 18), fill=light)
    d.ellipse(box(12.3, 11.8, 15.7, 14.2), fill=rgb(0x3B2F25))
    d.ellipse(box(9.2, 8.2, 11.2, 10.4), fill=rgb(0x3B2F25))
    d.ellipse(box(16.8, 8.2, 18.8, 10.4), fill=rgb(0x3B2F25))
    d.arc(box(12, 13, 16, 17), 30, 150, fill=rgb(0x3B2F25), width=s(0.8))
    return outlined(img)


# Body plans for people; Zoe is drawn on the child one
CHILD = dict(w=36, h=52, cx=18, cy=17.5, r=10, neck=28, waist=38, hem=45, shoulder=5.5, hem_w=10,
             leg_top=43, foot=48, sole=50.5, legs=(14.5, 21.5), leg_w=1.6, arm=((12.5, 31), (8, 40)), arm_w=3.2)
ADULT = dict(w=40, h=69, cx=20, cy=13, r=9, neck=23, waist=44, hem=53, shoulder=7, hem_w=11.5,
             leg_top=48, foot=64, sole=67.5, legs=(16.5, 23.5), leg_w=2, arm=((13, 26), (9, 46)), arm_w=3.6)


def person(g, skin, hair, style, top, bottom=None, outfit="dress", sleeves=True, shoes=0x8B2E2E,
           glasses=False, mustache=False, ribbon=None):
    """A person on body plan g (CHILD or ADULT).

    style: pigtails, long, bob, curly, ponytail, bun, short, bald
    outfit: dress, skirt, pants, shorts
    """
    img, d = canvas(g["w"], g["h"])
    skin, hair, top = rgb(skin), rgb(hair), rgb(top)
    bottom = rgb(bottom) if bottom is not None else top
    cx, cy, r = g["cx"], g["cy"], g["r"]

    # Hair that falls behind the body
    if style == "pigtails":
        for x in (cx - r - 2, cx + r + 2):
            d.ellipse(box(x - 5, cy - 5.5, x + 5, cy + 5.5), fill=hair)
            if ribbon:
                d.ellipse(box(x - 1.8, cy - 6.5, x + 1.8, cy - 3), fill=rgb(ribbon))
    elif style == "long":
        d.rounded_rectangle(box(cx - r - 1.5, cy - r * 0.5, cx + r + 1.5, cy + r + 7), radius=s(r * 0.5), fill=hair)
    elif style == "bob":
        d.rounded_rectangle(box(cx - r - 1.5, cy - r * 0.5, cx + r + 1.5, cy + r * 0.85), radius=s(r * 0.5), fill=hair)
    elif style == "curly":
        for a in range(150, 400, 22):
            px, py = cx + r * 1.0 * math.cos(math.radians(a)), cy + r * 0.95 * math.sin(math.radians(a))
            d.ellipse(box(px - r * 0.42, py - r * 0.42, px + r * 0.42, py + r * 0.42), fill=hair)
        for side in (-1, 1):
            d.ellipse(box(cx + side * r * 1.05 - r * 0.4, cy + r * 0.2, cx + side * r * 1.05 + r * 0.4, cy + r * 1.0), fill=hair)
    elif style == "ponytail":
        d.ellipse(box(cx + r * 0.5, cy - r * 0.4, cx + r * 1.45, cy + r * 1.5), fill=hair)
    elif style == "bun":
        d.ellipse(box(cx - r * 0.5, cy - r * 1.55, cx + r * 0.5, cy - r * 0.6), fill=hair)

    # Arms
    (ax1, ay1), (ax2, ay2) = g["arm"]
    for side in (-1, 1):
        sx1, sx2 = cx + side * (cx - ax1), cx + side * (cx - ax2)
        if sleeves:
            mx, my = sx1 + (sx2 - sx1) * 0.55, ay1 + (ay2 - ay1) * 0.55
            d.line(S((sx1, ay1), (sx2, ay2)), fill=skin, width=s(g["arm_w"] * 0.85))
            d.line(S((sx1, ay1), (mx, my)), fill=top, width=s(g["arm_w"]))
        else:
            d.line(S((sx1, ay1), (sx2, ay2)), fill=skin, width=s(g["arm_w"]))
        d.ellipse(box(sx2 - g["arm_w"] * 0.6, ay2 - g["arm_w"] * 0.6, sx2 + g["arm_w"] * 0.6, ay2 + g["arm_w"] * 0.6), fill=skin)

    # Legs and shoes
    trousers = outfit == "pants"
    for lx in g["legs"]:
        lw = g["leg_w"] + (0.4 if trousers else 0)
        d.rectangle(box(lx - lw, g["leg_top"], lx + lw, g["foot"]), fill=bottom if trousers else skin)
        d.rounded_rectangle(box(lx - lw - 1.6, g["foot"] - 1, lx + lw + 1.6, g["sole"]), radius=s(1.5), fill=rgb(shoes))

    # Clothes
    sh, n, wst = g["shoulder"], g["neck"], g["waist"]
    if outfit == "dress":
        d.polygon(S((cx - sh, n), (cx + sh, n), (cx + g["hem_w"], g["hem"]), (cx - g["hem_w"], g["hem"])), fill=top)
    else:
        if outfit == "skirt":
            d.polygon(S((cx - sh, wst - 1), (cx + sh, wst - 1), (cx + g["hem_w"], g["hem"]), (cx - g["hem_w"], g["hem"])), fill=bottom)
        else:
            low = g["leg_top"] + (1.5 if outfit == "shorts" else 0.5)
            d.rectangle(box(cx - sh - 0.5, wst - 1, cx + sh + 0.5, low), fill=bottom)
            if outfit == "shorts":
                d.line(S((cx, wst + 1), (cx, low)), fill=lerp(bottom, (0, 0, 0), 0.25), width=s(0.6))
        d.polygon(S((cx - sh, n), (cx + sh, n), (cx + sh + 1, wst), (cx - sh - 1, wst)), fill=top)
    d.polygon(S((cx - 3, n), (cx + 3, n), (cx, n + 3.5)), fill=rgb(0xFFF8EA))

    # Head
    if style in ("short", "bald"):
        for side in (-1, 1):
            ex = cx + side * r * 0.88
            d.ellipse(box(ex - r * 0.2, cy - r * 0.05, ex + r * 0.2, cy + r * 0.4), fill=skin)
        d.ellipse(box(cx - r * 0.88, cy - r * 0.95, cx + r * 0.88, cy + r * 1.0), fill=skin)
        if style == "short":
            d.chord(box(cx - r * 0.92, cy - r * 1.08, cx + r * 0.92, cy + r * 0.25), 180, 360, fill=hair)
            d.polygon(S((cx - r * 0.5, cy - r * 0.5), (cx + r * 0.1, cy - r * 0.55), (cx - r * 0.2, cy - r * 0.2)), fill=hair)
        else:
            for side in (-1, 1):
                ex = cx + side * r * 0.82
                d.ellipse(box(ex - r * 0.22, cy - r * 0.45, ex + r * 0.22, cy + r * 0.15), fill=hair)
    else:
        d.ellipse(box(cx - r, cy - r * 1.05, cx + r, cy + r * 1.05), fill=hair)
        d.ellipse(box(cx - r * 0.85, cy - r * 0.8, cx + r * 0.85, cy + r * 0.95), fill=skin)
        d.chord(box(cx - r * 0.95, cy - r * 1.1, cx + r * 0.95, cy + r * 0.35), 180, 360, fill=hair)
        if style == "curly":
            for a in range(200, 345, 28):
                px, py = cx + r * 0.6 * math.cos(math.radians(a)), cy - r * 0.15 + r * 0.6 * math.sin(math.radians(a))
                d.ellipse(box(px - r * 0.3, py - r * 0.3, px + r * 0.3, py + r * 0.3), fill=hair)

    # Face
    ink = rgb(0x3B2F25)
    ey = cy + r * 0.12
    for side in (-1, 1):
        ex = cx + side * r * 0.38
        d.ellipse(box(ex - r * 0.1, ey - r * 0.11, ex + r * 0.1, ey + r * 0.11), fill=ink)
        if glasses:
            d.ellipse(box(ex - r * 0.26, ey - r * 0.24, ex + r * 0.26, ey + r * 0.24), outline=ink, width=s(0.7))
        chx = cx + side * r * 0.62
        d.ellipse(box(chx - r * 0.16, cy + r * 0.3, chx + r * 0.16, cy + r * 0.55), fill=rgb(0xF4A3A0) if skin[0] > 200 else rgb(0xC9665A))
    if glasses:
        d.line(S((cx - r * 0.12, ey), (cx + r * 0.12, ey)), fill=ink, width=s(0.7))
    if mustache:
        for side in (-1, 1):
            mx = cx + side * r * 0.18
            d.ellipse(box(mx - r * 0.24, cy + r * 0.33, mx + r * 0.24, cy + r * 0.5), fill=hair)
        d.arc(box(cx - r * 0.2, cy + r * 0.38, cx + r * 0.2, cy + r * 0.72), 20, 160, fill=rgb(0x8B2E2E), width=s(0.9))
    else:
        d.arc(box(cx - r * 0.25, cy + r * 0.25, cx + r * 0.25, cy + r * 0.7), 20, 160, fill=rgb(0x8B2E2E), width=s(0.9))
    return outlined(img)


def figure_girl():
    return person(CHILD, 0xF8D9C0, 0xE8B84A, "long", 0xE86FB0, sleeves=False, shoes=0xE5534B)


def figure_girl2():
    return person(CHILD, 0xB5774C, 0x2B1D16, "curly", 0x8E6BD8, sleeves=False, shoes=0x3B2F25)


def figure_boy():
    return person(CHILD, 0xF2C7A0, 0x3B2A20, "short", 0x3F86D6, 0x2F4F7F, outfit="shorts", shoes=0x3B4A6B)


def figure_mom():
    return person(ADULT, 0xF6CFAE, 0x6B3F26, "bob", 0x2FA39A, shoes=0x8B2E2E)


def figure_dad():
    return person(ADULT, 0xF2C7A0, 0x4A2E1C, "short", 0x4C9A6A, 0x3B4A6B, outfit="pants", shoes=0x3B2F25)


def figure_grandpa():
    return person(ADULT, 0xF2C7A0, 0xD8D8D8, "bald", 0xC9A27A, 0x5A5A6A, outfit="pants", shoes=0x5A3A28,
                  glasses=True, mustache=True)


def figure_grandma():
    return person(ADULT, 0xF6CFAE, 0xE6E6E6, "bun", 0xA98BF5, shoes=0x6B4A30, glasses=True)


def figure_teacher():
    return person(ADULT, 0xD9A27A, 0x2B1D16, "ponytail", 0xE5534B, 0x3B4A6B, outfit="skirt", shoes=0x3B2F25)


def eyes(d, pts, size=1.1):
    for x, y in pts:
        d.ellipse(box(x - size, y - size * 1.1, x + size, y + size * 1.1), fill=rgb(0x3B2F25))


def figure_dog():
    img, d = canvas(30, 28)
    fur, light, ear = rgb(0xC98B4F), rgb(0xF4E2C4), rgb(0x8B5A2E)
    d.line(S((22, 23), (27, 15)), fill=fur, width=s(2.6))
    d.ellipse(box(7, 12, 23, 27), fill=fur)
    d.ellipse(box(11, 15, 19, 26), fill=light)
    d.ellipse(box(8.5, 22.5, 14, 27.5), fill=light)
    d.ellipse(box(16, 22.5, 21.5, 27.5), fill=light)
    d.ellipse(box(6, 2, 24, 18), fill=fur)
    d.polygon(S((7, 3), (3, 6), (4, 15), (9, 12)), fill=ear)
    d.polygon(S((23, 3), (27, 6), (26, 15), (21, 12)), fill=ear)
    d.ellipse(box(10.5, 9, 19.5, 17), fill=light)
    eyes(d, [(11.5, 8), (18.5, 8)])
    d.ellipse(box(13.3, 9.5, 16.7, 12), fill=rgb(0x3B2F25))
    d.ellipse(box(14, 14, 16, 17.5), fill=rgb(0xF07A8A))
    return outlined(img)


def figure_cat():
    img, d = canvas(26, 28)
    fur, light = rgb(0xF2A35E), rgb(0xFBE2C4)
    d.arc(box(14, 10, 26, 28), 270, 90, fill=fur, width=s(2.6))
    d.ellipse(box(5, 13, 19, 27), fill=fur)
    d.ellipse(box(8.5, 16, 15.5, 26.5), fill=light)
    for x in (5, 19):
        d.polygon(S((x - 3, 7), (x, -0.5 + 1), (x + 3, 7)), fill=fur)
    d.ellipse(box(3, 3, 21, 17), fill=fur)
    for y in (5, 7.5):
        d.line(S((10, y), (14, y)), fill=rgb(0xD2701A), width=s(0.8))
    eyes(d, [(9, 9.5), (15, 9.5)])
    d.polygon(S((11, 12), (13, 12), (12, 13.3)), fill=rgb(0xF07A8A))
    for side in (-1, 1):
        d.line(S((12 + side * 3, 13), (12 + side * 9, 12)), fill=rgb(0x8B5A2E), width=s(0.4))
    return outlined(img)


def figure_frog():
    img, d = canvas(26, 20)
    green, light = rgb(0x6DBE4F), rgb(0xB8E68E)
    d.ellipse(box(3, 7, 23, 19), fill=green)
    d.ellipse(box(8, 11, 18, 19), fill=light)
    for x in (7.5, 18.5):
        d.ellipse(box(x - 4, 1.5, x + 4, 9.5), fill=green)
        d.ellipse(box(x - 2.4, 3, x + 2.4, 8), fill=rgb(0xFFFFFF))
        d.ellipse(box(x - 1.1, 4.4, x + 1.1, 6.8), fill=rgb(0x3B2F25))
    d.arc(box(8, 6, 18, 13), 20, 160, fill=rgb(0x2E6B2A), width=s(0.9))
    d.ellipse(box(1, 15, 8, 19.5), fill=green)
    d.ellipse(box(18, 15, 25, 19.5), fill=green)
    return outlined(img)


def figure_owl():
    img, d = canvas(24, 30)
    brown, light, dark = rgb(0x9C6A3E), rgb(0xE2BC8F), rgb(0x7A5030)
    for x in (5, 19):
        d.polygon(S((x - 3, 9), (x, 1), (x + 3, 9)), fill=dark)
    d.ellipse(box(3, 5, 21, 28), fill=brown)
    d.ellipse(box(0.5, 13, 7, 25), fill=dark)
    d.ellipse(box(17, 13, 23.5, 25), fill=dark)
    d.ellipse(box(7, 15, 17, 27), fill=light)
    for y in (18, 21.5, 25):
        for x in (10, 14):
            d.arc(box(x - 1.5, y - 1, x + 1.5, y + 1.5), 20, 160, fill=brown, width=s(0.6))
    for x in (8.3, 15.7):
        d.ellipse(box(x - 4, 6.5, x + 4, 14.5), fill=rgb(0xF4E2C4))
        d.ellipse(box(x - 1.6, 8.9, x + 1.6, 12.1), fill=rgb(0x3B2F25))
        d.ellipse(box(x - 4, 6.5, x + 4, 14.5), outline=rgb(0x3B2F25), width=s(0.7))
    d.polygon(S((10.8, 13), (13.2, 13), (12, 16)), fill=rgb(0xF28C28))
    for x in (9, 15):
        d.line(S((x - 1.5, 28.5), (x + 1.5, 28.5)), fill=rgb(0xF28C28), width=s(1.2))
    return outlined(img)


def figure_bear():
    img, d = canvas(46, 44)
    fur, light = rgb(0x7A4E2C), rgb(0xC9A27A)
    for x in (12, 34):
        d.ellipse(box(x - 5.5, 1, x + 5.5, 12), fill=fur)
        d.ellipse(box(x - 3, 3.5, x + 3, 9.5), fill=light)
    d.ellipse(box(5, 18, 41, 43), fill=fur)
    d.ellipse(box(14, 24, 32, 42), fill=light)
    d.ellipse(box(2, 24, 11, 36), fill=fur)
    d.ellipse(box(35, 24, 44, 36), fill=fur)
    d.ellipse(box(7, 36, 18, 43.5), fill=light)
    d.ellipse(box(28, 36, 39, 43.5), fill=light)
    d.ellipse(box(9, 3, 37, 27), fill=fur)
    d.ellipse(box(16.5, 14, 29.5, 24), fill=light)
    d.ellipse(box(20, 14.5, 26, 18.5), fill=rgb(0x3B2F25))
    eyes(d, [(17, 11.5), (29, 11.5)], 1.3)
    d.arc(box(19.5, 17, 26.5, 22.5), 30, 150, fill=rgb(0x3B2F25), width=s(0.9))
    return outlined(img)


def figure_squirrel():
    img, d = canvas(26, 28)
    fur, light = rgb(0xE08A3C), rgb(0xFBE2C4)
    d.ellipse(box(12, 3, 26, 26), fill=rgb(0xD2701A))
    d.ellipse(box(15, 6, 23, 20), fill=fur)
    d.ellipse(box(4, 12, 17, 27), fill=fur)
    d.ellipse(box(7, 16, 14, 26.5), fill=light)
    d.ellipse(box(8.5, 17.5, 12.5, 21.5), fill=rgb(0x9C6A3E))
    d.polygon(S((5, 6), (6.5, 0.5), (9, 5)), fill=fur)
    d.ellipse(box(2, 3, 15, 15), fill=fur)
    d.ellipse(box(2.5, 9, 9, 14.5), fill=light)
    eyes(d, [(8.5, 7.5)])
    d.ellipse(box(1.5, 9.4, 3.7, 11.2), fill=rgb(0x3B2F25))
    return outlined(img)


def figure_sheep():
    img, d = canvas(32, 26)
    wool, face = rgb(0xFFFFFF), rgb(0x6B5A50)
    for x in (10, 15, 21, 26):
        d.rectangle(box(x - 1.2, 17, x + 1.2, 25), fill=face)
    for cx, cy, r in [(12, 11, 6), (19, 9, 6.5), (25, 12, 5.5), (15, 16, 6), (22, 16, 6), (9, 15, 5)]:
        d.ellipse(box(cx - r, cy - r, cx + r, cy + r), fill=wool)
    d.ellipse(box(1, 6, 11, 17), fill=face)
    d.ellipse(box(3, 3, 10, 9), fill=wool)
    d.ellipse(box(8, 7, 13, 10), fill=face)
    d.ellipse(box(3.2, 9.6, 5.2, 11.8), fill=rgb(0xFFFFFF))
    d.ellipse(box(3.7, 10.2, 4.9, 11.6), fill=rgb(0x3B2F25))
    d.arc(box(2, 11, 6, 15), 30, 150, fill=rgb(0x3B2F25), width=s(0.6))
    return outlined(img)


FIGURES = {
    "zoe": figure_zoe, "ted": figure_ted,
    "girl": figure_girl, "girl2": figure_girl2, "boy": figure_boy,
    "mom": figure_mom, "dad": figure_dad, "grandpa": figure_grandpa, "grandma": figure_grandma, "teacher": figure_teacher,
    "dog": figure_dog, "cat": figure_cat, "frog": figure_frog, "owl": figure_owl, "bear": figure_bear,
    "squirrel": figure_squirrel, "sheep": figure_sheep,
}


# ---------------- Props ----------------

def prop_float():
    img, d = canvas(26, 12)
    d.ellipse(box(0.5, 0.5, 25.5, 11.5), fill=rgb(0xF26D8F))
    for a in (20, 110, 200, 290):
        d.pieslice(box(0.5, 0.5, 25.5, 11.5), a, a + 30, fill=rgb(0xFFFFFF))
    d.ellipse(box(7.5, 3.5, 18.5, 8.5), fill=(0, 0, 0, 0))
    return outlined(img)


def prop_ball():
    img, d = canvas(15, 15)
    for i, color in enumerate([0xE5534B, 0xFFFFFF, 0x3F86D6, 0xFFD45C, 0xFFFFFF, 0x4C9A6A]):
        d.pieslice(box(0.5, 0.5, 14.5, 14.5), i * 60 - 90, i * 60 - 30, fill=rgb(color))
    d.ellipse(box(6, 6, 9, 9), fill=rgb(0xFFFFFF))
    d.ellipse(box(3, 3, 5.5, 5), fill=(255, 255, 255, 200))
    return outlined(img)


def prop_sandcastle():
    img, d = canvas(32, 26)
    sand, dark = rgb(0xE8C27A), rgb(0xD2A55A)
    d.rectangle(box(4, 13, 28, 26), fill=sand)
    for x1, x2, top in [(2, 10, 8), (22, 30, 8), (11, 21, 4)]:
        d.rectangle(box(x1, top, x2, 26), fill=sand)
        for cx in range(int(x1), int(x2), 3):
            d.rectangle(box(cx, top - 2, cx + 1.6, top), fill=sand)
    d.rectangle(box(2, 21, 30, 22), fill=dark)
    d.pieslice(box(13, 16, 19, 26), 180, 360, fill=rgb(0x9C6A3E))
    d.rectangle(box(13, 21, 19, 26), fill=rgb(0x9C6A3E))
    d.line(S((16, 2), (16, -3 + 3)), fill=rgb(0x7A5030), width=s(0.8))
    d.line(S((16, 4), (16, 0.5)), fill=rgb(0x7A5030), width=s(0.8))
    d.polygon(S((16.5, 0.5), (21, 1.8), (16.5, 3.2)), fill=rgb(0xE5534B))
    return outlined(img)


def prop_book():
    img, d = canvas(24, 14)
    d.polygon(S((1, 3), (12, 5), (23, 3), (23, 13), (12, 14), (1, 13)), fill=rgb(0x3F86D6))
    d.polygon(S((2, 1.5), (12, 4), (12, 12.5), (2, 11)), fill=rgb(0xFFF6E2))
    d.polygon(S((22, 1.5), (12, 4), (12, 12.5), (22, 11)), fill=rgb(0xFBEBCB))
    for side in (-1, 1):
        for row in range(3):
            y = 5.5 + row * 2.2
            d.line(S((12 + side * 2.5, y + 0.6), (12 + side * 8, y - 0.6)), fill=rgb(0xD9C29A), width=s(0.6))
    return outlined(img)


def prop_blocks():
    img, d = canvas(24, 30)
    for x1, y1, color in [(1, 19, 0xE5534B), (12, 19, 0x3F86D6), (6.5, 8, 0xFFD45C)]:
        d.rectangle(box(x1, y1, x1 + 11, y1 + 11), fill=rgb(color))
        d.rectangle(box(x1 + 2.5, y1 + 2.5, x1 + 8.5, y1 + 8.5), outline=lerp(rgb(color), (255, 255, 255), 0.5), width=s(0.8))
    d.polygon(S((6.5, 8), (17.5, 8), (12, 0.5)), fill=rgb(0x4C9A6A))
    return outlined(img)


def prop_pancakes():
    img, d = canvas(26, 18)
    d.ellipse(box(0.5, 12, 25.5, 17.5), fill=rgb(0xFFFFFF))
    for y in (11, 8, 5):
        d.ellipse(box(3, y - 1, 23, y + 4), fill=rgb(0xC98B4F))
        d.ellipse(box(3, y - 2, 23, y + 2.5), fill=rgb(0xE8B04A))
    d.polygon(S((9, 3), (17, 3), (18, 7), (16, 9), (14, 6), (12, 9.5), (10, 6)), fill=rgb(0xF2A30F))
    d.rectangle(box(11, 1, 15, 3.5), fill=rgb(0xFFF3A0))
    return outlined(img)


def prop_cake():
    img, d = canvas(22, 26)
    d.ellipse(box(0.5, 21, 21.5, 25.5), fill=rgb(0xFFFFFF))
    d.rounded_rectangle(box(3, 14, 19, 23), radius=s(1.5), fill=rgb(0xF5A9C8))
    d.rounded_rectangle(box(5, 8, 17, 15), radius=s(1.5), fill=rgb(0xF5A9C8))
    for y1, x1, x2 in [(14, 3, 19), (8, 5, 17)]:
        d.rectangle(box(x1, y1, x2, y1 + 1.6), fill=rgb(0xFFFFFF))
        for x in range(int(x1) + 1, int(x2), 3):
            d.ellipse(box(x, y1, x + 2, y1 + 3.5), fill=rgb(0xFFFFFF))
    d.rectangle(box(10.2, 2.5, 11.8, 8), fill=rgb(0x3F86D6))
    d.ellipse(box(9.5, -0.5 + 0.5, 12.5, 3.5), fill=rgb(0xFFD45C))
    return outlined(img)


def prop_food():
    img, d = canvas(28, 16)
    d.ellipse(box(0.5, 10, 27.5, 15.5), fill=rgb(0xFFFFFF))
    d.ellipse(box(3, 11, 25, 14.5), outline=rgb(0x8EC5F0), width=s(0.6))
    d.polygon(S((3, 12), (13, 12), (8, 2)), fill=rgb(0x6DBE4F))
    d.polygon(S((3.5, 11), (12.5, 11), (8, 3)), fill=rgb(0xE8C27A))
    d.line(S((5, 9), (11, 9)), fill=rgb(0xFFD45C), width=s(1))
    d.ellipse(box(15, 3.5, 24, 12), fill=rgb(0xE5534B))
    d.line(S((19.5, 4), (19.5, 1.5)), fill=rgb(0x7A5030), width=s(0.8))
    d.ellipse(box(20, 0.5, 23.5, 2.8), fill=rgb(0x4C9A6A))
    return outlined(img)


def prop_paints():
    img, d = canvas(26, 16)
    d.ellipse(box(0.5, 2, 21, 15.5), fill=rgb(0xE2BC8F))
    d.ellipse(box(3, 9, 7, 13), fill=(0, 0, 0, 0))
    for x, y, color in [(5, 5, 0xE5534B), (10, 4, 0xFFD45C), (15, 5.5, 0x3F86D6), (16, 10.5, 0x4C9A6A), (11, 11.5, 0xE86FB0)]:
        d.ellipse(box(x - 1.8, y - 1.6, x + 1.8, y + 1.6), fill=rgb(color))
    d.line(S((14, 15), (25, 1.5)), fill=rgb(0xB0703A), width=s(1.2))
    d.ellipse(box(23, 0, 26, 3.5), fill=rgb(0xE5534B))
    return outlined(img)


def prop_chest():
    img, d = canvas(26, 22)
    wood, dark, gold = rgb(0x9C6A3E), rgb(0x7A5030), rgb(0xFFD45C)
    d.rectangle(box(1, 9, 25, 21), fill=wood)
    d.chord(box(1, 1, 25, 17), 180, 360, fill=dark)
    d.rectangle(box(1, 8, 25, 10), fill=gold)
    for x in (5, 21):
        d.rectangle(box(x - 1, 2.5, x + 1, 21), fill=gold)
    d.rounded_rectangle(box(10.5, 7.5, 15.5, 13.5), radius=s(1), fill=gold)
    d.ellipse(box(12.3, 9.5, 13.7, 11), fill=dark)
    return outlined(img)


def prop_door():
    """The magic door: arched, purple, with a golden frame and a glow."""
    img, d = canvas(32, 50)
    halo = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(halo).rounded_rectangle(box(4, 3, 28, 49), radius=s(12), fill=(255, 214, 140, 150))
    img.alpha_composite(halo.filter(ImageFilter.GaussianBlur(s(2.5))))
    d = ImageDraw.Draw(img)
    gold, purple = rgb(0xFFD45C), rgb(0x6E5AA8)
    d.rounded_rectangle(box(6, 5, 26, 49), radius=s(10), fill=gold)
    d.rounded_rectangle(box(8, 7, 24, 49), radius=s(8), fill=purple)
    d.rounded_rectangle(box(11, 12, 21, 26), radius=s(5), outline=lerp(purple, (255, 255, 255), 0.35), width=s(1))
    d.rounded_rectangle(box(11, 30, 21, 45), radius=s(1.5), outline=lerp(purple, (255, 255, 255), 0.35), width=s(1))
    d.ellipse(box(19.5, 27, 22.5, 30), fill=gold)
    for x, y, size in [(3, 12, 2.5), (29, 20, 2), (2, 34, 1.8), (30, 40, 2.4)]:
        sparkle(d, x, y, size, (255, 236, 170, 255))
    return img


PROPS = {
    "float": prop_float, "ball": prop_ball, "sandcastle": prop_sandcastle, "book": prop_book,
    "blocks": prop_blocks, "pancakes": prop_pancakes, "cake": prop_cake, "food": prop_food,
    "paints": prop_paints, "chest": prop_chest, "door": prop_door,
}


# ---------------- Small moving bits ----------------

def fx_heart():
    img, d = canvas(10, 9)
    d.ellipse(box(0.5, 0.5, 5.5, 5.5), fill=rgb(0xE5534B))
    d.ellipse(box(4.5, 0.5, 9.5, 5.5), fill=rgb(0xE5534B))
    d.polygon(S((0.8, 3.8), (9.2, 3.8), (5, 8.6)), fill=rgb(0xE5534B))
    d.ellipse(box(2, 1.6, 3.6, 3), fill=(255, 255, 255, 180))
    return img


def fx_note():
    img, d = canvas(9, 12)
    color = rgb(0x5B4E8A)
    d.ellipse(box(0.5, 7.5, 5.5, 11.5), fill=color)
    d.line(S((5, 9.5), (5, 1)), fill=color, width=s(1.1))
    d.polygon(S((5, 0.5), (8.5, 3), (8.5, 5.5), (5, 3.5)), fill=color)
    return img


def fx_butterfly():
    img, d = canvas(13, 10)
    d.ellipse(box(0.5, 0.5, 6.2, 6), fill=rgb(0x5DA9E9))
    d.ellipse(box(6.8, 0.5, 12.5, 6), fill=rgb(0x5DA9E9))
    d.ellipse(box(1.5, 5, 6, 9.5), fill=rgb(0xE86FB0))
    d.ellipse(box(7, 5, 11.5, 9.5), fill=rgb(0xE86FB0))
    d.line(S((6.5, 1.5), (6.5, 9)), fill=rgb(0x3B2F25), width=s(1))
    return img


def fx_fish():
    img, d = canvas(13, 8)
    d.polygon(S((8, 4), (12.5, 0.5), (12.5, 7.5)), fill=rgb(0xF28C28))
    d.ellipse(box(0.5, 1, 10, 7), fill=rgb(0xFFB04A))
    d.ellipse(box(2.4, 2.8, 3.8, 4.2), fill=rgb(0x3B2F25))
    return img


def fx_bird():
    img, d = canvas(12, 6)
    d.arc(box(0.5, 1, 6.5, 9), 200, 330, fill=rgb(0x3B2F25), width=s(1.1))
    d.arc(box(5.5, 1, 11.5, 9), 210, 340, fill=rgb(0x3B2F25), width=s(1.1))
    return img


FX = {"heart": fx_heart, "note": fx_note, "butterfly": fx_butterfly, "fish": fx_fish, "bird": fx_bird}


# ---------------- Output ----------------

PAPER = 0xFBF6EA  # page background behind the scene
RADIUS = 10


def round_corners(img):
    """Bake rounded corners on the page color; clipping at runtime would need a big layer buffer."""
    mask = Image.new("L", (s(W), s(H)), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, s(W) - 1, s(H) - 1], radius=s(RADIUS), fill=255)
    out = Image.new("RGBA", (W, H), rgb(PAPER) + (255,))
    out.paste(img, (0, 0), mask.resize((W, H), Image.LANCZOS))
    return out


def c_array(data):
    return "\n".join("  " + ", ".join(f"0x{v:02x}" for v in data[i:i + 24]) + "," for i in range(0, len(data), 24))


def write_c(name, img, rgb565, path, what):
    w, h = img.size
    data = bytearray()
    if rgb565:
        for r, g, b in img.convert("RGB").getdata():
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            data += bytes((v & 255, v >> 8))
    else:
        for r, g, b, a in img.getdata():
            data += bytes((b, g, r, a))
    cf, bpp = ("LV_COLOR_FORMAT_RGB565", 2) if rgb565 else ("LV_COLOR_FORMAT_ARGB8888", 4)
    with open(path, "w") as f:
        f.write(f"""/**
 * @file {name}.c
 * @brief {w}x{h} {what} (generated by scripts/gen_scenes.py)
 */

#include "lvgl.h"

static const uint8_t {name}_map[] = {{
{c_array(data)}
}};

const lv_image_dsc_t {name} = {{
  .header.cf = {cf},
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.w = {w},
  .header.h = {h},
  .header.stride = {w * bpp},
  .data_size = sizeof({name}_map),
  .data = {name}_map,
}};
""")


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_dir = os.path.join(root, "src", "ui", "images")
    scenes, sprites = {}, {}
    for name, draw in SCENES.items():
        img = Image.new("RGBA", (s(W), s(H)))
        draw(img)
        scenes[name] = round_corners(img.resize((W, H), Image.LANCZOS))
        write_c(f"scene_{name}", scenes[name], True, os.path.join(out_dir, f"scene_{name}.c"), f'"{name}" story scene, RGB565')
    for kind, table in (("figure", FIGURES), ("prop", PROPS), ("fx", FX)):
        for name, draw in table.items():
            big = draw()
            img = big.resize((big.width // SS, big.height // SS), Image.LANCZOS)
            sprites[f"{kind}_{name}"] = img
            write_c(f"{kind}_{name}", img, False, os.path.join(out_dir, f"{kind}_{name}.c"), f'"{name}" {kind}, ARGB8888')
    print(f"wrote {len(scenes)} scenes and {len(sprites)} sprites to src/ui/images/")

    if "--preview" in sys.argv:
        # Every scene with a few figures where the app puts them, then all the sprites, 2x
        cols = 3
        rows = math.ceil(len(scenes) / cols)
        sheet = Image.new("RGB", (cols * (W + 6), rows * (H + 6) + 90), (255, 255, 255))
        casts = [["mom", "zoe", "ted"], ["girl", "girl2", "zoe"], ["grandpa", "zoe", "ted"], ["dad", "boy", "teacher"]]
        for i, img in enumerate(scenes.values()):
            tile = img.copy()
            x = 212
            for name in reversed(casts[i % len(casts)]):
                fig = sprites[f"figure_{name}"]
                x -= fig.width - 2
                tile.alpha_composite(fig, (x, 92 - fig.height))
            sheet.paste(tile.convert("RGB"), ((i % cols) * (W + 6), (i // cols) * (H + 6)))
        x, y = 4, rows * (H + 6) + 4
        for name, img in sprites.items():
            if x + img.width > sheet.width - 4:
                x, y = 4, y + 48
            sheet.paste(img, (x, y), img)
            x += img.width + 6
        sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save("scenes.png")
        print("wrote scenes.png")


if __name__ == "__main__":
    main()
