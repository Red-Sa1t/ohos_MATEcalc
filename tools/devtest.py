#!/usr/bin/env python3
"""Device test driver for the MATE Calculator HarmonyOS port.

Uses `hdc shell uitest` to dump the ArkUI layout, click nodes by text/id and
inject key events, then reads back the display text.

  python tools/devtest.py dump                 # print clickable nodes
  python tools/devtest.py click <text>         # click node whose text matches
  python tools/devtest.py keys <code> [...]    # key event (up to 3 = combo)
  python tools/devtest.py type <chars>         # type characters key by key
  python tools/devtest.py display              # print display + status
  python tools/devtest.py run <script>         # run a scenario file
"""
import json
import os
import subprocess
import sys
import time

HDC = os.environ.get('HDC', r'D:\Tools\DevEco Studio\sdk\default\openharmony\toolchains\hdc.exe')
BUNDLE = 'com.neuohos.matecalc'

KEYS = {
    '0': [2000], '1': [2001], '2': [2002], '3': [2003], '4': [2004], '5': [2005], '6': [2006], '7': [2007],
    '8': [2008], '9': [2009], '+': [2047, 2058], '-': [2057], '*': [2047, 2008], '/': [2064], '.': [2044],
    '(': [2047, 2008 + 1], ')': [2047, 2000], '^': [2047, 2006], '!': [2047, 2001], '%': [2047, 2005],
    '=': [2058], ' ': [2050], ',': [2043], '|': [2047, 2061],
}
for i, ch in enumerate('abcdefghijklmnopqrstuvwxyz'):
    KEYS[ch] = [2017 + i]
    KEYS[ch.upper()] = [2047, 2017 + i]


def hdc(*args):
    result = subprocess.run([HDC, 'shell'] + list(args), capture_output=True, text=True, encoding='utf-8',
                            errors='replace')
    return result.stdout


def layout():
    hdc('uitest', 'dumpLayout', '-p', '/data/local/tmp/layout.json', '-b', BUNDLE)
    out = subprocess.run([HDC, 'shell', 'cat', '/data/local/tmp/layout.json'], capture_output=True)
    return json.loads(out.stdout.decode('utf-8', errors='replace'))


def nodes(tree):
    stack = [tree]
    while stack:
        node = stack.pop()
        yield node
        stack.extend(reversed(node.get('children', [])))


def attrs(node):
    return node.get('attributes', {})


def bounds(node):
    # "[x0,y0][x1,y1]"
    b = attrs(node).get('bounds', '')
    parts = b.replace('][', ',').strip('[]').split(',')
    if len(parts) != 4:
        return None
    return [int(p) for p in parts]


def find(tree, text, exact=True, index=0):
    matches = []
    for node in nodes(tree):
        a = attrs(node)
        for key in ('text', 'id', 'key', 'description'):
            value = a.get(key, '')
            if value and (value == text if exact else text in value):
                b = bounds(node)
                if b and b[2] > b[0] and b[3] > b[1]:
                    matches.append(node)
                break
    if len(matches) > index:
        return matches[index]
    return None


def click_node(node):
    b = bounds(node)
    x, y = (b[0] + b[2]) // 2, (b[1] + b[3]) // 2
    hdc('uitest', 'uiInput', 'click', str(x), str(y))
    time.sleep(0.25)


def click(text, index=0, tree=None):
    tree = tree or layout()
    node = find(tree, text, True, index) or find(tree, text, False, index)
    if node is None:
        raise SystemExit(f'node not found: {text!r}')
    click_node(node)


def keys(*codes):
    hdc('uitest', 'uiInput', 'keyEvent', *[str(c) for c in codes])
    time.sleep(0.15)


def type_text(text):
    for ch in text:
        if ch not in KEYS:
            raise SystemExit(f'no key for {ch!r}')
        keys(*KEYS[ch])


def display(tree=None):
    tree = tree or layout()
    # The display Text carries accessibilityText = display string; the status is the next Text
    for node in nodes(tree):
        a = attrs(node)
        if a.get('type') == 'Text' and a.get('id') == 'calc_display':
            return a.get('text', '').replace('│', '').replace('​', '')
    return ''


def status(tree=None):
    tree = tree or layout()
    for node in nodes(tree):
        a = attrs(node)
        if a.get('id') == 'calc_status':
            return a.get('text', '')
    return ''


def run(script):
    """Scenario lines: click <text> | type <chars> | keys <codes> | expect <display> | status <text> | sleep <s>"""
    failures = 0
    for lineno, raw in enumerate(open(script, encoding='utf-8'), 1):
        line = raw.rstrip('\n')
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        cmd, _, arg = line.strip().partition(' ')
        if cmd == 'click':
            click(arg)
        elif cmd == 'type':
            type_text(arg)
        elif cmd == 'keys':
            keys(*arg.split())
        elif cmd == 'sleep':
            time.sleep(float(arg))
        elif cmd in ('expect', 'status'):
            time.sleep(0.3)
            tree = layout()
            actual = display(tree) if cmd == 'expect' else status(tree)
            ok = actual == arg
            failures += 0 if ok else 1
            print(f"{'PASS' if ok else 'FAIL'} {script}:{lineno} {cmd} {arg!r} -> {actual!r}")
        else:
            raise SystemExit(f'{script}:{lineno}: unknown command {cmd}')
    return failures


def main():
    cmd = sys.argv[1]
    if cmd == 'dump':
        for node in nodes(layout()):
            a = attrs(node)
            if a.get('text') or a.get('id') or a.get('clickable') == 'true':
                print(a.get('type'), repr(a.get('id', '')), repr(a.get('text', '')), a.get('bounds'),
                      repr(a.get('description', '')))
    elif cmd == 'click':
        click(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 0)
    elif cmd == 'keys':
        keys(*sys.argv[2:])
    elif cmd == 'type':
        type_text(sys.argv[2])
    elif cmd == 'display':
        tree = layout()
        print(repr(display(tree)), repr(status(tree)))
    elif cmd == 'run':
        total = sum(run(s) for s in sys.argv[2:])
        print('FAILURES:', total)
        sys.exit(1 if total else 0)


if __name__ == '__main__':
    main()
