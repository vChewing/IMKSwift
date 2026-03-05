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

**Swift 6 では `IMKInputController` を直接使用することはできません。** ヘッダーを直接オーバーライドしようとしても、Swift は元の Xcode SDK ヘッダーを引き続き読み込み、API 衝突が発生します。唯一の解決策は、**異なる名前のサブクラス** — `IMKInputSessionController` — を使用することです。これはすべての機能を継承しながら、正しく `@MainActor` が注釈された API を提供します。

> ⚠️ **重要：** Swift 6 で直接 `IMKInputController` をサブクラス化すると、コンパイルエラーに直面します。これらのエラーは、オブジェクトを `@MainActor` に無理やり押し込む醜いポインタ操作によってのみ「修正」できます。そんなことはしないでください。代わりに `IMKInputSessionController` を使用してください。

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
- **macOS** 10.13 以降（ご使用の Swift のバージョンによる）
- **Xcode** 16.0 以降

## インストール

### Swift Package Manager

`Package.swift` に IMKSwift を追加します：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.05"),
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

## 使用方法

### 基本的な入力メソッドコントローラセットアップ

```swift
import IMKSwift

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  override public func handle(_ event: NSEvent?, client sender: any IMKTextInput) -> Bool {
    // 完全な型安全性を備えたイベント処理
    guard let event else { return false }
    
    // 入力を処理...
    return true
  }
  
  override func inputText(_ string: String, client sender: any IMKTextInput) -> Bool {
    // テキスト入力処理
    return true
  }
  
  override func candidates(_ sender: any IMKTextInput) -> [Any]? {
    // 候補提案を返す
    return nil
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

### 3. コントローラでオブジェクトを強参照しない

**`IMKInputSessionController` のサブクラスは、ビジネスロジックオブジェクトを直接保持すべきではありません。** これは、高頻度の入力メソッド切り替え（例：CapsLock を使用した英数/日本語切り替え）を処理する上で重要です。

ユーザーが入力メソッドを頻繁に切り替えると、システムは毎回新しい `IMKInputController` インスタンスを作成します。コントローラが大きなオブジェクトへの強参照を保持している場合、ARC クリーンアップに目に見えて遅延が発生します。

**推奨パターン：** クライアントオブジェクトをキーとする弱キーキャッシュ（`NSMapTable`）を使用します：

```swift
import IMKSwift

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  // セッションへの弱参照を使用
  weak var session: InputSession?
  
  override public init(server: IMKServer, delegate: Any?, client inputClient: any IMKTextInput) {
    super.init(server: server, delegate: delegate, client: inputClient)
    
    // セッションを初期化またはキャッシュから取得
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
