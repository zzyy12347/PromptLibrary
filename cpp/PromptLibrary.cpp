#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <objbase.h>

#include <algorithm>
#include <cwctype>
#include <functional>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ole32.lib")

struct PromptItem {
    std::wstring id;
    std::wstring title;
    std::wstring category;
    std::wstring tags;
    std::wstring content;
};

struct MindNode {
    std::wstring id;
    std::wstring title;
    std::wstring promptTitle;
    std::vector<MindNode> children;
};

struct MindMap {
    std::wstring id;
    std::wstring title;
    MindNode root;
};

static const wchar_t* APP_TITLE = L"AI 小说提示词库";
static HINSTANCE g_instance = nullptr;
static HWND g_main = nullptr;
static HWND g_search = nullptr;
static HWND g_category = nullptr;
static HWND g_list = nullptr;
static HWND g_count = nullptr;
static HWND g_title = nullptr;
static HWND g_editCategory = nullptr;
static HWND g_tags = nullptr;
static HWND g_content = nullptr;
static HWND g_status = nullptr;
static HWND g_mindMap = nullptr;
static HWND g_mapSelector = nullptr;
static HWND g_mapTree = nullptr;
static HWND g_nodeTitle = nullptr;
static HWND g_nodePrompt = nullptr;
static HWND g_mapStatus = nullptr;
static HFONT g_font = nullptr;
static HFONT g_titleFont = nullptr;
static std::wstring g_dataPath = L"prompts.json";
static std::wstring g_markdownPath = L"原创小说写作提示词库.md";
static std::wstring g_mindMapPath = L"mindmaps.json";
static std::vector<PromptItem> g_prompts;
static std::vector<int> g_filtered;
static std::vector<MindMap> g_mindMaps;
static std::vector<std::wstring> g_treeNodeIds;
static int g_current = -1;
static int g_currentMap = 0;
static std::wstring g_selectedNodeId;
static std::wstring g_dragNodeId;
static HTREEITEM g_dragHoverItem = nullptr;
static bool g_draggingNode = false;
static int g_dragDropMode = 0;
static bool g_loading = false;
static bool g_loadingMapUi = false;

enum ControlId {
    IDC_SEARCH = 1001,
    IDC_CATEGORY = 1002,
    IDC_LIST = 1003,
    IDC_NEW = 1004,
    IDC_COPY = 1005,
    IDC_SAVE = 1006,
    IDC_DELETE = 1007,
    IDC_MINDMAP = 1008
};

enum MindMapControlId {
    IDC_MAP_SELECT = 2001,
    IDC_MAP_PREV = 2002,
    IDC_MAP_NEXT = 2003,
    IDC_MAP_NEW = 2004,
    IDC_MAP_RENAME = 2005,
    IDC_MAP_DELETE = 2006,
    IDC_MAP_IMPORT = 2007,
    IDC_MAP_EXPORT = 2008,
    IDC_MAP_TREE = 2009,
    IDC_NODE_TITLE = 2010,
    IDC_NODE_PROMPT = 2011,
    IDC_NODE_SAVE = 2012,
    IDC_NODE_ADD = 2013,
    IDC_NODE_DELETE = 2014
};

struct MapHotspot {
    RECT rect{};
    std::wstring label;
    std::wstring promptTitle;
    std::wstring nodeId;
};

static std::vector<MapHotspot> g_mindHotspots;

static std::wstring ToWide(const std::string& text) {
    if (text.empty()) return L"";
    int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), result.data(), length);
    return result;
}

static std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) return "";
    int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), result.data(), length, nullptr, nullptr);
    return result;
}

static bool ReadUtf8File(const std::wstring& path, std::wstring& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string bytes = buffer.str();
    if (bytes.size() >= 3 && (unsigned char)bytes[0] == 0xEF && (unsigned char)bytes[1] == 0xBB && (unsigned char)bytes[2] == 0xBF) {
        bytes.erase(0, 3);
    }
    out = ToWide(bytes);
    return true;
}

static bool WriteUtf8File(const std::wstring& path, const std::wstring& text) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    std::string bytes = ToUtf8(text);
    file.write(bytes.data(), (std::streamsize)bytes.size());
    return true;
}

static bool FileExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring JoinPath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) return right;
    if (left.back() == L'\\' || left.back() == L'/') return left + right;
    return left + L"\\" + right;
}

static std::wstring ParentDirectory(std::wstring path) {
    while (!path.empty() && (path.back() == L'\\' || path.back() == L'/')) path.pop_back();
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L"";
    return path.substr(0, pos);
}

static std::wstring CurrentDirectory() {
    DWORD length = GetCurrentDirectoryW(0, nullptr);
    std::wstring path(length ? length - 1 : 0, L'\0');
    if (length > 1) GetCurrentDirectoryW(length, path.data());
    return path;
}

static std::wstring ExecutableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring path(buffer, length);
    return ParentDirectory(path);
}

static std::wstring Trim(const std::wstring& value) {
    size_t start = 0;
    while (start < value.size() && iswspace(value[start])) ++start;
    size_t end = value.size();
    while (end > start && iswspace(value[end - 1])) --end;
    return value.substr(start, end - start);
}

static std::wstring Lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return (wchar_t)towlower(ch);
    });
    return value;
}

static std::wstring GetText(HWND hwnd) {
    int length = GetWindowTextLengthW(hwnd);
    std::wstring text(length, L'\0');
    if (length > 0) GetWindowTextW(hwnd, text.data(), length + 1);
    return text;
}

static std::wstring GetEditText(HWND hwnd) {
    int length = GetWindowTextLengthW(hwnd);
    std::wstring text(length, L'\0');
    if (length > 0) GetWindowTextW(hwnd, text.data(), length + 1);
    return text;
}

static void SetText(HWND hwnd, const std::wstring& text) {
    SetWindowTextW(hwnd, text.c_str());
}

static std::wstring NewId() {
    GUID guid;
    if (FAILED(CoCreateGuid(&guid))) return std::to_wstring(GetTickCount64());
    wchar_t buffer[64]{};
    StringFromGUID2(guid, buffer, 64);
    std::wstring id = buffer;
    if (!id.empty() && id.front() == L'{') id.erase(id.begin());
    if (!id.empty() && id.back() == L'}') id.pop_back();
    return id;
}

class JsonReader {
public:
    explicit JsonReader(const std::wstring& source) : text(source) {}

    bool ReadPrompts(std::vector<PromptItem>& items) {
        Skip();
        if (!Consume(L'{')) return false;
        while (true) {
            Skip();
            if (Consume(L'}')) break;
            std::wstring key;
            if (!ReadString(key)) return false;
            Skip();
            if (!Consume(L':')) return false;
            Skip();
            if (key == L"prompts") {
                if (!ReadPromptArray(items)) return false;
            } else {
                if (!SkipValue()) return false;
            }
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L'}')) break;
        }
        return true;
    }

    bool ReadMindMaps(std::vector<MindMap>& maps) {
        Skip();
        if (!Consume(L'{')) return false;
        while (true) {
            Skip();
            if (Consume(L'}')) break;
            std::wstring key;
            if (!ReadString(key)) return false;
            Skip();
            if (!Consume(L':')) return false;
            Skip();
            if (key == L"mindmaps") {
                if (!ReadMindMapArray(maps)) return false;
            } else {
                if (!SkipValue()) return false;
            }
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L'}')) break;
        }
        return true;
    }

