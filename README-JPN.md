# IMKSwift

Swift 6 以降向けにモダナイズされた **InputMethodKit** オーバーレイ。macOS 上で安全、並行処理対応、かつ型安全な入力メソッドエンジンを構築できます。

## 概要

IMKSwift は [vChewing プロジェクト](https://github.com/vChewing) の一部であり、Apple の InputMethodKit フレームワークに対する Swift ネイティブな代替実装を提供します。Objective-C の相互運用性と Swift 6 の厳密な並行処理モデルを組み合わせ、モダンな Swift コードでより扱いやすい `@MainActor` 分離 API を提供します。

InputMethodKit は macOS 10.5 Leopard 時代に登場し、ARC、Sandbox、Swift の登場よりも前に生まれました。つまり、これは2世代の技術パラダイムシフトをまたいだレガシーフレームワークです。IMKSwift はこの溝を埋め、現代の Swift 開発者が古い Objective-C パターンと格闘することなく、入力メソッドを構築できるようにします。

安全でない型キャスト、生の `id` 型、そして暗黙的なグローバル状態の処理に苦労する代わりに、IMKSwift は以下を提供します：

- **すべての API に対する明示的な `@MainActor` 分離**
- **生の `id` ポインタの代わりとなる、具体的で Null 許容性が明示された型**
- **完全な Swift 6 並行処理サポート**
- **完全な InputMethodKit サーフェス** と安全な使用法の改善

## なぜ IMKInputSessionController を使うのか？

Apple の `IMKInputController` は MainActor（メインアクター）上で実行されるように設計されていますが、その Objective-C ヘッダーには適切な `@MainActor` 注釈がありません。Swift にインポートすると、根本的な問題が発生します：元の SDK ヘッダーは分離されていない API を公開し、Swift 6 の厳格モードで**解決不可能な並行性チェックエラー**を引き起こします。

**Swift 6 では `IMKInputController` を直接使用することはできません。** ヘッダーを直接オーバーライドしようとしても、Swift は元の Xcode SDK ヘッダーを引き続き読み込み、API 衝突が発生します。`IMKInputSessionController` は、クリーンで `@MainActor` 分離された API サーフェスを提供します。

> ⚠️ **`IMKInputSessionController` をサブクラス化しないでください。** v26.07.24 以降、IMKSwift は**注入（Injection）ベースのアーキテクチャ**を採用しています。サブクラス化すると、controller のライフサイクル（`alloc` / `dealloc` / `retain` / `release`）が Swift ARC に公開され、CapsLock 切り替え時の遅延を引き起こす MainActor の競合を再導入します。代わりに、すべての IMK コールバックは `applicationDidFinishLaunching` で class-level static blocks を介して注入されます。Controller のライフサイクルは完全に ObjC MRC（`-fno-objc-arc`）内に留まり、Swift は raw `uintptr_t` アドレスを通じてのみ controller と対話します——決して ARC 管理の参照を経由しません。

## 機能

### 型安全性とNull許容性
すべての API には明示的な Null 許容性注釈（`_Nullable`、`_Nonnull`）が含まれており、一般的な `id` の代わりに具体的な Objective-C 型（`NSString`、`NSAttributedString`、`NSDictionary`、`NSArray`、`NSEvent` など）を使用します。

### MainActor 分離
すべてのメソッドとプロパティは `@MainActor` でマークされており、コンパイル時の呼び出し元 (Call-site) の安全性を保証し、並行処理コードでのデータ競合 (データレース) を防止します。

### 完全な InputMethodKit カバレッジ
IMKSwift は以下の InputMethodKit コンポーネントを再エクスポートして強化します：

- **IMKCandidates** — 候補パネルの管理と表示
- **IMKServer** — 入力メソッドセッションサーバー
- **IMKInputSessionController** — 入力メソッドのイベント処理とコンポジション（Swift 6+ で**唯一**推奨される基底クラス）
- **IMKTextInput** — テキスト編集クライアントプロトコル
- **サポートプロトコル** — IMKStateSetting、IMKMouseHandling、IMKServerInput

### Swift 6 対応
Swift 6 の厳密な並行処理モデルを念頭に置いて設計されています。すべての API は適切に分離されており、並行コンテキスト環境においてデータ競合なしに使用できます。

## 要件

- **Swift** 6.2 以降
- **Xcode** 16.0 以降
- **macOS**:
    - macOS 10.13 High Sierra とそれ以降（ご使用の Swift のバージョン次第）。
    - コード自身は macOS 10.09 Mavericks で実行可であるが、それに応ずる macOS SDK と libARCLite が必要である。

## インストール

### Swift Package Manager

`Package.swift` に IMKSwift を追加します：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.07.24"),
```

その後、ターゲットの依存関係として追加します：

```swift
.target(
  name: "MyInputMethod",
  dependencies: [
    .product(name: "IMKSwift", package: "IMKSwift"),
  ]
)
```

## 使い方

### デプロイアーキテクチャの概要

IMKSwift v26.07.24 以降は**注入（Injection）ベースのアーキテクチャ**を採用しており、`IMKInputSessionController` をサブクラス化する必要はありません。すべての IMK コールバックは class-level static blocks を介して注入されます——起動時に一度だけ設定され、すべての controller インスタンスで共有されます。

**Info.plist 設定:** controller class を `IMKInputSessionController` として直接宣言します（カスタムサブクラス不要）。以下のキーはすべて `IMKInputSessionController` を指す必要があります——controller のライフサイクルが Swift ARC に一切触れないようにするためです：

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

**ビルド設定:** `IMKSwiftModernHeaders` ターゲットに `-fno-objc-arc` を指定します：

```swift
// Package.swift
.target(
  name: "IMKSwiftModernHeaders",
  cSettings: [.unsafeFlags(["-fno-objc-arc"])]
),
```

**起動時注入:** すべての IMK コールバックは `applicationDidFinishLaunching` で一度だけ注入されます。各 block は raw `uintptr_t` アドレスを受け取ります（ObjC オブジェクト参照ではなく）—— retain/release は実行されず、ARC のオーバーヘッドを完全に回避します。Swift 側は raw アドレスを介して controller に対応する session を検索し、ビジネスロジックにルーティングします。

### 利用可能な Configure Blocks

| Block | 目的 | シグネチャ |
|-------|------|-----------|
| `configureActivatingServer` | Controller 有効化 | `(clientAddr, selfAddr) -> Void` |
| `configureDeactivatingServer` | Controller 無効化 | `(clientAddr, selfAddr) -> Void` |
| `configureDealloc` | Controller 解放直前 | `(selfAddr) -> Void` |
| `configureShowingPreferences` | 環境設定を表示 | `(clientAddr, selfAddr) -> Void` |
| `configureHidingPallettes` | UI パネルを非表示 | `(selfAddr) -> Void` |
| `configureInputControllerWillClose` | Controller 終了直前 | `(selfAddr) -> Void` |
| `configureProvidingSelectionRange` | 選択範囲を提供 | `(selfAddr) -> NSRange` |
| `configureProvidingIMEMenu` | IME メニュー構築 | `(selfAddr) -> NSMenu?` |
| `configureProvidingComposedString` | 未確定文字列を提供 | `(clientAddr, selfAddr) -> Any?` |
| `configureAutoCommittingComposition` | 自動確定 | `(clientAddr, selfAddr) -> Void` |
| `configureProvidingRecognizedEvents` | 認識済みイベントマスク | `(clientAddr, selfAddr) -> NSUInteger` |
| `configureHandlingGivenNullableEvent` | NSEvent 処理 | `(nsEventPtr, clientAddr, selfAddr) -> Bool` |
| `configureSettingObjCValue` | `setValue:` 処理 | `(valuePtr, tag, clientAddr, selfAddr) -> Void` |

> **注意:** すべてのアドレスは `uintptr_t` です。`UnsafeRawPointer(bitPattern:)` + `Unmanaged<...>.fromOpaque(...).takeUnretainedValue()` を使用して Swift 側でオブジェクトを復元します。`takeUnretainedValue()` の前に毎回 `IMKControllerLifetimeTracker` で controller の生存を確認してください。

### 基本的な注入例

```swift
import IMKSwift

// ── applicationDidFinishLaunching で一度だけ注入 ──

IMKInputSessionController.configureActivatingServer { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.activate()
}

IMKInputSessionController.configureDeactivatingServer { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.deactivate()
}

IMKInputSessionController.configureDealloc { selfAddr in
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
  InputSession.showPreferences()
}

IMKInputSessionController.configureProvidingIMEMenu { selfAddr in
  return InputSession.buildMenu(for: selfAddr)
}

IMKInputSessionController.configureProvidingComposedString { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return nil }
  return session.currentComposedString
}

