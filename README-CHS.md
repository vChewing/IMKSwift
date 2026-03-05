# IMKSwift

为 Swift 6 及更高版本提供的现代化 **InputMethodKit** 封装，可在 macOS 上安全、并发且类型安全地构建输入法引擎。

## 概述

IMKSwift 是 [唯音专案 (vChewing Project)](https://github.com/vChewing) 的一部分，提供了 Apple InputMethodKit 框架的 Swift 原生替代方案。它结合了 Objective-C 的互操作性和 Swift 6 严格的并发模型，提供了 `@MainActor` 隔离的 API，在现代 Swift 代码中更容易使用。

与其艰难地处理不安全的类型转换、裸露的 `id` 类型和隐式的全局状态，IMKSwift 提供：

- **明确的 `@MainActor` 隔离** 在每个 API 上
- **具体的、带有空值性（Nullability）注解的类型** 而非裸露的 `id` 指针
- **完整的 Swift 6 并发支持**
- **完整的 InputMethodKit 接口** 以及安全使用的改进

## 功能特性

### 类型安全与空值性
所有 API 都包含明确的空值性注解（`_Nullable`、`_Nonnull`）并使用具体的 Objective-C 类型（`NSString`、`NSAttributedString`、`NSDictionary`、`NSArray`、`NSEvent` 等），而非泛用的 `id`。

### MainActor 隔离
每个方法和属性都标记为 `@MainActor`，确保了编译时的调用点（Call-site）安全性，并防止并发代码中的数据竞争（Data race）。

### 完整的 InputMethodKit 覆盖
IMKSwift 重新导出并增强了以下 InputMethodKit 组件：

- **IMKCandidates** — 候选词面板管理和显示
- **IMKServer** — 输入法会话服务器
- **IMKInputController** — 输入法事件处理与组字（Composition）
- **IMKTextInput** — 文本编辑客户端协议
- **辅助协议** — IMKStateSetting、IMKMouseHandling、IMKServerInput

### 适配 Swift 6
考虑到 Swift 6 严格的并发模型而构建。所有 API 都被适当隔离，可以在并发环境中使用而不会产生数据竞争。

## 系统要求

- **Swift** 6.2 或更高版本
- **macOS** 取决于您在使用的 Swift 版本
- **Xcode** 16.0 或更高版本

## 安装

### Swift Package Manager

在你的 `Package.swift` 中添加 IMKSwift：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.05"),
```

然后将其添加为目标的依赖：

```swift
.target(
  name: "MyInputMethod",
  dependencies: [
    .product(name: "IMKSwift", package: "IMKSwift"),
  ]
)
```

## 使用方式

### 基本输入法控制器设置

```swift
import IMKSwift

class MyInputMethodController: IMKInputController {
  override func handle(_ event: NSEvent?, client sender: some IMKTextInput) -> Bool {
    // 具有完整类型安全的事件处理
    guard let event else { return false }
    
    // 处理输入...
    return true
  }
  
  override func inputText(_ string: String, client sender: some IMKTextInput) -> Bool {
    // 文本输入处理
    return true
  }
  
  override func candidates(_ sender: some IMKTextInput) -> [Any]? {
    // 返回候选词建议
    return nil
  }
}
```

### 使用候选词面板

```swift
// 创建并显示候选词面板
let candidates = IMKCandidates(
  server: server,
  panelType: .horizontal,
  styleType: .default
)

candidates.show(.below)
```

### 组字（Composition）管理

```swift
// 使用属性更新组字状态
updateComposition()

// 访问选择和替换范围
let selRange = selectionRange()
let replaceRange = replacementRange()

// 提交组字
commitComposition(client)
```

## 架构

### 结构

- **IMKSwift** — 包含协议定义与扩展的主要 Swift 库
- **IMKSwiftModernHeaders** — 现代化的 Objective-C 头文件，具有 `@MainActor` 注解和类型改进

### 扩展模式

所有的 InputMethodKit 类都通过 Objective-C 中的类别（Category）进行扩展（Extension），并通过 Swift 中的协议遵循重新导出。这保持了完整的 API 兼容性，同时增加了类型安全和并发保障。

## API 文档

### 核心协议

#### IMKInputSessionControllerProtocol

主线程隔离的超级协议（Super-protocol），涵盖：
- `IMKStateSetting` — 激活、停用、偏好设置
- `IMKInputController` — 事件处理、组字、候选词管理
- `IMKMouseHandling` — 鼠标事件处理
- `IMKServerInput` — 文本和事件输入

下游代码应该子类化 `IMKInputSessionController`（而不仅仅是 `IMKInputController`）来应用这些改进。

### 关键方法

**状态管理：**
- `activateServer(_:)` — 激活输入法
- `deactivateServer(_:)` — 停用输入法
- `showPreferences(_:)` — 显示偏好设置对话框

**文本与组字：**
- `inputText(_:key:modifiers:client:)` — 处理键盘输入
- `updateComposition()` — 更新当前组字
- `cancelComposition()` — 取消进行中的组字
- `commitComposition(_:)` — 提交组字

**候选词管理：**
- `candidates(_:)` — 提供候选词建议
- `candidateSelected(_:)` — 处理候选词选择
- `candidateSelectionChanged(_:)` — 处理选择改变

**鼠标处理：**
- `mouseDown(onCharacterIndex:coordinate:withModifier:continueTracking:client:)`
- `mouseUp(onCharacterIndex:coordinate:withModifier:client:)`
- `mouseMoved(onCharacterIndex:coordinate:withModifier:client:)`

## 与原始 InputMethodKit 的差异

| 功能 | InputMethodKit | IMKSwift |
|------|----------------|----------|
| **并发** | 无隔离 | 完整的 `@MainActor` 隔离 |
| **类型安全** | 裸露的 `id` 类型 | 具体的命名类型 |
| **空值性** | 隐式 | 明确注解 |
| **Swift 支持** | 基本桥接 | 完整的 Swift 6 集成 |
| **候选词面板** | 手动设置 | 简化的 API |


## 相关专案

此库是 [唯音专案 (vChewing Project)](https://github.com/vChewing) 的一部分，该专案是 macOS 的现代、开源输入法框架。

## 许可证

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

详见 [LICENSE](LICENSE)。
