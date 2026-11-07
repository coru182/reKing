#!/usr/bin/env python3
"""The loading help screen's controller picture, in the game's own picture space: its 512x448 picture four times
larger (2048x1792), which the game stretches over the whole 16:9 screen. The app finds where the Xbox picture's face
buttons are and lays this picture out from there (ps5/controller_ps5.cpp).

  python3 tools/mkcontroller.py
      ps5/art/controller.svg (drawn as the player sees the screen, 3840x2160) into ps5/art/controller.png, the
      picture the app carries. Needs rsvg-convert (librsvg2-bin) and Pillow.
  python3 tools/mkcontroller.py --image my-controller.png [--width 1540] [--out art/controller.png]
      a picture of a controller, front view, on a white or transparent background, instead: cut out, given the
      Xbox picture's teal glow, and placed where the Xbox controller was (centred on it, its top under the game's
      L2 / R2 labels, --width pixels wide on a 4K screen). build-ps5.sh uses art/controller.png over the drawing.
      Needs Pillow and numpy."""
import argparse
import io
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
SCREEN = (3840, 2160)
PICTURE = (2048, 1792)
CENTRE_X, TOP = 1545, 392  # the Xbox controller's centre and the top of its triggers, on a 4K screen
GLOW = (47, 154, 160)


def from_svg():
    svg = ROOT / 'ps5/art/controller.svg'
    png = subprocess.run(['rsvg-convert', '-w', str(SCREEN[0]), '-h', str(SCREEN[1]), str(svg)], check=True,
                         capture_output=True).stdout
    return Image.open(io.BytesIO(png)).convert('RGB')


def cut_out(path):
    """the controller with its background made transparent: a transparent one kept, a white one flooded from the
    edges (so white inside the controller stays)"""
    import numpy as np
    im = Image.open(path).convert('RGBA')
    a = np.asarray(im).copy()
    if a[..., 3].min() < 250:
        return im
    marked = im.convert('RGB')
    w, h = marked.size
    for xy in [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)]:
        if min(marked.getpixel(xy)) >= 240:
            ImageDraw.floodfill(marked, xy, (255, 0, 255), thresh=14)
    m = np.asarray(marked)
    background = (m[..., 0] == 255) & (m[..., 1] == 0) & (m[..., 2] == 255)
    alpha = Image.fromarray(np.where(background, 0, 255).astype(np.uint8))
    alpha = alpha.filter(ImageFilter.MinFilter(3)).filter(ImageFilter.GaussianBlur(0.6))  # no white fringe
    a[..., 3] = np.asarray(alpha)
    return Image.fromarray(a)


def from_image(path, width):
    import numpy as np
    pad = cut_out(path)
    box = pad.getbbox()
    pad = pad.crop(box)
    k = width / pad.width
    size = (round(pad.width * k), round(pad.height * k))
    pad = pad.convert('RGBa').resize(size, Image.LANCZOS).convert('RGBA')
    at = (round(CENTRE_X - size[0] / 2), TOP)
    alpha = Image.new('L', SCREEN, 0)
    alpha.paste(pad.getchannel('A'), at)
    # the Xbox picture's halo: teal, wide and soft, stronger close in
    a = np.asarray(alpha).astype(np.float32) / 255
    far = np.asarray(alpha.filter(ImageFilter.GaussianBlur(95))).astype(np.float32) / 255
    near = np.asarray(alpha.filter(ImageFilter.GaussianBlur(28))).astype(np.float32) / 255
    glow = np.clip(far * 1.6 * 0.85 + near * 0.6, 0, 1)
    screen = np.zeros((SCREEN[1], SCREEN[0], 3), np.float32)
    screen[:] = glow[..., None] * np.array(GLOW, np.float32)
    layer = Image.new('RGBA', SCREEN, (0, 0, 0, 0))
    layer.paste(pad, at)
    l = np.asarray(layer).astype(np.float32)
    screen = screen * (1 - a[..., None]) + l[..., :3] * a[..., None]
    return Image.fromarray(np.clip(screen + 0.5, 0, 255).astype(np.uint8))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--image', type=Path, help='a picture of a controller to use instead of the drawing')
    p.add_argument('--width', type=int, default=1540, help='its width on a 4K screen (the Xbox one is about 1670)')
    p.add_argument('--out', type=Path, help='default: ps5/art/controller.png, or art/controller.png with --image')
    p.add_argument('--preview', type=Path, help='also the screen as the player sees it, as a PNG')
    args = p.parse_args()
    screen = from_image(args.image, args.width) if args.image else from_svg()
    out = args.out or ROOT / ('art/controller.png' if args.image else 'ps5/art/controller.png')
    out.parent.mkdir(parents=True, exist_ok=True)
    screen.resize(PICTURE, Image.LANCZOS).save(out, optimize=True)
    if args.preview:
        screen.save(args.preview)
    print(f'{out}: {out.stat().st_size // 1024} KB')


if __name__ == '__main__':
    main()
