# IMKSwift

為 Swift 6 及更新版本提供的現代化 **InputMethodKit** 套件，可在 macOS 上安全、並行且型別安全地建置輸入法引擎。

## 概述

IMKSwift 是 [唯音專案 (vChewing Project)](https://github.com/vChewing) 的一部分，提供了蘋果 InputMethodKit 框架的 Swift 原生替代品。它結合了 Objective-C 互操作性和 Swift 6 的嚴格並行模型，提供了 `@MainActor` 隔離的 API，在現代 Swift 程式碼中更容易使用。

與其艱難地處理不安全的型別轉換、裸露的 `id` 型別和隱含的全域狀態相比，IMKSwift 提供：

- **明確的 `@MainActor` 隔離** 在每個 API 上
- **具體的、可空性標註的型別** 而非裸露的 `id` 指標
- **完整的 Swift 6 並行支援**
- **完整的 InputMethodKit 介面** 以及安全使用的改進

## 功能特性

### 型別安全與可空性
所有 API 都包含明確的可空性標註（`_Nullable`、`_Nonnull`）並使用具體的 Objective-C 型別（`NSString`、`NSAttributedString`、`NSDictionary`、`NSArray`、`NSEvent` 等），而非泛用的 `id`。

### MainActor 隔離
每個方法和屬性都標記為 `@MainActor`，確保了編譯時的呼叫站點安全性，並防止並行程式碼中的資料競爭。

### 完整的 InputMethodKit 覆蓋
IMKSwift 重新匯出並增強了以下 InputMethodKit 元件：

- **IMKCandidates** — 選字面板管理和顯示
- **IMKServer** — 輸入法會話伺服器
- **IMKInputController** — 輸入法事件處理與組字 (Composition)
- **IMKTextInput** — 文字編輯用戶端協定
- **輔助協定** — IMKStateSetting、IMKMouseHandling、IMKServerInput

### Swift 6 就緒
考慮到 Swift 6 的嚴格並行模型而建置。所有 API 都被適當隔離，可以在並行環境中使用而不會產生資料競爭。

## 系統需求

- **Swift** 6.2 或更新版本
- **macOS** 取決於您在使用的 Swift 版本
- **Xcode** 16.0 或更新版本

## 安裝

### Swift Package Manager

在你的 `Package.swift` 中新增 IMKSwift：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.05"),
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

class MyInputMethodController: IMKInputController {
  override func handle(_ event: NSEvent?, client sender: any IMKTextInput) -> Bool {
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
  panelType: .horizontal,
  styleType: .default
)

candidates.show(.below)
```

### 組字管理

```swift
// 使用屬性更新組字
updateComposition()

// 存取選擇和替換範圍
let selRange = selectionRange()
let replaceRange = replacementRange()

// 確認組字
commitComposition(client)
```

## 架構

### 結構

- **IMKSwift** — 主要 Swift 函式庫，包含協定定義與擴充
- **IMKSwiftModernHeaders** — 現代化的 Objective-C 標頭，具有 `@MainActor` 標註和型別改進

### 擴充模式

所有 InputMethodKit 類別都透過 Objective-C 中的類別進行擴充，並透過 Swift 中的協定遵循重新匯出。這保持了完整的 API 相容性，同時增加了型別安全和並行保障。

## API 文件

### 核心協定

#### IMKInputSessionControllerProtocol

主要執行緒隔離的超級協定 (Super-protocol)，涵蓋：
- `IMKStateSetting` — 啟用、停用、偏好設定
- `IMKInputController` — 事件處理、組字、選字管理
- `IMKMouseHandling` — 滑鼠事件處理
- `IMKServerInput` — 文字和事件輸入

下游代碼應該子類別化 `IMKInputSessionController`（而非僅 `IMKInputController`）來套用這些改進。

### 關鍵方法

**狀態管理：**
- `activateServer(_:)` — 啟用輸入法
- `deactivateServer(_:)` — 停用輸入法
- `showPreferences(_:)` — 顯示偏好設定對話框

**文字與組字：**
- `inputText(_:key:modifiers:client:)` — 處理鍵盤輸入
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
| **選字面板** | 手動設置 | 簡化的 API |

## 相關專案

此程式庫是 [唯音專案 (vChewing Project)](https://github.com/vChewing) 的一部分，該專案是 macOS 的現代、開源輸入法框架。

## 授權

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

詳見 [LICENSE](LICENSE)。
