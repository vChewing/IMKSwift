# IMKSwift

為 Swift 6 及更新版本提供的現代化 **InputMethodKit** 套件，可在 macOS 上安全、並行且型別安全地建置輸入法引擎。

## 概述

IMKSwift 是 [唯音專案 (vChewing Project)](https://github.com/vChewing) 的一部分，為蘋果的 InputMethodKit 框架提供 Swift 原生的封裝層。它同時做兩件事。

其一，把 IMK 的 Objective-C 介面以 Swift 6 的語言重新宣告：具體型別取代 `id`、明確的可空性、每個入口都標上 `@MainActor`。其二，修補 IMKServer 自 10.5 以來就帶著的控制器生命週期缺陷——這類缺陷改幾次標頭都碰不到，因為它們住在 IMKServer 的私有狀態裡，不在宣告裡。

InputMethodKit 的歷史可追溯到 macOS 10.5 Leopard——早於 ARC、Sandbox 和 Swift 的問世。這是一個橫跨兩代技術變革的祖產級框架。IMKSwift 彌合這道鴻溝，讓現代 Swift 開發者無需與古老的 Objective-C 模式搏鬥，就能建置輸入法。

與其艱難地處理不安全的型別轉換、裸露的 `id` 型別和隱含的全域狀態，IMKSwift 提供：

- **明確的 `@MainActor` 隔離** 在每個 API 上
- **具體的、可空性標註的型別** 而非裸露的 `id` 指標
- **完整的 Swift 6 並行支援**
- **完整的 InputMethodKit 介面** 以及安全使用的改進
- **修補過的控制器生命週期**（以 `-fno-objc-arc` 編譯）：位址索引的存活追蹤、孤兒控制器剔除、延遲拆除、客體連線終止，以及四枚僅供診斷的 kill switch

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

### 控制器生命週期之修補

本套件另附一層 Objective-C（`IMKSwiftModernHeaders`，以 `-fno-objc-arc` 編譯），處理那些改標頭永遠碰不到的事：

- 每一枚存活的控制器都以原始位址登記於 `IMKControllerLifetimeTracker`；
- 孤兒控制器會被從 IMKServer 的私有 `_controllers` 字典中剔除，其持有的客體連線包裝物件亦一併終止；
- `deactivateServer:` 之後延遲三秒才拆除控制器，使頻繁的 CapsLock 切換不至於拆掉一條馬上就要被重用的連線；
- 四枚僅供診斷的開關，加一套穩定的日誌詞彙，使現場事故無需另出特製版即可二分定位。

### Swift 6 就緒
考慮到 Swift 6 的嚴格並行模型而建置。所有 API 都被適當隔離，可以在並行環境中使用而不會產生資料競爭。

## 系統需求

- **Swift** 6.2 或更新版本
- **Xcode** 16.0 或更新版本（現行的私有面標頭係以 26.x 驗證）
- **macOS**：
    - macOS 10.13 High Sierra 及更新版本（此為 Swift Concurrency 的執行期下限；實際下限取決於 Swift 版本）。
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

### 3. 不要在控制器中強持有物件，且客體綁定必須逐事件重建

兩條鐵律，都是踩出來的。

**其一：你的 `IMKInputSessionController` 子類別不應強持有業務邏輯物件。** 每一次 CapsLock 切換或客體切換，都可能產生一枚新的控制器副本；每切一次就拆掉一整張大型物件圖，那樣的 ARC churn 會直接反映成輸入延遲。

**其二：不要把「啟用當下記下的客體位址」當成不變的事實。** IMKServer 是以客體代理的位址為鍵，維護它私有的控制器表；客體脈絡一翻動，系統給你的就是一個新的代理。一個記著「我的客體是 0x…9d0」的會話，會繼續把字送給一枚早已退場的控制器。症狀很安靜：中英切換的提示照樣出現，按鍵卻從此不再送達。

請以控制器自身的位址為鍵、從一個小型的會話池派發，並在每一次事件上重新認領客體：

```swift
@MainActor
final class SessionPool {
  static let shared = SessionPool(capacity: 4)
  private var slots: [UInt: InputSession] = [:]

  func session(for ctlAddr: UInt) -> InputSession { /* ... */ }
  func reap(using tracker: IMKControllerLifetimeTracker) { /* 收掉控制器已死的槽位 */ }
}

// 於事件回呼中、派發之前：
session.adoptClientAddr(ctlAddr)  // 以送事件的人為準，而非啟用時的記錄
```

兩條推論值得寫進腦子裡：

- **不要等 `-dealloc`。** IMKServer 會讓控制器成為孤兒，卻從不釋放它；而 `sessionFinished:` 是按客體代理逐一派發的，永遠到不了那個孤兒身上。請以生命週期追蹤器查「該控制器位址是否仍存活」來回收會話。
- **不要在套件本體被替換之前就殺掉輸入法行程。** IMK 會依註冊的套件按需重啟輸入法；在安裝中途終止它，只會讓舊程式碼以新的 PID 回來。請在替換之後再抓取、終止、升級為 `SIGKILL`、並驗證（見 §7）。

「退場副本」的危險值得一句具體的說明：若兩枚控制器曾共用同一個會話槽，第一枚退場時該槽的客體綁定會被清空，而第二枚仍繼續透過它派發。這正是一個「以極性配對的雙緩衝」會出的毛病，也是本設計改以位址為鍵、並逐事件刷新綁定的理由。

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

### 7. 把 IMKServer 的私有面當成有觀測點的黑箱

你修不了 IMKServer，也沒法用斷點偵錯它。你能做的是：讓它的行為可被觀測，並且讓你自己每一項干涉都能單獨關掉。

IMKSwift 的 Objective-C 層因此只吐一套很小、很穩定的詞彙：

| 詞彙 | 含義 |
|---|---|
| `ControllerInit:` | 一枚控制器副本被建立，含其位址與世代 |
| `Prune:` | 檢查了私有的 `_controllers` 表，或剔除了一個孤兒 |
| `ActivateServer:` / `DeactivateServer:` | 客體的啟用與退場 |
| `DelayedDealloc:` | 延遲拆除被排定、跳過或執行 |
| `EventEntry:` | 按鍵事件被投遞到哪一枚控制器、其客體是否仍存活 |
| `guard fail:` | 事件已抵達輸入法，但沒有可用的客體代理 |

在此之上，四枚僅供診斷的開關（以類別方法安裝；唯音把它們接到「開發道場」面板）讓每一項干涉都能被單獨停用：

| 開關 | 停用什麼 |
|---|---|
| `configureServerControllerPruningDisabled(_:)` | KVC 剔除 |
| `configureEvictedClientInvalidationDisabled(_:)` | 對被剔除控制器的客體條目作失效處理（只關掉淘汰行為中較激進的那一半） |
| `configureClientWrapperTerminationDisabled(_:)` | 對包裝物件呼叫 `+terminateForClient*`，但保留 dealloc 簿記 |
| `configureControllerDelayedDeallocationDisabled(_:)` | 整個延遲拆除 |

每一個 block 都是在「決策當下」求值、且從不快取，所以對執行中的行程改動對應的 `_IMKSwift_disable…` 偏好設定，下一次就會生效。

十六種原始組合實際只坍縮成九種可分辨的行為，因為其中兩枚開關被另兩枚所支配。當使用者回報「跑過截圖工具之後，修飾鍵就切不動了」，你只要那幾個日誌詞彙與四枚開關的狀態；日誌會告訴你：事件從未送達、事件送給了已死的客體、還是事件送給了錯的客體——這是三種不同的病，有三種不同的修法，而沒有觀測點時你一種都分不出來。

## 架構

### 結構

- **IMKSwift** — 主要的 Swift 函式庫：協定定義、`@MainActor` 擴充、以及 Swift 端的會話管線
- **IMKSwiftModernHeaders** — 現代化的 Objective-C 標頭，以及 MRC（`-fno-objc-arc`）的私有面層：`IMKSwift.m`（剔除、延遲釋放、客體連線終止之掛鉤）與 `IMKControllerLifetimeTracker.m`（位址存活與世代簿記）
- **消費端** — 你的 `IMKInputSessionController` 子類別，以及能在位址翻動中存活的會話池

### 三層、兩種語言

- **宣告層。** `IMKInputSessionController` 是 `IMKInputController` 的具體子類別，存在目的只有一個：繞開 Swift 與 Objective-C 互通上的限制。原始 SDK 標頭曝露的 API 沒有 `@MainActor` 隔離，又無法在不產生 API 衝突的前提下重新宣告，因此一個改了名字的子類別是唯一可行的答案。它以 `#pragma clang attribute` 標上 `@MainActor`，並在 Swift 端以 `IMKInputSessionControllerProtocol` 鏡射其完整介面。
- **修補層。** 缺陷住在 IMKServer 的私有狀態裡，所以修法只能是 Objective-C：以 KVC 取得 `_private._controllers`、帶世代簿記的剔除、dealloc 時機的掛鉤、以及對客體連線包裝類別呼叫 `+terminateForClient*`。這些程式碼跑在你的輸入法行程內，以 `-fno-objc-arc` 編譯，好讓控制器副本的 retain／release churn 不至於浮現為輸入延遲。
- **政策層。** 哪一枚控制器拿到哪一份會話、會話何時改綁客體、何時退場，都是你的輸入法自己決定的事。IMKSwift 只提供位址、存活查詢與掛鉤點；它刻意不持有你的會話模型。

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

| 面向 | InputMethodKit | IMKSwift |
|---------|----------------|----------|
| **並行** | 無隔離 | 完整的 `@MainActor` 隔離 |
| **型別安全** | 裸露的 `id` 型別 | 具體、具名的型別 |
| **可空性** | 隱含 | 明確標註 |
| **Swift 支援** | 基本橋接 | 完整的 Swift 6 整合 |
| **Swift 6 的基底類別** | `IMKInputController`（不可用） | `IMKInputSessionController`（可用） |
| **控制器生命週期** | 每切一次 CapsLock 就漏掉一枚孤兒控制器；`sessionFinished:` 永遠不來 | 位址索引追蹤、KVC 剔除、延遲拆除、客體連線終止 |
| **客體綁定** | 隱含，且在代理翻動後失效 | 逐事件重建 |
| **可觀測性** | 沉默 | 穩定的日誌詞彙，加四枚僅供診斷的開關 |

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
