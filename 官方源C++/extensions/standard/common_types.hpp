// common_types.hpp
#pragma once

class Interpreter;  // 前向声明

using EntryFunction = std::function<void(Interpreter&)>;