private:
    const std::wstring& text;
    size_t pos = 0;

    void Skip() {
        while (pos < text.size() && iswspace(text[pos])) ++pos;
    }

    bool Consume(wchar_t expected) {
        Skip();
        if (pos < text.size() && text[pos] == expected) {
            ++pos;
            return true;
        }
        return false;
    }

    bool ReadString(std::wstring& out) {
        Skip();
        if (pos >= text.size() || text[pos] != L'"') return false;
        ++pos;
        out.clear();
        while (pos < text.size()) {
            wchar_t ch = text[pos++];
            if (ch == L'"') return true;
            if (ch != L'\\') {
                out.push_back(ch);
                continue;
            }
            if (pos >= text.size()) return false;
            wchar_t esc = text[pos++];
            switch (esc) {
            case L'"': out.push_back(L'"'); break;
            case L'\\': out.push_back(L'\\'); break;
            case L'/': out.push_back(L'/'); break;
            case L'b': out.push_back(L'\b'); break;
            case L'f': out.push_back(L'\f'); break;
            case L'n': out.push_back(L'\n'); break;
            case L'r': out.push_back(L'\r'); break;
            case L't': out.push_back(L'\t'); break;
            case L'u': {
                if (pos + 4 > text.size()) return false;
                unsigned value = 0;
                for (int i = 0; i < 4; ++i) {
                    wchar_t h = text[pos++];
                    value <<= 4;
                    if (h >= L'0' && h <= L'9') value += h - L'0';
                    else if (h >= L'a' && h <= L'f') value += 10 + h - L'a';
                    else if (h >= L'A' && h <= L'F') value += 10 + h - L'A';
                    else return false;
                }
                out.push_back((wchar_t)value);
                break;
            }
            default:
                out.push_back(esc);
                break;
            }
        }
        return false;
    }

    bool ReadPromptArray(std::vector<PromptItem>& items) {
        if (!Consume(L'[')) return false;
        while (true) {
            Skip();
            if (Consume(L']')) return true;
            PromptItem item;
            if (!ReadPromptObject(item)) return false;
            if (!item.title.empty()) items.push_back(item);
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L']')) return true;
            return false;
        }
    }

    bool ReadPromptObject(PromptItem& item) {
        if (!Consume(L'{')) return false;
        while (true) {
            Skip();
            if (Consume(L'}')) return true;
            std::wstring key;
            std::wstring value;
            if (!ReadString(key)) return false;
            Skip();
            if (!Consume(L':')) return false;
            Skip();
            if (!ReadString(value)) {
                if (!SkipValue()) return false;
            } else {
                if (key == L"id") item.id = value;
                else if (key == L"title") item.title = value;
                else if (key == L"category") item.category = value;
                else if (key == L"tags") item.tags = value;
                else if (key == L"content") item.content = value;
            }
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L'}')) return true;
            return false;
        }
    }

    bool ReadMindMapArray(std::vector<MindMap>& maps) {
        if (!Consume(L'[')) return false;
        while (true) {
            Skip();
            if (Consume(L']')) return true;
            MindMap map;
            if (!ReadMindMapObject(map)) return false;
            if (!map.title.empty() && !map.root.title.empty()) maps.push_back(map);
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L']')) return true;
            return false;
        }
    }

    bool ReadMindMapObject(MindMap& map) {
        if (!Consume(L'{')) return false;
        while (true) {
            Skip();
            if (Consume(L'}')) return true;
            std::wstring key;
            if (!ReadString(key)) return false;
            Skip();
            if (!Consume(L':')) return false;
            Skip();
            if (key == L"root") {
                if (!ReadMindNodeObject(map.root)) return false;
            } else {
                std::wstring value;
                if (ReadString(value)) {
                    if (key == L"id") map.id = value;
                    else if (key == L"title") map.title = value;
                } else if (!SkipValue()) {
                    return false;
                }
            }
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L'}')) return true;
            return false;
        }
    }

    bool ReadMindNodeArray(std::vector<MindNode>& nodes) {
        if (!Consume(L'[')) return false;
        while (true) {
            Skip();
            if (Consume(L']')) return true;
            MindNode node;
            if (!ReadMindNodeObject(node)) return false;
            if (!node.title.empty()) nodes.push_back(node);
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L']')) return true;
            return false;
        }
    }

    bool ReadMindNodeObject(MindNode& node) {
        if (!Consume(L'{')) return false;
        while (true) {
            Skip();
            if (Consume(L'}')) return true;
            std::wstring key;
            if (!ReadString(key)) return false;
            Skip();
            if (!Consume(L':')) return false;
            Skip();
            if (key == L"children") {
                if (!ReadMindNodeArray(node.children)) return false;
            } else {
                std::wstring value;
                if (ReadString(value)) {
                    if (key == L"id") node.id = value;
                    else if (key == L"title") node.title = value;
                    else if (key == L"promptTitle") node.promptTitle = value;
                } else if (!SkipValue()) {
                    return false;
                }
            }
            Skip();
            if (Consume(L',')) continue;
            if (Consume(L'}')) return true;
            return false;
        }
    }

    bool SkipValue() {
        Skip();
        if (pos >= text.size()) return false;
        if (text[pos] == L'"') {
            std::wstring ignored;
            return ReadString(ignored);
        }
        if (text[pos] == L'{') {
            ++pos;
            int depth = 1;
            while (pos < text.size() && depth > 0) {
                if (text[pos] == L'"') {
                    std::wstring ignored;
                    if (!ReadString(ignored)) return false;
                } else if (text[pos] == L'{') {
                    ++depth;
                    ++pos;
                } else if (text[pos] == L'}') {
                    --depth;
                    ++pos;
                } else {
                    ++pos;
                }
            }
            return depth == 0;
        }
        if (text[pos] == L'[') {
            ++pos;
            int depth = 1;
            while (pos < text.size() && depth > 0) {
                if (text[pos] == L'"') {
                    std::wstring ignored;
                    if (!ReadString(ignored)) return false;
                } else if (text[pos] == L'[') {
                    ++depth;
                    ++pos;
                } else if (text[pos] == L']') {
                    --depth;
                    ++pos;
                } else {
                    ++pos;
                }
            }
            return depth == 0;
        }
        while (pos < text.size() && wcschr(L",}] \r\n\t", text[pos]) == nullptr) ++pos;
        return true;
    }
};

