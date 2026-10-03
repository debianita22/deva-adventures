#!/usr/bin/env python3
"""Deva's Awesome Adventures - lossless PNG packing (1.0).

An image with at most 256 different colours (RGBA, transparency included) is saved as an indexed PNG
with exactly those colours in its palette (a tRNS chunk for the alpha): decoded, it gives back the
very same RGBA values, in about a third of the bytes, and it inflates faster on the console. Images
with more colours stay RGB(A). Nothing is quantized.

  python3 tools/art/pngpal.py <file.png ...>          rewrite in place when smaller (and verify)
  from pngpal import save_png                          used by build_art.py for every PNG it writes
"""
import io
import os
import sys

import numpy as np
from PIL import Image


def indexed(rgba):
    """(palette image, None) for <= 256 colours, else (None, reason)."""
    flat = rgba.reshape(-1, 4)
    colours, inverse = np.unique(flat, axis=0, return_inverse=True)
    if len(colours) > 256:
        return None, "%d colours" % len(colours)
    idx = inverse.reshape(rgba.shape[:2]).astype(np.uint8)
    im = Image.fromarray(idx, "P")
    pal = colours[:, :3].astype(np.uint8).flatten().tolist()
    im.putpalette(pal + [0] * (768 - len(pal)))
    alpha = colours[:, 3].astype(np.uint8)
    if (alpha < 255).any():
        im.info["transparency"] = bytes(alpha.tolist())
    return im, None


def encode(img):
    """The smallest lossless PNG bytes of a PIL image (indexed when possible)."""
    rgba = np.asarray(img.convert("RGBA"))
    has_alpha = img.mode in ("RGBA", "LA") or ("transparency" in img.info)
    best = io.BytesIO()
    (img if has_alpha else img.convert("RGB")).save(best, "PNG", optimize=True)
    pal, _ = indexed(rgba)
    if pal is not None:
        out = io.BytesIO()
        pal.save(out, "PNG", optimize=True, transparency=pal.info.get("transparency"))
        if out.tell() < best.tell():
            best = out
    return best.getvalue()


def same_pixels(a_bytes, b_bytes):
    a = np.asarray(Image.open(io.BytesIO(a_bytes)).convert("RGBA"))
    b = np.asarray(Image.open(io.BytesIO(b_bytes)).convert("RGBA"))
    return a.shape == b.shape and (a == b).all()


def save_png(img, path):
    """build_art.py: write img to path as the smallest lossless PNG."""
    with open(path, "wb") as f:
        f.write(encode(img))


def main(paths):
    saved = 0
    for p in paths:
        old = open(p, "rb").read()
        new = encode(Image.open(io.BytesIO(old)))
        if len(new) >= len(old):
            print("%-34s %8d  kept" % (os.path.basename(p), len(old)))
            continue
        if not same_pixels(old, new):
            print("%-34s pixels differ: kept" % os.path.basename(p))
            continue
        with open(p, "wb") as f:
            f.write(new)
        saved += len(old) - len(new)
        print("%-34s %8d -> %8d" % (os.path.basename(p), len(old), len(new)))
    print("saved %d bytes" % saved)


if __name__ == "__main__":
    main(sys.argv[1:])
