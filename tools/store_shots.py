#!/usr/bin/env python3
"""Stage showcase scenes on the device and capture the app window.

  python tools/store_shots.py <out_dir>

Writes <out_dir>/raw/<scene>.png (the app window cropped from a full-screen
capture). tools/store_compose.py turns them into store screenshots.
"""
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))
import devtest  # noqa: E402
from devtest import HDC, click, keys, layout, nodes, attrs, bounds, type_text  # noqa: E402

CTRL, SHIFT, ALT = 2072, 2047, 2045
ENTER, ESC = 2054, 2070


def mode(name):
    click('模式')
    time.sleep(0.5)
    click(name)
    time.sleep(1.5)


def window_bounds():
    tree = layout()
    x0 = y0 = x1 = y1 = None
    for node in nodes(tree):
        a = attrs(node)
        if a.get('id') == 'ContainerModalTitleRow':
            b = bounds(node)
            x0, y0 = b[0], b[1]
        elif a.get('id') == 'ContainerModalStack':
            b = bounds(node)
            x1, y1 = b[2], b[3]
    return x0, y0, x1, y1


def capture(out_dir, name):
    time.sleep(0.8)
    box = window_bounds()
    remote = '/data/local/tmp/store_shot.png'
    subprocess.run([HDC, 'shell', 'snapshot_display', '-t', 'png', '-f', remote], capture_output=True)
    local = os.path.abspath(os.path.join(out_dir, 'raw', f'{name}_full.png'))
    subprocess.run([HDC, 'file', 'recv', remote, local], capture_output=True)
    from PIL import Image
    im = Image.open(local)
    im.crop(box).save(os.path.join(out_dir, 'raw', f'{name}.png'))
    os.remove(local)
    print(name, box)


def history(on):
    # Ctrl+Shift+H toggles the history panel; read the menu state from the layout
    tree = layout()
    shown = any(attrs(n).get('type') == 'List' for n in nodes(tree))
    if shown != on:
        keys(CTRL, SHIFT, 2024)
        time.sleep(1.2)


def main(out_dir):
    os.makedirs(os.path.join(out_dir, 'raw'), exist_ok=True)

    # 1. Advanced mode with history: scientific expressions and complex numbers
    mode('高级')
    history(True)
    keys(CTRL, SHIFT, 2071)  # Clear History
    keys(ESC); type_text('2^64'); keys(ENTER)
    keys(ESC); type_text('1/7'); keys(ENTER)
    keys(ESC); type_text('ln'); type_text('(10)'); keys(ENTER)
    keys(ESC); type_text('sin30+'); keys(CTRL, 2034); type_text('2*'); keys(CTRL, 2032)
    capture(out_dir, '1_advanced')

    # 2. Programming mode: hexadecimal value in the 64-bit editor
    history(False)
    mode('编程')
    keys(CTRL, 2024)  # Ctrl+H hexadecimal
    keys(ESC); type_text('DEADBEEF'); keys(ENTER)
    capture(out_dir, '2_programming')
    keys(CTRL, 2020)  # back to decimal

    # 3. Financial mode: currency conversion and the periodic payment dialog
    mode('财务')
    keys(ESC); type_text('1000')
    time.sleep(1.0)
    click('Pmt')
    time.sleep(0.8)
    fields = [n for n in nodes(layout()) if attrs(n).get('type') == 'TextInput']
    for node, value in zip(fields, ('300000', '0.004', '360')):
        b = bounds(node)
        subprocess.run([HDC, 'shell', 'uitest', 'uiInput', 'inputText', str((b[0] + b[2]) // 2),
                        str((b[1] + b[3]) // 2), value], capture_output=True)
        time.sleep(0.3)
    capture(out_dir, '3_financial')
    keys(ENTER)  # calculate
    time.sleep(0.5)

    # 4. Basic mode
    mode('基本')
    keys(ESC); type_text('(1250+380)*12'); keys(ENTER)
    keys(ESC); type_text('19560*0.85')
    capture(out_dir, '4_basic')
    keys(ENTER)


if __name__ == '__main__':
    main(sys.argv[1])
