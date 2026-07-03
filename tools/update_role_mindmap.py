import json
import uuid
from pathlib import Path


MINDMAPS_JSON = Path(__file__).resolve().parents[1] / "mindmaps.json"


def new_id() -> str:
    return str(uuid.uuid4()).upper()


def node(title: str, children=None, prompt_title: str | None = None) -> dict:
    return {
        "id": new_id(),
        "title": title,
        "promptTitle": prompt_title if prompt_title is not None else title,
        "children": children or [],
    }


def main() -> None:
    data = json.loads(MINDMAPS_JSON.read_text(encoding="utf-8"))
    target = None
    for mindmap in data.get("mindmaps", []):
        if mindmap.get("title") in {"主角塑造", "角色塑造"}:
            target = mindmap
            break

    if not target:
        raise RuntimeError("未找到“主角塑造/角色塑造”导图")

    target["title"] = "角色塑造"
    target["root"]["title"] = "角色塑造"
    target["root"]["promptTitle"] = "角色塑造"

    protagonist = None
    for child in target["root"].get("children", []):
        if child.get("title") == "主角":
            protagonist = child
            break

    if not protagonist:
        protagonist = node("主角")
        target["root"].setdefault("children", []).insert(0, protagonist)

    protagonist["children"] = [
        node("基础信息", [
            node("姓名：黑泽凌"),
            node("前世：凌操、宇智波凌"),
            node("身份/职业：黑泽家新生“接班人”（原定种男）、重生者、写轮眼持有者"),
        ]),
        node("当前状态", [
            node("婴儿（初生）"),
            node("查克拉极少"),
            node("身体虚弱但精神力强大"),
        ]),
        node("性格画像", [
            node("极度理智冷静", [
                node("重生后迅速接受现实，没有惊慌失措"),
                node("看穿“种男”福利的圈养本质：只要前面加上“黑泽家”三个字，那就不是享受，是圈养"),
            ]),
            node("杀伐果断/行动力强", [
                node("判断局势不对后，立刻消耗濒死查克拉强行开启写轮眼发动幻术"),
                node("明知强行施术会榨干自己，仍冒险改命"),
            ]),
            node("洞察力敏锐", [
                node("通过医院、香火、血等气味迅速判断环境异常"),
                node("通过老妇人的纹身推断黑泽家的黑道本质"),
                node("通过奶水甜味判断奶水有问题"),
            ]),
        ]),
        node("能力/金手指", [
            node("显性能力：无（目前为婴儿，无法行动）"),
            node("前世记忆", [
                node("熟知忍界历史与斗争经验"),
                node("剧情虽变，但心性与策略犹在"),
            ]),
            node("写轮眼", [
                node("目前已开启单勾玉"),
                node("具备幻术暗示能力"),
                node("伊邪那岐已用，目前仅为常规瞳术"),
            ]),
            node("查克拉", [
                node("保留一丝火影世界的查克拉种子"),
            ]),
        ]),
        node("核心驱动力", [
            node("表层目标", [
                node("生存"),
                node("摆脱被当作“种男/工具”的命运"),
                node("在黑泽家站稳脚跟"),
            ]),
            node("深层动机/执念", [
                node("不甘心失败"),
                node("前世即使知道剧情仍被收割，这一世想赢过这个世界"),
                node("掌控自己的命运"),
                node("核心心声：我还是不甘心"),
            ]),
        ]),
    ]

    MINDMAPS_JSON.write_text(
        json.dumps(data, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print("updated 角色塑造 -> 主角")


if __name__ == "__main__":
    main()
