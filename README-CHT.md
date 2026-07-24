# IMKSwift

為 Swift 6 及更新版本提供的現代化 **InputMethodKit** 套件，可在 macOS 上安全、並行且型別安全地建置輸入法引擎。

## 概述

IMKSwift 是 [唯音專案 (vChewing Project)](https://github.com/vChewing) 的一部分，提供了蘋果 InputMethodKit 框架的 Swift 原生替代品。它結合了 Objective-C 互操作性和 Swift 6 的嚴格並行模型，提供了 `@MainActor` 隔離的 API，在現代 Swift 程式碼中更容易使用。

InputMethodKit 的歷史可追溯到 macOS 10.5 Leopard——早於 ARC、Sandbox 和 Swift 的問世。這是一個橫跨兩代技術變革的祖產級框架。IMKSwift 彌合了這道鴻溝，讓現代 Swift 開發者無需與古老的 Objective-C 模式搏鬥，就能建置輸入法。

與其艱難地處理不安全的型別轉換、裸露的 `id` 型別和隱含的全域狀態相比，IMKSwift 提供：

- **明確的 `@MainActor` 隔離** 在每個 API 上
- **具體的、可空性標註的型別** 而非裸露的 `id` 指標
- **完整的 Swift 6 並行支援**
- **完整的 InputMethodKit 介面** 以及安全使用的改進

## 為什麼使用 IMKInputSessionController？

蘋果的 `IMKInputController` 設計上是在 MainActor 上運行的，但其 Objective-C 標頭缺乏適當的 `@MainActor` 標註。當匯入 Swift 時，這會產生根本性的問題：原始 SDK 標頭曝露的 API 沒有執行緒隔離，導致在 Swift 6 嚴格模式下出現**無法解決的並行檢查錯誤**。

**在 Swift 6 中你根本無法直接使用 `IMKInputController`。** 即使你嘗試直接覆寫標頭，Swift 仍然會讀取原始的 Xcode SDK 標頭，導致 API 衝突。`IMKInputSessionController` 提供了乾淨、經 `@MainActor` 隔離的 API 表面。

> ⚠️ **禁止 subclass `IMKInputSessionController`。** 從 v26.07.24 起，IMKSwift 採用**基於注入（Injection）的架構**。subclass 會將 controller 的生命週期（`alloc` / `dealloc` / `retain` / `release`）曝露給 Swift ARC，重新引入導致 CapsLock 切換卡頓的 MainActor 競爭條件。取而代之的是，所有 IMK 回呼均由 class-level static blocks 在 `applicationDidFinishLaunching` 中注入。Controller 生命週期完全留在 ObjC MRC（`-fno-objc-arc`）內，Swift 僅透過 raw `uintptr_t` 位址與 controller 互動——絕不經過 ARC 管理的參照。

## 功能特性

### 型別安全與可空性
所有 API 都包含明確的可空性標註（`_Nullable`、`Nonnull`）並使用具體的 Objective-C 型別（`NSString`、`NSAttributedString`、`NSDictionary`、`NSArray`、`NSEvent` 等），而非泛用的 `id`。

### MainActor 隔離
每個方法和屬性都標記為 `@MainActor`，確保了編譯時的呼叫站點安全性，並防止並行程式碼中的資料競爭。

### 完整的 InputMethodKit 覆蓋
IMKSwift 重新匯出並增強了以下 InputMethodKit 元件：

- **IMKCandidates** — 選字面板管理和顯示
- **IMKServer** — 輸入法會話伺服器
- **IMKInputSessionController** — 輸入法事件處理與組字（Swift 6+ **唯一**建議使用的基礎類別）
- **IMKTextInput** — 文字編輯用戶端協定
- **輔助協定** — IMKStateSetting、IMKMouseHandling、IMKServerInput

### Swift 6 就緒
考慮到 Swift 6 的嚴格並行模型而建置。所有 API 都被適當隔離，可以在並行環境中使用而不會產生資料競爭。

## 系統需求

- **Swift** 6.2 或更新版本
- **Xcode** 16.0 或更新版本
- **macOS**：
    - macOS 10.13 High Sierra 及更新版本（取決於使用的 Swift 版本）。
    - 程式碼本身可在 macOS 10.09 Mavericks 上執行，但需要對應的 macOS SDK 與 libARCLite。

## 安裝

### Swift Package Manager

在你的 `Package.swift` 中新增 IMKSwift：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.07.24"),
```

然後將其新增為目標的依賴：

```swift
.target(
  name: "MyInputMethod",
  dependencies: [
    .product(name: "IMKSwift", package: "IMKSwift"),
  ]
)
```

## 使用方式

### 部署架構總覽

IMKSwift v26.07.24 起採用**基於注入（Injection）的架構**，不再需要 subclass `IMKInputSessionController`。所有 IMK 回呼均透過 **class-level static blocks** 注入——在啟動時一次性設定，由所有 controller 實例共享。每個 block 接收 raw `uintptr_t` 記憶體位址而非物件參照——不執行 retain/release，徹底避開 ARC 開銷。

**Info.plist 配置：** 直接將 controller class 宣告為 `IMKInputSessionController`（無需自訂子類別）。以下 keys 必須全部指向 `IMKInputSessionController`，以確保 controller 生命週期絕不接觸 Swift ARC：

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

**編譯設定：** 必須對 `IMKSwiftModernHeaders` 以 `-fno-objc-arc` 編譯：

```swift
// Package.swift
.target(
  name: "IMKSwiftModernHeaders",
  cSettings: [.unsafeFlags(["-fno-objc-arc"])]
),
```

**啟動注入：** 所有 configure blocks 必須在 `applicationDidFinishLaunching` 內、**於**首個 `IMKServer` 初始化**之前**設定完成。這些 blocks 是 class-level static——設定一次，永久共享。

### 可用的 Configure Blocks

| Block | 用途 | 簽名 |
|-------|------|------|
| `configureActivatingServer` | Controller 被啟用 | `(clientAddr, selfAddr) -> Void` |
| `configureDeactivatingServer` | Controller 被停用 | `(clientAddr, selfAddr) -> Void` |
| `configureDealloc` | Controller 即將釋放 | `(selfAddr) -> Void` |
| `configureShowingPreferences` | 顯示偏好設定 | `(clientAddr, selfAddr) -> Void` |
| `configureHidingPallettes` | 隱藏 UI 面板 | `(selfAddr) -> Void` |
| `configureInputControllerWillClose` | Controller 即將關閉 | `(selfAddr) -> Void` |
| `configureProvidingSelectionRange` | 提供選取範圍 | `(selfAddr) -> NSRange` |
| `configureProvidingIMEMenu` | 構建 IME 選單 | `(selfAddr) -> NSMenu?` |
| `configureProvidingComposedString` | 提供組字內容 | `(clientAddr, selfAddr) -> Any?` |
| `configureAutoCommittingComposition` | 自動遞交組字 | `(clientAddr, selfAddr) -> Void` |
| `configureProvidingRecognizedEvents` | 已辨識的事件遮罩 | `(clientAddr, selfAddr) -> NSUInteger` |
| `configureHandlingGivenNullableEvent` | 處理 NSEvent | `(nsEventPtr, clientAddr, selfAddr) -> Bool` |
| `configureSettingObjCValue` | 處理 `setValue:` | `(valuePtr, tag, clientAddr, selfAddr) -> Void` |

> **注意：** 所有位址均為 `uintptr_t`。使用 `UnsafeRawPointer(bitPattern:)` + `Unmanaged<...>.fromOpaque(...).takeUnretainedValue()` 在 Swift 端恢復物件。每次 `takeUnretainedValue()` 前應以 `IMKControllerLifetimeTracker` 複查 controller 是否仍存活。

### 基本注入範例

```swift
import IMKSwift

// ── applicationDidFinishLaunching: 在 IMKServer init 之前一次性注入 ──

IMKInputSessionController.configureActivatingServer { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.activate()
}

IMKInputSessionController.configureDeactivatingServer { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.deactivate()
}

IMKInputSessionController.configureDealloc { selfAddr in
  // 延遲釋放 + XPC 清理 + session 對照表拆除
  InputSession.cleanupController(addr: selfAddr)
}

IMKInputSessionController.configureHandlingGivenNullableEvent { nsEventPtr, clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return false }
  guard let rawPtr = UnsafeRawPointer(bitPattern: nsEventPtr) else { return false }
  let event = Unmanaged<NSEvent>.fromOpaque(rawPtr).takeUnretainedValue()
  return session.handle(event: event)
}

