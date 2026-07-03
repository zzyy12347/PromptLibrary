import json
import uuid
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET


WORKSPACE = Path(__file__).resolve().parents[1]
MINDMAPS_JSON = WORKSPACE / "mindmaps.json"

EMMX_FILES = [
    Path(r"C:\Users\pcgame\Desktop\新坑\世界设定.emmx"),
    Path(r"C:\Users\pcgame\Desktop\新坑\作品大纲.emmx"),
    Path(r"C:\Users\pcgame\Desktop\新坑\主角塑造.emmx"),
    Path(r"C:\Users\pcgame\Desktop\新坑\事件细纲.emmx"),
]


def new_id() -> str:
    return str(uuid.uuid4()).upper()


def node_text(shape: ET.Element) -> str:
    parts: list[str] = []
    for tp in shape.findall(".//TextBlock/Text/pp/tp"):
        if tp.text:
            parts.append(tp.text)
    text = "".join(parts).strip()
    if text:
        return text
    fallback = shape.get("Name") or shape.get("Text") or ""
    return fallback.strip()


def child_ids(shape: ET.Element) -> list[str]:
    sub = shape.find("./LevelData/SubLevel")
    if sub is None:
        return []
    value = sub.get("V") or ""
    return [item for item in value.split(";") if item]


def parse_emmx(path: Path) -> dict:
    with zipfile.ZipFile(path) as archive:
        xml_bytes = archive.read("page/page.xml")

    root_xml = ET.fromstring(xml_bytes)
    shapes: dict[str, dict] = {}
    children_by_id: dict[str, list[str]] = {}
    referenced_children: set[str] = set()

    for shape in root_xml.findall("./Shape"):
        shape_id = shape.get("ID")
        shape_type = shape.get("Type", "")
        if not shape_id or shape_type == "MMConnector":
            continue
        title = node_text(shape)
        if not title:
            continue
        children = child_ids(shape)
        children_by_id[shape_id] = children
        referenced_children.update(children)
        shapes[shape_id] = {
            "id": new_id(),
            "sourceId": shape_id,
            "title": title,
            "promptTitle": title,
            "children": [],
            "type": shape_type,
        }

    main_id = next((sid for sid, item in shapes.items() if item["type"] == "MainIdea"), None)
    if not main_id:
        roots = [sid for sid in shapes if sid not in referenced_children]
        main_id = roots[0] if roots else next(iter(shapes))

    def build(source_id: str, seen: set[str] | None = None) -> dict:
        seen = seen or set()
        seen.add(source_id)
        source = shapes[source_id]
        item = {
            "id": source["id"],
            "title": source["title"],
            "promptTitle": source["promptTitle"],
            "children": [],
        }
        for child_source_id in children_by_id.get(source_id, []):
            if child_source_id in shapes and child_source_id not in seen:
                item["children"].append(build(child_source_id, seen.copy()))
        return item

    root = build(main_id)

    for source_id in shapes:
        if source_id == main_id:
            continue
        if source_id not in referenced_children and source_id not in children_by_id.get(main_id, []):
            built = build(source_id)
            if built["id"] != root["id"]:
                root["children"].append(built)

    title = path.stem
    if root["title"] and root["title"] != title:
        title = root["title"]

    return {
        "id": new_id(),
        "title": title,
        "root": root,
    }


def load_existing() -> dict:
    if not MINDMAPS_JSON.exists():
        return {"mindmaps": []}
    return json.loads(MINDMAPS_JSON.read_text(encoding="utf-8"))


def main() -> None:
    existing = load_existing()
    maps = existing.get("mindmaps", [])
    imported = [parse_emmx(path) for path in EMMX_FILES]
    imported_titles = {item["title"] for item in imported}
    maps = [item for item in maps if item.get("title") not in imported_titles]
    maps.extend(imported)
    MINDMAPS_JSON.write_text(
        json.dumps({"mindmaps": maps}, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    for item in imported:
        print(f"imported: {item['title']}")
    print(f"saved: {MINDMAPS_JSON}")


if __name__ == "__main__":
    main()
