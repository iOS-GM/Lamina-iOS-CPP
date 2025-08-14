/*
 * 合并后的模块加载器实现
 */
#include "module_loader.hpp"
#include <iostream>

// Helper functions for conversion
LaminaValue ModuleLoader::valueToLamina(const Value& val) {
    LaminaValue result = LAMINA_MAKE_NULL();
    
    switch (val.type) {
        case Value::Type::Null:
            result = LAMINA_MAKE_NULL();
            break;
        case Value::Type::Bool:
            result = LAMINA_MAKE_BOOL(std::get<bool>(val.data));
            break;
        case Value::Type::Int:
            result = LAMINA_MAKE_INT(std::get<int>(val.data));
            break;
        case Value::Type::Float:
            result = LAMINA_MAKE_DOUBLE(std::get<double>(val.data));
            break;
        case Value::Type::String:
            result = LAMINA_MAKE_STRING(val.to_string().c_str());
            break;
        default:
            result = LAMINA_MAKE_NULL();
            break;
    }
    return result;
}

Value ModuleLoader::laminaToValue(const LaminaValue& val) {
    Value result;
    switch (val.type) {
        case LAMINA_TYPE_NULL:
            result = Value();
            break;
        case LAMINA_TYPE_BOOL:
            result = Value(val.data.bool_val != 0);
            break;
        case LAMINA_TYPE_INT:
            result = Value(val.data.int_val);
            break;
        case LAMINA_TYPE_DOUBLE:
            result = Value(val.data.double_val);
            break;
        case LAMINA_TYPE_STRING:
            result = Value(std::string(val.data.string_val));
            break;
        default:
            result = Value();
            break;
    }
    return result;
}

std::vector<LaminaValue> ModuleLoader::vectorToLamina(const std::vector<Value>& vals) {
    std::vector<LaminaValue> result;
    result.reserve(vals.size());
    for (const auto& val : vals) {
        result.push_back(valueToLamina(val));
    }
    return result;
}

std::vector<Value> ModuleLoader::laminaToVector(const LaminaValue* vals, int count) {
    std::vector<Value> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.push_back(laminaToValue(vals[i]));
    }
    return result;
}

// 构造函数
ModuleLoader::ModuleLoader(const std::string& path) : m_path(path) {
    std::cout << "Loading module: " << path << std::endl;
    
    // 基本安全检查
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Module file does not exist: " << path << std::endl;
        return;
    }
    
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.close();
    
    if (fileSize < 1024) {
        std::cerr << "Module file too small: " << path << std::endl;
        return;
    }
    
    // 加载动态库
#ifdef _WIN32
    m_handle = LoadLibraryA(path.c_str());
    if (!m_handle) {
        std::cerr << "Failed to load module: " << path << " Error: " << GetLastError() << std::endl;
        return;
    }
#else
    m_handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!m_handle) {
        std::cerr << "Failed to load module: " << path << " Error: " << dlerror() << std::endl;
        return;
    }
#endif

    // 验证模块签名
    const char* (*sig_func)() = nullptr;
#ifdef _WIN32
    sig_func = (const char* (*)())GetProcAddress((HMODULE)m_handle, "lamina_module_signature");
#else
    sig_func = (const char* (*)())dlsym(m_handle, "lamina_module_signature");
#endif
    
    if (!sig_func) {
        std::cerr << "Module missing signature function: " << path << std::endl;
        unload();
        return;
    }

    const char* signature = sig_func();
    if (!signature || strcmp(signature, "LAMINA_MODULE_V2") != 0) {
        std::cerr << "Invalid module signature: " << (signature ? signature : "null") << std::endl;
        unload();
        return;
    }

    // 获取初始化函数
    LaminaModuleExports* (*init_func)() = nullptr;
#ifdef _WIN32
    init_func = (LaminaModuleExports* (*)())GetProcAddress((HMODULE)m_handle, "lamina_module_init");
#else
    init_func = (LaminaModuleExports* (*)())dlsym(m_handle, "lamina_module_init");
#endif
    
    if (!init_func) {
        std::cerr << "Module missing init function: " << path << std::endl;
        unload();
        return;
    }

    // 调用初始化函数
    m_exports = init_func();
    if (!m_exports) {
        std::cerr << "Module initialization failed: " << path << std::endl;
        unload();
        return;
    }
}

// 析构函数
ModuleLoader::~ModuleLoader() {
    unload();
}

void ModuleLoader::unload() {
    if (m_handle) {
#ifdef _WIN32
        FreeLibrary((HMODULE)m_handle);
#else
        dlclose(m_handle);
#endif
        m_handle = nullptr;
    }
    m_exports = nullptr;
}

// 动态库加载部分实现
void* ModuleLoader::findSymbol(const std::string& symbolName) {
    void* symbol = nullptr;
#ifdef _WIN32
    if (!m_handle) {
        return nullptr;
    }
    symbol = (void*)GetProcAddress((HMODULE)m_handle, symbolName.c_str());
    if (!symbol) {
        std::cerr << "Error looking up symbol '" << symbolName << "' in DLL." << std::endl;
        return nullptr;
    }
#elif __linux__
    if (!m_handle) {
        return nullptr;
    }
    dlerror();
    symbol = dlsym(m_handle, symbolName.c_str());
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Error looking up symbol '" << symbolName << "': " << dlsym_error << std::endl;
        return nullptr;
    }
#else
    // 对于其他 Unix-like 系统（如 macOS）
    if (!m_handle) {
        return nullptr;
    }
    dlerror();
    symbol = dlsym(m_handle, symbolName.c_str());
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Error looking up symbol '" << symbolName << "': " << dlsym_error << std::endl;
        return nullptr;
    }
