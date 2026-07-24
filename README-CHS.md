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

Apple 的 `IMKInputController` 设计上是在 MainActor 上运行的，但其 Objective-C 头文件缺乏适当的 `@MainActor` 标注。当导入 Swift 时，这会产生根本性问题：原始 SDK 头文件暴露的 API 没有线程隔离，导致在 Swift 6 严格模式下出现**无法解决的并发检查错误**。

**在 Swift 6 中你根本无法直接使用 `IMKInputController`。** 即使你尝试直接覆盖头文件，Swift 仍然会读取原始的 Xcode SDK 头文件，导致 API 冲突。`IMKInputSessionController` 提供了干净、经 `@MainActor` 隔离的 API 表面。

> ⚠️ **禁止 subclass `IMKInputSessionController`。** 从 v26.07.24 起，IMKSwift 采用**基于注入（Injection）的架构**。subclass 会将 controller 的生命周期（`alloc` / `dealloc` / `retain` / `release`）暴露给 Swift ARC，重新引入导致 CapsLock 切换卡顿的 MainActor 竞争条件。取而代之的是，所有 IMK 回调均由 class-level static blocks 在 `applicationDidFinishLaunching` 中注入。Controller 生命周期完全留在 ObjC MRC（`-fno-objc-arc`）内，Swift 仅透过 raw `uintptr_t` 位址与 controller 交互——绝不经过 ARC 管理的引用。

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
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.07.24"),
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

### 部署架构总览

IMKSwift 从 v26.07.24 起采用了**基于注入（Injection）的架构**，不再需要 subclass `IMKInputSessionController`。所有 IMK 回调均通过 class-level static blocks 注入——在启动时一次性设定，由所有 controller 实例共享。

**Info.plist 配置：** 直接将 controller class 声明为 `IMKInputSessionController`（无需自定义子类）。以下 keys 必须全部指向 `IMKInputSessionController`，以确保 controller 生命周期绝不接触 Swift ARC：

```xml
<key>InputMethodConnectionName</key>
<string>$(PRODUCT_BUNDLE_IDENTIFIER)_Connection</string>
<key>InputMethodServerControllerClass</key>
<string>IMKInputSessionController</string>
<key>InputMethodServerDataSourceClass</key>
<string>IMKInputSessionController</string>
<key>InputMethodServerDelegateClass</key>
<string>IMKInputSessionController</string>
<key>InputMethodSessionController</key>
<string>IMKInputSessionController</string>
```

**编译配置：** 你的工程需要同时依赖 IMKSwift 的两个 target，并对 `IMKSwiftModernHeaders` 设定 `-fno-objc-arc`：

```swift
// Package.swift
.target(
  name: "IMKSwiftModernHeaders",
  cSettings: [.unsafeFlags(["-fno-objc-arc"])]
),
```

**启动注入：** 所有 IMK 回调通过 configure blocks 在 `applicationDidFinishLaunching` 阶段一次性注入。每个 block 接收 raw `uintptr_t` 位址（而非 ObjC 对象引用）——不执行 retain/release，彻底避开 ARC 开销。Swift 端通过 raw 位址查询 controller 对应的 session 并路由到业务逻辑。

### 可用的 Configure Blocks

