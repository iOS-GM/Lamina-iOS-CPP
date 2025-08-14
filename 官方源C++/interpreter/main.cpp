// main.cpp
#include "interpreter.hpp"  // 首先包含
#include "module_loader.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "trackback.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include "repl_input.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Lamina REPL. Press Ctrl+C or :exit to exit.\n";
        std::cout << "Type :help for help.\n";
        Interpreter interpreter;
        int lineno = 1;
        while (true) {
            try {
                std::string prompt = Interpreter::supports_colors() ? "\033[1;32m>\033[0m " : "> ";
                std::string line;
                try {
                    line = repl_readline(prompt);
                } catch (const CtrlCException&) {
                    break;
                }
                if (std::cin.eof()) break;
                if (line.empty()) {
                    ++lineno;
                    continue;
                }
                
                if (line == ":exit") {
                    break;
                }
                
                if (line == ":help") {
                    std::cout << "Lamina Interpreter Commands:\n";
                    std::cout << "  :exit - Exit interpreter\n";
                    std::cout << "  :help - Show this help message\n";
                    std::cout << "  :vars - Show all variables\n";
                    std::cout << "  :clear - Clear screen\n";
                    ++lineno;
                    continue;
                }
                
                if (line == ":vars") {
                    interpreter.printVariables();
                    ++lineno;
                    continue;
                }
                
                if (line == ":clear") {
                    #ifdef __APPLE__
                    // iOS 专用清屏方案
                    std::cout << "\033[2J\033[1;1H";
                    #elif _WIN32
                    auto result = system("cls");
                    (void)result;
                    #else
                    auto result = system("clear");
                    (void)result;
                    #endif
                    ++lineno;
                    continue;
                }

                auto tokens = Lexer::tokenize(line);
                auto ast = Parser::parse(tokens);
                if (!ast) {
                    print_traceback("<stdin>", lineno);
                } else {
                    auto* block = dynamic_cast<BlockStmt*>(ast.get());
                    if (block) {
                        interpreter.save_repl_ast(std::move(ast));
                        
                        try {
                            for (auto& stmt : block->statements) {
                                try {
                                    interpreter.execute(stmt);
                                } catch (const RuntimeError& re) {
                                    interpreter.print_stack_trace(re, true);
                                    break;
                                } catch (const ReturnException&) {
                                    Interpreter::print_warning("Return statement used outside function (line " + std::to_string(lineno) + ")", true);
                                    break;
                                } catch (const BreakException&) {
                                    Interpreter::print_warning("Break statement used outside loop (line " + std::to_string(lineno) + ")", true);
                                    break;
                                } catch (const ContinueException&) {
                                    Interpreter::print_warning("Continue statement used outside loop (line " + std::to_string(lineno) + ")", true);
                                    break;
                                } catch (const std::exception& e) {
                                    Interpreter::print_error(e.what(), true);
                                    break;
                                }
                            }
                        } catch (const RuntimeError& re) {
                            interpreter.print_stack_trace(re, true);
                        } catch (const ReturnException&) {
                            Interpreter::print_warning("Return statement used outside function", true);
                        } catch (...) {
                            Interpreter::print_error("Unknown error occurred", true);
                        }
                    }
                }
            } catch (...) {
                Interpreter::print_error("REPL environment caught exception, but recovered", true);
            }
            ++lineno;
        }
        return 0;
    }
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Unable to open file: " << argv[1] << std::endl;
        return 1;
    }
    std::cout << "Executing file: " << argv[1] << std::endl;
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    auto tokens = Lexer::tokenize(source);
    auto ast = Parser::parse(tokens);
    Interpreter interpreter;
    
    // 加载minimal模块
    std::cout << "Loading minimal module..." << std::endl;
    try {
        auto moduleLoader = std::make_unique<ModuleLoader>("minimal.dll");
        if (moduleLoader->isLoaded()) {
            std::cout << "Module loaded successfully, registering to interpreter..." << std::endl;
            if (moduleLoader->registerToInterpreter(interpreter)) {
                std::cout << "Module registered successfully!" << std::endl;
            } else {
                std::cerr << "Failed to register module to interpreter!" << std::endl;
            }
        } else {
            std::cerr << "Failed to load module!" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during module loading: " << e.what() << std::endl;
    }
    
    if (!ast) {
        print_traceback(argv[1], 1);
        return 2;
    }
    
    auto* block = dynamic_cast<BlockStmt*>(ast.get());
    if (block) {
        int currentLine = 0;
        for (auto& stmt : block->statements) {
            currentLine++;
            try {
                interpreter.execute(stmt);
            } catch (const RuntimeError& re) {
                interpreter.print_stack_trace(re, true);
            } catch (const ReturnException&) {
                Interpreter::print_warning("Return statement used outside function (line " + std::to_string(currentLine) + ")", true);
            } catch (const BreakException&) {
                Interpreter::print_warning("Break statement used outside loop (line " + std::to_string(currentLine) + ")", true);
            } catch (const ContinueException&) {
                Interpreter::print_warning("Continue statement used outside loop (line " + std::to_string(currentLine) + ")", true);
            } catch (const std::exception& e) {
                Interpreter::print_error(std::string(e.what()) + " (line " + std::to_string(currentLine) + ")", true);
            } catch (...) {
                Interpreter::print_error("Unknown error (line " + std::to_string(currentLine) + ")", true);
            }
        }
        std::cout << "\nProgram execution completed." << std::endl;
    }
    return 0;
}