static std::wstring EscapeJson(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size() + 16);
    for (wchar_t ch : value) {
        switch (ch) {
        case L'"': out += L"\\\""; break;
        case L'\\': out += L"\\\\"; break;
        case L'\b': out += L"\\b"; break;
        case L'\f': out += L"\\f"; break;
        case L'\n': out += L"\\n"; break;
        case L'\r': out += L"\\r"; break;
        case L'\t': out += L"\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

static std::wstring DataPath() {
    return g_dataPath;
}

static std::wstring MarkdownPath() {
    return g_markdownPath;
}

static void ResolveDataPaths() {
    std::wstring cwd = CurrentDirectory();
    std::wstring exe = ExecutableDirectory();
    std::wstring buildRoot = ParentDirectory(ParentDirectory(exe));

    std::vector<std::wstring> folders = {cwd, exe, buildRoot};
    for (const auto& folder : folders) {
        std::wstring candidate = JoinPath(folder, L"prompts.json");
        if (FileExists(candidate)) {
            g_dataPath = candidate;
            break;
        }
    }
    for (const auto& folder : folders) {
        std::wstring candidate = JoinPath(folder, L"mindmaps.json");
        if (FileExists(candidate)) {
            g_mindMapPath = candidate;
            break;
        }
    }
    for (const auto& folder : folders) {
        std::wstring candidate = JoinPath(folder, L"原创小说写作提示词库.md");
        if (FileExists(candidate)) {
            g_markdownPath = candidate;
            if (!FileExists(g_dataPath)) g_dataPath = JoinPath(folder, L"prompts.json");
            if (!FileExists(g_mindMapPath)) g_mindMapPath = JoinPath(folder, L"mindmaps.json");
            break;
        }
    }
}

static std::wstring CleanHeading(std::wstring line) {
    while (!line.empty() && line.front() == L'#') line.erase(line.begin());
    line = Trim(line);
    size_t i = 0;
    while (i < line.size() && (iswdigit(line[i]) || line[i] == L'.' || iswspace(line[i]))) ++i;
    return Trim(line.substr(i));
}

static std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    std::wstring current;
    for (wchar_t ch : text) {
        if (ch == L'\n') {
            if (!current.empty() && current.back() == L'\r') current.pop_back();
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}

static std::vector<PromptItem> ImportMarkdown() {
    std::wstring text;
    if (!ReadUtf8File(MarkdownPath(), text)) return {};

    std::vector<PromptItem> items;
    std::wstring category = L"未分类";
    PromptItem current;
    std::vector<std::wstring> contentLines;

    auto flush = [&]() {
        if (current.title.empty()) return;
        std::wstring content;
        for (size_t i = 0; i < contentLines.size(); ++i) {
            if (i) content += L"\r\n";
            content += contentLines[i];
        }
        current.id = NewId();
        current.category = category;
        current.tags = category;
        current.content = Trim(content);
        items.push_back(current);
        current = PromptItem{};
        contentLines.clear();
    };

    for (const std::wstring& line : SplitLines(text)) {
        if (line.rfind(L"### ", 0) == 0) {
            flush();
            current.title = CleanHeading(line);
        } else if (line.rfind(L"## ", 0) == 0) {
            flush();
            std::wstring heading = CleanHeading(line);
            if (!heading.empty() && heading != L"使用方式") category = heading;
        } else if (!current.title.empty()) {
            contentLines.push_back(line);
        }
    }
    flush();
    return items;
}

static bool LoadPrompts() {
    std::wstring json;
    if (ReadUtf8File(DataPath(), json)) {
        std::vector<PromptItem> loaded;
        JsonReader reader(json);
        if (reader.ReadPrompts(loaded) && !loaded.empty()) {
            g_prompts = loaded;
            return true;
        }
    }
    g_prompts = ImportMarkdown();
    return !g_prompts.empty();
}

static bool SavePrompts() {
    std::wstring json = L"{\r\n  \"prompts\": [\r\n";
    for (size_t i = 0; i < g_prompts.size(); ++i) {
        const auto& item = g_prompts[i];
        json += L"    {\r\n";
        json += L"      \"id\": \"" + EscapeJson(item.id) + L"\",\r\n";
        json += L"      \"title\": \"" + EscapeJson(item.title) + L"\",\r\n";
        json += L"      \"category\": \"" + EscapeJson(item.category) + L"\",\r\n";
        json += L"      \"tags\": \"" + EscapeJson(item.tags) + L"\",\r\n";
        json += L"      \"content\": \"" + EscapeJson(item.content) + L"\"\r\n";
        json += L"    }";
        if (i + 1 < g_prompts.size()) json += L",";
        json += L"\r\n";
    }
    json += L"  ]\r\n}\r\n";
    return WriteUtf8File(DataPath(), json);
}

static std::wstring MindMapPath() {
    return g_mindMapPath;
}

static MindNode MakeMindNode(const std::wstring& title, const std::wstring& promptTitle, std::vector<MindNode> children = {}) {
    MindNode node;
    node.id = NewId();
    node.title = title;
    node.promptTitle = promptTitle;
    node.children = std::move(children);
    return node;
}

static MindMap DefaultNovelFlowMap() {
    MindMap map;
    map.id = NewId();
    map.title = L"AI写小说全流程";
    map.root = MakeMindNode(
        L"AI写小说全流程",
        L"从创意到开篇",
        {
            MakeMindNode(L"第一步：扫榜", L"热门题材发散", {
                MakeMindNode(L"找到要写的对标书", L"热门题材发散"),
                MakeMindNode(L"验证题材有读者", L"热门题材发散")
            }),
            MakeMindNode(L"第二步：制作大纲", L"长篇网文总纲", {
                MakeMindNode(L"书名", L"标题与简介"),
                MakeMindNode(L"简介", L"标题与简介")
            }),
            MakeMindNode(L"第三步：制作细纲", L"单章细纲", {
                MakeMindNode(L"世界设定", L"玄幻世界观"),
                MakeMindNode(L"角色塑造", L"主角人设卡"),
                MakeMindNode(L"事件细纲", L"连续 10 章规划")
            }),
            MakeMindNode(L"第四步：生成正文", L"保持风格续写", {
                MakeMindNode(L"根据细纲生成正文", L"保持风格续写"),
                MakeMindNode(L"人审修稿", L"章节从草稿到精修"),
                MakeMindNode(L"逻辑问题", L"逻辑与节奏检查"),
                MakeMindNode(L"删除冗余废话", L"章节从草稿到精修")
            }),
            MakeMindNode(L"第五步：发布", L"平台风格适配")
        }
    );
    return map;
}

static void EnsureMindNodeIds(MindNode& node) {
    if (node.id.empty()) node.id = NewId();
    for (auto& child : node.children) EnsureMindNodeIds(child);
}

static void EnsureMindMapIds(MindMap& map) {
    if (map.id.empty()) map.id = NewId();
    EnsureMindNodeIds(map.root);
}

static void SerializeMindNode(const MindNode& node, std::wstring& json, int indent) {
    std::wstring pad(indent, L' ');
    std::wstring childPad(indent + 2, L' ');
    json += pad + L"{\r\n";
    json += childPad + L"\"id\": \"" + EscapeJson(node.id) + L"\",\r\n";
    json += childPad + L"\"title\": \"" + EscapeJson(node.title) + L"\",\r\n";
    json += childPad + L"\"promptTitle\": \"" + EscapeJson(node.promptTitle) + L"\",\r\n";
    json += childPad + L"\"children\": [";
    if (!node.children.empty()) json += L"\r\n";
    for (size_t i = 0; i < node.children.size(); ++i) {
        SerializeMindNode(node.children[i], json, indent + 4);
        if (i + 1 < node.children.size()) json += L",";
        json += L"\r\n";
    }
    json += childPad + L"]\r\n";
    json += pad + L"}";
}

static std::wstring SerializeMindMaps(const std::vector<MindMap>& maps) {
    std::wstring json = L"{\r\n  \"mindmaps\": [\r\n";
    for (size_t i = 0; i < maps.size(); ++i) {
        const auto& map = maps[i];
        json += L"    {\r\n";
        json += L"      \"id\": \"" + EscapeJson(map.id) + L"\",\r\n";
        json += L"      \"title\": \"" + EscapeJson(map.title) + L"\",\r\n";
        json += L"      \"root\": ";
        SerializeMindNode(map.root, json, 6);
        json += L"\r\n    }";
        if (i + 1 < maps.size()) json += L",";
        json += L"\r\n";
    }
    json += L"  ]\r\n}\r\n";
    return json;
}

static bool SaveMindMaps() {
    return WriteUtf8File(MindMapPath(), SerializeMindMaps(g_mindMaps));
}

static bool LoadMindMaps() {
    std::wstring json;
    if (ReadUtf8File(MindMapPath(), json)) {
        std::vector<MindMap> loaded;
        JsonReader reader(json);
        if (reader.ReadMindMaps(loaded) && !loaded.empty()) {
            for (auto& map : loaded) EnsureMindMapIds(map);
            g_mindMaps = loaded;
            g_currentMap = std::min(g_currentMap, (int)g_mindMaps.size() - 1);
            return true;
        }
    }
    g_mindMaps = {DefaultNovelFlowMap()};
    g_currentMap = 0;
    SaveMindMaps();
    return true;
}

static MindMap* CurrentMindMap() {
    if (g_currentMap < 0 || g_currentMap >= (int)g_mindMaps.size()) return nullptr;
    return &g_mindMaps[g_currentMap];
}

static MindNode* FindNodeById(MindNode& node, const std::wstring& id) {
    if (node.id == id) return &node;
    for (auto& child : node.children) {
        if (MindNode* found = FindNodeById(child, id)) return found;
    }
    return nullptr;
}

static const MindNode* FindNodeByIdConst(const MindNode& node, const std::wstring& id) {
    if (node.id == id) return &node;
    for (const auto& child : node.children) {
        if (const MindNode* found = FindNodeByIdConst(child, id)) return found;
    }
    return nullptr;
}

static bool DeleteNodeById(MindNode& node, const std::wstring& id) {
    auto it = std::remove_if(node.children.begin(), node.children.end(), [&](const MindNode& child) {
        return child.id == id;
    });
    if (it != node.children.end()) {
        node.children.erase(it, node.children.end());
        return true;
    }
    for (auto& child : node.children) {
        if (DeleteNodeById(child, id)) return true;
    }
    return false;
}

static bool ContainsNodeId(const MindNode& node, const std::wstring& id) {
    if (node.id == id) return true;
    for (const auto& child : node.children) {
        if (ContainsNodeId(child, id)) return true;
    }
    return false;
}

static bool ExtractNodeById(MindNode& parent, const std::wstring& id, MindNode& extracted) {
    for (auto it = parent.children.begin(); it != parent.children.end(); ++it) {
        if (it->id == id) {
            extracted = std::move(*it);
            parent.children.erase(it);
            return true;
        }
    }
    for (auto& child : parent.children) {
        if (ExtractNodeById(child, id, extracted)) return true;
    }
    return false;
}

static bool InsertSiblingById(MindNode& parent, const std::wstring& targetId, const MindNode& nodeToInsert, bool after) {
    for (auto it = parent.children.begin(); it != parent.children.end(); ++it) {
        if (it->id == targetId) {
            parent.children.insert(after ? std::next(it) : it, nodeToInsert);
            return true;
        }
    }
    for (auto& child : parent.children) {
        if (InsertSiblingById(child, targetId, nodeToInsert, after)) return true;
    }
    return false;
}

static bool MoveMindNode(const std::wstring& sourceId, const std::wstring& targetId, int mode) {
    MindMap* map = CurrentMindMap();
    if (!map || sourceId.empty() || targetId.empty() || sourceId == targetId) return false;
    if (sourceId == map->root.id) return false;

    const MindNode* source = FindNodeByIdConst(map->root, sourceId);
    if (!source || ContainsNodeId(*source, targetId)) return false;

    MindNode moved;
    if (!ExtractNodeById(map->root, sourceId, moved)) return false;

    bool inserted = false;
    if (mode == 2) {
        if (MindNode* target = FindNodeById(map->root, targetId)) {
            target->children.push_back(std::move(moved));
            inserted = true;
        }
    } else if (targetId == map->root.id) {
        if (mode < 2) {
            map->root.children.insert(map->root.children.begin(), std::move(moved));
        } else {
            map->root.children.push_back(std::move(moved));
        }
        inserted = true;
    } else {
        inserted = InsertSiblingById(map->root, targetId, moved, mode > 2);
    }

    if (!inserted) {
        map->root.children.push_back(std::move(moved));
    }
    g_selectedNodeId = sourceId;
    SaveMindMaps();
    return inserted;
}

static void SetStatus(const std::wstring& text) {
    if (g_status) SetWindowTextW(g_status, text.c_str());
}

static void AddCategories() {
    SendMessageW(g_category, CB_RESETCONTENT, 0, 0);
    SendMessageW(g_category, CB_ADDSTRING, 0, (LPARAM)L"全部分类");
    std::vector<std::wstring> categories;
    for (const auto& item : g_prompts) {
        if (!item.category.empty() && std::find(categories.begin(), categories.end(), item.category) == categories.end()) {
            categories.push_back(item.category);
        }
    }
    std::sort(categories.begin(), categories.end());
    for (const auto& category : categories) {
        SendMessageW(g_category, CB_ADDSTRING, 0, (LPARAM)category.c_str());
    }
    SendMessageW(g_category, CB_SETCURSEL, 0, 0);
}

static bool Matches(const PromptItem& item, const std::wstring& keyword, const std::wstring& category) {
    if (category != L"全部分类" && item.category != category) return false;
    if (keyword.empty()) return true;
    std::wstring haystack = Lower(item.title + L"\n" + item.category + L"\n" + item.tags + L"\n" + item.content);
    return haystack.find(keyword) != std::wstring::npos;
}

static void LoadToEditor(int index) {
    if (index < 0 || index >= (int)g_prompts.size()) return;
    g_loading = true;
    const auto& item = g_prompts[index];
    g_current = index;
    SetText(g_title, item.title);
    SetText(g_editCategory, item.category);
    SetText(g_tags, item.tags);
    SetText(g_content, item.content);
    SetStatus(L"正在编辑：" + item.title);
    g_loading = false;
}

static void ClearEditor() {
    g_current = -1;
    SetText(g_title, L"");
    SetText(g_editCategory, L"");
    SetText(g_tags, L"");
    SetText(g_content, L"");
    SetStatus(L"新提示词");
}

static void ApplyFilters() {
    if (!g_list) return;
    std::wstring keyword = Lower(Trim(GetText(g_search)));
    wchar_t categoryBuffer[256]{};
    int selected = (int)SendMessageW(g_category, CB_GETCURSEL, 0, 0);
    if (selected >= 0) SendMessageW(g_category, CB_GETLBTEXT, selected, (LPARAM)categoryBuffer);
    std::wstring category = categoryBuffer[0] ? categoryBuffer : L"全部分类";

    g_filtered.clear();
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < (int)g_prompts.size(); ++i) {
        if (!Matches(g_prompts[i], keyword, category)) continue;
        g_filtered.push_back(i);
        std::wstring label = g_prompts[i].title + L" - " + g_prompts[i].category;
        SendMessageW(g_list, LB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    std::wstring count = L"共 " + std::to_wstring(g_filtered.size()) + L" 条，全部 " + std::to_wstring(g_prompts.size()) + L" 条";
    SetWindowTextW(g_count, count.c_str());

    if (!g_filtered.empty()) {
        int listIndex = 0;
        for (int i = 0; i < (int)g_filtered.size(); ++i) {
            if (g_filtered[i] == g_current) {
                listIndex = i;
                break;
            }
        }
        SendMessageW(g_list, LB_SETCURSEL, listIndex, 0);
        LoadToEditor(g_filtered[listIndex]);
    } else {
        ClearEditor();
        SetStatus(L"没有匹配的提示词");
    }
}

static bool ReadEditor(PromptItem& item) {
    item.title = Trim(GetText(g_title));
    item.category = Trim(GetText(g_editCategory));
    item.tags = Trim(GetText(g_tags));
    item.content = Trim(GetEditText(g_content));
    if (item.category.empty()) item.category = L"未分类";
    if (item.title.empty()) {
        MessageBoxW(g_main, L"请先填写标题。", APP_TITLE, MB_ICONWARNING);
        SetFocus(g_title);
        return false;
    }
    if (item.content.empty()) {
        MessageBoxW(g_main, L"请先填写提示词内容。", APP_TITLE, MB_ICONWARNING);
        SetFocus(g_content);
        return false;
    }
    item.id = (g_current >= 0) ? g_prompts[g_current].id : NewId();
    return true;
}

static void SaveCurrent() {
    PromptItem item;
    if (!ReadEditor(item)) return;
    if (g_current >= 0 && g_current < (int)g_prompts.size()) {
        g_prompts[g_current] = item;
    } else {
        g_prompts.push_back(item);
        g_current = (int)g_prompts.size() - 1;
    }
    if (!SavePrompts()) {
        MessageBoxW(g_main, L"保存失败，请确认当前文件夹可写。", APP_TITLE, MB_ICONERROR);
        return;
    }
    AddCategories();
    ApplyFilters();
    SetStatus(L"已保存");
}

static void DeleteCurrent() {
    if (g_current < 0 || g_current >= (int)g_prompts.size()) {
        MessageBoxW(g_main, L"请先选择要删除的提示词。", APP_TITLE, MB_ICONINFORMATION);
        return;
    }
    std::wstring message = L"确定删除“" + g_prompts[g_current].title + L"”吗？";
    if (MessageBoxW(g_main, message.c_str(), APP_TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    g_prompts.erase(g_prompts.begin() + g_current);
    g_current = -1;
    SavePrompts();
    AddCategories();
    ApplyFilters();
    SetStatus(L"已删除");
}

static void CopyCurrent() {
    std::wstring text = Trim(GetEditText(g_content));
    if (text.empty()) {
        MessageBoxW(g_main, L"当前没有可复制的提示词内容。", APP_TITLE, MB_ICONINFORMATION);
        return;
    }
    if (!OpenClipboard(g_main)) return;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory) {
        void* target = GlobalLock(memory);
        memcpy(target, text.c_str(), bytes);
        GlobalUnlock(memory);
        SetClipboardData(CF_UNICODETEXT, memory);
    }
    CloseClipboard();
    SetStatus(L"提示词已复制到剪贴板");
}

static void FocusPromptByTitle(const std::wstring& title) {
    if (title.empty()) return;
    int found = -1;
    for (int i = 0; i < (int)g_prompts.size(); ++i) {
        if (g_prompts[i].title == title || g_prompts[i].title.find(title) != std::wstring::npos || title.find(g_prompts[i].title) != std::wstring::npos) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        SendMessageW(g_category, CB_SETCURSEL, 0, 0);
        SetText(g_search, title);
        ApplyFilters();
        SetStatus(L"已搜索：" + title);
        SetForegroundWindow(g_main);
        return;
    }

    SendMessageW(g_category, CB_SETCURSEL, 0, 0);
    SetText(g_search, g_prompts[found].title);
    ApplyFilters();
    g_current = found;
    for (int i = 0; i < (int)g_filtered.size(); ++i) {
        if (g_filtered[i] == found) {
            SendMessageW(g_list, LB_SETCURSEL, i, 0);
            break;
        }
    }
    LoadToEditor(found);
    SetForegroundWindow(g_main);
    SetFocus(g_content);
}

static void DrawMindText(HDC dc, const RECT& rect, const std::wstring& text, COLORREF color, HFONT font, UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE) {
    HFONT oldFont = (HFONT)SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    RECT copy = rect;
    DrawTextW(dc, text.c_str(), -1, &copy, format);
    SelectObject(dc, oldFont);
}

static void DrawRoundedNode(HDC dc, const RECT& rect, const std::wstring& label, COLORREF fill, COLORREF text, HFONT font, int radius = 8) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, fill);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
    DrawMindText(dc, rect, label, text, font);
}

static void DrawPolyline(HDC dc, const std::vector<POINT>& points) {
    if (points.size() < 2) return;
    Polyline(dc, points.data(), (int)points.size());
}

static void AddHotspot(const RECT& rect, const std::wstring& label, const std::wstring& promptTitle, const std::wstring& nodeId = L"") {
    g_mindHotspots.push_back(MapHotspot{rect, label, promptTitle, nodeId});
}

static RECT MindMapCanvasRect(const RECT& client) {
    RECT canvas = client;
    canvas.left = 318;
    canvas.top = 158;
    canvas.right -= 16;
    canvas.bottom -= 34;
    if (canvas.right < canvas.left + 220) canvas.right = canvas.left + 220;
    if (canvas.bottom < canvas.top + 160) canvas.bottom = canvas.top + 160;
    return canvas;
}

static int MindLeafCount(const MindNode& node) {
    if (node.children.empty()) return 1;
    int total = 0;
    for (const auto& child : node.children) total += MindLeafCount(child);
    return std::max(1, total);
}

static int MindDepth(const MindNode& node) {
    int depth = 1;
    for (const auto& child : node.children) depth = std::max(depth, 1 + MindDepth(child));
    return depth;
}

static void DrawMindNodeRecursive(
    HDC dc,
    const MindNode& node,
    int depth,
    int maxDepth,
    int left,
    int right,
    int top,
    int bottom,
    HFONT nodeFont,
    HFONT rootFont,
    HPEN linePen
) {
    int centerY = (top + bottom) / 2;
    int availableW = right - left;
    int columnW = std::max(150, availableW / std::max(1, maxDepth));
    int x = left + depth * columnW;
    int nodeW = depth == 0 ? 210 : 165;
    int nodeH = depth == 0 ? 44 : 34;
    RECT nodeRect{x, centerY - nodeH / 2, x + nodeW, centerY + nodeH / 2};

    COLORREF fill = depth == 0 ? RGB(23, 151, 229) : RGB(76, 90, 111);
    COLORREF text = depth == 0 ? RGB(255, 255, 255) : RGB(221, 230, 236);
    DrawRoundedNode(dc, nodeRect, node.title, fill, text, depth == 0 ? rootFont : nodeFont, 8);
    AddHotspot(nodeRect, node.title, node.promptTitle, node.id);

    if (node.children.empty()) return;

    int totalLeaves = MindLeafCount(node);
    int cursor = top;
    POINT from{nodeRect.right, centerY};
    HGDIOBJ oldPen = SelectObject(dc, linePen);
    for (const auto& child : node.children) {
        int childLeaves = MindLeafCount(child);
        int childTop = cursor;
        int childBottom = cursor + MulDiv(bottom - top, childLeaves, totalLeaves);
        if (&child == &node.children.back()) childBottom = bottom;
        int childCenterY = (childTop + childBottom) / 2;
        int childX = left + (depth + 1) * columnW;
        POINT mid{from.x + 24, from.y};
        POINT down{mid.x, childCenterY};
        POINT to{childX, childCenterY};
        DrawPolyline(dc, {from, mid, down, to});
        cursor = childBottom;
    }
    SelectObject(dc, oldPen);

    cursor = top;
    for (const auto& child : node.children) {
        int childLeaves = MindLeafCount(child);
        int childTop = cursor;
        int childBottom = cursor + MulDiv(bottom - top, childLeaves, totalLeaves);
        if (&child == &node.children.back()) childBottom = bottom;
        DrawMindNodeRecursive(dc, child, depth + 1, maxDepth, left, right, childTop, childBottom, nodeFont, rootFont, linePen);
        cursor = childBottom;
    }
}

static void DrawMindMap(HDC dc, const RECT& canvas) {
    g_mindHotspots.clear();

    HBRUSH bg = CreateSolidBrush(RGB(29, 39, 51));
    FillRect(dc, &canvas, bg);
    DeleteObject(bg);

    HFONT mainFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    HFONT stepFont = CreateFontW(21, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    HFONT itemFont = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");

    HPEN linePen = CreatePen(PS_SOLID, 3, RGB(132, 164, 191));
    RECT hint{canvas.left + 16, canvas.top + 10, canvas.right - 16, canvas.top + 38};
    DrawMindText(dc, hint, L"单击节点选中，双击节点定位对应提示词", RGB(137, 160, 178), itemFont, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    if (MindMap* map = CurrentMindMap()) {
        int marginX = 34;
        int marginY = 56;
        int maxDepth = std::max(1, MindDepth(map->root));
        DrawMindNodeRecursive(
            dc,
            map->root,
            0,
            maxDepth,
            canvas.left + marginX,
            canvas.right - marginX,
            canvas.top + marginY,
            canvas.bottom - marginY,
            stepFont,
            mainFont,
            linePen
        );
    } else {
        RECT empty{canvas.left, canvas.top, canvas.right, canvas.bottom};
        DrawMindText(dc, empty, L"暂无思维导图", RGB(210, 222, 231), mainFont);
    }

    DeleteObject(linePen);
    DeleteObject(mainFont);
    DeleteObject(stepFont);
    DeleteObject(itemFont);
}

static void SetMapStatus(const std::wstring& text) {
    if (g_mapStatus) SetWindowTextW(g_mapStatus, text.c_str());
}

static void RegenerateMindNodeIds(MindNode& node) {
    node.id = NewId();
    for (auto& child : node.children) RegenerateMindNodeIds(child);
}

static void RefreshMapSelector() {
    if (!g_mapSelector) return;
    g_loadingMapUi = true;
    SendMessageW(g_mapSelector, CB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < g_mindMaps.size(); ++i) {
        std::wstring title = std::to_wstring(i + 1) + L" / " + std::to_wstring(g_mindMaps.size()) + L"  " + g_mindMaps[i].title;
        SendMessageW(g_mapSelector, CB_ADDSTRING, 0, (LPARAM)title.c_str());
    }
    if (!g_mindMaps.empty()) {
        g_currentMap = std::max(0, std::min(g_currentMap, (int)g_mindMaps.size() - 1));
        SendMessageW(g_mapSelector, CB_SETCURSEL, g_currentMap, 0);
    }
    g_loadingMapUi = false;
}

static void InsertMindTreeNode(HWND tree, HTREEITEM parent, const MindNode& node) {
    int index = (int)g_treeNodeIds.size();
    g_treeNodeIds.push_back(node.id);
    TVINSERTSTRUCTW insert{};
    insert.hParent = parent;
    insert.hInsertAfter = TVI_LAST;
    insert.item.mask = TVIF_TEXT | TVIF_PARAM;
    insert.item.pszText = const_cast<LPWSTR>(node.title.c_str());
    insert.item.lParam = index;
    HTREEITEM item = TreeView_InsertItem(tree, &insert);
    for (const auto& child : node.children) InsertMindTreeNode(tree, item, child);
    TreeView_Expand(tree, item, TVE_EXPAND);
}

static std::wstring TreeItemNodeId(HTREEITEM item) {
    if (!g_mapTree || !item) return L"";
    TVITEMW tv{};
    tv.mask = TVIF_PARAM;
    tv.hItem = item;
    if (!TreeView_GetItem(g_mapTree, &tv)) return L"";
    int index = (int)tv.lParam;
    if (index < 0 || index >= (int)g_treeNodeIds.size()) return L"";
    return g_treeNodeIds[index];
}

static HTREEITEM FindTreeItemByNodeId(HTREEITEM item, const std::wstring& id) {
    while (item) {
        if (TreeItemNodeId(item) == id) return item;
        HTREEITEM child = TreeView_GetChild(g_mapTree, item);
        if (HTREEITEM found = FindTreeItemByNodeId(child, id)) return found;
        item = TreeView_GetNextSibling(g_mapTree, item);
    }
    return nullptr;
}

static void SelectTreeNodeById(const std::wstring& id) {
    if (!g_mapTree || id.empty()) return;
    HTREEITEM root = TreeView_GetRoot(g_mapTree);
    if (HTREEITEM item = FindTreeItemByNodeId(root, id)) {
        TreeView_SelectItem(g_mapTree, item);
        TreeView_EnsureVisible(g_mapTree, item);
    }
}

static void RefreshMindTree() {
    if (!g_mapTree) return;
    g_loadingMapUi = true;
    g_treeNodeIds.clear();
    TreeView_DeleteAllItems(g_mapTree);
    if (MindMap* map = CurrentMindMap()) {
        InsertMindTreeNode(g_mapTree, nullptr, map->root);
        if (g_selectedNodeId.empty()) g_selectedNodeId = map->root.id;
    }
    g_loadingMapUi = false;
    SelectTreeNodeById(g_selectedNodeId);
}

static MindNode* SelectedMindNode() {
    MindMap* map = CurrentMindMap();
    if (!map) return nullptr;
    if (g_selectedNodeId.empty()) g_selectedNodeId = map->root.id;
    return FindNodeById(map->root, g_selectedNodeId);
}

static void LoadSelectedNodeToEditor() {
    g_loadingMapUi = true;
    if (MindNode* node = SelectedMindNode()) {
        SetText(g_nodeTitle, node->title);
        SetText(g_nodePrompt, node->promptTitle);
        SetMapStatus(L"正在编辑节点：" + node->title);
    } else {
        SetText(g_nodeTitle, L"");
        SetText(g_nodePrompt, L"");
        SetMapStatus(L"未选择节点");
    }
    g_loadingMapUi = false;
}

static void RefreshMindMapUi(bool reloadTree = true) {
    RefreshMapSelector();
    if (reloadTree) RefreshMindTree();
    LoadSelectedNodeToEditor();
    if (g_mindMap) InvalidateRect(g_mindMap, nullptr, TRUE);
}

static void SaveSelectedNode() {
    MindMap* map = CurrentMindMap();
    MindNode* node = SelectedMindNode();
    if (!map || !node) return;
    std::wstring title = Trim(GetText(g_nodeTitle));
    if (title.empty()) {
        MessageBoxW(g_mindMap, L"节点标题不能为空。", APP_TITLE, MB_ICONWARNING);
        SetFocus(g_nodeTitle);
        return;
    }
    node->title = title;
    node->promptTitle = Trim(GetText(g_nodePrompt));
    if (node->id == map->root.id) map->title = title;
    SaveMindMaps();
    RefreshMindMapUi(true);
    SetMapStatus(L"节点已保存");
}

static void AddChildNode() {
    MindMap* map = CurrentMindMap();
    MindNode* node = SelectedMindNode();
    if (!map) return;
    if (!node) node = &map->root;
    MindNode child = MakeMindNode(L"新节点", L"");
    g_selectedNodeId = child.id;
    node->children.push_back(child);
    SaveMindMaps();
    RefreshMindMapUi(true);
    SetMapStatus(L"已添加子节点");
}

static void DeleteSelectedNode() {
    MindMap* map = CurrentMindMap();
    if (!map || g_selectedNodeId.empty()) return;
    if (g_selectedNodeId == map->root.id) {
        MessageBoxW(g_mindMap, L"根节点不能删除。可以删除整张导图。", APP_TITLE, MB_ICONINFORMATION);
        return;
    }
    if (MessageBoxW(g_mindMap, L"确定删除当前节点及其子节点吗？", APP_TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    DeleteNodeById(map->root, g_selectedNodeId);
    g_selectedNodeId = map->root.id;
    SaveMindMaps();
    RefreshMindMapUi(true);
    SetMapStatus(L"节点已删除");
}

static void NewMindMapPage() {
    MindMap map;
    map.id = NewId();
    map.title = L"新思维导图";
    map.root = MakeMindNode(L"新思维导图", L"");
    g_mindMaps.push_back(map);
    g_currentMap = (int)g_mindMaps.size() - 1;
    g_selectedNodeId = g_mindMaps[g_currentMap].root.id;
    SaveMindMaps();
    RefreshMindMapUi(true);
    SetMapStatus(L"已新建导图");
}

static void DeleteMindMapPage() {
    if (g_mindMaps.empty()) return;
    if (MessageBoxW(g_mindMap, L"确定删除当前思维导图吗？", APP_TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    g_mindMaps.erase(g_mindMaps.begin() + g_currentMap);
    if (g_mindMaps.empty()) g_mindMaps.push_back(DefaultNovelFlowMap());
    g_currentMap = std::max(0, std::min(g_currentMap, (int)g_mindMaps.size() - 1));
    g_selectedNodeId = g_mindMaps[g_currentMap].root.id;
    SaveMindMaps();
    RefreshMindMapUi(true);
    SetMapStatus(L"导图已删除");
}

static void SelectRootForRename() {
    if (MindMap* map = CurrentMindMap()) {
        g_selectedNodeId = map->root.id;
        RefreshMindMapUi(true);
        SetFocus(g_nodeTitle);
        SendMessageW(g_nodeTitle, EM_SETSEL, 0, -1);
        SetMapStatus(L"修改根节点标题后点“保存节点”，即可重命名导图");
    }
}

static std::wstring PickFile(bool save, const wchar_t* filter, const wchar_t* defaultExt) {
    wchar_t fileName[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_mindMap;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = save ? (OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST) : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
    BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    return ok ? std::wstring(fileName) : L"";
}

static std::wstring PickJsonFile(bool save) {
    return PickFile(save, L"JSON 文件 (*.json)\0*.json\0所有文件 (*.*)\0*.*\0", L"json");
}

static std::wstring PickMarkdownFile(bool save) {
    return PickFile(save, L"Markdown 文件 (*.md)\0*.md\0所有文件 (*.*)\0*.*\0", L"md");
}

static std::wstring EscapeMarkdownText(std::wstring text) {
    std::replace(text.begin(), text.end(), L'\r', L' ');
    std::replace(text.begin(), text.end(), L'\n', L' ');
    return Trim(text);
}

static void SerializeMindNodeMarkdown(const MindNode& node, std::wstring& markdown, int depth) {
    std::wstring indent(depth * 2, L' ');
    markdown += indent + L"- " + EscapeMarkdownText(node.title) + L"\r\n";
    if (!node.promptTitle.empty() && node.promptTitle != node.title) {
        markdown += indent + L"  - 关联提示词：" + EscapeMarkdownText(node.promptTitle) + L"\r\n";
    }
    for (const auto& child : node.children) {
        SerializeMindNodeMarkdown(child, markdown, depth + 1);
    }
}

static std::wstring SerializeMindMapMarkdown(const MindMap& map) {
    std::wstring markdown = L"# " + EscapeMarkdownText(map.title) + L"\r\n\r\n";
    SerializeMindNodeMarkdown(map.root, markdown, 0);
    return markdown;
}

static void ExportCurrentMindMap() {
    MindMap* map = CurrentMindMap();
    if (!map) return;
    std::wstring path = PickMarkdownFile(true);
    if (path.empty()) return;
    bool ok = WriteUtf8File(path, SerializeMindMapMarkdown(*map));
    SetMapStatus(ok ? L"导图已导出为 Markdown" : L"导出失败");
}

static void ImportMindMaps() {
    std::wstring path = PickJsonFile(false);
    if (path.empty()) return;
    std::wstring json;
    if (!ReadUtf8File(path, json)) {
        MessageBoxW(g_mindMap, L"无法读取导入文件。", APP_TITLE, MB_ICONERROR);
        return;
    }
    std::vector<MindMap> imported;
    JsonReader reader(json);
    if (!reader.ReadMindMaps(imported) || imported.empty()) {
        MessageBoxW(g_mindMap, L"导入文件格式不正确。", APP_TITLE, MB_ICONERROR);
        return;
    }
    for (auto& map : imported) {
        map.id = NewId();
        RegenerateMindNodeIds(map.root);
        if (map.title.empty()) map.title = map.root.title.empty() ? L"导入的思维导图" : map.root.title;
        g_mindMaps.push_back(map);
    }
    g_currentMap = (int)g_mindMaps.size() - (int)imported.size();
    g_selectedNodeId = g_mindMaps[g_currentMap].root.id;
    SaveMindMaps();
    RefreshMindMapUi(true);
    SetMapStatus(L"导入完成");
}

static void CreateMindMapControls(HWND hwnd) {
    if (g_mindMaps.empty()) LoadMindMaps();
    CreateWindowW(L"BUTTON", L"上一页", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 16, 14, 72, 30, hwnd, (HMENU)IDC_MAP_PREV, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"下一页", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 96, 14, 72, 30, hwnd, (HMENU)IDC_MAP_NEXT, g_instance, nullptr);
    g_mapSelector = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 178, 14, 280, 260, hwnd, (HMENU)IDC_MAP_SELECT, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"新建导图", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 468, 14, 88, 30, hwnd, (HMENU)IDC_MAP_NEW, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"重命名", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 564, 14, 82, 30, hwnd, (HMENU)IDC_MAP_RENAME, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"删除导图", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 654, 14, 88, 30, hwnd, (HMENU)IDC_MAP_DELETE, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"导入", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 750, 14, 72, 30, hwnd, (HMENU)IDC_MAP_IMPORT, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"导出", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 830, 14, 72, 30, hwnd, (HMENU)IDC_MAP_EXPORT, g_instance, nullptr);

    CreateWindowW(L"STATIC", L"节点结构", WS_CHILD | WS_VISIBLE, 16, 58, 120, 24, hwnd, nullptr, g_instance, nullptr);
    g_mapTree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, 16, 84, 280, 520, hwnd, (HMENU)IDC_MAP_TREE, g_instance, nullptr);

    CreateWindowW(L"STATIC", L"节点标题", WS_CHILD | WS_VISIBLE, 318, 58, 90, 24, hwnd, nullptr, g_instance, nullptr);
    g_nodeTitle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 318, 84, 260, 30, hwnd, (HMENU)IDC_NODE_TITLE, g_instance, nullptr);
    CreateWindowW(L"STATIC", L"关联提示词标题", WS_CHILD | WS_VISIBLE, 598, 58, 130, 24, hwnd, nullptr, g_instance, nullptr);
    g_nodePrompt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 598, 84, 260, 30, hwnd, (HMENU)IDC_NODE_PROMPT, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"保存节点", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 318, 122, 90, 30, hwnd, (HMENU)IDC_NODE_SAVE, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"添加子节点", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 416, 122, 100, 30, hwnd, (HMENU)IDC_NODE_ADD, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"删除节点", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 524, 122, 90, 30, hwnd, (HMENU)IDC_NODE_DELETE, g_instance, nullptr);
    g_mapStatus = CreateWindowW(L"STATIC", L"准备好了", WS_CHILD | WS_VISIBLE, 16, 614, 880, 24, hwnd, nullptr, g_instance, nullptr);

    for (int id : {IDC_MAP_PREV, IDC_MAP_NEXT, IDC_MAP_NEW, IDC_MAP_RENAME, IDC_MAP_DELETE, IDC_MAP_IMPORT, IDC_MAP_EXPORT, IDC_NODE_SAVE, IDC_NODE_ADD, IDC_NODE_DELETE}) {
        SendMessageW(GetDlgItem(hwnd, id), WM_SETFONT, (WPARAM)g_font, TRUE);
    }
    for (HWND control : {g_mapSelector, g_mapTree, g_nodeTitle, g_nodePrompt, g_mapStatus}) {
        SendMessageW(control, WM_SETFONT, (WPARAM)g_font, TRUE);
    }
    RefreshMindMapUi(true);
}

static void ResizeMindMapControls(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    MoveWindow(g_mapSelector, 178, 14, std::max(180, width - 760), 260, TRUE);
    int buttonX = std::max(468, width - 452);
    MoveWindow(GetDlgItem(hwnd, IDC_MAP_NEW), buttonX, 14, 88, 30, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_MAP_RENAME), buttonX + 96, 14, 82, 30, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_MAP_DELETE), buttonX + 186, 14, 88, 30, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_MAP_IMPORT), buttonX + 282, 14, 72, 30, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_MAP_EXPORT), buttonX + 362, 14, 72, 30, TRUE);
    MoveWindow(g_mapTree, 16, 84, 280, std::max(220, height - 128), TRUE);
    MoveWindow(g_nodeTitle, 318, 84, std::max(180, (width - 650) / 2), 30, TRUE);
    MoveWindow(g_nodePrompt, 598, 84, std::max(180, width - 618), 30, TRUE);
    MoveWindow(g_mapStatus, 16, height - 28, std::max(260, width - 32), 24, TRUE);
}

static int DropModeFromTreePoint(HTREEITEM item, POINT treePoint) {
    RECT itemRect{};
    if (!item || !TreeView_GetItemRect(g_mapTree, item, &itemRect, TRUE)) return 2;
    int height = std::max(1, (int)(itemRect.bottom - itemRect.top));
    int offset = treePoint.y - itemRect.top;
    if (offset < height / 3) return 1;
    if (offset > height * 2 / 3) return 3;
    return 2;
}

static std::wstring DropModeText(int mode) {
    if (mode == 1) return L"插到目标上面";
    if (mode == 3) return L"插到目标后面";
    return L"放入目标下面";
}

static HTREEITEM HitTestMindTreeFromWindowPoint(HWND hwnd, LPARAM lParam, int* modeOut = nullptr) {
    if (!g_mapTree) return nullptr;
    POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    MapWindowPoints(hwnd, g_mapTree, &pt, 1);
    TVHITTESTINFO hit{};
    hit.pt = pt;
    HTREEITEM item = TreeView_HitTest(g_mapTree, &hit);
    if (modeOut && item) *modeOut = DropModeFromTreePoint(item, pt);
    return item;
}

static void BeginTreeNodeDrag(NMTREEVIEWW* dragInfo) {
    std::wstring nodeId = TreeItemNodeId(dragInfo->itemNew.hItem);
    MindMap* map = CurrentMindMap();
    if (!map || nodeId.empty() || nodeId == map->root.id) {
        SetMapStatus(L"根节点不能拖拽；可以拖动它下面的节点调整结构");
        return;
    }
    g_draggingNode = true;
    g_dragNodeId = nodeId;
    g_dragHoverItem = dragInfo->itemNew.hItem;
    g_dragDropMode = 2;
    SetCapture(g_mindMap);
    TreeView_SelectItem(g_mapTree, dragInfo->itemNew.hItem);
    TreeView_SelectDropTarget(g_mapTree, dragInfo->itemNew.hItem);
    SetMapStatus(L"拖动中：上半区=插上面，中间=作为子节点，下半区=插后面");
}

static void UpdateTreeNodeDrag(HWND hwnd, LPARAM lParam) {
    if (!g_draggingNode) return;
    int mode = 2;
    HTREEITEM item = HitTestMindTreeFromWindowPoint(hwnd, lParam, &mode);
    g_dragHoverItem = item;
    g_dragDropMode = mode;
    TreeView_SelectDropTarget(g_mapTree, item);
    std::wstring targetId = TreeItemNodeId(item);
    if (!targetId.empty()) {
        const MindMap* map = CurrentMindMap();
        const MindNode* target = map ? FindNodeByIdConst(map->root, targetId) : nullptr;
        SetMapStatus(target ? (DropModeText(mode) + L"：" + target->title) : L"拖动中");
    }
}

static void FinishTreeNodeDrag() {
    if (!g_draggingNode) return;
    ReleaseCapture();
    TreeView_SelectDropTarget(g_mapTree, nullptr);
    std::wstring sourceId = g_dragNodeId;
    std::wstring targetId = TreeItemNodeId(g_dragHoverItem);
    int mode = g_dragDropMode;
    g_draggingNode = false;
    g_dragNodeId.clear();
    g_dragHoverItem = nullptr;
    g_dragDropMode = 0;

    if (targetId.empty()) {
        SetMapStatus(L"未选择目标，已取消拖拽");
        return;
    }
    if (!MoveMindNode(sourceId, targetId, mode)) {
        SetMapStatus(L"不能这样移动节点");
        return;
    }
    RefreshMindMapUi(true);
    SetMapStatus(L"节点已移动并保存");
}

static LRESULT CALLBACK MindMapProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateMindMapControls(hwnd);
        return 0;
    case WM_SIZE:
        ResizeMindMapControls(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        DrawMindMap(dc, MindMapCanvasRect(rc));
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom == IDC_MAP_TREE && hdr->code == TVN_BEGINDRAGW) {
            BeginTreeNodeDrag((NMTREEVIEWW*)lParam);
        } else if (hdr->idFrom == IDC_MAP_TREE && hdr->code == TVN_SELCHANGEDW && !g_loadingMapUi) {
            NMTREEVIEWW* changed = (NMTREEVIEWW*)lParam;
            int index = (int)changed->itemNew.lParam;
            if (index >= 0 && index < (int)g_treeNodeIds.size()) {
                g_selectedNodeId = g_treeNodeIds[index];
                LoadSelectedNodeToEditor();
                InvalidateRect(hwnd, nullptr, TRUE);
            }
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if (g_draggingNode) {
            FinishTreeNodeDrag();
            return 0;
        }
        break;
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int notify = HIWORD(wParam);
        if (id == IDC_MAP_SELECT && notify == CBN_SELCHANGE && !g_loadingMapUi) {
            int selected = (int)SendMessageW(g_mapSelector, CB_GETCURSEL, 0, 0);
            if (selected >= 0 && selected < (int)g_mindMaps.size()) {
                g_currentMap = selected;
                g_selectedNodeId = g_mindMaps[g_currentMap].root.id;
                RefreshMindMapUi(true);
            }
        } else if (id == IDC_MAP_PREV && notify == BN_CLICKED) {
            if (!g_mindMaps.empty()) {
                g_currentMap = (g_currentMap - 1 + (int)g_mindMaps.size()) % (int)g_mindMaps.size();
                g_selectedNodeId = g_mindMaps[g_currentMap].root.id;
                RefreshMindMapUi(true);
            }
        } else if (id == IDC_MAP_NEXT && notify == BN_CLICKED) {
            if (!g_mindMaps.empty()) {
                g_currentMap = (g_currentMap + 1) % (int)g_mindMaps.size();
                g_selectedNodeId = g_mindMaps[g_currentMap].root.id;
                RefreshMindMapUi(true);
            }
        } else if (id == IDC_MAP_NEW && notify == BN_CLICKED) {
            NewMindMapPage();
        } else if (id == IDC_MAP_RENAME && notify == BN_CLICKED) {
            SelectRootForRename();
        } else if (id == IDC_MAP_DELETE && notify == BN_CLICKED) {
            DeleteMindMapPage();
        } else if (id == IDC_MAP_IMPORT && notify == BN_CLICKED) {
            ImportMindMaps();
        } else if (id == IDC_MAP_EXPORT && notify == BN_CLICKED) {
            ExportCurrentMindMap();
        } else if (id == IDC_NODE_SAVE && notify == BN_CLICKED) {
            SaveSelectedNode();
        } else if (id == IDC_NODE_ADD && notify == BN_CLICKED) {
            AddChildNode();
        } else if (id == IDC_NODE_DELETE && notify == BN_CLICKED) {
            DeleteSelectedNode();
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        for (const auto& hotspot : g_mindHotspots) {
            if (PtInRect(&hotspot.rect, pt)) {
                g_selectedNodeId = hotspot.nodeId;
                LoadSelectedNodeToEditor();
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }
        }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        for (const auto& hotspot : g_mindHotspots) {
            if (PtInRect(&hotspot.rect, pt)) {
                FocusPromptByTitle(hotspot.promptTitle);
                return 0;
            }
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (g_draggingNode) {
            UpdateTreeNodeDrag(hwnd, lParam);
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return 0;
        }
        POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        bool over = false;
        for (const auto& hotspot : g_mindHotspots) {
            if (PtInRect(&hotspot.rect, pt)) {
                over = true;
                break;
            }
        }
        SetCursor(LoadCursor(nullptr, over ? IDC_HAND : IDC_ARROW));
        return 0;
    }
    case WM_CLOSE:
        if (g_draggingNode) {
            ReleaseCapture();
            TreeView_SelectDropTarget(g_mapTree, nullptr);
            g_draggingNode = false;
            g_dragNodeId.clear();
            g_dragHoverItem = nullptr;
        }
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_DESTROY:
        g_mindMap = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

static void ShowMindMap() {
    if (g_mindMap && IsWindow(g_mindMap)) {
        ShowWindow(g_mindMap, SW_SHOW);
        SetForegroundWindow(g_mindMap);
        InvalidateRect(g_mindMap, nullptr, TRUE);
        return;
    }

    WNDCLASSW wc{};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MindMapProc;
    wc.hInstance = g_instance;
    wc.lpszClassName = L"PromptMindMapWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    g_mindMap = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"AI写小说全流程 - 思维导图",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1040,
        720,
        g_main,
        nullptr,
        g_instance,
        nullptr
    );
    if (!g_mindMap) return;
    ShowWindow(g_mindMap, SW_SHOW);
    UpdateWindow(g_mindMap);
}

static HWND Label(HWND parent, const wchar_t* text, int x, int y, int w, int h, HFONT font = nullptr) {
    HWND hwnd = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, g_instance, nullptr);
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)(font ? font : g_font), TRUE);
    return hwnd;
}

