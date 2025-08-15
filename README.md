# Lamina-iOS-CPP

Lamina 编程语言的 iOS 编译版本！

## 官方资源
- GitHub 官方仓库: [Lamina-dev/Lamina](https://github.com/Lamina-dev/Lamina)
- 越狱 deb 包下载源: [AD's Repo](https://ios-gm.github.io/zqzb/)

> 仓库所有者：AD
> 项目负责人：AD

#### 事先说明：
- 本项目由AD在iOS系统上成功编译！
- 若后续有人需要用的可以使用半预编译文件夹里面的文件配合编译指南的教程来快速编译
- 若需要尝试从头开始编译的我们的仓库提供了Lamina官方的cpp和hpp可以根据编译指南来进行编译

## 开源协议
本项目遵循 **GNU LESSER GENERAL PUBLIC LICENSE Version 2.1** 开源协议。

## 半预编译版本说明

### 兼容性
- ✅ 确认支持 iOS 越狱系统
- ❓ 其他系统兼容性待测试

### 编译要求
- 必须组件：
  - iOS SDK
  - Clang 编译器
  - Theos 开发工具链

### 注意事项
1. 半预编译文件夹内的文件应全部包含
2. 缺失文件可能导致功能不完整或运行失败
3. 推荐使用完整编译工具链

## 各平台推荐编译工具

| 平台 | 推荐工具 | 备注 |
|------|----------|------|
| iOS (越狱) | Clang + Theos + iOS SDK | 支持有根/无根越狱 |
| Windows | GCC (MinGW-w64) / MSVC | Visual Studio 可选 |
| Linux | GCC (默认) / Clang | 多数发行版自带 |
| macOS | Clang (默认) / GCC | 需 Xcode 命令行工具 |
| Android | NDK Clang / CMake | 需 Android NDK |

### 编译指南

#### C语言基础编译签名
clang -arch arm64 \
      -isysroot /path/to/SDK \
      -mios-version-min=版本号 \
      -std=c11 \  # C11标准
      源文件.c -o 输出文件
ldid -S 输出文件
chmod +x 输出文件
./输出文件

#### C++语言基础编译签名
clang++ -arch arm64 \
        -isysroot /path/to/SDK \
        -mios-version-min=版本号 \
        -std=c++17 \  # C++17标准
        源文件.cpp -o 输出文件
ldid -S 输出文件
chmod +x 输出文件
./输出文件

### 高级签名编译
#### C版本
clang -arch arm64 \
      -isysroot /path/to/SDK \
      -mios-version-min=版本号 \
      源文件.c -o 输出文件

#### C++版本
clang++ -arch arm64 \
        -isysroot /path/to/SDK \
        -mios-version-min=版本号 \
        源文件.cpp -o 输出文件

#### 通用签名步骤
ldid -Sentitlements.xml 输出文件
chmod +x 输出文件
./输出文件

#### 完整编译模板(无签名版本)
##### C版本
clang \
    -arch arm64 \
    -isysroot /path/to/SDK \
    -mios-version-min=版本号 \
    -std=gnu11 \
    -fobjc-arc \
    -framework Foundation \
    -I ./include \
    -L ./lib \
    -l自定义库 \
    源文件.c \
    -o 输出文件

##### C++版本
clang++ \
    -arch arm64 \
    -isysroot /path/to/SDK \
    -mios-version-min=版本号 \
    -std=gnu++17 \
    -fobjc-arc \
    -framework Foundation \
    -I ./include \
    -L ./lib \
    -l自定义库 \
    源文件.cpp \
    -o 输出文件

#### 半预编译快速完成(需要进入文件夹内)
##### C版本
clang -arch arm64 \
      -isysroot /path/to/SDK \
      -mios-version-min=版本号 \
      *.o \
      -o 输出文件

##### C++版本
clang++ -arch arm64 \
        -isysroot /path/to/SDK \
        -mios-version-min=版本号 \
        *.o \
        -o 输出文件

## SDK 路径参考
- 无根越狱: `/var/jb/theos/sdks/`
- 有根越狱: `/var/theos/sdks/` 或 `/theos/sdks/`

> 注意：实际路径可能因设备而异

## 签名说明

### 普通签名
```bash
ldid -S 目标文件
```

### 高级权限签名
```bash
ldid -Sentitlements.xml 目标文件
```

**高级签名优势**：
- 获取更多系统权限
- 防止被系统终止
- 支持沙盒逃逸功能

> 注意：高级签名需要有效的 entitlements.xml 文件

## 常见问题
1. **签名失败**：
   - 确认文件是可执行格式
   - 检查权限设置 (chmod +x)
   - 验证 entitlements.xml 有效性

2. **编译错误**：
   - 检查 SDK 路径是否正确
   - 确认架构匹配 (arm64/armv7)
   - 验证依赖库完整性

3. **运行崩溃**：
   - 尝试使用高级签名
   - 检查系统版本兼容性
   - 验证依赖框架是否存在