IMKInputSessionController.configureShowingPreferences { clientAddr, selfAddr in
  InputSession.showPreferences()
}

IMKInputSessionController.configureProvidingIMEMenu { selfAddr in
  return InputSession.buildMenu(for: selfAddr)
}

IMKInputSessionController.configureProvidingComposedString { clientAddr, selfAddr in
  return InputSession.session(for: selfAddr)?.currentComposedString
}

// ── 業務邏輯（純 Swift，不繼承任何 IMK 型別）──

@MainActor
public final class InputSession {
  /// 極性雙緩衝池：兩枚 singleton，以 generation parity 路由。
  fileprivate static let sessionEven = InputSession(preallocated: ())
  fileprivate static let sessionOdd  = InputSession(preallocated: ())

  /// 依 controller 的 generation parity 解析對應 session。
  static func session(for ctlAddr: UInt) -> InputSession? {
    let tracker = IMKControllerLifetimeTracker.shared()
    guard tracker.isAddressAlive(ctlAddr) else { return nil }
    let parity = tracker.generationForAddress(ctlAddr) % 2
    return parity == 0 ? sessionEven : sessionOdd
  }

  static func cleanupController(addr: UInt) { /* 3 秒延遲釋放 + XPC 清理 */ }

  func handle(event: NSEvent) -> Bool { return true }