static void CreateControls(HWND hwnd) {
    g_titleFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    g_font = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");

    Label(hwnd, L"提示词库", 16, 16, 180, 30, g_titleFont);
    g_search = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 16, 58, 300, 28, hwnd, (HMENU)IDC_SEARCH, g_instance, nullptr);
    g_category = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 16, 94, 300, 260, hwnd, (HMENU)IDC_CATEGORY, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"新建", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 16, 132, 146, 32, hwnd, (HMENU)IDC_NEW, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"复制", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 170, 132, 146, 32, hwnd, (HMENU)IDC_COPY, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"思维导图", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 16, 172, 300, 32, hwnd, (HMENU)IDC_MINDMAP, g_instance, nullptr);
    g_list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY, 16, 216, 300, 390, hwnd, (HMENU)IDC_LIST, g_instance, nullptr);
    g_count = Label(hwnd, L"", 16, 616, 300, 24);

    Label(hwnd, L"编辑提示词", 350, 16, 180, 30, g_titleFont);
    CreateWindowW(L"BUTTON", L"保存", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 92, 32, hwnd, (HMENU)IDC_SAVE, g_instance, nullptr);
    CreateWindowW(L"BUTTON", L"删除", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 92, 32, hwnd, (HMENU)IDC_DELETE, g_instance, nullptr);

    Label(hwnd, L"标题", 350, 58, 90, 24);
    g_title = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 350, 84, 560, 30, hwnd, nullptr, g_instance, nullptr);
    Label(hwnd, L"分类", 350, 124, 90, 24);
    g_editCategory = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 350, 150, 270, 30, hwnd, nullptr, g_instance, nullptr);
    Label(hwnd, L"标签", 640, 124, 90, 24);
    g_tags = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 640, 150, 270, 30, hwnd, nullptr, g_instance, nullptr);
    Label(hwnd, L"提示词内容", 350, 190, 120, 24);
    g_content = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN, 350, 216, 560, 390, hwnd, nullptr, g_instance, nullptr);
    g_status = Label(hwnd, L"准备好了", 350, 616, 500, 24);

    std::vector<HWND> controls = {g_search, g_category, g_list, g_title, g_editCategory, g_tags, g_content};
    for (HWND control : controls) SendMessageW(control, WM_SETFONT, (WPARAM)g_font, TRUE);
    for (int id : {IDC_NEW, IDC_COPY, IDC_SAVE, IDC_DELETE, IDC_MINDMAP}) {
        HWND button = GetDlgItem(hwnd, id);
        SendMessageW(button, WM_SETFONT, (WPARAM)g_font, TRUE);
    }
}

