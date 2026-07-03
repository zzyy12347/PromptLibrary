import json
import re
import tkinter as tk
from dataclasses import dataclass, asdict
from pathlib import Path
from tkinter import messagebox, ttk
from uuid import uuid4


APP_TITLE = "AI 小说提示词库"
BASE_DIR = Path(__file__).resolve().parent
DATA_FILE = BASE_DIR / "prompts.json"
SOURCE_MD = BASE_DIR / "原创小说写作提示词库.md"


@dataclass
class PromptItem:
    id: str
    title: str
    category: str
    tags: str
    content: str


def clean_heading(line: str) -> str:
    line = re.sub(r"^#+\s*", "", line).strip()
    return re.sub(r"^\d+(?:\.\d+)*\.?\s*", "", line).strip()


def import_markdown_prompts() -> list[PromptItem]:
    if not SOURCE_MD.exists():
        return []

    text = SOURCE_MD.read_text(encoding="utf-8")
    prompts: list[PromptItem] = []
    category = "未分类"
    current_title: str | None = None
    current_lines: list[str] = []

    def flush_current() -> None:
        nonlocal current_title, current_lines
        if not current_title:
            return
        content = "\n".join(current_lines).strip()
        prompts.append(
            PromptItem(
                id=str(uuid4()),
                title=current_title,
                category=category,
                tags=category,
                content=content,
            )
        )
        current_title = None
        current_lines = []

    for raw_line in text.splitlines():
        line = raw_line.rstrip()
        if line.startswith("## ") and not line.startswith("### "):
            flush_current()
            heading = clean_heading(line)
            if heading and heading not in {"使用方式"}:
                category = heading
            continue
        if line.startswith("### "):
            flush_current()
            current_title = clean_heading(line)
            current_lines = []
            continue
        if current_title:
            current_lines.append(line)

    flush_current()
    return prompts


def load_prompts() -> list[PromptItem]:
    if DATA_FILE.exists():
        raw = json.loads(DATA_FILE.read_text(encoding="utf-8"))
        return [PromptItem(**item) for item in raw.get("prompts", [])]

    prompts = import_markdown_prompts()
    save_prompts(prompts)
    return prompts


