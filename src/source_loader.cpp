#include "source_loader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <ranges>
#include <string>
#include <unordered_set>
#include <cctype>
#include <vector>
#include <algorithm>


namespace {
    struct SourceFile {
        std::filesystem::path canonical_path;
        std::string display_path;
    };

    struct ImportContext {
        std::unordered_set<std::string> visited;
        std::vector<SourceFile> stack;
        std::filesystem::path project_root;
    };

    bool is_in_stack(const ImportContext& context, const std::string& key) {
        return std::ranges::any_of(context.stack, [&](const auto& p) { 
            return std::filesystem::weakly_canonical(p.canonical_path).string() == key; });
    }

    bool parse_import_line(const std::string& line, std::string& import_path) {
        std::size_t i = 0;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;

        const std::string keyword = "import";
        if (line.compare(i, keyword.size(), keyword) != 0) return false;

        i += keyword.size();
        if (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) return false;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;

        if (i >= line.size() || line[i] != '"') throw ImportError("malformed import directive");
        std::size_t start = ++i;
        while (i < line.size() && line[i] != '"') ++i;

        if (i >= line.size()) throw ImportError("malformed import directive");
        import_path = line.substr(start, i++ - start);
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;

        if (i >= line.size() || line[i] != ';') throw ImportError("malformed import directive");
        ++i;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;

        if (i != line.size()) throw ImportError("malformed import directive");
        return true;
    }

    std::string make_display_path(const std::filesystem::path& path, const std::filesystem::path& project_root) {
        return std::filesystem::relative(path, project_root).generic_string();
    }

    std::filesystem::path find_project_root() {
        auto dir = std::filesystem::current_path();

        while (!dir.empty()) {
            if (std::filesystem::exists(dir / "CMakeLists.txt") &&
                std::filesystem::exists(dir / "runtime") &&
                std::filesystem::exists(dir / "src")) {
                return dir;
            }

            auto parent = dir.parent_path();
            if (parent == dir) break;
            dir = parent;
        }

        return std::filesystem::current_path();
    }

    void expand_file(const std::filesystem::path& input_path, const std::string& display_path, const std::string& import_spec, ImportContext& context, LoadedSource& loaded) {
        std::filesystem::path canonical = std::filesystem::weakly_canonical(input_path);
        std::string key = canonical.string();
        
        if (is_in_stack(context, key)) throw ImportError("cyclic import involving '" + import_spec + "'");
        if (context.visited.contains(key)) return;
        context.visited.insert(key);
        context.stack.push_back({canonical, display_path});
        
        std::ifstream inFile(canonical);
        if (!inFile) throw ImportError("cannot open import '" + import_spec + "'");
        // std::stringstream out;
        std::string line;
        size_t line_number = 0;

        while (std::getline(inFile, line)) {
            std::string new_import_spec;
            if (parse_import_line(line, new_import_spec)) {
                std::filesystem::path resolved = canonical.parent_path() / new_import_spec;
                expand_file(resolved, make_display_path(resolved, context.project_root), new_import_spec, context, loaded);
            } else {
                loaded.content += line + '\n';
                loaded.line_mapping.push_back(SourceLineMapping{ .file = display_path, .original_line = line_number + 1 });
            }
            ++line_number;
        }
        context.stack.pop_back();
    }
}


LoadedSource load_source_with_imports(const std::filesystem::path& input_path) {
    ImportContext context;
    LoadedSource loaded;
    context.project_root = find_project_root();

    auto absolute_path = std::filesystem::absolute(input_path);
    expand_file(input_path, make_display_path(absolute_path, context.project_root), input_path.string(), context, loaded);
    return loaded;
}