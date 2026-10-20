#!/usr/bin/env python3
"""Compose app store screenshots from the window captures of store_shots.py.

  python tools/store_compose.py <store_dir>

Reads <store_dir>/raw/<scene>.png and writes <store_dir>/screenshot_<n>.png
(2560x1440, 16:9): gradient background, caption, and the app window with
rounded corners and a soft shadow. The market requires 16:9, at least
1920x1080, PNG/JPG under 5 MB.
"""
import os
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 2560, 1440
TOP, BOTTOM = (64, 118, 200), (30, 66, 138)
FONT_BOLD = r'C:\Windows\Fonts\msyhbd.ttc'
FONT_REGULAR = r'C:\Windows\Fonts\msyh.ttc'

SCENES = [
    ('1_advanced', '高精度科学计算',
     '基于 GNU MPFR/MPC 的任意精度运算，支持三角、对数、复数、阶乘与历史记录'),
    ('2_programming', '程序员模式',
     '二、八、十、十六进制一键切换，64 位位编辑器，与、或、异或、移位、补码'),
    ('3_financial', '财务计算与货币换算',
     '贷款还款、折旧、终值现值等 10 种财务函数，IMF / ECB 汇率换算'),
    ('4_basic', '简洁高效，键盘友好',
     '完整的 PC 键盘操作与快捷键，撤销重做、千位分隔、深色模式'),
]


def background():
    img = Image.new('RGB', (W, H))
    draw = ImageDraw.Draw(img)
    for y in range(H):
        t = y / (H - 1)
        draw.line([(0, y), (W, y)], fill=tuple(int(TOP[i] + (BOTTOM[i] - TOP[i]) * t) for i in range(3)))
    return img


def rounded(img, radius):
    mask = Image.new('L', img.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, img.width - 1, img.height - 1), radius=radius, fill=255)
    out = img.convert('RGBA')
    out.putalpha(mask)
    return out


def compose(raw_path, title, subtitle, out_path):
    canvas = background().convert('RGBA')
    draw = ImageDraw.Draw(canvas)
    title_font = ImageFont.truetype(FONT_BOLD, 92)
    sub_font = ImageFont.truetype(FONT_REGULAR, 44)
    draw.text((W // 2, 105), title, font=title_font, fill=(255, 255, 255), anchor='mm')
    draw.text((W // 2, 200), subtitle, font=sub_font, fill=(225, 234, 250), anchor='mm')

    window = Image.open(raw_path).convert('RGB')
    area_w, area_h = W - 320, H - 290 - 60
    scale = min(area_w / window.width, area_h / window.height)
    window = window.resize((int(window.width * scale), int(window.height * scale)), Image.LANCZOS)
    window = rounded(window, int(18 * scale * 2))
    x = (W - window.width) // 2
    y = 290 + (area_h - window.height) // 2

    shadow = Image.new('RGBA', canvas.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rounded_rectangle((x + 10, y + 24, x + window.width + 10, y + window.height + 24),
                                             radius=40, fill=(0, 0, 0, 110))
    canvas = Image.alpha_composite(canvas, shadow.filter(ImageFilter.GaussianBlur(28)))
    canvas.alpha_composite(window, (x, y))
    canvas.convert('RGB').save(out_path, optimize=True)


def main(store_dir):
    for n, (scene, title, subtitle) in enumerate(SCENES, 1):
        out = os.path.join(store_dir, f'screenshot_{n}.png')
        compose(os.path.join(store_dir, 'raw', f'{scene}.png'), title, subtitle, out)
        print(out, os.path.getsize(out) // 1024, 'KB')


if __name__ == '__main__':
    main(sys.argv[1])
