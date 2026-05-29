#!/usr/bin/env python3
"""fullsize_signals.hpp の float 配列を float32 LE のバイナリ(.bin)へ抽出する。

EMBED_FILES でファームウェアに埋め込むためのデータ生成ツール。
元の C++ ソース(.hpp)の値をそのまま抽出するため、値の同一性を保証する。
（信号値は +-1.000000f のみで float32 で厳密表現可能 = ビット一致）

使い方:
    python3 tools/hpp_to_bin.py <input.hpp> <output_dir>
"""
import re
import struct
import sys
from pathlib import Path

# 抽出対象の配列名（fullsize_signals.hpp 内）
ARRAYS = [
    "translation_signal_left_45900",
    "translation_signal_right_45900",
    "rotation_signal_left_45900",
    "rotation_signal_right_45900",
]

FLOAT_RE = re.compile(r"[-+]?[0-9]+\.[0-9]+(?:[eE][-+]?[0-9]+)?f?")


def extract_array(text: str, name: str):
    # `... name[<N>] = { ... };` のブロックを取り出す
    m = re.search(re.escape(name) + r"\s*\[\s*(\d+)\s*\]\s*=\s*\{", text)
    if not m:
        raise SystemExit(f"array not found: {name}")
    declared = int(m.group(1))
    start = m.end()
    end = text.index("};", start)
    body = text[start:end]
    values = [float(tok.rstrip("fF")) for tok in FLOAT_RE.findall(body)]
    if len(values) != declared:
        raise SystemExit(
            f"{name}: count mismatch declared={declared} found={len(values)}")
    return values


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    src = Path(sys.argv[1]).read_text(encoding="latin-1")
    out_dir = Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)
    for name in ARRAYS:
        values = extract_array(src, name)
        blob = struct.pack("<%df" % len(values), *values)
        out = out_dir / (name + ".bin")
        out.write_bytes(blob)
        print(f"{name}: {len(values)} floats -> {out} ({len(blob)} bytes)")


if __name__ == "__main__":
    main()
