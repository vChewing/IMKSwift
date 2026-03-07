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

**在 Swift 6 中你根本無法直接使用 `IMKInputController`。** 即使你嘗試直接覆寫標頭，Swift 仍然會讀取原始的 Xcode SDK 標頭，導致 API 衝突。唯一的解決方案是使用**不同名稱的子類別**——`IMKInputSessionController`——它繼承了所有功能，同時提供正確標註 `@MainActor` 的 API。

> ⚠️ **重要：** 如果你在 Swift 6 中直接子類別化 `IMKInputController`，你將面臨編譯錯誤，而這些錯誤只能透過丑陋的指標操作來「修正」，強行將物件塞到 `@MainActor` 上。別這麼做。請使用 `IMKInputSessionController`。

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
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.07"),
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

### 基本輸入法控制器設置

```swift
import IMKSwift

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  override public func handle(_ event: NSEvent?, client sender: any IMKTextInput) -> Bool {
    // 具有完整型別安全的事件處理
    guard let event else { return false }
    
    // 處理輸入...
    return true
  }
  
  override func inputText(_ string: String, client sender: any IMKTextInput) -> Bool {
    // 文字輸入處理
    return true
  }
  
  override func candidates(_ sender: any IMKTextInput) -> [Any]? {
    // 返回候選字詞建議
    return nil
  }
}
```

### 使用選字面板

```swift
// 建立並顯示選字面板
let candidates = IMKCandidates(
  server: server,
  panelType: .horizontal
)

candidates.show(.below)
```

### 組字管理

```swift
// 更新組字
updateComposition()

// 存取選擇和替換範圍
let selRange = selectionRange()
let replaceRange = replacementRange()

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

### 3. 不要在控制器中強持有物件

**你的 `IMKInputSessionController` 子類別不應直接持有業務邏輯物件。** 這對於處理高頻率輸入法切換（例如使用 CapsLock 在中英輸入法之間切換）至關重要。

當使用者頻繁切換輸入法時，系統每次都會建立新的 `IMKInputController` 副本。如果你的控制器持有對大型物件的強參照，ARC 清理會造成明顯的卡頓。

**建議模式：** 使用以客戶端物件為鍵的弱鍵快取（`NSMapTable`）：

```swift
import IMKSwift

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  // 使用對會話的弱參照
  weak var session: InputSession?
  
  override public init(server: IMKServer, delegate: Any?, client inputClient: any IMKTextInput) {
    super.init(server: server, delegate: delegate, client: inputClient)
    
    // 初始化或取得快取的會話
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
