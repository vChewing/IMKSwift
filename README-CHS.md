# IMKSwift

为 Swift 6 及更高版本提供的现代化 **InputMethodKit** 封装，可在 macOS 上安全、并发且类型安全地构建输入法引擎。

## 概述

IMKSwift 是 [唯音项目 (vChewing Project)](https://github.com/vChewing) 的一部分，为苹果的 InputMethodKit 框架提供 Swift 原生的封装层。它同时做两件事。

其一，把 IMK 的 Objective-C 接口以 Swift 6 的语言重新声明：具体类型取代 `id`、明确的空值性、每个入口都标上 `@MainActor`。其二，修补 IMKServer 自 10.5 以来就带着的控制器生命周期缺陷——这类缺陷改多少次头文件都碰不到，因为它们住在 IMKServer 的私有状态里，不在声明里。

InputMethodKit 的历史可追溯到 macOS 10.5 Leopard——早于 ARC、Sandbox 和 Swift 的问世。这是一个横跨两代技术变革的遗产级框架。IMKSwift 弥合这道鸿沟，让现代 Swift 开发者无需与古老的 Objective-C 模式搏斗，就能构建输入法。

与其艰难地处理不安全的类型转换、裸露的 `id` 类型和隐含的全局状态，IMKSwift 提供：

- **明确的 `@MainActor` 隔离** 在每个 API 上
- **具体的、空值性标注的类型** 而非裸露的 `id` 指针
- **完整的 Swift 6 并发支持**
- **完整的 InputMethodKit 接口** 以及安全使用的改进
- **修补过的控制器生命周期**（以 `-fno-objc-arc` 编译）：按地址索引的存活追踪、孤儿控制器剔除、延迟拆除、客体连接终止，以及四个仅供诊断的 kill switch

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

### 控制器生命周期之修补

本套件另附一层 Objective-C（`IMKSwiftModernHeaders`，以 `-fno-objc-arc` 编译），处理那些改头文件永远碰不到的事：

- 每一个存活的控制器都以原始地址登记于 `IMKControllerLifetimeTracker`；
- 孤儿控制器会被从 IMKServer 的私有 `_controllers` 字典中剔除，其持有的客体连接包装对象亦一并终止；
- `deactivateServer:` 之后延迟三秒才拆除控制器，使频繁的 CapsLock 切换不至于拆掉一条马上就会被重用的连接；
- 四个仅供诊断的开关，加一套稳定的日志词汇，使现场事故无需另出特制版即可二分定位。

### 适配 Swift 6
考虑到 Swift 6 严格的并发模型而构建。所有 API 都被适当隔离，可以在并发环境中使用而不会产生数据竞争。

## 系统要求

- **Swift** 6.2 或更新版本
- **Xcode** 16.0 或更新版本（现行的私有面头文件系以 26.x 验证）
- **macOS**：
    - macOS 10.13 High Sierra 及更新版本（此为 Swift Concurrency 的运行时下限；实际下限取决于 Swift 版本）。
    - 代码本身可在 macOS 10.09 Mavericks 上运行，但需要对应的 macOS SDK 与 libARCLite。

## 安装

### Swift Package Manager

在你的 `Package.swift` 中添加 IMKSwift：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.07"),
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

### 3. 不要在控制器中强持有对象，且客体绑定必须逐事件重建

两条铁律，都是踩出来的。

**其一：你的 `IMKInputSessionController` 子类别不应强持有业务逻辑对象。** 每一次 CapsLock 切换或客体切换，都可能产生一个新的控制器副本；每切一次就拆掉一整张大型对象图，那样的 ARC churn 会直接反映成输入延迟。

**其二：不要把「启用当下记下的客体地址」当成不变的事实。** IMKServer 是以客体代理的地址为键，维护它私有的控制器表；客体脉络一翻动，系统给你的就是一个新的代理。一个记着「我的客体是 0x…9d0」的会话，会继续把字送给一个早已退役的控制器。症状很安静：中英切换的提示照样出现，按键却从此不再送达。

请以控制器自身的地址为键、从一个容量不超过四个的会话池派发，并在每一次事件上重新认领客体：

```swift
@MainActor
final class SessionPool {
  static let shared = SessionPool(capacity: 4)
  private var slots: [UInt: InputSession] = [:]

  func session(for ctlAddr: UInt) -> InputSession { /* ... */ }
  func reap(using tracker: IMKControllerLifetimeTracker) { /* 收掉控制器已死的槽位 */ }
}

// 在事件回调中、派发之前：
session.adoptClientAddr(ctlAddr)  // 以送事件的人为准，而非启用时的记录
```

两条推论值得写进脑子里：

- **不要等 `-dealloc`。** IMKServer 会让控制器成为孤儿，却从不释放它；而 `sessionFinished:` 是按客体代理逐一派发的，永远到不了那个孤儿身上。请以生命周期追踪器查「该控制器地址是否仍存活」来回收会话。
- **不要在包本体被替换之前就杀掉输入法进程。** IMK 会按需从已注册的包重启输入法；在安装中途终止它，只会让旧代码以新的 PID 回来。请在替换之后再抓取、终止、升级为 `SIGKILL`、并验证（见 §7）。

「退役副本」的危险值得一句具体的说明：若两个控制器曾共用同一个会话槽，第一个退役时该槽的客体绑定会被清空，而第二个仍继续透过它派发。这正是一个「以奇偶配对的双缓冲」会出的毛病，也是本设计改以地址为键、并逐事件刷新绑定的理由。

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

### 7. 把 IMKServer 的私有面当成有观测点的黑箱

你修不了 IMKServer，也没法用断点调试它。你能做的是：让它的行为可被观测，并且让你自己每一项干涉都能单独关掉。

IMKSwift 的 Objective-C 层因此只吐一套很小、很稳定的词汇：

| 词汇 | 含义 |
|---|---|
| `ControllerInit:` | 一个控制器副本被创建，含其地址与世代 |
| `Prune:` | 检查了私有的 `_controllers` 表，或剔除了一条孤儿 |
| `ActivateServer:` / `DeactivateServer:` | 客体的启用与退役 |
| `DelayedDealloc:` | 延迟拆除被排定、跳过或执行 |
| `EventEntry:` | 按键事件被投递到哪一个控制器、其客体是否仍存活 |
| `guard fail:` | 事件已抵达输入法，但没有可用的客体代理 |

在此之上，四个仅供诊断的开关（以类别方法安装；唯音把它们接到「开发者道场」面板）让每一项干涉都能被单独停用：

| 开关 | 停用什么 |
|---|---|
| `configureServerControllerPruningDisabled(_:)` | KVC 修剪 |
| `configureEvictedClientInvalidationDisabled(_:)` | 对被修剪控制器的客体条目作失效处理（只关掉淘汰行为中较激进的那一半） |
| `configureClientWrapperTerminationDisabled(_:)` | 对包装对象调用 `+terminateForClient*`，但保留 dealloc 簿记 |
| `configureControllerDelayedDeallocationDisabled(_:)` | 整个延迟拆除 |

每一个 block 都是在「决策当下」求值、且从不缓存，所以对运行中的进程改动对应的 `_IMKSwift_disable…` 偏好设置，下一次就会生效。

十六种原始组合实际只坍缩成九种可分辨的行为，因为其中两个开关被另两个所支配。当用户回报「跑过截图工具之后，修饰键就切不动了」，你只要那几个日志词汇与四个开关的状态；日志会告诉你：事件从未送达、事件送给了已死的客体、还是事件送给了错的客体——这是三种不同的病，有三种不同的修法，而没有观测点时你一种都分不出来。

## 架构

### 结构

- **IMKSwift** — 主要的 Swift 库：协议定义、`@MainActor` 扩展、以及 Swift 端的会话管线
- **IMKSwiftModernHeaders** — 现代化的 Objective-C 头文件，以及 MRC（`-fno-objc-arc`）的私有面层：`IMKSwift.m`（剔除、延迟释放、客体连接终止之挂钩）与 `IMKControllerLifetimeTracker.m`（地址存活与世代簿记）
- **消费端** — 你的 `IMKInputSessionController` 子类别，以及能在地址翻动中存活的会话池

### 三层、两种语言

- **声明层。** `IMKInputSessionController` 是 `IMKInputController` 的具体子类别，存在目的只有一个：绕开 Swift 与 Objective-C 互通上的限制。原始 SDK 头文件暴露的 API 没有 `@MainActor` 隔离，又无法在不产生 API 冲突的前提下重新声明，因此一个改了名字的子类别是唯一可行的答案。它以 `#pragma clang attribute` 标上 `@MainActor`，并在 Swift 端以 `IMKInputSessionControllerProtocol` 镜像其完整接口。
- **修补层。** 缺陷住在 IMKServer 的私有状态里，所以修法只能是 Objective-C：以 KVC 取得 `_private._controllers`、带世代簿记的剔除、dealloc 时机的挂钩、以及对客体连接包装类别调用 `+terminateForClient*`。这些代码跑在你的输入法进程内，以 `-fno-objc-arc` 编译，好让控制器副本的 retain／release churn 不至于浮现为输入延迟。
- **策略层。** 哪一个控制器拿到哪一份会话、会话何时改绑客体、何时退役，都是你的输入法自己决定的事。IMKSwift 只提供地址、存活查询与挂钩点；它刻意不持有你的会话模型。

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

| 面向 | InputMethodKit | IMKSwift |
|---------|----------------|----------|
| **并发** | 无隔离 | 完整的 `@MainActor` 隔离 |
| **类型安全** | 裸露的 `id` 类型 | 具体、具名的类型 |
| **空值性** | 隐含 | 明确标注 |
| **Swift 支持** | 基本桥接 | 完整的 Swift 6 整合 |
| **Swift 6 的基类别** | `IMKInputController`（不可用） | `IMKInputSessionController`（可用） |
| **控制器生命周期** | 每切一次 CapsLock 就漏掉一个孤儿控制器；`sessionFinished:` 永远不来 | 地址索引追踪、KVC 剔除、延迟拆除、客体连接终止 |
| **客体绑定** | 隐含，且在代理翻动后失效 | 逐事件重建 |
| **可观测性** | 沉默 | 稳定的日志词汇，加四个仅供诊断的开关 |

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