| Block | 用途 | 签名 |
|-------|------|------|
| `configureActivatingServer` | Controller 被启用 | `(clientAddr, selfAddr) -> Void` |
| `configureDeactivatingServer` | Controller 被停用 | `(clientAddr, selfAddr) -> Void` |
| `configureDealloc` | Controller 即将释放 | `(selfAddr) -> Void` |
| `configureShowingPreferences` | 显示偏好设定 | `(clientAddr, selfAddr) -> Void` |
| `configureHidingPallettes` | 隐藏 UI 面板 | `(selfAddr) -> Void` |
| `configureInputControllerWillClose` | Controller 即将关闭 | `(selfAddr) -> Void` |
| `configureProvidingSelectionRange` | 提供选取范围 | `(selfAddr) -> NSRange` |
| `configureProvidingIMEMenu` | 构建 IME 选单 | `(selfAddr) -> NSMenu?` |
| `configureProvidingComposedString` | 提供组字内容 | `(clientAddr, selfAddr) -> Any?` |
| `configureAutoCommittingComposition` | 自动递交组字 | `(clientAddr, selfAddr) -> Void` |
| `configureProvidingRecognizedEvents` | 已识别的事件遮罩 | `(clientAddr, selfAddr) -> NSUInteger` |
| `configureHandlingGivenNullableEvent` | 处理 NSEvent | `(nsEventPtr, clientAddr, selfAddr) -> Bool` |
| `configureSettingObjCValue` | 处理 `setValue:` | `(valuePtr, tag, clientAddr, selfAddr) -> Void` |

> **注意：** 所有位址均为 `uintptr_t`。使用 `UnsafeRawPointer(bitPattern:)` + `Unmanaged<...>.fromOpaque(...).takeUnretainedValue()` 在 Swift 端恢复对象。每次 `takeUnretainedValue()` 前应以 `IMKControllerLifetimeTracker` 复查 controller 是否仍存活。

### 基本注入示例

```swift
import IMKSwift

// ── applicationDidFinishLaunching 时一次性注入 ──

IMKInputSessionController.configureActivatingServer { clientAddr, selfAddr in
  // controller 被激活：将 selfAddr 路由到对应的 InputSession
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.activate()
}

IMKInputSessionController.configureDeactivatingServer { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.deactivate()
}

IMKInputSessionController.configureDealloc { selfAddr in
  // controller 即将释放：清理 session 映射，终止 XPC 连线
  InputSession.cleanupController(addr: selfAddr)
}

IMKInputSessionController.configureHandlingGivenNullableEvent { nsEventPtr, clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return false }
  let event = Unmanaged<NSEvent>.fromOpaque(
    UnsafeRawPointer(bitPattern: nsEventPtr)!
  ).takeUnretainedValue()
  return session.handle(event: event)
}

IMKInputSessionController.configureShowingPreferences { clientAddr, selfAddr in
  // 叫出偏好设定画面
  InputSession.showPreferences()
}

IMKInputSessionController.configureProvidingIMEMenu { selfAddr in
  // 动态构建 IME 选单
  return InputSession.buildMenu(for: selfAddr)
}

IMKInputSessionController.configureProvidingComposedString { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return nil }
  return session.currentComposedString
}

// ── 业务逻辑（纯 Swift，不继承任何 IMK 类型）──

@MainActor
public final class InputSession {
  /// 极性双缓冲 Session 池：两枚 singleton，以 controller generation parity 路由。
  fileprivate static let sessionEven = InputSession(preallocated: ())
  fileprivate static let sessionOdd  = InputSession(preallocated: ())

  /// 依 raw controller 位址的 generation parity 解析对应 session。
  static func session(for ctlAddr: UInt) -> InputSession? {
    let tracker = IMKControllerLifetimeTracker.shared()
    guard tracker.isAddressAlive(ctlAddr) else { return nil }
    let parity = tracker.generationForAddress(ctlAddr) % 2
    return parity == 0 ? sessionEven : sessionOdd
  }

  static func cleanupController(addr: UInt) {
    // 3 秒延迟释放 + XPC 清理（详见最佳实践 #3）
  }

  func handle(event: NSEvent) -> Bool {
    // 处理键盘事件
    return true
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

### 组字管理

```swift
// 更新组字状态
updateComposition()

// 访问选择和替换范围
let selRange = selectionRange()
let replaceRange = replacementRange()

