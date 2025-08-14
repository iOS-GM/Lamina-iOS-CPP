/*
 * 模块加载器头文件 - 最终修复版
 */
#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <fstream>
#include <cstring>
#include "module_api.hpp"
#include "value.hpp"

// 前向声明 Interpreter
class Interpreter;

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#ifdef __linux__
#include <link.h>
#include <elf.h>
#endif
#endif

class ModuleLoader {
public:
    // 类型定义
    using EntryFunction = std::function<void(Interpreter&)>;

    // 构造函数/析构函数
    explicit ModuleLoader(const std::string& path);
    ~ModuleLoader();

    // 动态库操作
    void* findSymbol(const std::string& symbolName);
    bool isLoaded() const { return m_handle != nullptr; }
    
    // 入口函数查找
    std::vector<EntryFunction> findEntryFunctions();

    // 模块管理
    void unload();
    bool isModuleLoaded() const { return m_handle != nullptr && m_exports != nullptr; }
    bool registerToInterpreter(Interpreter& interpreter);
    
    Value callModuleFunction(const std::string& func_name, const std::vector<Value>& args);
    const std::string& getPath() const { return m_path; }
    const LaminaModuleInfo* getModuleInfo() const { 
        return m_exports ? &m_exports->info : nullptr; 
    }

private:
    void* m_handle = nullptr;
    std::string m_path;
    LaminaModuleExports* m_exports = nullptr;

    // 转换工具
    static LaminaValue valueToLamina(const Value& val);
    static Value laminaToValue(const LaminaValue& val);
    static std::vector<LaminaValue> vectorToLamina(const std::vector<Value>& vals);
    static std::vector<Value> laminaToVector(const LaminaValue* vals, int count);
};