// ── ビジネスロジック（純粋な Swift、IMK 型を継承しない）──

@MainActor
public final class InputSession {
  /// パリティベースのダブルバッファ Session プール
  fileprivate static let sessionEven = InputSession(preallocated: ())
  fileprivate static let sessionOdd  = InputSession(preallocated: ())

  /// raw controller アドレスの generation parity で session を解決
  static func session(for ctlAddr: UInt) -> InputSession? {
    let parity = IMKControllerLifetimeTracker.shared().generation(for: ctlAddr) % 2
    return parity == 0 ? sessionEven : sessionOdd
  }

  static func cleanupController(addr: UInt) {
    // 3秒の遅延解放 + XPC クリーンアップ（ベストプラクティス #3 参照）
  }

  func handle(event: NSEvent) -> Bool {
    return true
  }
}
```

### 候補パネルの操作

```swift
// 候補パネルを作成して表示
let candidates = IMKCandidates(
  server: server,
  panelType: .horizontal
)

candidates.show(.below)
```

### コンポジション管理

```swift
// コンポジション（未確定文字列）を更新
updateComposition()

// 選択範囲と置換範囲にアクセス
let selRange = selectionRange()
let replaceRange = replacementRange()

// コンポジションを確定
commitComposition(sender)
```

### デプロイチェックリスト

リリース前に以下の全項目が対処されていることを確認してください：

- [ ] **Info.plist:** `InputMethodServerControllerClass` = `IMKInputSessionController`（カスタムサブクラスではない）。
- [ ] **Package.swift:** `IMKSwiftModernHeaders` ターゲットに `cSettings: [.unsafeFlags(["-fno-objc-arc"])]` が設定されている。
- [ ] **注入タイミング:** すべての `configure*` blocks は `applicationDidFinishLaunching` 内で、最初の `IMKServer.init(name:bundleIdentifier:)` より**前に**設定する。
- [ ] **KVC prune パス:** 単調増加する controller generation カウンターを維持。追跡 controller が 2 を超えたら、`[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]` で最古の非アクティブ controller を削除。
- [ ] **Session ルーティング:** `generation % 2` のパリティでセッションにルーティングする（client アドレスではなく）。正確に 2 つの事前割り当て static singleton を使用。
- [ ] **遅延解放:** `deactivateServer:` 後に 3 秒のタイマーをスケジュール。発火時：保持している blocks を解放、client wrapper の `+terminateForClient*` private class method を呼び出し、controller↔session マッピングをクリア。時間内に再有効化された場合はタイマーをキャンセル。
- [ ] **XPC クリーンアップ:** dealloc 時に `+terminateForClientXPCConn:` / `+terminateForClientDOProxy:` を呼び出す。`respondsToSelector:` で `_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper` の順に試行する。（macOS 27 は wrapper クラスを分割。macOS ≤ 10.15 はサフィックスなしの名前を使用。）
- [ ] **Dangling pointer 保護:** raw controller アドレスで `takeUnretainedValue()` を呼ぶ前に、毎回 `IMKControllerLifetimeTracker.shared().isTracked(addr:)` で生存を確認。
- [ ] **IME メニュー注入:** `class_addMethod` + `imp_implementationWithBlock` を使用して、実行時に `IMKInputSessionController` にメニューアクションを登録。`@objc` Selector の公開は不要。
- [ ] **メモリメトリクス:** `task_vm_info.internal` で anonymous private memory を読み取る（`phys_footprint` は Apple Silicon で GPU/neural engine 共有メモリを含むため使用しない）。読み取り前に `purgeMallocZones()` でアロケータキャッシュをフラッシュ。

## ベストプラクティス

### 1. NSConnection 命名規則

入力メソッドの `Info.plist` の `InputMethodConnectionName` キーは、**以下に設定する必要があります**：

```
$(PRODUCT_BUNDLE_IDENTIFIER)_Connection
```

> ⚠️ この命名規則は macOS 10.7 Lion から必須です。これに従わない場合、Sandbox を有効にした後、入力メソッドが正常に読み込まれなくなります。`Console.app` で NSConnection 関連のエラーメッセージが表示されます。

### 2. App Sandbox を有効にする

必ず App Sandbox を有効にしてください。脆弱的な NSConnection メカニズムを使用する必要があるため、Sandbox を有効にしない場合、Apple は入力メソッドが安全であることを信頼する術がありません。Sandbox の有効化は、ユーザーに対する最良のセキュリティ証明となります。

推奨される `entitlements` ファイル：

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

### 3. MRC とパリティベースのダブルバッファ Session プールを使用する

**`IMKSwiftModernHeaders` ターゲットを `-fno-objc-arc` でコンパイルしてください。** MainActor 上での ARC の retain/release の高頻度なやり取りは、CapsLock 切り替え時に体感できる遅延を引き起こします。ARC を無効にすると遅延は完全に解消されますが、クリーンアップを手動で処理する必要があります。

macOS 10.9〜27 にわたる IMKServer のリバースエンジニアリングにより、根本的な欠陥が明らかになりました：`_private._controllers` は client proxy アドレスをキーとしていますが、CapsLock の切り替えごとに**新しい** proxy（異なるアドレス）が作成されます。古い controller は永久的に孤立し——`sessionFinished:` は決して発火しません。IMK は現在の proxy にしかディスパッチしないためです。CapsLock の切り替えごとに IMKInputController とその NSXPCConnection がリークします。

**推奨アーキテクチャ（vChewing「Phase 115」）：**

- **KVC ベースの孤立 controller プルーニング：** init 時に単調増加する generation number を割り当てます。`_controllers` が 2 エントリを超えたら、KVC（`[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]`）で最古の非アクティブ controller を削除します。

- **パリティベースのダブルバッファ Session プール：** `generation % 2` で controller を 2 つの static `InputSession` singleton（偶数→session A、奇数→session B）にルーティングします。LRU キャッシュ、動的割り当て、エビクションロジックを完全に排除します。

- **遅延解放と XPC クリーンアップ：** `deactivateServer:` 後に 3 秒のタイマーをスケジュールします。発火時：blocks を解放、client wrapper の private `+terminateForClient*` を呼び出し（macOS 15 Sequoia 以降互換のため `_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper` の順に試行）、マッピングをクリアします。再有効化された場合はキャンセルします。

- **IME メニュー注入：** `class_addMethod` + `imp_implementationWithBlock` を使用して、`@objc` Selector の公開なしに IME メニューを動的に構築します。controller の生存数と anonymous private memory（`task_vm_info.internal`）をライブメトリクスとして注入し、読み取り前に malloc zones をパージします。

### 4. Swift Package ライブラリとしてコードを構造化する

macOS の入力メソッドはブレークポイントを使用してデバッグできません—これにより、クライアントアプリケーションとデスクトップ全体が凍結します。唯一実現可能なアプローチは、モッククライアントを使用したユニットテストです。

入力メソッドを Swift Package として構成します：
- **コアライブラリ** — ユニットテストでテスト可能なビジネスロジック
- **入力メソッドターゲット** — IMKSwift をコアライブラリに接続する薄いラッパー

これにより、標準の AppKit アプリを作成してタイピングプロセスをシミュレートし、Instruments を使用してメモリリークを検出することもできます。

### 5. IMKCandidates の使用を避ける

システム提供の `IMKCandidates` には重大な問題があり、特に macOS 26 では LiquidGlass レンダリングにより視覚的な不具合（透明な背景に白いテキスト）が発生します。**SwiftUI または AppKit を使用して、独自の候補パネルを実装することを検討してください**。

> Apple 自身の NumberInput サンプルでも `IMKCandidates` を避けています。

### 6. メモリ管理

ユーザーのコンピュータメモリは貴重です。macOS 26 の AppKit の非効率性により、入力メソッドが 80–200 MB の RAM を消費する可能性がありますが：

- **`activateServer()` でメモリ使用量を監視**し、1024 MB を超えた場合は自己終了します（`NSNotification` でユーザーに通知）
- **NSWindow の数を最小限に** — macOS 26 以降、NSWindow のメモリはシステムによって再利用されません。可能な限り、パネル（ツールチップ、候補ウィンドウなど）を単一の `NSPanel` に統合します

## アーキテクチャ

### 構造

- **IMKSwift** — プロトコル定義と拡張機能を含むメイン Swift ライブラリ
- **IMKSwiftModernHeaders** — `@MainActor` 注釈と型の改善を備えたモダンな Objective-C ヘッダー

### IMKInputSessionController の解決策

`IMKInputSessionController` は `IMKInputController` の具体的なサブクラスであり、Swift/Objective-C の相互運用性の制限を回避するために存在します。

**問題点：**
- `IMKInputController` の元の SDK ヘッダーは、`@MainActor` 分離なしで API を公開しています
- Swift はこれらを非分離としてインポートし、厳格な並行エラーを引き起こします
- 既存のクラスに対してヘッダーをオーバーライドすると、API 衝突が発生します

**解決策：**
- 同じ API サーフェスを持つ新しいサブクラス（`IMKInputSessionController`）を作成する
- Objective-C の `#pragma clang attribute` を介して `@MainActor` 分離を適用する
- Swift は、適切に分離されたメソッドを持つ新しいクラスとして認識します

