#!/usr/bin/env python3
"""The help screen's controller picture: ps5/art/controller.svg (drawn as the player sees it, 3840x2160) into
ps5/art/controller.png, the game's own picture space: its 512x448 picture four times larger (2048x1792), which the
game stretches over the whole 16:9 screen. Needs rsvg-convert (librsvg2-bin) and Pillow.
usage: python3 tools/mkcontroller.py"""
import io
import subprocess
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
svg = ROOT / 'ps5/art/controller.svg'
png = subprocess.run(['rsvg-convert', '-w', '3840', '-h', '2160', str(svg)], check=True, capture_output=True).stdout
screen = Image.open(io.BytesIO(png)).convert('RGB')
picture = screen.resize((2048, 1792), Image.LANCZOS)
out = ROOT / 'ps5/art/controller.png'
picture.save(out, optimize=True)
print(f'{out}: {out.stat().st_size // 1024} KB')
