#!/usr/bin/env python3
"""Draw the layered app icon (1024x1024 foreground/background) and the start
window icon for the HarmonyOS port, after the MATE calculator icon design."""
import sys
from PIL import Image, ImageDraw

S = 1024


def rounded(draw, box, radius, fill):
    draw.rounded_rectangle(box, radius=radius, fill=fill)


def background():
    img = Image.new('RGBA', (S, S))
    top, bottom = (88, 141, 214), (46, 93, 170)
    draw = ImageDraw.Draw(img)
    for y in range(S):
        t = y / (S - 1)
        draw.line([(0, y), (S, y)], fill=tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3)) + (255,))
    return img


def calculator(size):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    k = size / 1024.0
    # Body (inside the ~2/3 safe zone of layered icons)
    x0, y0, x1, y1 = 262 * k, 196 * k, 762 * k, 828 * k
    rounded(d, (x0 + 10 * k, y0 + 16 * k, x1 + 10 * k, y1 + 16 * k), 60 * k, (0, 0, 0, 60))
    rounded(d, (x0, y0, x1, y1), 60 * k, (236, 238, 241, 255))
    rounded(d, (x0, y0, x1, y1 - 8 * k), 60 * k, (247, 248, 250, 255))
    # Display
    rounded(d, (300 * k, 238 * k, 724 * k, 368 * k), 22 * k, (64, 74, 66, 255))
    rounded(d, (312 * k, 250 * k, 712 * k, 356 * k), 16 * k, (174, 198, 160, 255))
    # Digits on the display: "42"
    seg = (52, 62, 50, 255)
    w = 14 * k
    # 4
    d.rectangle((566 * k, 270 * k, 566 * k + w, 306 * k), fill=seg)
    d.rectangle((566 * k, 300 * k, 616 * k, 300 * k + w), fill=seg)
    d.rectangle((602 * k, 270 * k, 602 * k + w, 338 * k), fill=seg)
    # 2
    d.rectangle((640 * k, 270 * k, 690 * k, 270 * k + w), fill=seg)
    d.rectangle((676 * k, 270 * k, 676 * k + w, 306 * k), fill=seg)
    d.rectangle((640 * k, 300 * k, 690 * k, 300 * k + w), fill=seg)
    d.rectangle((640 * k, 300 * k, 640 * k + w, 338 * k), fill=seg)
    d.rectangle((640 * k, 326 * k, 690 * k, 326 * k + w), fill=seg)
    # Keys 4 x 4
    colors = [(255, 255, 255, 255)] * 3 + [(214, 224, 240, 255)]
    kw, kh, gap = 90 * k, 82 * k, 18 * k
    for row in range(4):
        for col in range(4):
            kx = 300 * k + col * (kw + gap)
            ky = 406 * k + row * (kh + gap)
            fill = colors[col]
            if row == 3 and col == 3:
                fill = (230, 120, 60, 255)
            rounded(d, (kx, ky + 6 * k, kx + kw, ky + kh + 6 * k), 16 * k, (190, 194, 200, 255))
            rounded(d, (kx, ky, kx + kw, ky + kh), 16 * k, fill)
    return img


def main(out_dir, app_dir):
    bg = background()
    fg = calculator(S)
    for d in (out_dir, app_dir):
        bg.save(f'{d}/background.png')
        fg.save(f'{d}/foreground.png')
    start = Image.alpha_composite(bg, fg)
    mask = Image.new('L', (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, S, S), radius=230, fill=255)
    start.putalpha(mask)
    start.resize((144, 144), Image.LANCZOS).save(f'{out_dir}/startIcon.png')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
