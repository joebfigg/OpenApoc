"""Build an OpenApoc battle unit image pack from a folder of PNG frames.

Frames must be named by index (000.png, 001.png, ...) matching the frame
layout of the vanilla pack being restyled - dump that layout first with
dump_pack_frames.ps1 and keep frame indices identical, since the animation
packs address frames by position. Missing indices become empty entries,
exactly like the gaps vanilla packs contain.

Usage:
  python build_imagepack.py <frames_dir> <pack_name> [--repo <repo_root>]

Writes:
  data/mods/modern_weapons/data/modern_weapons/units/<pack_name>/NNN.png
  data/mods/modern_weapons/data/imagepacks/<pack_name>       (the pack zip)

The pack is then referenced from armor / agent type entries as
  BATTLEUNITIMAGEPACK_<pack_name>
once a matching key is added to the mod gamestate's image pack section.
"""
import argparse
import os
import shutil
import zipfile

DEFAULT_REPO = r"C:\Users\joebf\Developer\projects\openapoc"
OFFSET_X, OFFSET_Y = 23, 34  # matches vanilla unit packs

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("frames_dir")
    ap.add_argument("pack_name")
    ap.add_argument("--repo", default=DEFAULT_REPO)
    args = ap.parse_args()

    mod = os.path.join(args.repo, "data", "mods", "marine_armor", "data")
    png_dst = os.path.join(mod, "marine_armor", "units", args.pack_name)
    pack_dst_dir = os.path.join(mod, "imagepacks")
    os.makedirs(png_dst, exist_ok=True)
    os.makedirs(pack_dst_dir, exist_ok=True)

    frames = {}
    for name in os.listdir(args.frames_dir):
        stem, ext = os.path.splitext(name)
        if ext.lower() == ".png" and stem.isdigit():
            frames[int(stem)] = name
    if not frames:
        raise SystemExit("no numbered .png frames found in " + args.frames_dir)
    top = max(frames)

    entries = []
    for i in range(top + 1):
        if i in frames:
            shutil.copyfile(os.path.join(args.frames_dir, frames[i]),
                            os.path.join(png_dst, "%03d.png" % i))
            entries.append("<entry>marine_armor/units/%s/%03d.png</entry>"
                           % (args.pack_name, i))
        else:
            entries.append("<entry></entry>")

    xml = ('<?xml version="1.0" encoding="UTF-8"?><imagepack>'
           "<image_offset><x>%d</x><y>%d</y></image_offset><images>%s"
           "<sizeHint>%d</sizeHint></images></imagepack>"
           % (OFFSET_X, OFFSET_Y, "".join(entries), top + 1))

    pack_path = os.path.join(pack_dst_dir, args.pack_name)
    with zipfile.ZipFile(pack_path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("imagepack.xml", xml)

    print("frames: %d present of %d slots" % (len(frames), top + 1))
    print("pngs  -> %s" % png_dst)
    print("pack  -> %s" % pack_path)

if __name__ == "__main__":
    main()