static void ResizeControls(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int leftW = 332;
    int gap = 18;
    int rightX = leftW + gap;
    int rightW = std::max(360, width - rightX - 18);
    int bottomY = height - 34;

    MoveWindow(g_search, 16, 58, leftW - 32, 28, TRUE);
    MoveWindow(g_category, 16, 94, leftW - 32, 260, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_NEW), 16, 132, (leftW - 42) / 2, 32, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_COPY), 16 + (leftW - 42) / 2 + 10, 132, (leftW - 42) / 2, 32, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_MINDMAP), 16, 172, leftW - 32, 32, TRUE);
    MoveWindow(g_list, 16, 216, leftW - 32, std::max(150, height - 262), TRUE);
    MoveWindow(g_count, 16, bottomY, leftW - 32, 24, TRUE);

    MoveWindow(GetDlgItem(hwnd, IDC_SAVE), width - 222, 16, 92, 32, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_DELETE), width - 118, 16, 92, 32, TRUE);
    MoveWindow(g_title, rightX, 84, rightW, 30, TRUE);
    MoveWindow(g_editCategory, rightX, 150, (rightW - 18) / 2, 30, TRUE);
    MoveWindow(g_tags, rightX + (rightW - 18) / 2 + 18, 150, (rightW - 18) / 2, 30, TRUE);
    MoveWindow(g_content, rightX, 216, rightW, std::max(180, height - 266), TRUE);
    MoveWindow(g_status, rightX, bottomY, rightW, 24, TRUE);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateControls(hwnd);
        if (!LoadPrompts()) {
            SetStatus(L"未找到提示词数据，可以新建第一条提示词");
        }
        AddCategories();
        ApplyFilters();
        return 0;
    case WM_SIZE:
        ResizeControls(hwnd);
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int notify = HIWORD(wParam);
        if (id == IDC_SEARCH && notify == EN_CHANGE && !g_loading) ApplyFilters();
        else if (id == IDC_CATEGORY && notify == CBN_SELCHANGE) ApplyFilters();
        else if (id == IDC_LIST && notify == LBN_SELCHANGE) {
            int selected = (int)SendMessageW(g_list, LB_GETCURSEL, 0, 0);
            if (selected >= 0 && selected < (int)g_filtered.size()) LoadToEditor(g_filtered[selected]);
        } else if (id == IDC_NEW && notify == BN_CLICKED) {
            SendMessageW(g_list, LB_SETCURSEL, (WPARAM)-1, 0);
            ClearEditor();
            SetFocus(g_title);
        } else if (id == IDC_COPY && notify == BN_CLICKED) {
            CopyCurrent();
        } else if (id == IDC_SAVE && notify == BN_CLICKED) {
            SaveCurrent();
        } else if (id == IDC_DELETE && notify == BN_CLICKED) {
            DeleteCurrent();
        } else if (id == IDC_MINDMAP && notify == BN_CLICKED) {
            ShowMindMap();
        }
        return 0;
    }
    case WM_KEYDOWN:
        if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'S') {
            SaveCurrent();
            return 0;
        }
        if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'N') {
            ClearEditor();
            SetFocus(g_title);
            return 0;
        }
        if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'F') {
            SetFocus(g_search);
            SendMessageW(g_search, EM_SETSEL, 0, -1);
            return 0;
        }
        break;
    case WM_DESTROY:
        if (g_font) DeleteObject(g_font);
        if (g_titleFont) DeleteObject(g_titleFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    g_instance = instance;
    ResolveDataPaths();
    INITCOMMONCONTROLSEX icc{sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icc);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"PromptLibraryWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    g_main = CreateWindowExW(
        0,
        wc.lpszClassName,
        APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1120,
        720,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!g_main) return 1;
    ShowWindow(g_main, showCommand);
    UpdateWindow(g_main);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}
