#!/usr/bin/env python3
"""App icon for the HarmonyOS port of MATE Calculator.

The icon is described once as a list of shapes on a 1024x1024 canvas and
rendered both as PNG (4x supersampled for anti-aliasing) and as SVG.

  make_icons.py project <entry_media_dir> <appscope_media_dir>
      layered icon (foreground.png / background.png 1024x1024) and the
      144x144 start window icon used by the DevEco project
  make_icons.py export <out_dir>
      the complete icon set: full-bleed and rounded composites in several
      sizes, the layers, and SVG versions
"""
import os
import sys
from PIL import Image, ImageDraw

S = 1024
SUPERSAMPLE = 4
# HarmonyOS presents icons in a rounded square; this radius approximates it
CORNER_RADIUS = 230

BG_TOP = (88, 141, 214)
BG_BOTTOM = (46, 93, 170)


def foreground_shapes():
    """(kind, (x0, y0, x1, y1), radius, rgba) on the 1024 canvas"""
    shapes = []

    def rect(box, color, radius=0):
        shapes.append(('rect', box, radius, color))

    # Body (inside the ~2/3 safe zone of layered icons), with a soft shadow
    x0, y0, x1, y1 = 262, 196, 762, 828
    rect((x0 + 10, y0 + 16, x1 + 10, y1 + 16), (0, 0, 0, 60), 60)
    rect((x0, y0, x1, y1), (236, 238, 241, 255), 60)
    rect((x0, y0, x1, y1 - 8), (247, 248, 250, 255), 60)
    # LCD display
    rect((300, 238, 724, 368), (64, 74, 66, 255), 22)
    rect((312, 250, 712, 356), (174, 198, 160, 255), 16)
    # Seven-segment "42"
    seg = (52, 62, 50, 255)
    w = 14
    for box in [(566, 270, 566 + w, 306), (566, 300, 616, 300 + w), (602, 270, 602 + w, 338),
                (640, 270, 690, 270 + w), (676, 270, 676 + w, 306), (640, 300, 690, 300 + w),
                (640, 300, 640 + w, 338), (640, 326, 690, 326 + w)]:
        rect(box, seg)
    # 4 x 4 keys: digits white, operators light blue, "=" orange
    kw, kh, gap = 90, 82, 18
    for row in range(4):
        for col in range(4):
            kx = 300 + col * (kw + gap)
            ky = 406 + row * (kh + gap)
            fill = (214, 224, 240, 255) if col == 3 else (255, 255, 255, 255)
            if row == 3 and col == 3:
                fill = (230, 120, 60, 255)
            rect((kx, ky + 6, kx + kw, ky + kh + 6), (190, 194, 200, 255), 16)
            rect((kx, ky, kx + kw, ky + kh), fill, 16)
    return shapes


# PNG rendering

def render_background(size):
    img = Image.new('RGBA', (size, size))
    draw = ImageDraw.Draw(img)
    for y in range(size):
        t = y / (size - 1)
        color = tuple(int(BG_TOP[i] + (BG_BOTTOM[i] - BG_TOP[i]) * t) for i in range(3)) + (255,)
        draw.line([(0, y), (size, y)], fill=color)
    return img


def render_foreground(size):
    big = size * SUPERSAMPLE
    k = big / S
    img = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    # Draw each shape on its own layer so translucent shadows blend correctly
    for kind, box, radius, color in foreground_shapes():
        layer = Image.new('RGBA', (big, big), (0, 0, 0, 0))
        d = ImageDraw.Draw(layer)
        scaled = tuple(v * k for v in box)
        if radius:
            d.rounded_rectangle(scaled, radius=radius * k, fill=color)
        else:
            d.rectangle(scaled, fill=color)
        img = Image.alpha_composite(img, layer)
    return img.resize((size, size), Image.LANCZOS)


def rounded_mask(size):
    big = size * SUPERSAMPLE
    mask = Image.new('L', (big, big), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, big - 1, big - 1), radius=CORNER_RADIUS * big / S, fill=255)
    return mask.resize((size, size), Image.LANCZOS)


def composite(size, rounded):
    img = Image.alpha_composite(render_background(size), render_foreground(size))
    if rounded:
        img.putalpha(rounded_mask(size))
    return img


# SVG rendering

def svg_color(rgba):
    r, g, b, a = rgba
    return f'rgb({r},{g},{b})' + (f'" fill-opacity="{a / 255:.3f}' if a < 255 else '')


def svg_document(include_background, include_foreground, rounded):
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{S}" height="{S}" viewBox="0 0 {S} {S}">']
    parts.append('<defs><linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">'
                 f'<stop offset="0" stop-color="rgb{BG_TOP}"/><stop offset="1" stop-color="rgb{BG_BOTTOM}"/>'
                 '</linearGradient>')
    if rounded:
        parts.append(f'<clipPath id="shape"><rect width="{S}" height="{S}" rx="{CORNER_RADIUS}"/></clipPath>')
    parts.append('</defs>')
    parts.append('<g clip-path="url(#shape)">' if rounded else '<g>')
    if include_background:
        parts.append(f'<rect width="{S}" height="{S}" fill="url(#bg)"/>')
    if include_foreground:
        for kind, (x0, y0, x1, y1), radius, color in foreground_shapes():
            rx = f' rx="{radius}"' if radius else ''
            parts.append(f'<rect x="{x0}" y="{y0}" width="{x1 - x0}" height="{y1 - y0}"{rx} fill="{svg_color(color)}"/>')
    parts.append('</g></svg>')
    return '\n'.join(parts) + '\n'


def write(path, text):
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(text)


def project(entry_media, appscope_media):
    bg = render_background(S)
    fg = render_foreground(S)
    for d in (entry_media, appscope_media):
        bg.save(os.path.join(d, 'background.png'))
        fg.save(os.path.join(d, 'foreground.png'))
    composite(144, True).save(os.path.join(entry_media, 'startIcon.png'))


def export(out_dir):
    layered = os.path.join(out_dir, 'layered')
    os.makedirs(layered, exist_ok=True)
    render_background(S).save(os.path.join(layered, 'background.png'))
    render_foreground(S).save(os.path.join(layered, 'foreground.png'))
    write(os.path.join(layered, 'background.svg'), svg_document(True, False, False))
    write(os.path.join(layered, 'foreground.svg'), svg_document(False, True, False))

    for size in (1024, 512, 216, 144, 48):
        composite(size, False).save(os.path.join(out_dir, f'matecalc_{size}.png'))
        composite(size, True).save(os.path.join(out_dir, f'matecalc_{size}_rounded.png'))
    write(os.path.join(out_dir, 'matecalc.svg'), svg_document(True, True, False))
    write(os.path.join(out_dir, 'matecalc_rounded.svg'), svg_document(True, True, True))


if __name__ == '__main__':
    if sys.argv[1] == 'project':
        project(sys.argv[2], sys.argv[3])
    elif sys.argv[1] == 'export':
        export(sys.argv[2])
    else:
        raise SystemExit(__doc__)
