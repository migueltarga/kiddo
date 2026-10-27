#!/usr/bin/env python3
"""Draw the home screen background as an LVGL RGB565 image.

A bedtime scene: dusk sky, crescent moon, stars, rolling hills and an open
book glowing on the front hill, with the "Kiddo" wordmark. Drawn at 4x and
scaled down for smooth edges. The buttons and the tagline are LVGL widgets
on top (the tagline is translated), so their spots are kept clear.

Needs Pillow and macOS's SF Rounded font (system python3 has both):

    python3 scripts/gen_home.py            # writes src/ui/images/image_home.c
    python3 scripts/gen_home.py --preview  # also writes image_home.png
"""

import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 240, 320
SS = 4  # supersampling
FONT = "/System/Library/Fonts/SFNSRounded.ttf"


def lerp(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def hexrgb(h):
    return ((h >> 16) & 255, (h >> 8) & 255, h & 255)


def s(v):
    """Scale a final-pixel measure to the supersampled canvas."""
    return round(v * SS)


def sky(img):
    stops = [(0.0, 0x1C1F4A), (0.45, 0x3B3577), (0.66, 0x7B5A93), (0.76, 0xD88A86), (0.82, 0xF4B27A)]
    draw = ImageDraw.Draw(img)
    for y in range(img.height):
        t = y / img.height
        for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
            if t <= t1:
                color = lerp(hexrgb(c0), hexrgb(c1), (t - t0) / (t1 - t0))
                break
        else:
            color = hexrgb(stops[-1][1])
        draw.line([(0, y), (img.width, y)], fill=color)


def glow(img, cx, cy, radius, color, strength):
    """Soft round light, added on top of the image."""
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(layer).ellipse(
        [s(cx - radius), s(cy - radius), s(cx + radius), s(cy + radius)], fill=color + (strength,))
    layer = layer.filter(ImageFilter.GaussianBlur(s(radius) / 2))
    img.alpha_composite(layer)


def moon(img, cx, cy, r):
    glow(img, cx, cy, r * 2.2, (255, 236, 190), 70)
    mask = Image.new("L", img.size, 0)
    d = ImageDraw.Draw(mask)
    d.ellipse([s(cx - r), s(cy - r), s(cx + r), s(cy + r)], fill=255)
    # The bite that makes the crescent
    bx, by, br = cx + r * 0.55, cy - r * 0.35, r * 0.9
    d.ellipse([s(bx - br), s(by - br), s(bx + br), s(by + br)], fill=0)
    img.paste(Image.new("RGBA", img.size, (255, 243, 214, 255)), (0, 0), mask)


def sparkle(draw, x, y, size, color):
    """Four-point star."""
    k = size * 0.28
    pts = [(x, y - size), (x + k, y - k), (x + size, y), (x + k, y + k),
           (x, y + size), (x - k, y + k), (x - size, y), (x - k, y - k)]
    draw.polygon([(s(px), s(py)) for px, py in pts], fill=color)


def stars(img):
    rng = random.Random(7)
    draw = ImageDraw.Draw(img)
    placed = 0
    while placed < 38:
        x, y = rng.uniform(6, W - 6), rng.uniform(6, 175)
        if math.hypot(x - 178, y - 62) < 46:      # moon
            continue
        if 30 < x < 210 and 92 < y < 178:         # wordmark and tagline
            continue
        r = rng.choice([0.6, 0.8, 0.8, 1.1])
        a = rng.randint(140, 255)
        draw.ellipse([s(x - r), s(y - r), s(x + r), s(y + r)], fill=(255, 246, 220, a))
        placed += 1
    for x, y, size in [(52, 46, 5), (112, 26, 3.5), (214, 140, 4), (20, 158, 3)]:
        sparkle(draw, x, y, size, (255, 236, 180, 255))


def hill(img, base, amp, freq, phase, color):
    pts = [(0, s(H))]
    for x in range(0, W + 1, 2):
        y = base - amp * math.sin(x / W * math.pi * freq + phase)
        pts.append((s(x), s(y)))
    pts.append((s(W), s(H)))
    ImageDraw.Draw(img).polygon(pts, fill=color)


def tree(draw, x, ground, h, color):
    draw.rectangle([s(x - 1), s(ground - h * 0.45), s(x + 1), s(ground + 2)], fill=color)
    r = h * 0.32
    draw.ellipse([s(x - r), s(ground - h), s(x + r), s(ground - h + 2 * r)], fill=color)


def book(img, cx, top):
    """Open book seen from the front, pages curving up from the spine."""
    glow(img, cx, top - 6, 46, (255, 214, 140), 120)
    draw = ImageDraw.Draw(img)

    def page(side):
        pts = []
        for i in range(21):  # top edge, spine to outer corner
            t = i / 20
            x = cx + side * 34 * t
            y = top + 5 - 9 * math.sin(t * math.pi * 0.85)
            pts.append((x, y))
        pts += [(cx + side * 34, top + 22), (cx, top + 26)]
        return [(s(x), s(y)) for x, y in pts]

    # Cover peeking out under the pages
    cover = [(cx - 38, top + 4), (cx, top + 9), (cx + 38, top + 4), (cx + 38, top + 25), (cx, top + 30), (cx - 38, top + 25)]
    draw.polygon([(s(x), s(y)) for x, y in cover], fill=hexrgb(0xD2701A))
    draw.polygon(page(-1), fill=hexrgb(0xFFF6E2))
    draw.polygon(page(1), fill=hexrgb(0xFBEBCB))
    draw.line([(s(cx), s(top + 5)), (s(cx), s(top + 26))], fill=hexrgb(0xE2C79A), width=s(1))
    # Lines of text
    for side in (-1, 1):
        for row in range(3):
            y = top + 11 + row * 4
            draw.line([(s(cx + side * 6), s(y - 1)), (s(cx + side * 27), s(y - 3 + row * 0.3))],
                      fill=hexrgb(0xE5D2AE), width=s(1))

    # Magic drifting up from the pages
    for x, y, size in [(cx - 12, top - 14, 3), (cx + 8, top - 24, 4), (cx - 2, top - 40, 2.5), (cx + 20, top - 10, 2)]:
        sparkle(draw, x, y, size, (255, 236, 170, 255))


def wordmark(img, text, cy):
    font = ImageFont.truetype(FONT, s(50))
    try:
        font.set_variation_by_name("Heavy")
    except (OSError, ValueError):
        pass
    draw = ImageDraw.Draw(img)
    box = draw.textbbox((0, 0), text, font=font)
    x = (img.width - (box[2] - box[0])) / 2 - box[0]
    y = s(cy) - (box[3] - box[1]) / 2 - box[1]

    shadow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).text((x, y + s(3)), text, font=font, fill=(18, 14, 48, 170))
    img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(s(2.5))))
    draw.text((x, y), text, font=font, fill=hexrgb(0xFFF3D6))


