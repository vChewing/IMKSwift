# IMKSwift

为 Swift 6 及更高版本提供的现代化 **InputMethodKit** 封装，可在 macOS 上安全、并发且类型安全地构建输入法引擎。

## 概述

IMKSwift 是 [唯音专案 (vChewing Project)](https://github.com/vChewing) 的一部分，提供了 Apple InputMethodKit 框架的 Swift 原生替代方案。它结合了 Objective-C 的互操作性和 Swift 6 严格的并发模型，提供了 `@MainActor` 隔离的 API，在现代 Swift 代码中更容易使用。

InputMethodKit 的历史可追溯到 macOS 10.5 Leopard——早于 ARC、Sandbox 和 Swift 的问世。这是一个横跨两代技术变革的遗产级框架。IMKSwift 弥合了这道鸿沟，让现代 Swift 开发者无需与古老的 Objective-C 模式搏斗，就能构建输入法。

与其艰难地处理不安全的类型转换、裸露的 `id` 类型和隐式的全局状态，IMKSwift 提供：

- **明确的 `@MainActor` 隔离** 在每个 API 上
- **具体的、带有空值性（Nullability）注解的类型** 而非裸露的 `id` 指针
- **完整的 Swift 6 并发支持**
- **完整的 InputMethodKit 接口** 以及安全使用的改进

## 为什么使用 IMKInputSessionController？

Apple 的 `IMKInputController` 设计上是在 MainActor 上运行的，但其 Objective-C 头文件缺乏适当的 `@MainActor` 标注。当导入 Swift 时，这会产生根本性的问题：原始 SDK 头文件暴露的 API 没有线程隔离，导致在 Swift 6 严格模式下出现**无法解决的并发检查错误**。

**在 Swift 6 中你根本无法直接使用 `IMKInputController`。** 即使你尝试直接覆盖头文件，Swift 仍然会读取原始的 Xcode SDK 头文件，导致 API 冲突。唯一的解决方案是使用**不同名称的子类**——`IMKInputSessionController`——它继承了所有功能，同时提供正确标注 `@MainActor` 的 API。

> ⚠️ **重要：** 如果你在 Swift 6 中直接子类化 `IMKInputController`，你将面临编译错误，而这些错误只能通过丑陋的指针操作来「修复」，强行将对象塞到 `@MainActor` 上。别这么做。请使用 `IMKInputSessionController`。

## 功能特性

### 类型安全与空值性
所有 API 都包含明确的空值性注解（`_Nullable`、`_Nonnull`）并使用具体的 Objective-C 类型（`NSString`、`NSAttributedString`、`NSDictionary`、`NSArray`、`NSEvent` 等），而非泛用的 `id`。

### MainActor 隔离
每个方法和属性都标记为 `@MainActor`，确保了编译时的调用点（Call-site）安全性，并防止并发代码中的数据竞争（Data race）。

### 完整的 InputMethodKit 覆盖
IMKSwift 重新导出并增强了以下 InputMethodKit 组件：

- **IMKCandidates** — 候选词面板管理和显示
- **IMKServer** — 输入法会话服务器
- **IMKInputSessionController** — 输入法事件处理与组字（Swift 6+ **唯一**建议使用的基础类）
- **IMKTextInput** — 文本编辑客户端协议
- **辅助协议** — IMKStateSetting、IMKMouseHandling、IMKServerInput

### 适配 Swift 6
考虑到 Swift 6 严格的并发模型而构建。所有 API 都被适当隔离，可以在并发环境中使用而不会产生数据竞争。

## 系统要求

- **Swift** 6.2 或更高版本
- **Xcode** 16.0 或更高版本
- **macOS**：
    - macOS 10.13 High Sierra 及更高版本（取决于使用的 Swift 版本）。
    - 代码本身可在 macOS 10.09 Mavericks 上运行，但需要对应的 macOS SDK 与 libARCLite。

## 安装

### Swift Package Manager

在你的 `Package.swift` 中添加 IMKSwift：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.06"),
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

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  override public func handle(_ event: NSEvent?, client sender: any IMKTextInput) -> Bool {
    // 具有完整类型安全的事件处理
    guard let event else { return false }
    
    // 处理输入...
    return true
  }
  
  override func inputText(_ string: String, client sender: any IMKTextInput) -> Bool {
    // 文本输入处理
    return true
  }
  
  override func candidates(_ sender: any IMKTextInput) -> [Any]? {
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
  panelType: .horizontal
)

candidates.show(.below)
```

### 组字（Composition）管理

```swift
// 更新组字状态
updateComposition()

// 访问选择和替换范围
let selRange = selectionRange()
let replaceRange = replacementRange()

// 提交组字
commitComposition(sender)
```

## 最佳实践

### 1. NSConnection 名称规范

输入法的 `Info.plist` 中的 `InputMethodConnectionName` 字段**必须**设置为：

```
$(PRODUCT_BUNDLE_IDENTIFIER)_Connection
```

> ⚠️ 此命名规范自 macOS 10.7 Lion 起为强制要求。若不遵守，开启 Sandbox 后输入法将无法正常加载。你会在 `Console.app` 中看到 NSConnection 相关的错误资讯。

### 2. 开启 App Sandbox

务必开启 App Sandbox。鉴于你被迫使用脆弱的 NSConnection 机制，不开启 Sandbox 的话就连 Apple 都根本没有任何手段相信你的输入法是安全的。对 Sandbox 的启用，就是最佳的资讯安全投名状。

建议的 `entitlements` 文件内容：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>com.apple.security.app-sandbox</key>
  <true/>
  <key>com.apple.security.files.bookmarks.app-scope</key>
  <true/>
  <key>com.apple.security.files.user-selected.read-write</key>
  <true/>
  <key>com.apple.security.network.client</key>
  <true/>
  <key>com.apple.security.temporary-exception.files.home-relative-path.read-write</key>
  <array>
    <string>/Library/Preferences/$(PRODUCT_BUNDLE_IDENTIFIER).plist</string>
  </array>
  <key>com.apple.security.temporary-exception.mach-register.global-name</key>
  <string>$(PRODUCT_BUNDLE_IDENTIFIER)_Connection</string>
  <key>com.apple.security.temporary-exception.shared-preference.read-only</key>
  <string>$(PRODUCT_BUNDLE_IDENTIFIER)</string>
</dict>
</plist>
```

### 3. 不要在控制器中强持有对象

**你的 `IMKInputSessionController` 子类不应直接持有业务逻辑对象。** 这对于处理高频率输入法切换（例如使用 CapsLock 在中英输入法之间切换）至关重要。

当用户频繁切换输入法时，系统每次都会创建新的 `IMKInputController` 副本。如果你的控制器持有对大型对象的强引用，ARC 清理会造成明显的卡顿。

**建议模式：** 使用以客户对象键的弱键缓存（`NSMapTable`）：

```swift
import IMKSwift

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  // 使用对会话的弱引用
  weak var session: InputSession?
  
  override public init(server: IMKServer, delegate: Any?, client inputClient: any IMKTextInput) {
    super.init(server: server, delegate: delegate, client: inputClient)
    
    // 初始化或获取缓存的会话
    self.session = InputSessionCache.session(for: inputClient, controller: self)
  }
}

@MainActor
final class InputSessionCache {
  private static let cache = NSMapTable<NSObject, InputSession>.weakToStrongObjects()
  
  static func session(for client: any IMKTextInput, controller: MyInputMethodController) -> InputSession {
    let clientObj = client as! NSObject
    if let cached = cache.object(forKey: clientObj) {
      cached.reassign(controller: controller)
      return cached
    }
    let newSession = InputSession(controller: controller)
    cache.setObject(newSession, forKey: clientObj)
    return newSession
  }
}

@MainActor
final class InputSession {
  weak var controller: MyInputMethodController?
  
  init(controller: MyInputMethodController) {
    self.controller = controller
  }
  
  func reassign(controller: MyInputMethodController) {
    self.controller = controller
  }
}
```

### 4. 以 Swift Package 库形式组织代码

macOS 输入法无法使用断点调试——这会冻结客户端应用程序和你的整个桌面。唯一可行的方法是使用模拟客户端进行单元测试。

将输入法组织为 Swift Package：
- **核心库** — 业务逻辑，可通过单元测试测试
- **输入法目标** — 连接 IMKSwift 与核心库的薄包装层

这也允许你编写标准的 AppKit 应用程序来模拟打字过程，并使用 Instruments 检测运存泄漏。

### 5. 避免使用 IMKCandidates

系统提供的 `IMKCandidates` 有严重问题，尤其是在 macOS 26 上，LiquidGlass 渲染导致视觉故障（透明背景上的白色文字）。**考虑使用 SwiftUI 或 AppKit 实现你自己的候选词面板**。

> 连 Apple 自己的 NumberInput 示例都避免使用 `IMKCandidates`。

### 6. 运存管理

用户的运存空间寸土寸金。虽然 macOS 26 的 AppKit 低效可能导致输入法占用 80–200 MB：

- **在 `activateServer()` 中监控运存使用量**，若超过 1024 MB 则自我终止（通过 `NSNotification` 通知用户）
- **最小化 NSWindow 数量** — 从 macOS 26 开始，NSWindow 的运存永远不会被系统回收。尽可能将面板（工具提示、候选词窗等）整合到单一 `NSPanel` 中

## 架构

### 结构

- **IMKSwift** — 包含协议定义与扩展的主要 Swift 库
- **IMKSwiftModernHeaders** — 现代化的 Objective-C 头文件，具有 `@MainActor` 注解和类型改进

### IMKInputSessionController 解决方案

`IMKInputSessionController` 是 `IMKInputController` 的具体子类，它的存在完全是为了解决 Swift/Objective-C 互操作性限制。

**问题所在：**
- `IMKInputController` 的原始 SDK 头文件暴露的 API 没有 `@MainActor` 隔离
- Swift 将这些导入为非隔离的，导致严格的并发错误
- 你无法为现有类覆盖头文件而不造成 API 冲突

**解决方案：**
- 创建一个新的子类（`IMKInputSessionController`）具有相同的 API 接口
- 通过 Objective-C 中的 `#pragma clang attribute` 应用 `@MainActor` 隔离
- Swift 看到一个全新的类，其方法具有正确的隔离

**你会得到什么：**
- 所有方法都具有 `@MainActor` 隔离
- 明确的空值性注解（`Nonnull` / `Nullable`）
- 使用具体的 Objective-C 类型而非裸露的 `id`

在配套的 Swift 模块中，`IMKInputSessionController` 被扩展以遵循 `IMKInputSessionControllerProtocol`，这是一个 Swift 原生的 `@MainActor` 协议，镜像了其完整的 API 接口。

## API 文档

### 核心协议

#### IMKInputSessionControllerProtocol

主线程隔离的协议，涵盖：
- `IMKStateSetting` — 激活、停用、偏好设置
- `IMKMouseHandling` — 鼠标事件处理
- `IMKServerInput` — 文本和事件输入
- 组字管理 — `updateComposition()`、`commitComposition(_:)` 等

### 关键方法

**状态管理：**
- `activateServer(_:)` — 激活输入法
- `deactivateServer(_:)` — 停用输入法
- `showPreferences(_:)` — 显示偏好设置对话框

**文本与组字：**
- `inputText(_:key:modifiers:client:)` — 处理键盘输入（详细参数）
- `inputText(_:client:)` — 处理键盘输入（简化版）
- `handle(_:client:)` — 处理原始 `NSEvent`
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
| **Swift 6 基础类** | `IMKInputController`（损坏） | `IMKInputSessionController`（正常） |

## 相关专案

此库是 [唯音专案 (vChewing Project)](https://github.com/vChewing) 的一部分。该专案是 macOS 平台打字速度最快的音韵类中文输入法，基于大千声韵并击打字原理与 DAG-DP 组句技术，同时具备原生的简体中文与繁体中文打字能力。

唯音专案推出此套件馈赠社会，也希望能得到一些捐助。详细资讯可洽[唯音输入法的软件主页](https://vchewing.github.io/README.html)。

## 许可证

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

详见 [LICENSE](LICENSE)。