  var currentComposedString: Any? { nil }
  static func showPreferences() {}
  static func buildMenu(for addr: UInt) -> NSMenu? { nil }
  func activate() {}
  func deactivate() {}
}
```

### 使用選字面板

```swift
let candidates = IMKCandidates(server: server, panelType: .horizontal)
candidates.show(.below)
```

### 組字管理

```swift
updateComposition()
let selRange = selectionRange()
let replaceRange = replacementRange()
commitComposition(sender)
```

### 部署核對清單

釋出前請確認以下所有項目均已處理：

- [ ] **Info.plist:** `InputMethodServerControllerClass` = `IMKInputSessionController`（非自訂子類別）。
- [ ] **Package.swift:** `IMKSwiftModernHeaders` target 設有 `cSettings: [.unsafeFlags(["-fno-objc-arc"])]`。
- [ ] **注入時機：** 所有 `configure*` blocks 在 `applicationDidFinishLaunching` 內、**於** `IMKServer.init(name:bundleIdentifier:)` **之前**設定完成。
- [ ] **KVC prune 路徑：** 維護單調遞增的 controller generation 計數器；當追蹤的 controller 數量超過 2 時，透過 `[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]` 移除最舊的非活躍 controller。
- [ ] **Session 路由：** 以 `generation % 2` parity 路由 controller 至 session（而非以 client 位址為鍵），使用恰好兩枚預先配置的 static singleton。
- [ ] **延遲釋放：** `deactivateServer:` 後排程 3 秒計時器。觸發時：釋放持有的 blocks、呼叫 client wrapper 的 `+terminateForClient*` private class method、清除 controller↔session 對照表。若 controller 在時窗內被重新啟用則取消計時器。
- [ ] **XPC 清理：** 在 dealloc 時呼叫 client wrapper class 的 `+terminateForClientXPCConn:` / `+terminateForClientDOProxy:`。使用 `respondsToSelector:` 依序嘗試：`_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper`。（macOS 15 Sequoia 起將 wrapper class 拆分；macOS ≤ 10.15 使用無後綴名稱。）
- [ ] **Dangling pointer 防護：** 每次對 raw controller 位址呼叫 `takeUnretainedValue()` 前，先以 `IMKControllerLifetimeTracker.shared().isAddressAlive(addr)` 確認其仍存活。IMKSwift 隨附此 tracker——它會自動追蹤每個透過 `IMKInputSessionController` 分配的 controller，並在 dealloc 時透過 associated-object sentinel 自動取消註冊。使用 `generationForAddress(_:)` 進行 parity-based session 路由。
- [ ] **IME 選單注入：** 使用 `class_addMethod` + `imp_implementationWithBlock` 在執行期將選單動作註冊至 `IMKInputSessionController`。無需 `@objc` Selector 暴露。
- [ ] **記憶體指標：** 透過 `task_vm_info.internal` 讀取 anonymous private memory（勿用 `phys_footprint`——後者在 Apple Silicon 上會計入 GPU/neural engine 共享記憶體）。讀取前呼叫 `purgeMallocZones()` 以排乾 allocator cache。

// 確認組字
commitComposition(sender)
```

## 最佳實踐

### 1. NSConnection 名稱規範

輸入法的 `Info.plist` 中的 `InputMethodConnectionName` 欄位**必須**設定為：

```
$(PRODUCT_BUNDLE_IDENTIFIER)_Connection
```

