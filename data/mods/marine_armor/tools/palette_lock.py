"""Lock restyled unit frames to one shared palette across every body-part layer.

Frames are generated independently, so hue drifts between them - and worse,
between layers, which would give a trooper an olive torso and khaki legs. This
builds ONE master palette from the frames whose hue sits near the median of the
whole set (all layers pooled), then remaps every frame onto it. Outliers are
pulled back into the family by construction; no regeneration needed.

Usage:
  python palette_lock.py <out_dir> <src_dir> [<src_dir> ...] [--colors N]

Each <src_dir> is remapped into <out_dir>/<basename of src_dir>.
"""
import argparse
import colorsys
import os

import numpy as np
from PIL import Image


def opaque_pixels(path):
    a = np.array(Image.open(path).convert("RGBA"))
    return a[:, :, :3][a[:, :, 3] > 0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out_dir")
    ap.add_argument("src_dirs", nargs="+")
    ap.add_argument("--colors", type=int, default=24)
    ap.add_argument("--hue-tolerance", type=float, default=0.06)
    args = ap.parse_args()

    frames = []
    for d in args.src_dirs:
        for n in sorted(os.listdir(d)):
            if n.endswith(".png") and not n.startswith("_"):
                frames.append((d, n))
    if not frames:
        raise SystemExit("no frames found")

    # pass 1: mean hue per frame, pooled across all layers
    hues, cache = {}, {}
    for d, n in frames:
        px = opaque_pixels(os.path.join(d, n))
        cache[(d, n)] = px
        if len(px) < 8:
            continue
        h, _, s = colorsys.rgb_to_hsv(*(px.mean(axis=0) / 255.0))
        if s > 0.05:
            hues[(d, n)] = h
    if not hues:
        raise SystemExit("no colored frames to derive a palette from")

    median_h = float(np.median(list(hues.values())))

    def dist(h):
        d = abs(h - median_h)
        return min(d, 1.0 - d)

    inliers = [k for k, h in hues.items() if dist(h) < args.hue_tolerance]
    outliers = len(hues) - len(inliers)
    print("median hue %.3f | inliers %d | outliers %d | frames %d"
          % (median_h, len(inliers), outliers, len(frames)))

    # pass 2: one master palette from the inliers
    pool = np.concatenate([cache[k] for k in inliers]) if inliers else \
        np.concatenate([v for v in cache.values() if len(v)])
    pool = pool[:: max(1, len(pool) // 200000)]
    master = Image.fromarray(pool.reshape(-1, 1, 3).astype("uint8"), "RGB") \
        .quantize(colors=args.colors, method=Image.MEDIANCUT)
    pal = master.getpalette()[: args.colors * 3]
    pal_img = Image.new("P", (1, 1))
    pal_img.putpalette(pal + [0] * (768 - len(pal)))

    # pass 3: remap everything onto it
    for d, n in frames:
        dst_dir = os.path.join(args.out_dir, os.path.basename(d.rstrip("\\/")))
        os.makedirs(dst_dir, exist_ok=True)
        im = Image.open(os.path.join(d, n)).convert("RGBA")
        alpha = im.split()[3]
        locked = im.convert("RGB").quantize(palette=pal_img, dither=Image.NONE).convert("RGB")
        out = Image.new("RGBA", im.size, (0, 0, 0, 0))
        out.paste(locked, (0, 0))
        out.putalpha(alpha)
        out.save(os.path.join(dst_dir, n))

    print("locked %d frames across %d layers to %d shared colors -> %s"
          % (len(frames), len(args.src_dirs), args.colors, args.out_dir))


if __name__ == "__main__":
    main()
