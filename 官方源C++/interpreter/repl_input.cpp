#include "repl_input.hpp"
#include <iostream>
#include <vector>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

std::string repl_readline(const std::string& prompt) {
    std::string buffer;
    size_t cursor = 0;
    size_t maxlen = 0;
    std::cout << prompt << std::flush;
    #ifdef _WIN32
    static std::vector<std::string> history;
    static int history_max = 100;
    int history_index = -1;
    std::string current_edit;
    while (true) {
        int ch = _getch();
        // Ctrl+C
        if (ch == 3) { // Ctrl+C
            std::cout << "^C" << std::endl;
            throw CtrlCException();
        }
        // Ctrl+R
        if (ch == 18) { // Ctrl+R
            std::string search;
            int search_idx = -1;
            bool found = false;
            size_t last_display_len = 0;
            while (true) {
                std::string match_str;
                if (found && search_idx >= 0) {
                    match_str = history[history.size() - 1 - search_idx];
                }
                std::string prompt_str = "(reverse-i-search): " + search;
                std::string display = prompt_str;
                if (!match_str.empty()) display += "  " + match_str;
                std::cout << "\r";
                for (size_t i = 0; i < std::max(last_display_len, display.size()); ++i) std::cout << ' ';
                std::cout << "\r";
                std::cout << prompt_str;
                if (!match_str.empty()) std::cout << "  \033[36m" << match_str << "\033[0m";
                std::cout << std::flush;
                last_display_len = display.size();
                int sch = _getch();
                // Windows: 224
                if (sch == 224) {
                    int arrow = _getch();
                    if (arrow == 72 || arrow == 80) {
                        std::cout << "\r";
                        for (size_t i = 0; i < last_display_len; ++i) std::cout << ' ';
                        std::cout << "\r" << prompt << buffer << std::flush;
                        break;
                    } // (75/77)ignore
                    continue;
                }
                if (sch == 13) { // Enter
                    if (found && search_idx >= 0) {
                        buffer = history[history.size() - 1 - search_idx];
                        cursor = buffer.size();
                        maxlen = std::max(maxlen, buffer.size());
                    }
                    // clear line
                    std::cout << "\r";
                    for (size_t i = 0; i < last_display_len; ++i) std::cout << ' ';
                    std::cout << "\r" << prompt << buffer << std::flush;
                    break;
                } else if (sch == 27) { // Esc
                    std::cout << "\r";
                    for (size_t i = 0; i < last_display_len; ++i) std::cout << ' ';
                    std::cout << "\r" << prompt << buffer << std::flush;
                    break;
                } else if (sch == 8 || sch == 127) { // Backspace
                    if (!search.empty()) search.pop_back();
                } else if (sch == 18) { // Ctrl+R again
                    if (!search.empty() && !history.empty()) {
                        int next_idx = search_idx == -1 ? 0 : search_idx + 1;
                        found = false;
                        for (int i = next_idx; i < (int)history.size(); ++i) {
                            if (history[history.size() - 1 - i].find(search) != std::string::npos) {
                                search_idx = i;
                                found = true;
                                break;
                            }
                        }
                    }
                } else if (sch >= 32 && sch <= 126) {
                    search += (char)sch;
                    search_idx = -1;
                } else if (sch == 8 || sch == 127) { // Backspace
                    if (!search.empty()) search.pop_back();
                    search_idx = -1;
                }
                found = false;
                if (!search.empty() && !history.empty()) {
                    int start_idx = (search_idx == -1) ? 0 : search_idx;
                    for (int i = start_idx; i < (int)history.size(); ++i) {
                        if (history[history.size() - 1 - i].find(search) != std::string::npos) {
                            search_idx = i;
                            found = true;
                            break;
                        }
                    }
                }
            }
            continue;
        }
        if (ch == 13) { // Enter
            std::cout << std::endl;
            break;
        } else if (ch == 8 || ch == 127) { // Backspace
            if (cursor > 0) {
                buffer.erase(buffer.begin() + cursor - 1);
                --cursor;
                if (buffer.size() > maxlen) maxlen = buffer.size();
                std::cout << "\b";
                for (size_t i = cursor; i < buffer.size(); ++i) std::cout << buffer[i];
                for (size_t i = buffer.size(); i < maxlen; ++i) std::cout << ' ';
                for (size_t i = cursor; i < maxlen; ++i) std::cout << "\b";
                std::cout << std::flush;
            }
        } else if (ch == 224) {
            int key = _getch();
            if (key == 75) { // Left
                if (cursor > 0) {
                    std::cout << "\b" << std::flush;
                    --cursor;
                }
            } else if (key == 77) { // Right
                if (cursor < buffer.size()) {
                    std::cout << buffer[cursor];
                    ++cursor;
                }
            } else if (key == 72) { // Up
                if (!history.empty() && (history_index + 1 < (int)history.size())) {
                    if (history_index == -1) {
                        current_edit = buffer;
                    }
                    ++history_index;
                    buffer = history[history.size() - 1 - history_index];
                    cursor = buffer.size();
                    maxlen = std::max(maxlen, buffer.size());
                    std::cout << "\r" << prompt;
                    for (size_t i = 0; i < maxlen; ++i) std::cout << ' ';
                    std::cout << "\r" << prompt << buffer << std::flush;
                }
            } else if (key == 80) { // Down
                if (history_index > 0) {
                    --history_index;
                    buffer = history[history.size() - 1 - history_index];
                    cursor = buffer.size();
                    maxlen = std::max(maxlen, buffer.size());
                    std::cout << "\r" << prompt;
                    for (size_t i = 0; i < maxlen; ++i) std::cout << ' ';
                    std::cout << "\r" << prompt << buffer << std::flush;
                } else if (history_index == 0) {
                    history_index = -1;
                    buffer = current_edit;
                    cursor = buffer.size();
                    std::cout << "\r" << prompt;
                    for (size_t i = 0; i < maxlen; ++i) std::cout << ' ';
                    std::cout << "\r" << prompt << buffer << std::flush;
                }
            }
        } else if (ch >= 32 && ch <= 126) { // Printable
            buffer.insert(buffer.begin() + cursor, (char)ch);
            ++cursor;
            if (buffer.size() > maxlen) maxlen = buffer.size();
            for (size_t i = cursor - 1; i < buffer.size(); ++i) std::cout << buffer[i];
            for (size_t i = buffer.size(); i < maxlen; ++i) std::cout << ' ';
            for (size_t i = cursor; i < maxlen; ++i) std::cout << "\b";
            std::cout << std::flush;
            if (history_index != -1) { history_index = -1; }
        }
    }
    if (!buffer.empty() && (history.empty() || buffer != history.back())) {
        history.push_back(buffer);
        if ((int)history.size() > history_max) history.erase(history.begin());
    }
#else
    static std::vector<std::string> history;
    static int history_max = 100;
    int history_index = -1;
    std::string current_edit;
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    size_t maxlen_unix = 0;
    while (true) {
        char ch = getchar();
        // Ctrl+C
        if (ch == 3) { // Ctrl+C
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 恢复终端设置
            std::cout << "^C" << std::endl;
            throw CtrlCException();
        }
        // Ctrl+R
        if (ch == 18) { // Ctrl+R
            std::string search;
            int search_idx = -1;
            bool found = false;
            size_t last_display_len = 0;
            while (true) {
                std::string match_str;
                if (found && search_idx >= 0) {
                    match_str = history[history.size() - 1 - search_idx];
                }
                std::string prompt_str = "(reverse-i-search): " + search;
                std::string display = prompt_str;
                if (!match_str.empty()) display += "  " + match_str;
                // clc
                std::cout << "\r";
                for (size_t i = 0; i < std::max(last_display_len, display.size()); ++i) std::cout << ' ';
                std::cout << "\r";
                std::cout << prompt_str;
                if (!match_str.empty()) std::cout << "  \033[36m" << match_str << "\033[0m";
                std::cout << std::flush;
                last_display_len = display.size();
                char sch = getchar();
                if (sch == 27) {
                    int next1 = getchar();
                    if (next1 == '[') {
                        int next2 = getchar();
                        if (next2 == 'A' || next2 == 'B') {
                            std::cout << "\r";
                            for (size_t i = 0; i < last_display_len; ++i) std::cout << ' ';
                            std::cout << "\r" << prompt << buffer << std::flush;
                            break;
                        } else if (next2 == 'C' || next2 == 'D') {
                            continue;
                        } else {
                            ungetc(next2, stdin);
                        }
                    } else {
                        ungetc(next1, stdin);
                    }
                }
                if (sch == '\n' || sch == '\r') {
                    if (found && search_idx >= 0) {
                        buffer = history[history.size() - 1 - search_idx];
                        cursor = buffer.size();
                        maxlen_unix = std::max(maxlen_unix, buffer.size());
                    }
                    // clc
                    std::cout << "\r";
                    for (size_t i = 0; i < last_display_len; ++i) std::cout << ' ';
                    std::cout << "\r" << prompt << buffer << std::flush;
                    break;
                } else if (sch == 27) { // Esc
                    std::cout << "\r";
                    for (size_t i = 0; i < last_display_len; ++i) std::cout << ' ';
                    std::cout << "\r" << prompt << buffer << std::flush;
                    break;
                } else if (sch == 127 || sch == 8) { // Backspace
                    if (!search.empty()) search.pop_back();
                } else if (sch == 18) { // Ctrl+R again
                    if (!search.empty() && !history.empty()) {
                        int next_idx = search_idx + 1;
                        found = false;
                        for (int i = next_idx; i < (int)history.size(); ++i) {
                            if (history[history.size() - 1 - i].find(search) != std::string::npos) {
                                search_idx = i;
                                found = true;
                                break;
                            }
                        }
                    }
                } else if (sch >= 32 && sch <= 126) {
                    search += (char)sch;
                }
                found = false;
                if (!search.empty() && !history.empty()) {
                    for (int i = 0; i < (int)history.size(); ++i) {
                        if (history[history.size() - 1 - i].find(search) != std::string::npos) {
                            search_idx = i;
                            found = true;
                            break;
                        }
                    }
                }
            }
            continue;
        }
        if (ch == '\n' || ch == '\r') {
            std::cout << std::endl;
            break;
        } else if (ch == 127 || ch == 8) { // Backspace
            if (cursor > 0) {
                buffer.erase(buffer.begin() + cursor - 1);
                --cursor;
                if (buffer.size() > maxlen_unix) maxlen_unix = buffer.size();
                std::cout << "\b";
                for (size_t i = cursor; i < buffer.size(); ++i) std::cout << buffer[i];
                for (size_t i = buffer.size(); i < maxlen_unix; ++i) std::cout << ' ';
                for (size_t i = cursor; i < maxlen_unix; ++i) std::cout << "\b";
                std::cout << std::flush;
            }
        } else if (ch == 27) { // Escape sequence
            char seq1 = getchar();
            if (seq1 == '[') {
                char seq2 = getchar();
                if (seq2 == 'D') { // Left
                    if (cursor > 0) {
                        std::cout << "\b" << std::flush;
                        --cursor;
                    }
                } else if (seq2 == 'C') { // Right
                    if (cursor < buffer.size()) {
                        std::cout << buffer[cursor];
                        ++cursor;
                    }
                } else if (seq2 == 'A') { // Up
                    if (!history.empty() && (history_index + 1 < (int)history.size())) {
                        if (history_index == -1) {
                            current_edit = buffer;
                        }
                        ++history_index;
                        buffer = history[history.size() - 1 - history_index];
                        cursor = buffer.size();
                        maxlen_unix = std::max(maxlen_unix, buffer.size());
                        std::cout << "\r" << prompt;
                        for (size_t i = 0; i < maxlen_unix; ++i) std::cout << ' ';
                        std::cout << "\r" << prompt << buffer << std::flush;
                    }
                } else if (seq2 == 'B') { // Down
                    if (history_index > 0) {
                        --history_index;
                        buffer = history[history.size() - 1 - history_index];
                        cursor = buffer.size();
                        maxlen_unix = std::max(maxlen_unix, buffer.size());
                        std::cout << "\r" << prompt;
                        for (size_t i = 0; i < maxlen_unix; ++i) std::cout << ' ';
                        std::cout << "\r" << prompt << buffer << std::flush;
                    } else if (history_index == 0) {
                        history_index = -1;
                        buffer = current_edit;
                        cursor = buffer.size();
                        std::cout << "\r" << prompt;
                        for (size_t i = 0; i < maxlen_unix; ++i) std::cout << ' ';
                        std::cout << "\r" << prompt << buffer << std::flush;
                    }
                }
            }
        } else if (ch >= 32 && ch <= 126) { // Printable
            buffer.insert(buffer.begin() + cursor, ch);
            ++cursor;
            if (buffer.size() > maxlen_unix) maxlen_unix = buffer.size();
            for (size_t i = cursor - 1; i < buffer.size(); ++i) std::cout << buffer[i];
            for (size_t i = buffer.size(); i < maxlen_unix; ++i) std::cout << ' ';
            for (size_t i = cursor; i < maxlen_unix; ++i) std::cout << "\b";
            std::cout << std::flush;
            if (history_index != -1) { history_index = -1; }
        }
    }
    if (!buffer.empty() && (history.empty() || buffer != history.back())) {
        history.push_back(buffer);
        if ((int)history.size() > history_max) history.erase(history.begin());
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    return buffer;
}
