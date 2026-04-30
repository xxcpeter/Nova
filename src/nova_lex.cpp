#include "lexer.h"
#include "token.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <filesystem>


std::string read_file(const std::string& path) {
    std::ifstream inFile(path);
    if (!inFile) throw std::runtime_error("Invalid path");
    std::stringstream buffer;
    
    buffer << inFile.rdbuf();
    
    return buffer.str();
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: nova_lex [file_path]" << std::endl;
        return 0;
    }
    std::string path = argv[1];
    std::string name = std::filesystem::path(path).filename().string();
    std::string source = read_file(path);
    
    try {
        Lexer lexer(source, name);
        auto token_list = lexer.tokenize();
        for (const auto& i : token_list) 
            std::cout << i << std::endl;
    } catch (const LexError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
