"""Regrade the cityscape palettes toward a neon-noir 'Night City' look.

The cityscape is indexed-colour art rendered through three 256-entry palettes -
pal_01 (day), pal_02 (twilight), pal_03 (night) - which the engine interpolates
by time of day. Regrading those three files restyles every tile in the city at
once, perfectly coherently, without touching a single sprite.

The grade splits the palette by luminance:
  * dark/mid entries  -> cool desaturated blue-steel (concrete, shadow, street)
  * bright entries    -> neon (magenta / cyan / sodium), saturation pushed up
Entries 252-255 are left alone: the engine overwrites them each frame with the
pulsating owner/enemy/neutral indicators.

Palette files are raw 256*3 bytes, 6-bit VGA (0-63).

Usage: python grade_city_palette.py <src_dir> <out_dir> [--strength 1.0]
"""
import argparse
import colorsys
import os
import struct

RESERVED_FROM = 252  # engine-owned indicator colours


def read_pal(path):
    b = open(path, "rb").read()
    if len(b) != 768:
        raise SystemExit("%s: expected 768 bytes, got %d" % (path, len(b)))
    return [tuple(b[i * 3:i * 3 + 3]) for i in range(256)]


def write_pal(path, pal):
    out = bytearray()
    for r, g, b in pal:
        out += struct.pack("BBB", r & 63, g & 63, b & 63)
    open(path, "wb").write(bytes(out))


def lerp(a, b, t):
    return a + (b - a) * t


def grade_entry(rgb, variant, strength):
    """variant: 'day' | 'twilight' | 'night'."""
    r, g, b = [c / 63.0 for c in rgb]
    h, s, v = colorsys.rgb_to_hsv(r, g, b)

    if variant == "day":
        # Overcast dusk: never full daylight again.
        tint_h, tint_s, gain, lift = 0.60, 0.28, 0.72, 0.04
        neon_cut, neon_boost = 0.80, 0.55
    elif variant == "twilight":
        tint_h, tint_s, gain, lift = 0.66, 0.42, 0.55, 0.03
        neon_cut, neon_boost = 0.70, 0.85
    else:  # night
        tint_h, tint_s, gain, lift = 0.68, 0.55, 0.38, 0.02
        neon_cut, neon_boost = 0.62, 1.0

    if v >= neon_cut:
        # Bright entries become light sources. Warm hues -> sodium/magenta,
        # cool hues -> cyan; saturation pushed hard, value kept high.
        nh = 0.86 if (h < 0.12 or h > 0.88 or 0.05 < h < 0.18) else 0.50
        if 0.25 < h < 0.45:            # greens -> cyan rather than magenta
            nh = 0.48
        ns = min(1.0, lerp(s, 0.95, neon_boost))
        nv = min(1.0, lerp(v, 1.0, 0.35))
        nh = lerp(h, nh, neon_boost)
    else:
        # Everything else: cool, desaturated, darkened.
        nh = lerp(h, tint_h, 0.72)
        ns = lerp(s * 0.55, tint_s, 0.5)
        nv = max(0.0, v * gain + lift)

    nh = lerp(h, nh, strength)
    ns = lerp(s, ns, strength)
    nv = lerp(v, nv, strength)
    nr, ng, nb = colorsys.hsv_to_rgb(nh, ns, nv)
    return (int(round(nr * 63)), int(round(ng * 63)), int(round(nb * 63)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src_dir")
    ap.add_argument("out_dir")
    ap.add_argument("--strength", type=float, default=1.0)
    args = ap.parse_args()
    os.makedirs(args.out_dir, exist_ok=True)

    for name, variant in (("pal_01.dat", "day"),
                          ("pal_02.dat", "twilight"),
                          ("pal_03.dat", "night")):
        src = os.path.join(args.src_dir, name)
        pal = read_pal(src)
        out = []
        for i, entry in enumerate(pal):
            if i >= RESERVED_FROM or i == 0:
                out.append(entry)
            else:
                out.append(grade_entry(entry, variant, args.strength))
        write_pal(os.path.join(args.out_dir, name), out)
        print("graded %s (%s)" % (name, variant))


if __name__ == "__main__":
    main()