**得られるもの：**
- すべてのメソッドに `@MainActor` 分離がある
- 明示的な Null 許容性注釈（`Nonnull` / `Nullable`）
- 生の `id` の代わりに具体的な Objective-C 型を使用

companion Swift モジュールでは、`IMKInputSessionController` は `IMKInputSessionControllerProtocol` に準拠するように拡張されています。これは、その完全な API サーフェスをミラーリングする Swift ネイティブの `@MainActor` プロトコルです。

## API ドキュメント

### コアプロトコル

#### IMKInputSessionControllerProtocol

メインアクター分離のプロトコル。以下を含む：
- `IMKStateSetting` — アクティベーション、非アクティベーション、環境設定
- `IMKMouseHandling` — マウスイベント処理
- `IMKServerInput` — テキストとイベント入力
- コンポジション管理 — `updateComposition()`、`commitComposition(_:)` など

### 主要メソッド

**状態管理：**
- `activateServer(_:)` — 入力メソッドを有効化
- `deactivateServer(_:)` — 入力メソッドを無効化
- `showPreferences(_:)` — 環境設定ダイアログを表示

**テキスト入力とコンポジション：**
- `inputText(_:key:modifiers:client:)` — キーボード入力を処理（詳細パラメータ）
- `inputText(_:client:)` — キーボード入力を処理（簡易版）
- `handle(_:client:)` — 生の `NSEvent` を処理
- `updateComposition()` — 現在のコンポジションを更新
- `cancelComposition()` — 進行中のコンポジションをキャンセル
- `commitComposition(_:)` — コンポジションを確定

