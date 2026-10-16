#!/usr/bin/env python3
"""Convert mate-calc gettext catalogs into JSON tables for the HarmonyOS app.

Usage: po2json.py <po_dir> <out_dir>

Each <lang>.po becomes <out_dir>/<lang>.json mapping msgid -> msgstr
(context entries use "context\\u0004msgid" keys, like gettext). Fuzzy and
untranslated entries are skipped. Only messages the HarmonyOS UI can show are
kept (help/appdata/desktop strings are dropped) to keep the tables small.
"""

import json
import os
import re
import sys

# Source files whose strings are not used by the HarmonyOS port
SKIP_SOURCES = ('help/', 'data/', '.desktop', '.appdata', 'mate-calc-cmd.c', 'mate-calc.c:')


def unquote(s):
    s = s.strip()
    if not (s.startswith('"') and s.endswith('"')):
        return ''
    body = s[1:-1]
    return bytes(body, 'utf-8').decode('unicode_escape').encode('latin-1').decode('utf-8')


def parse_po(path):
    entries = []
    current = {}
    field = None

    def flush():
        if 'msgid' in current:
            entries.append(dict(current))
        current.clear()

    with open(path, encoding='utf-8') as f:
        for raw in f:
            line = raw.rstrip('\n')
            if not line.strip():
                flush()
                field = None
                continue
            if line.startswith('#,'):
                if 'msgid' in current:
                    flush()
                current['flags'] = line[2:]
                continue
            if line.startswith('#:'):
                if 'msgid' in current:
                    flush()
                current['refs'] = current.get('refs', '') + line[2:]
                continue
            if line.startswith('#'):
                continue
            m = re.match(r'^(msgctxt|msgid_plural|msgid|msgstr(?:\[\d+\])?)\s+(".*")$', line)
            if m:
                key, value = m.group(1), m.group(2)
                if key == 'msgctxt' and 'msgid' in current:
                    flush()
                field = key
                current[field] = unquote(value)
            elif line.startswith('"') and field:
                current[field] += unquote(line)
    flush()
    return entries


def convert(po_path):
    table = {}
    for e in parse_po(po_path):
        msgid = e.get('msgid', '')
        if not msgid:
            continue
        if 'fuzzy' in e.get('flags', ''):
            continue
        refs = e.get('refs', '')
        if refs and all(any(skip in ref for skip in SKIP_SOURCES) for ref in refs.split()):
            continue
        msgstr = e.get('msgstr', e.get('msgstr[0]', ''))
        if not msgstr:
            continue
        key = msgid
        if 'msgctxt' in e:
            key = e['msgctxt'] + '\u0004' + msgid
        table[key] = msgstr
        # Plural forms: keep the "other" form under the plural msgid
        plural = e.get('msgid_plural')
        if plural and plural not in table:
            table[plural] = e.get('msgstr[1]') or msgstr
    return table


def main():
    po_dir, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)
    languages = []
    for name in sorted(os.listdir(po_dir)):
        if not name.endswith('.po'):
            continue
        lang = name[:-3]
        table = convert(os.path.join(po_dir, name))
        if len(table) < 50:
            continue
        with open(os.path.join(out_dir, lang + '.json'), 'w', encoding='utf-8', newline='\n') as f:
            json.dump(table, f, ensure_ascii=False, separators=(',', ':'), sort_keys=True)
        languages.append(lang)
    with open(os.path.join(out_dir, 'languages.json'), 'w', encoding='utf-8', newline='\n') as f:
        json.dump(languages, f)
    print('%d languages written to %s' % (len(languages), out_dir))


if __name__ == '__main__':
    main()
