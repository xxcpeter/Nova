#include "sema.h"
#include "ast.h"
#include "parser.h"
#include "lexer.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <string>


std::string read_file(const std::string& path) {
    std::ifstream inFile(path);
    if (!inFile) throw std::runtime_error("Invalid path");
    std::stringstream buffer;
    
    buffer << inFile.rdbuf();
    
    return buffer.str();
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: nova_sema [file_path]" << std::endl;
        return 0;
    }
    std::string path = argv[1];
    std::string name = std::filesystem::path(path).filename().string();
    std::string source = read_file(path);
        
    try {
        Lexer lexer(source, name);
        auto token_list = lexer.tokenize();
        Parser parser(token_list, name);
        auto program = parser.parse_program();
        SemanticAnalyzer sema;
        sema.analyze(*program);
        std::cout << "Semantic analysis succeeded" << std::endl;
    } catch (const SemaError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const ParseError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const LexError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
