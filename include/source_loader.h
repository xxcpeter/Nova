#pragma once

#include <string>
#include <filesystem>


std::string load_source_with_imports(const std::filesystem::path& input_path);


class ImportError : public std::runtime_error {
public:
    ImportError(const std::string& message) : std::runtime_error("ImportError: " + message) {}
};