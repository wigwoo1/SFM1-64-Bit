#!/usr/bin/env python3
"""Regression checks for the installed SFM dock-layout tree inventory."""
from __future__ import annotations

import argparse
from pathlib import Path

from inventory_installed_sfm_layouts import inventory


def walk(node: dict):
    yield node
    for child in node.get("children", []):
        yield from walk(child)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--layouts", type=Path, default=Path(
        r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\tools\layouts\sfm"))
    args = parser.parse_args()
    files = sorted(args.layouts.glob("layout*.ini"), key=lambda p: p.name)
    rows = [inventory(path) for path in files]
    expected = ["Default", "Clip Editing", "Motion Editing", "Keyframing", "Sound Editing"]
    if [row["name"] for row in rows] != expected:
        raise AssertionError(f"installed presets changed: {[row['name'] for row in rows]}")
    default = rows[0]["main_window_tree"]
    if default["orientation"] != 1 or [child["saved_size"] for child in default["children"]] != [237, 758]:
        raise AssertionError("Default root splitter differs from installed layout1.ini")
    left, right = default["children"]
    if left["tabs"] != ["Animation Set Editor", "Element Viewer"]:
        raise AssertionError("Default left tab group differs from installed layout1.ini")
    timeline_split = right
    if timeline_split["type"] != "splitter" or timeline_split["orientation"] != 2:
        raise AssertionError("Default main area is not split vertically")
    if [child["saved_size"] for child in timeline_split["children"]] != [663, 330]:
        raise AssertionError("Default viewport/Timestrip splitter sizes differ")
    if timeline_split["children"][0]["tabs"] != ["Primary Viewport", "Console"]:
        raise AssertionError("Default viewport tab group differs")
    if timeline_split["children"][1]["tabs"] != ["Timestrip"]:
        raise AssertionError("Default Timestrip placement differs")
    if rows[1]["main_window_tree"]["orientation"] != 2:
        raise AssertionError("Clip Editing root orientation differs")
    for row in rows[2:]:
        panes = [tab for node in walk(row["main_window_tree"]) for tab in node.get("tabs", [])]
        if "Primary Viewport" not in panes or "Secondary Viewport" not in panes or "Timestrip" not in panes:
            raise AssertionError(f"incomplete multiview preset tree: {row['name']}")
    print("installed SFM layout inventory passed layouts=5 default_tree=exact clip_editing_orientation=vertical multi_view_presets=3")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
