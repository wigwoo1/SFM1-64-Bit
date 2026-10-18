import json
from pathlib import Path


audit_path = Path("outputs/graph-editor-context-menu-linkage-audit.json")
audit = json.loads(audit_path.read_text(encoding="utf-8-sig"))
name = "Graph Editor upper/lower TimeStrip custom-context popup"
chain = next((item for item in audit["chains"] if item.get("name") in {
    "CQTimestripWidget custom-context popup",
    name,
}), None)
if chain is None:
    chain = {
        "name": name,
        "evidence": [
            "outputs/timestrip-custom-context-popup-linkage-verification.json",
            "outputs/timestrip-menu-provider-callchain-verification.json",
            "outputs/graph-editor-interaction-action-provider-verification.json",
        ],
    }
    audit["chains"].append(chain)
chain["name"] = name
chain["verified_path"] = [
    "Graph Editor setup labels and constructs Graph Editor Upper Time Strip/CQUpperTimestripWidget and Graph Editor Lower Time Strip/CQLowerTimestripWidget; both derived constructors call CQTimestripWidget base initializer",
    "CQTimestripWidget initializer sets context-menu policy 3 and QObject::connects customContextMenuRequested to OnCustomContextMenuRequested on the same object",
    "Installed meta-object local method 0 and vtable metacall dispatch to helper 0x1007C660, which calls popup builder 0x1019C6E0",
    "The byte-verified builder creates and executes a Timestrip QMenu and calls Bookmark/Curve/key QAction provider 0x100B25E0",
]
chain["proof_scope"] = "The Graph Editor upper/lower TimeStrip instances and their inherited widget signal-to-popup-to-action-provider route are directly matched against installed ifm.dll bytes and its current hash. This proves the TimeStrip context menu is used in Graph Editor. It does not join the independent CQGraphEditorView context selection handler or shared OpenContextMenu event relay, and it does not cover every context menu in the application. The shared base is also used by Motion Editor upper/lower timestrip subclasses."
audit["linkage_status"] = "The Graph Editor upper/lower TimeStrip route is byte-linked to the popup builder and Bookmark/Curve/key provider. The panel OpenContextMenu route remains only a generic event relay. Its service pointer lives in the zero-filled tail of ifm.dll .data and has no direct store in the saved disassembly, so its initializer/consumer is likely supplied outside this static image; no visible-menu relationship is established. The separate CQGraphEditorView hit-test route remains unlinked to this popup."
audit["next_targets"] = [
    "Inspect sfm.exe startup and runtime module initialization for the source of zero-filled, non-IAT global 0x10F4C7E4; resolve the object implementation behind virtual slot +0x5C and test whether it reaches a visible Graph Editor context menu.",
    "Continue mapping Graph Editor context targets and action callbacks, including selection-specific enablement and operation effects.",
    "If a live SFM window becomes available, compare upper/lower TimeStrip popup items and behavior with the byte-derived provider path.",
]
audit_path.write_text(json.dumps(audit, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

menu_path = Path("outputs/timestrip-menu-provider-callchain-verification.json")
menu = json.loads(menu_path.read_text(encoding="utf-8-sig"))
edge = "The Graph Editor upper/lower TimeStrip instances are linked to this widget-level route by outputs/timestrip-custom-context-popup-linkage-verification.json: CQTimestripWidget customContextMenuRequested -> Qt slot OnCustomContextMenuRequested -> this menu builder."
if edge not in menu.get("interpretation", []):
    menu.setdefault("interpretation", []).append(edge)
menu["limitations"] = [
    "The Graph Editor upper/lower TimeStrip owner and widget-signal-to-popup route are now byte-verified in outputs/timestrip-custom-context-popup-linkage-verification.json; the same base is also used by Motion Editor upper/lower timestrip widgets.",
    "The separate CQGraphEditorView selection/hit-test handler and shared panel OpenContextMenu service relay remain unlinked from this TimeStrip popup path.",
    "No live menu presentation, item order, enabled-state behavior, or callback side effect was exercised.",
    "This proves the Graph Editor upper/lower TimeStrip popup, not all Graph Editor/application context menus or full editor parity.",
]
menu_path.write_text(json.dumps(menu, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
print(json.dumps({
    "audit_chain_count": len(audit["chains"]),
    "new_chain_added": any(chain.get("name") == name for chain in audit["chains"]),
    "menu_interpretation_count": len(menu["interpretation"]),
}, indent=2))
