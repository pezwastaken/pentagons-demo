#!/usr/bin/env python3

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PRELUDE = ROOT / "all_headers.hh"
ENTRY = ROOT / "main.cc"
EXTS = {".cc", ".hh"}
SKIP_DIRS = {".git", ".cache", "build"}

BASE = ["g++", "-std=c++20", "-g", "-fsanitize=undefined", f"-I{ROOT}"]

entries = []
for path in sorted(ROOT.rglob("*")):
    if path.suffix not in EXTS or path == PRELUDE:
        continue
    if SKIP_DIRS & set(path.relative_to(ROOT).parts):
        continue

    args = list(BASE)
    if path != ENTRY:
        # main.cc already includes the prelude itself
        args += ["-include", str(PRELUDE)]
    args += ["-c", str(path)]

    entries.append({
        "directory": str(ROOT),
        "file": str(path),
        "arguments": args,
    })

(ROOT / "compile_commands.json").write_text(json.dumps(entries, indent=2))
print(f"wrote {len(entries)} entries")
