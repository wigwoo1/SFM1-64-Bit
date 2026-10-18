#!/usr/bin/env python3
"""Inventory installed SFM Qt layout INIs without modifying the installation."""
from __future__ import annotations

import argparse
import configparser
import json
import re
from pathlib import Path


LAYOUT_RE = re.compile(r"^(\d+)\\(.*)$")


def inventory(path: Path) -> dict:
    parser = configparser.ConfigParser(interpolation=None, strict=True)
    parser.optionxform = str
    with path.open("r", encoding="utf-8-sig") as source:
        parser.read_file(source)
    general = parser["General"]
    if int(general["Version"]) != 1:
        raise ValueError(f"unsupported layout version in {path}")
    index = int(general["Index"])
    # Installed v1 files store their indexed layout keys in this shared section.
    section_name = "Layouts_9"
    if section_name not in parser:
        raise ValueError(f"missing {section_name} in {path}")
    data = parser[section_name]
    fields: dict[str, str] = {}
    for key, value in data.items():
        match = LAYOUT_RE.match(key)
        if match and int(match.group(1)) == index + 1:
            fields[match.group(2)] = value

    def tab_window(path: str, saved_size: int | None) -> dict:
        tabs_path = f"{path}\\Tabs"
        count = int(fields.get(f"{tabs_path}\\size", "0"))
        tabs = [fields[f"{tabs_path}\\{tab}\\lookupString"]
                for tab in range(1, count + 1)
                if f"{tabs_path}\\{tab}\\lookupString" in fields]
        return {
            "type": "tab_window",
            "saved_size": saved_size,
            "current_tab": int(fields.get(f"{path}\\currentTab", "0")),
            "tabs": tabs,
        }

    def splitter(path: str, saved_size: int | None = None) -> dict:
        count = int(fields.get(f"{path}\\size", "0"))
        orientation = int(fields.get(f"{path}\\Orientation", "0"))
        children = []
        for child_index in range(1, count + 1):
            child_path = f"{path}\\{child_index}"
            raw_size = fields.get(f"{child_path}\\Size")
            child_size = int(raw_size) if raw_size is not None else None
            nested_path = f"{child_path}\\Splitter\\Splitters"
            if f"{nested_path}\\size" in fields:
                children.append(splitter(nested_path, child_size))
            else:
                children.append(tab_window(f"{child_path}\\TabWindow", child_size))
        return {
            "type": "splitter",
            "path": path.removeprefix("MainWindow\\"),
            "saved_size": saved_size,
            "orientation": orientation,
            "children": children,
        }

    main_window_tree = splitter("MainWindow\\Splitters")
    prefix = "MainWindow\\Splitters\\"
    splitter_settings = []
    for key, value in fields.items():
        if not key.startswith(prefix):
            continue
        suffix = key[len(prefix):]
        leaf = suffix.rsplit("\\", 1)[-1]
        if leaf.casefold() not in ("size", "orientation"):
            continue
        path_part = suffix.rsplit("\\", 1)[0] if "\\" in suffix else "root"
        if path_part == "MainWindow\\Splitters":
            path_part = "root"
        splitter_settings.append({"path": path_part, "property": leaf,
                                  "value": int(value)})
    tabs = []
    base = f"{index + 1}\\MainWindow\\Splitters\\"
    for key, value in data.items():
        if "\\Tabs\\" in key and key.endswith("\\lookupString"):
            tabs.append({"path": key[len(base):].removesuffix("\\lookupString"), "name": value})
    active_tabs = []
    for key, value in data.items():
        if key.startswith(base) and key.endswith("\\currentTab"):
            active_tabs.append({"path": key[len(base):].removesuffix("\\currentTab"), "index": int(value)})
    splitter_settings.sort(key=lambda row: (row["path"], row["property"].casefold()))
    tabs.sort(key=lambda row: row["path"])
    active_tabs.sort(key=lambda row: row["path"])
    root_sizes = []
    root_count = int(data.get(f"{index + 1}\\MainWindow\\Splitters\\size", "0"))
    for child in range(1, root_count + 1):
        value = data.get(f"{index + 1}\\MainWindow\\Splitters\\{child}\\Size")
        if value is not None:
            root_sizes.append(int(value))
    geometry_key = f"{index + 1}\\WindowPercentageGeometry"
    recording_key = f"{index + 1}\\RecordingState"
    secondary_key = f"{index + 1}\\SecondaryWindows\\size"
    return {
        "file": path.name,
        "version": int(general["Version"]),
        "name": general["Name"],
        "index": index,
        "window_percentage_geometry": fields.get("WindowPercentageGeometry"),
        "recording_state": int(fields["RecordingState"]) if "RecordingState" in fields else None,
        "secondary_window_count": int(fields.get("SecondaryWindows\\size", "0")),
        "root_splitter_orientation": int(data.get(f"{index + 1}\\MainWindow\\Splitters\\Orientation", "0")),
        "root_splitter_sizes": root_sizes,
        "main_window_tree": main_window_tree,
        "splitter_settings": splitter_settings,
        "tabs": tabs,
        "active_tabs": active_tabs,
    }


def main() -> int:
    arg = argparse.ArgumentParser()
    arg.add_argument("--layouts", type=Path, default=Path(
        r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\tools\layouts\sfm"))
    arg.add_argument("--output", type=Path, default=Path("outputs/sfm-installed-layout-inventory.json"))
    args = arg.parse_args()
    layouts = sorted(args.layouts.glob("layout*.ini"), key=lambda p: int(re.search(r"\d+", p.stem).group()))
    if not layouts:
        raise SystemExit(f"no layout INIs found: {args.layouts}")
    rows = [inventory(path) for path in layouts]
    if len({row["index"] for row in rows}) != len(rows):
        raise SystemExit("duplicate installed layout index")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"source_directory": str(args.layouts), "layouts": rows}, indent=2) + "\n",
                           encoding="utf-8")
    print(f"layouts={len(rows)} output={args.output}")
    for row in rows:
        print(f"index={row['index']} name={row['name']} root_orientation={row['root_splitter_orientation']} "
              f"root_sizes={row['root_splitter_sizes']} panes={len(row['tabs'])} "
              f"active_tabs={len(row['active_tabs'])} secondary_windows={row['secondary_window_count']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
