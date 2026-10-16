#!/usr/bin/env bash
# Usage: tools/shot.sh <name>
# Captures the device screen into docs/screenshots/<name>.jpg (half size) and
# writes a preview copy to $PREVIEW_DIR/preview.png (default: current dir).
set -e
export DEVECO_HOME="${DEVECO_HOME:-D:/Tools/DevEco Studio}"
raw="${PREVIEW_DIR:-.}/raw.png"
out="docs/screenshots/$1.jpg"
rm -f "$raw"
# devecocli refuses to overwrite an existing file
devecocli ui screenshot --path "$raw" >/dev/null 2>&1
python -c "
from PIL import Image
im = Image.open(r'$raw').convert('RGB')
im = im.resize((im.width // 2, im.height // 2), Image.LANCZOS)
im.save(r'$out', quality=85, optimize=True)
im.save(r'${PREVIEW_DIR:-.}/preview.png')"
echo "$out"