// 提交组字
commitComposition(sender)
```

### 部署核对清单

释出前请确认以下所有项目均已处理：

- [ ] **Info.plist:** `InputMethodServerControllerClass` = `IMKInputSessionController`（非自订子类）。
- [ ] **Package.swift:** `IMKSwiftModernHeaders` target 设有 `cSettings: [.unsafeFlags(["-fno-objc-arc"])]`。
- [ ] **注入时机：** 所有 `configure*` blocks 在 `applicationDidFinishLaunching` 内、**于** `IMKServer.init(name:bundleIdentifier:)` **之前**设定完成。
- [ ] **KVC prune 路径：** 维护单调递增的 controller generation 计数器；当追踪的 controller 数量超过 2 时，透过 `[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]` 移除最旧的非活跃 controller。
- [ ] **Session 路由：** 以 `generation % 2` parity 路由 controller 至 session（而非以 client 位址为键），使用恰好两枚预先配置的 static singleton。
- [ ] **延迟释放：** `deactivateServer:` 后排程 3 秒计时器。触发时：释放持有的 blocks、调用 client wrapper 的 `+terminateForClient*` private class method、清除 controller↔session 对照表。若 controller 在时窗内被重新启用则取消计时器。
- [ ] **XPC 清理：** 在 dealloc 时调用 client wrapper class 的 `+terminateForClientXPCConn:` / `+terminateForClientDOProxy:`。使用 `respondsToSelector:` 依序尝试：`_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper`。（macOS 15 Sequoia 起将 wrapper class 拆分；macOS ≤ 10.15 使用无后缀名称。）
- [ ] **Dangling pointer 防护：** 每次对 raw controller 位址调用 `takeUnretainedValue()` 前，先以 `IMKControllerLifetimeTracker.shared().isTracked(addr:)` 确认其仍存活。
- [ ] **IME 选单注入：** 使用 `class_addMethod` + `imp_implementationWithBlock` 在执行期将选单动作注册至 `IMKInputSessionController`。无需 `@objc` Selector 暴露。
- [ ] **内存指标：** 透过 `task_vm_info.internal` 读取 anonymous private memory（勿用 `phys_footprint`——后者在 Apple Silicon 上会计入 GPU/neural engine 共享内存）。读取前调用 `purgeMallocZones()` 以排干 allocator cache。

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

### 3. 使用 MRC 与极性双缓冲 Session 池

**将 IMKSwiftModernHeaders target 以 `-fno-objc-arc` 编译。** ARC 在 MainActor 上的 retain/release 高频开销于 CapsLock 切换时导致可感知的卡顿。停用 ARC 可彻底消除卡顿，但需自行处理清理。

对 IMKServer 在 macOS 10.9–27 的逆向工程揭露了一个根本缺陷：`_private._controllers` 以 client proxy 位址为键，而 CapsLock 切换会建立**全新**的 proxy（不同位址）。旧 controller 成为永久孤弃——`sessionFinished:` 永远不会对它们触发，因为 IMK 只向当前 proxy 派送信息。每次 CapsLock 切换都泄漏一个 IMKInputController 及其 NSXPCConnection。

**建议架构（唯音输入法「Phase 115」）：**

- **以 KVC 清理孤弃 controller：** 在 init 时分配单调递增的 generation number。当 `_controllers` 超过 2 个条目时，透过 KVC（`[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]`）移除最旧的非活跃 controller。

- **极性双缓冲 Session 池：** 以 `generation % 2` 将 controller 路由至两枚 static `InputSession` singleton（偶数→session A，奇数→session B）。完全消除 LRU 缓存、动态分配与淘汰逻辑。

- **延迟释放与 XPC 清理：** `deactivateServer:` 后排程 3 秒计时器。触发时：释放 blocks、调用 client wrapper 的 private `+terminateForClient*`（依序尝试 `_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper` 以兼容 macOS 27）、清除映射。若重新启动则取消。

- **IME 选单注入：** 使用 `class_addMethod` + `imp_implementationWithBlock` 动态构建 IME 选单，无需 `@objc` Selector 暴露。注入即时指标：controller 存活数量与 anonymous private memory（`task_vm_info.internal`），读取前 purge malloc zones。

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