def save_prompts(prompts: list[PromptItem]) -> None:
    DATA_FILE.write_text(
        json.dumps({"prompts": [asdict(item) for item in prompts]}, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


class PromptLibraryApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.prompts = load_prompts()
        self.filtered: list[PromptItem] = []
        self.current_id: str | None = None

        self.search_var = tk.StringVar()
        self.category_var = tk.StringVar(value="全部分类")
        self.title_var = tk.StringVar()
        self.tags_var = tk.StringVar()

        self.setup_window()
        self.build_ui()
        self.refresh_categories()
        self.apply_filters()
        self.bind_shortcuts()

    def setup_window(self) -> None:
        self.root.title(APP_TITLE)
        self.root.geometry("1120x720")
        self.root.minsize(920, 580)

        style = ttk.Style()
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass
        style.configure("TFrame", background="#f7f6f2")
        style.configure("Panel.TFrame", background="#ffffff")
        style.configure("TLabel", background="#f7f6f2", foreground="#1e2428")
        style.configure("Panel.TLabel", background="#ffffff", foreground="#1e2428")
        style.configure("Muted.TLabel", background="#ffffff", foreground="#657077")
        style.configure("TButton", padding=(12, 7))
        style.configure("Primary.TButton", padding=(14, 8))
        style.configure("Danger.TButton", padding=(12, 7))

    def build_ui(self) -> None:
        self.root.columnconfigure(0, weight=0, minsize=340)
        self.root.columnconfigure(1, weight=1)
        self.root.rowconfigure(0, weight=1)

        left = ttk.Frame(self.root, style="Panel.TFrame", padding=14)
        left.grid(row=0, column=0, sticky="nsew")
        left.columnconfigure(0, weight=1)
        left.rowconfigure(4, weight=1)

        ttk.Label(left, text="提示词库", style="Panel.TLabel", font=("Microsoft YaHei UI", 17, "bold")).grid(
            row=0, column=0, sticky="w"
        )

        search_frame = ttk.Frame(left, style="Panel.TFrame")
        search_frame.grid(row=1, column=0, sticky="ew", pady=(14, 8))
        search_frame.columnconfigure(0, weight=1)
        self.search_entry = ttk.Entry(search_frame, textvariable=self.search_var)
        self.search_entry.grid(row=0, column=0, sticky="ew")

        self.category_box = ttk.Combobox(left, textvariable=self.category_var, state="readonly")
        self.category_box.grid(row=2, column=0, sticky="ew", pady=(0, 10))

        toolbar = ttk.Frame(left, style="Panel.TFrame")
        toolbar.grid(row=3, column=0, sticky="ew", pady=(0, 10))
        toolbar.columnconfigure((0, 1), weight=1)
        ttk.Button(toolbar, text="新建", command=self.new_prompt).grid(row=0, column=0, sticky="ew", padx=(0, 6))
        ttk.Button(toolbar, text="复制", command=self.copy_current).grid(row=0, column=1, sticky="ew", padx=(6, 0))

        list_frame = ttk.Frame(left, style="Panel.TFrame")
        list_frame.grid(row=4, column=0, sticky="nsew")
        list_frame.columnconfigure(0, weight=1)
        list_frame.rowconfigure(0, weight=1)

        self.prompt_list = tk.Listbox(
            list_frame,
            activestyle="none",
            borderwidth=0,
            highlightthickness=1,
            highlightbackground="#d7dbdc",
            highlightcolor="#55917f",
            selectbackground="#dcebe6",
            selectforeground="#17201d",
            font=("Microsoft YaHei UI", 10),
        )
        self.prompt_list.grid(row=0, column=0, sticky="nsew")
        list_scroll = ttk.Scrollbar(list_frame, orient="vertical", command=self.prompt_list.yview)
        list_scroll.grid(row=0, column=1, sticky="ns")
        self.prompt_list.configure(yscrollcommand=list_scroll.set)

        self.count_label = ttk.Label(left, text="", style="Muted.TLabel")
        self.count_label.grid(row=5, column=0, sticky="w", pady=(10, 0))

        right = ttk.Frame(self.root, padding=18)
        right.grid(row=0, column=1, sticky="nsew")
        right.columnconfigure(0, weight=1)
        right.rowconfigure(5, weight=1)

        header = ttk.Frame(right)
        header.grid(row=0, column=0, sticky="ew", pady=(0, 12))
        header.columnconfigure(0, weight=1)
        ttk.Label(header, text="编辑提示词", font=("Microsoft YaHei UI", 17, "bold")).grid(row=0, column=0, sticky="w")

        actions = ttk.Frame(header)
        actions.grid(row=0, column=1, sticky="e")
        ttk.Button(actions, text="保存", style="Primary.TButton", command=self.save_current).grid(row=0, column=0, padx=(0, 8))
        ttk.Button(actions, text="删除", style="Danger.TButton", command=self.delete_current).grid(row=0, column=1)

        ttk.Label(right, text="标题").grid(row=1, column=0, sticky="w")
        self.title_entry = ttk.Entry(right, textvariable=self.title_var)
        self.title_entry.grid(row=2, column=0, sticky="ew", pady=(4, 10))

        meta = ttk.Frame(right)
        meta.grid(row=3, column=0, sticky="ew", pady=(0, 10))
        meta.columnconfigure(0, weight=1)
        meta.columnconfigure(1, weight=1)

        ttk.Label(meta, text="分类").grid(row=0, column=0, sticky="w")
        self.edit_category_var = tk.StringVar()
        self.edit_category = ttk.Entry(meta, textvariable=self.edit_category_var)
        self.edit_category.grid(row=1, column=0, sticky="ew", padx=(0, 8), pady=(4, 0))

        ttk.Label(meta, text="标签").grid(row=0, column=1, sticky="w")
        self.tags_entry = ttk.Entry(meta, textvariable=self.tags_var)
        self.tags_entry.grid(row=1, column=1, sticky="ew", padx=(8, 0), pady=(4, 0))

        ttk.Label(right, text="提示词内容").grid(row=4, column=0, sticky="w")
        content_frame = ttk.Frame(right)
        content_frame.grid(row=5, column=0, sticky="nsew", pady=(4, 10))
        content_frame.columnconfigure(0, weight=1)
        content_frame.rowconfigure(0, weight=1)

        self.content_text = tk.Text(
            content_frame,
            wrap="word",
            undo=True,
            borderwidth=0,
            highlightthickness=1,
            highlightbackground="#cfd6d8",
            highlightcolor="#55917f",
            font=("Microsoft YaHei UI", 11),
            padx=12,
            pady=12,
        )
        self.content_text.grid(row=0, column=0, sticky="nsew")
        content_scroll = ttk.Scrollbar(content_frame, orient="vertical", command=self.content_text.yview)
        content_scroll.grid(row=0, column=1, sticky="ns")
        self.content_text.configure(yscrollcommand=content_scroll.set)

        self.status_var = tk.StringVar(value="准备好了")
        ttk.Label(right, textvariable=self.status_var).grid(row=6, column=0, sticky="w")

        self.search_var.trace_add("write", lambda *_: self.apply_filters())
        self.category_box.bind("<<ComboboxSelected>>", lambda _event: self.apply_filters())
        self.prompt_list.bind("<<ListboxSelect>>", self.on_select)

    def bind_shortcuts(self) -> None:
        self.root.bind("<Control-f>", lambda _event: self.focus_search())
        self.root.bind("<Control-s>", lambda _event: self.save_current())
        self.root.bind("<Control-n>", lambda _event: self.new_prompt())

    def focus_search(self) -> str:
        self.search_entry.focus_set()
        self.search_entry.select_range(0, tk.END)
        return "break"

    def refresh_categories(self) -> None:
        categories = sorted({item.category for item in self.prompts if item.category})
        self.category_box["values"] = ["全部分类", *categories]
        if self.category_var.get() not in self.category_box["values"]:
            self.category_var.set("全部分类")

    def apply_filters(self) -> None:
        keyword = self.search_var.get().strip().lower()
        category = self.category_var.get()

        def matched(item: PromptItem) -> bool:
            if category != "全部分类" and item.category != category:
                return False
            if not keyword:
                return True
            haystack = "\n".join([item.title, item.category, item.tags, item.content]).lower()
            return keyword in haystack

        self.filtered = [item for item in self.prompts if matched(item)]
        self.prompt_list.delete(0, tk.END)
        for item in self.filtered:
            self.prompt_list.insert(tk.END, f"{item.title}  ·  {item.category}")
        self.count_label.configure(text=f"共 {len(self.filtered)} 条，全部 {len(self.prompts)} 条")

        if self.filtered and self.current_id not in {item.id for item in self.filtered}:
            self.prompt_list.selection_set(0)
            self.load_prompt(self.filtered[0])
        elif not self.filtered:
            self.clear_editor(keep_status=True)

    def on_select(self, _event: tk.Event) -> None:
        selection = self.prompt_list.curselection()
        if not selection:
            return
        index = selection[0]
        if 0 <= index < len(self.filtered):
            self.load_prompt(self.filtered[index])

    def load_prompt(self, item: PromptItem) -> None:
        self.current_id = item.id
        self.title_var.set(item.title)
        self.edit_category_var.set(item.category)
        self.tags_var.set(item.tags)
        self.content_text.delete("1.0", tk.END)
        self.content_text.insert("1.0", item.content)
        self.status_var.set(f"正在编辑：{item.title}")

    def clear_editor(self, keep_status: bool = False) -> None:
        self.current_id = None
        self.title_var.set("")
        self.edit_category_var.set("")
        self.tags_var.set("")
        self.content_text.delete("1.0", tk.END)
        if not keep_status:
            self.status_var.set("新提示词")

    def new_prompt(self) -> str:
        self.prompt_list.selection_clear(0, tk.END)
        self.clear_editor()
        self.title_entry.focus_set()
        return "break"

    def get_editor_item(self) -> PromptItem | None:
        title = self.title_var.get().strip()
        category = self.edit_category_var.get().strip() or "未分类"
        tags = self.tags_var.get().strip()
        content = self.content_text.get("1.0", tk.END).strip()

        if not title:
            messagebox.showwarning(APP_TITLE, "请先填写标题。")
            self.title_entry.focus_set()
            return None
        if not content:
            messagebox.showwarning(APP_TITLE, "请先填写提示词内容。")
            self.content_text.focus_set()
            return None

        return PromptItem(
            id=self.current_id or str(uuid4()),
            title=title,
            category=category,
            tags=tags,
            content=content,
        )

    def save_current(self) -> str:
        item = self.get_editor_item()
        if not item:
            return "break"

        for index, existing in enumerate(self.prompts):
            if existing.id == item.id:
                self.prompts[index] = item
                break
        else:
            self.prompts.append(item)

        self.current_id = item.id
        save_prompts(self.prompts)
        self.refresh_categories()
        self.apply_filters()
        self.select_current_in_list()
        self.status_var.set("已保存")
        return "break"

    def select_current_in_list(self) -> None:
        if not self.current_id:
            return
        self.prompt_list.selection_clear(0, tk.END)
        for index, item in enumerate(self.filtered):
            if item.id == self.current_id:
                self.prompt_list.selection_set(index)
                self.prompt_list.see(index)
                break

    def delete_current(self) -> None:
        if not self.current_id:
            messagebox.showinfo(APP_TITLE, "请先选择要删除的提示词。")
            return
        title = self.title_var.get().strip() or "当前提示词"
        if not messagebox.askyesno(APP_TITLE, f"确定删除“{title}”吗？"):
            return
        self.prompts = [item for item in self.prompts if item.id != self.current_id]
        save_prompts(self.prompts)
        self.clear_editor(keep_status=True)
        self.refresh_categories()
        self.apply_filters()
        self.status_var.set("已删除")

    def copy_current(self) -> None:
        content = self.content_text.get("1.0", tk.END).strip()
        if not content:
            messagebox.showinfo(APP_TITLE, "当前没有可复制的提示词内容。")
            return
        self.root.clipboard_clear()
        self.root.clipboard_append(content)
        self.root.update()
        self.status_var.set("提示词已复制到剪贴板")


def main() -> None:
    root = tk.Tk()
    PromptLibraryApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
