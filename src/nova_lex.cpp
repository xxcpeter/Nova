#include "lexer.h"
#include "source_loader.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <filesystem>


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: nova_lex [file_path]" << std::endl;
        return 0;
    }
    std::string path = argv[1];
    std::string name = std::filesystem::path(path).filename().string();
    
    try {
        std::string source = load_source_with_imports(std::filesystem::path(path));
        
        Lexer lexer(source, name);
        auto token_list = lexer.tokenize();
        for (const auto& i : token_list) std::cout << i << std::endl;
    } catch (const LexError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const ImportError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