def render():
    img = Image.new("RGBA", (s(W), s(H)))
    sky(img)
    stars(img)
    moon(img, 178, 62, 24)
    wordmark(img, "Kiddo", 124)

    hill(img, 232, 14, 1.6, 0.6, hexrgb(0x3E3474))
    draw = ImageDraw.Draw(img)
    for x, g, h in [(30, 222, 22), (44, 226, 15), (196, 224, 20), (210, 228, 13)]:
        tree(draw, x, g, h, hexrgb(0x2E2760))
    hill(img, 258, 16, 1.1, 2.4, hexrgb(0x2B2560))
    hill(img, 286, 20, 0.9, -0.2, hexrgb(0x1E1A45))
    book(img, 120, 226)
    return img.resize((W, H), Image.LANCZOS).convert("RGB")


def write_c(img, path):
    data = bytearray()
    for r, g, b in img.getdata():
        v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        data += bytes((v & 255, v >> 8))
    rows = []
    for i in range(0, len(data), 24):
        rows.append("  " + ", ".join(f"0x{v:02x}" for v in data[i:i + 24]) + ",")
    with open(path, "w") as f:
        f.write(f"""/**
 * @file image_home.c
 * @brief {W}x{H} home screen background, RGB565 (generated by scripts/gen_home.py)
 */

#include "lvgl.h"

static const uint8_t image_home_map[] = {{
{chr(10).join(rows)}
}};

const lv_image_dsc_t image_home = {{
  .header.cf = LV_COLOR_FORMAT_RGB565,
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.w = {W},
  .header.h = {H},
  .header.stride = {W * 2},
  .data_size = sizeof(image_home_map),
  .data = image_home_map,
}};
""")


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    img = render()
    write_c(img, os.path.join(root, "src", "ui", "images", "image_home.c"))
    print("wrote src/ui/images/image_home.c")
    if "--preview" in sys.argv:
        img.save("image_home.png")
        print("wrote image_home.png")


if __name__ == "__main__":
    main()