> ⚠️ 此命名規範自 macOS 10.7 Lion 起為強制要求。若不遵守，開啟 Sandbox 後輸入法將無法正常載入。你會在 `Console.app` 中看到 NSConnection 相關的錯誤訊息。

### 2. 開啟 App Sandbox

務必開啟 App Sandbox。鑑於你被迫使用脆弱的 NSConnection 機制，不開啟 Sandbox 的話就連 Apple 都根本沒有任何手段相信你的輸入法是安全的。對 Sandbox 的啟用，就是最佳的資訊安全投名狀。

建議的 `entitlements` 檔案內容：

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

### 3. 使用 MRC 與極性雙緩衝 Session 池

**將你的 IMKInputSessionController 子類別以 `-fno-objc-arc` 編譯。** ARC 在 MainActor 上的 retain/release 高頻開銷於 CapsLock 切換時導致可感知的卡頓。停用 ARC 可徹底消除卡頓，但需自行處理清理。

對 IMKServer 在 macOS 10.9–27 的逆向工程揭露了一個根本缺陷：`_private._controllers` 以 client proxy 位址為鍵，而 CapsLock 切換會建立**全新**的 proxy（不同位址）。舊 controller 成為永久孤棄——`sessionFinished:` 永遠不會對它們觸發，因為 IMK 只向當前（新）proxy 派送訊息。每次 CapsLock 切換都洩漏一個 IMKInputController 及其 NSXPCConnection。

**建議架構（唯音輸入法「Phase 115」）：**

- **以 KVC 清理孤棄 controller：** 在 init 時分配單調遞增的 generation number。當 `_controllers` 超過 2 個條目時，透過 KVC（`[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]`）移除最舊的非活躍 controller。

- **極性雙緩衝 Session 池：** 以 `generation % 2` 將 controller 路由至兩枚 static `InputSession` singleton（偶數→session A，奇數→session B）。完全消除 LRU 快取、動態分配與淘汰邏輯。

- **延遲釋放與 XPC 清理：** `deactivateServer:` 後排程 3 秒計時器。觸發時：釋放 blocks、呼叫 client wrapper 的 private `+terminateForClient*`（依序嘗試 `_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper` 以相容 macOS 27）、清除對照表。若重新啟用則取消。

- **IME 選單注入：** 使用 `class_addMethod` + `imp_implementationWithBlock` 動態構建 IME 選單，無需 `@objc` Selector 暴露。注入即時指標：controller 存活數量與 anonymous private memory（`task_vm_info.internal`），讀取前 purge malloc zones。

```swift
// IMKInputSessionController（.m 檔案，以 -fno-objc-arc 編譯）：
// Class-level static blocks 透過 raw uintptr_t 位址向 Swift 分派。
// +IMKSwift_configureWithActivatingServer: 等在啟動時一次性設定。

// Swift 端——極性路由：
public final class InputSession {
  fileprivate static let sessionEven = InputSession(preallocated: ())
  fileprivate static let sessionOdd  = InputSession(preallocated: ())

  static func session(for ctlAddr: UInt) -> InputSession? {
    let parity = IMKControllerLifetimeTracker.shared().generation(for: ctlAddr) % 2
    return parity == 0 ? sessionEven : sessionOdd
  }
}
```

> **注意：** 極性池刻意不以 client 位址為鍵。它接受 CapsLock 會產生不同位址的新 proxy 這一現實，改以穩定的 controller generation 計數器作為路由鍵。

### 4. 以 Swift Package 函式庫形式組織程式碼

macOS 輸入法無法使用斷點偵錯——這會凍結客戶端應用程式和你的整個桌面。唯一可行的方法是使用模擬客戶端進行單元測試。

將輸入法組織為 Swift Package：
- **核心函式庫** — 業務邏輯，可透過單元測試測試
- **輸入法目標** — 連接 IMKSwift 與核心函式庫的薄包裝層

這也允許你編寫標準的 AppKit 應用程式來模擬打字過程，並使用 Instruments 偵測記憶體洩漏。

### 5. 避免使用 IMKCandidates

系統提供的 `IMKCandidates` 有嚴重問題，尤其是在 macOS 26 上，LiquidGlass 渲染導致視覺故障（透明背景上的白色文字）。**考慮使用 SwiftUI 或 AppKit 實作你自己的選字窗**。

> 連 Apple 自己的 NumberInput 範例都避免使用 `IMKCandidates`。

### 6. 記憶體管理

使用者的記憶體空間寸土寸金。雖然 macOS 26 的 AppKit 低效可能導致輸入法佔用 80–200 MB：

