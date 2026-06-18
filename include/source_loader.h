#pragma once

#include "token.h"
#include <string>
#include <vector>
#include <format>
#include <cstddef>
#include <stdexcept>
#include <filesystem>


struct SourceLineMapping {
    std::string file;
    size_t original_line;
};

struct LoadedSource {
    std::string content;
    std::vector<SourceLineMapping> line_mapping;
};


LoadedSource load_source_with_imports(const std::filesystem::path& input_path);


class ImportError : public std::runtime_error {
public:
    ImportError(const std::string& message, const SourceLocation& location) : 
        std::runtime_error(std::format("{}:{}:{}: ImportError: {}", 
            location.file, location.line, location.column, message)) {}
};