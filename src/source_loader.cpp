#include "source_loader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>


namespace {
    struct ImportContext {
        std::unordered_set<std::string> visited;
        std::vector<std::filesystem::path> stack;
    };

    bool is_in_stack(const ImportContext& context, const std::string& key) {
        return std::ranges::any_of(context.stack, [&](const auto& p) { 
            return std::filesystem::weakly_canonical(p).string() == key; });
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

    std::string expand_file(const std::filesystem::path& input_path, const std::string& display_path, ImportContext& context) {
        std::filesystem::path canonical = std::filesystem::weakly_canonical(input_path);
        std::string key = canonical.string();
        
        if (is_in_stack(context, key)) throw ImportError("cyclic import involving '" + display_path + "'");
        if (context.visited.contains(key)) return "";
        context.visited.insert(key);
        context.stack.push_back(canonical);
        
        std::ifstream inFile(canonical);
        if (!inFile) throw ImportError("cannot open import '" + display_path + "'");
        std::stringstream out;
        std::string line;

        while (std::getline(inFile, line)) {
            std::string import_path;
            if (parse_import_line(line, import_path)) {
                std::filesystem::path resolved = canonical.parent_path() / import_path;
                out << expand_file(resolved, import_path, context) << '\n';
            } else {
                out << line << '\n';
            }
        }

        context.stack.pop_back();
        return out.str();
    }
}


std::string load_source_with_imports(const std::filesystem::path& input_path) {
    ImportContext context;
    return expand_file(input_path, input_path.string(), context);
}