- **在 `activateServer()` 中監控記憶體使用量**，若超過 1024 MB 則自我終止（透過 `NSNotification` 通知使用者）
- **最小化 NSWindow 數量** — 從 macOS 26 開始，NSWindow 的記憶體永遠不會被回收。盡可能將面板（工具提示、選字窗等）整合到單一 `NSPanel` 中

## 架構

### 結構

- **IMKSwift** — 主要 Swift 函式庫，包含協定定義與擴充
- **IMKSwiftModernHeaders** — 現代化的 Objective-C 標頭，具有 `@MainActor` 標註和型別改進

### IMKInputSessionController 解決方案

`IMKInputSessionController` 是 `IMKInputController` 的具體子類別，它的存在完全是為了解決 Swift/Objective-C 互操作性限制。

**問題所在：**
- `IMKInputController` 的原始 SDK 標頭曝露的 API 沒有 `@MainActor` 隔離
- Swift 將這些匯入為非隔離的，導致嚴格的並行錯誤
- 你無法為現有類別覆寫標頭而不造成 API 衝突

**解決方案：**
- 建立一個新的子類別（`IMKInputSessionController`）具有相同的 API 介面
- 透過 Objective-C 中的 `#pragma clang attribute` 套用 `@MainActor` 隔離
- Swift 看到一個全新的類別，其方法具有正確的隔離

**你會得到什麼：**
- 所有方法都具有 `@MainActor` 隔離
- 明確的可空性標註（`Nonnull` / `Nullable`）
- 使用具體的 Objective-C 型別而非裸露的 `id`

在配套的 Swift 模組中，`IMKInputSessionController` 被擴充以遵循 `IMKInputSessionControllerProtocol`，這是一個 Swift 原生的 `@MainActor` 協定，鏡像了其完整的 API 介面。

## API 文件

### 核心協定

#### IMKInputSessionControllerProtocol

主執行緒隔離的協定，涵蓋：
- `IMKStateSetting` — 啟用、停用、偏好設定
- `IMKMouseHandling` — 滑鼠事件處理
- `IMKServerInput` — 文字和事件輸入
- 組字管理 — `updateComposition()`、`commitComposition(_:)` 等

### 關鍵方法

**狀態管理：**
- `activateServer(_:)` — 啟用輸入法
- `deactivateServer(_:)` — 停用輸入法
- `showPreferences(_:)` — 顯示偏好設定對話框

**文字與組字：**
- `inputText(_:key:modifiers:client:)` — 處理鍵盤輸入（詳細參數）
- `inputText(_:client:)` — 處理鍵盤輸入（簡化版）
- `handle(_:client:)` — 處理原始 `NSEvent`
- `updateComposition()` — 更新目前組字
- `cancelComposition()` — 取消進行中的組字
- `commitComposition(_:)` — 完成組字

**選字管理：**
- `candidates(_:)` — 提供候選字詞建議
- `candidateSelected(_:)` — 處理選字
- `candidateSelectionChanged(_:)` — 處理選擇變化

**滑鼠處理：**
- `mouseDown(onCharacterIndex:coordinate:withModifier:continueTracking:client:)`
- `mouseUp(onCharacterIndex:coordinate:withModifier:client:)`
- `mouseMoved(onCharacterIndex:coordinate:withModifier:client:)`

## 與原始 InputMethodKit 的差異

| 功能 | InputMethodKit | IMKSwift |
|------|----------------|----------|
| **並行** | 無隔離 | 完整 `@MainActor` 隔離 |
| **型別安全** | 裸露 `id` 型別 | 具體的命名型別 |
| **可空性** | 隱含 | 明確標註 |
| **Swift 支援** | 基本橋接 | 完整 Swift 6 整合 |
| **Swift 6 基礎類別** | `IMKInputController`（損壞） | `IMKInputSessionController`（正常） |

## 相關專案

此程式庫是 [唯音專案 (vChewing Project)](https://github.com/vChewing) 的一部分。該專案是 macOS 平台打字速度最快的音韻類中文輸入法，基於大千聲韻並擊打字原理與 DAG-DP 組句技術，同時具備原生的簡體中文與繁體中文打字能力。

唯音專案推出此套件餽贈社會，也希望能得到一些捐助。詳細資訊可洽[唯音輸入法的軟體主頁](https://vchewing.github.io/README.html)。

## 授權

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

詳見 [LICENSE](LICENSE)。
