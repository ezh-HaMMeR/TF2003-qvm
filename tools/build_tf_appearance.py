"""Bake TF disguise PCX skins and immutable corpse MDLs from installed TF assets.

Usage: python tools/build_tf_appearance.py --gamedir C:/Games/qwtf/fortress
The output is a fortress-directory overlay. Source assets are never modified.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

CLASSES = ["base", "tf_scout", "tf_snipe", "tf_sold", "tf_demo", "tf_medic",
           "tf_hwguy", "tf_pyro", "tf_spy", "tf_eng"]
COLORS = [13, 4, 12, 11]


def read_asset(gamedir, name):
    loose = gamedir / name
    if loose.is_file():
        return loose.read_bytes()
    for pak in sorted(gamedir.glob("pak*.pak"), reverse=True):
        data = pak.read_bytes()
        if data[:4] != b"PACK":
            raise ValueError(f"Invalid PAK: {pak}")
        offset, length = struct.unpack_from("<ii", data, 4)
        for pos in range(offset, offset + length, 64):
            entry, start, size = struct.unpack_from("<56sii", data, pos)
            if entry.split(b"\0")[0].decode("ascii").lower() == name.lower():
                return data[start:start + size]
    raise FileNotFoundError(name)


def decode_pcx(data):
    assert data[:4] == b"\x0a\x05\x01\x08" and data[65] == 1
    x0, y0, x1, y1 = struct.unpack_from("<4H", data, 4)
    width, height = x1 - x0 + 1, y1 - y0 + 1
    stride = struct.unpack_from("<H", data, 66)[0]
    assert stride >= width and data[-769] == 12
    pixels = bytearray()
    pos = 128
    while len(pixels) < stride * height:
        value = data[pos]
        pos += 1
        count = 1
        if value >= 192:
            count = value & 63
            value = data[pos]
            pos += 1
        pixels.extend([value] * count)
    assert len(pixels) == stride * height
    return width, height, b"".join(pixels[y * stride:y * stride + width] for y in range(height)), data[-768:]


def encode_pcx(width, height, pixels, palette):
    header = bytearray(128)
    header[:4] = b"\x0a\x05\x01\x08"
    struct.pack_into("<4H", header, 4, 0, 0, width - 1, height - 1)
    stride = (width + 1) & ~1
    header[65] = 1
    struct.pack_into("<HH", header, 66, stride, 1)
    encoded = bytearray(header)
    for y in range(height):
        row = pixels[y * width:(y + 1) * width] + bytes(stride - width)
        i = 0
        while i < stride:
            run = 1
            while i + run < stride and row[i + run] == row[i] and run < 63:
                run += 1
            if run > 1 or row[i] >= 192:
                encoded.append(192 | run)
            encoded.append(row[i])
            i += run
    return bytes(encoded) + b"\x0c" + palette


def bake(pixels, palette, top, bottom):
    mapping = bytearray(range(256))
    for start, color in [(16, top), (96, bottom)]:
        for i in range(16):
            mapping[start + i] = color * 16 + (15 - i if color >= 8 else i)
    # Keep pixels outside both player-remapping ramps, including custom tcN
    # values 1 and 6. Only non-fullbright candidates are used for these ramps.
    for i in range(256):
        mapped = mapping[i]
        if 16 <= mapped < 32 or 96 <= mapped < 112:
            rgb = palette[3 * mapped:3 * mapped + 3]
            mapping[i] = min((j for j in range(224) if not (16 <= j < 32 or 96 <= j < 112)),
                             key=lambda j: sum((rgb[k] - palette[3 * j + k]) ** 2 for k in range(3)))
    result = pixels.translate(bytes(mapping))
    assert not any(16 <= p < 32 or 96 <= p < 112 for p in result)
    return result


def split_mdl(data):
    assert data[:8] == b"IDPO\x06\x00\x00\x00"
    count, width, height = struct.unpack_from("<3i", data, 48)
    skins, offset = [], 84
    for _ in range(count):
        assert struct.unpack_from("<i", data, offset)[0] == 0, "Grouped source skins unsupported"
        offset += 4
        skins.append(data[offset:offset + width * height])
        offset += width * height
    return data[:84], width, height, skins, data[offset:]


def build(gamedir, outdir, tops):
    header, width, height, original, geometry = split_mdl(read_asset(gamedir, "progs/player.mdl"))
    assert len(original) == len(CLASSES)
    sources = []
    # Quake ignores the PCX trailer palette and uses gfx/palette.lmp.
    try:
        palette = read_asset(gamedir, "gfx/palette.lmp")
    except FileNotFoundError:
        palette = read_asset(gamedir.parent / "id1", "gfx/palette.lmp")
    assert len(palette) == 768
    pcx_sources = []
    for index, name in enumerate(CLASSES):
        if index == 0:
            continue
        data = read_asset(gamedir, f"skins/{name}.pcx")
        w, h, pixels, pal = decode_pcx(data)
        assert w >= width and h >= height
        pcx_sources.append((w, h, pixels))
        sources.append(b"".join(pixels[y * w:y * w + width] for y in range(height)))
    sources.insert(0, original[0])
    head_header, hw, hh, head_skins, head_geometry = split_mdl(read_asset(gamedir, "progs/headless.mdl"))
    assert len(head_skins) == 10 and (hw, hh) == (width, height)
    gib_header, gw, gh, gib_skins, gib_geometry = split_mdl(read_asset(gamedir, "progs/h_player.mdl"))
    assert len(gib_skins) == 10
    outputs = {}
    for team, (top, bottom) in enumerate(zip(tops, COLORS), 1):
        baked = [bake(p, palette, top, bottom) for p in sources]
        body = header + b"".join(struct.pack("<i", 0) + p for p in baked) + geometry
        head = head_header + b"".join(struct.pack("<i", 0) + bake(p, palette, top, bottom)
                                    for p in head_skins) + head_geometry
        outputs[f"progs/tfbody{team}.mdl"] = body
        outputs[f"progs/tfheadless{team}.mdl"] = head
        gib = gib_header + b"".join(struct.pack("<i", 0) + bake(p, palette, top, bottom)
                                  for p in gib_skins) + gib_geometry
        outputs[f"progs/tfhead{team}.mdl"] = gib
        assert split_mdl(gib)[4] == gib_geometry
        assert split_mdl(body)[4] == geometry and split_mdl(head)[4] == head_geometry
        for pc, (w, h, pixels) in enumerate(pcx_sources, 1):
            colored = bake(pixels, palette, top, bottom)
            pcx = encode_pcx(w, h, colored, palette)
            assert decode_pcx(pcx)[2] == colored
            outputs[f"skins/tf_dc{team}_{pc}.pcx"] = pcx
    outdir.mkdir(parents=True, exist_ok=True)
    for name, data in outputs.items():
        path = outdir / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    manifest = {"top_colors": tops, "bottom_colors": COLORS,
                "files": {name: {"size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                          for name, data in outputs.items()}}
    (outdir / "appearance-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Verified {len(outputs)} resources; geometry/animation bytes unchanged; palette ramps locked")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gamedir", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "assets/fortress")
    parser.add_argument("--top-colors", type=int, nargs=4, default=COLORS,
                        help="Actual tc1 tc2 tc3 tc4 server colors; regenerate when these change")
    args = parser.parse_args()
    if any(c < 0 or c > 13 for c in args.top_colors):
        parser.error("Client player palette colors must be in 0..13")
    build(args.gamedir, args.output, args.top_colors)