**候補管理：**
- `candidates(_:)` — 候補提案を提供
- `candidateSelected(_:)` — 候補選択を処理
- `candidateSelectionChanged(_:)` — 選択変更を処理

**マウス処理：**
- `mouseDown(onCharacterIndex:coordinate:withModifier:continueTracking:client:)`
- `mouseUp(onCharacterIndex:coordinate:withModifier:client:)`
- `mouseMoved(onCharacterIndex:coordinate:withModifier:client:)`

## 元の InputMethodKit との違い

| 機能 | InputMethodKit | IMKSwift |
|------|----------------|----------|
| **並行処理** | 分離なし | 完全な `@MainActor` 分離 |
| **型安全性** | 裸の `id` 型 | 具体的な命名型 |
| **Null許容性** | 暗黙的 | 明示的注釈 |
| **Swift サポート** | 基本的なブリッジング | 完全な Swift 6 統合 |
| **Swift 6 の基底クラス** | `IMKInputController`（破損） | `IMKInputSessionController`（動作） |

## 関連プロジェクト

このライブラリは [vChewing プロジェクト](https://github.com/vChewing) の一部です。macOS プラットフォームで最も高速な音韻類中国語入力メソッドであり、大千声韻並撃打ち込み原理と DAG-DP 言葉構成技術に基づいており、簡体字中国語と繁体字中国語の両方をネイティブにサポートしています。

vChewing プロジェクトはこのパッケージを社会に提供し、寄付を歓迎しています。詳細については、[vChewing 入力メソッドのホームページ](https://vchewing.github.io/README.html)をご覧ください。

## ライセンス

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

詳細は [LICENSE](LICENSE) を参照してください。