#endif
    return symbol;
}

std::vector<ModuleLoader::EntryFunction> ModuleLoader::findEntryFunctions() {
    std::vector<EntryFunction> entryFunctions;
    if (!m_handle) return entryFunctions;

#ifdef _WIN32
    // 只查找 _entry 符号
    void* sym = findSymbol("_entry");
    if (sym) {
        auto entryFunc = reinterpret_cast<void (*)(Interpreter&)>(sym);
        entryFunctions.push_back([entryFunc](Interpreter& interpreter) {
            entryFunc(interpreter);
        });
    }
#elif __ANDROID__
    // Android跳过
    void* sym = dlsym(m_handle, "_entry");
    if (sym) {
        auto entryFunc = reinterpret_cast<void (*)(Interpreter&)>(sym);
        entryFunctions.push_back([entryFunc](Interpreter& interpreter) {
            entryFunc(interpreter);
        });
    }    
#elif __linux__
    if (!m_handle) {
        return entryFunctions;
    }

    struct link_map* lmap;
    dlinfo(m_handle, RTLD_DI_LINKMAP, &lmap);

    const ElfW(Sym)* symtab = nullptr;
    const char* strtab = nullptr;
    size_t symtabsz = 0;

    for (ElfW(Dyn)* d = lmap->l_ld; d->d_tag != DT_NULL; ++d) {
        if (d->d_tag == DT_SYMTAB) {
            symtab = reinterpret_cast<const ElfW(Sym)*>(d->d_un.d_ptr);
        } else if (d->d_tag == DT_STRTAB) {
            strtab = reinterpret_cast<const char*>(d->d_un.d_ptr);
        } else if (d->d_tag == DT_SYMENT) {
            symtabsz = d->d_un.d_val;
        }
    }

    if (symtab && strtab && symtabsz > 0) {
        size_t symcount = (reinterpret_cast<const char*>(strtab) - reinterpret_cast<const char*>(symtab)) / symtabsz;

        for (size_t j = 0; j < symcount; ++j) {
            const ElfW(Sym)* sym = &symtab[j];
            if (sym->st_name != 0) {
                const char* symbolNamePtr = strtab + sym->st_name;
                std::string symbolName = symbolNamePtr;
                if (symbolName.find("_entry") != std::string::npos) {
                    void* symbolAddr = findSymbol(symbolName.c_str());
                    if (symbolAddr) {
                        auto entryFunc = reinterpret_cast<void (*)(Interpreter&)>(symbolAddr);
                        entryFunctions.emplace_back([entryFunc](Interpreter& interpreter) {
                            entryFunc(interpreter);
                        });
                    }
                }
            }
        }
    }
#else
    // 对于其他 Unix-like 系统（如 macOS），使用简单的 dlsym 查找
    void* sym = dlsym(m_handle, "_entry");
    if (sym) {
        auto entryFunc = reinterpret_cast<void (*)(Interpreter&)>(sym);
        entryFunctions.push_back([entryFunc](Interpreter& interpreter) {
            entryFunc(interpreter);
        });
    }
#endif
    return entryFunctions;
}

// 模块管理部分实现
Value ModuleLoader::callModuleFunction(const std::string& full_func_name, const std::vector<Value>& args) {
    if (!isModuleLoaded()) {
        std::cerr << "ERROR: Module not loaded" << std::endl;
        return Value();
    }
    
    // 处理带命名空间的函数名
    std::string actual_name = full_func_name;
    size_t dot_pos = full_func_name.find(".");
    if (dot_pos != std::string::npos) {
        std::string ns = full_func_name.substr(0, dot_pos);
        actual_name = full_func_name.substr(dot_pos + 1);
        if (ns != m_exports->info.namespace_name) {
            std::cerr << "ERROR: Namespace mismatch. Expected '" << m_exports->info.namespace_name 
                      << "' but got '" << ns << "'" << std::endl;
            return Value();
        }
    }
    
    // 查找函数
    LaminaFunctionEntry* target_func = nullptr;
    for (int i = 0; i < m_exports->function_count; ++i) {
        if (m_exports->functions[i].name &&
            std::string(m_exports->functions[i].name) == actual_name) {
            target_func = const_cast<LaminaFunctionEntry*>(&m_exports->functions[i]);
            break;
        }
    }
    
    if (!target_func) {
        std::cerr << "ERROR: Function '" << actual_name << "' not found in module" << std::endl;
        return Value();
    }
    
    if (!target_func->func) {
        std::cerr << "ERROR: Function '" << actual_name << "' has null pointer" << std::endl;
        return Value();
    }
    
    // 转换参数
    std::vector<LaminaValue> lamina_args = vectorToLamina(args);
    
    // 调用函数
    try {
        LaminaValue result = target_func->func(
            lamina_args.empty() ? nullptr : lamina_args.data(), 
            (int)lamina_args.size()
        );
        
        // 转换返回值
        Value ret_val = laminaToValue(result);
        return ret_val;
    } catch (...) {
        std::cerr << "ERROR: Exception during function call" << std::endl;
        return Value();
    }
}

bool ModuleLoader::registerToInterpreter(Interpreter& interpreter) {
    if (!isModuleLoaded()) {
        std::cerr << "ERROR: Cannot register unloaded module" << std::endl;
        return false;
    }
    
    // Module functions will be called through the call_module_function method
    // when the interpreter encounters a function call with dot notation
    return true;
}