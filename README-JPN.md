# IMKSwift

Swift 6 以降向けにモダナイズされた **InputMethodKit** オーバーレイ。macOS 上で安全、並行処理対応、かつ型安全な入力メソッドエンジンを構築できます。

## 概要

IMKSwift は [vChewing プロジェクト](https://github.com/vChewing) の一部であり、Apple の InputMethodKit フレームワークに対する Swift ネイティブな代替実装を提供します。Objective-C の相互運用性と Swift 6 の厳密な並行処理モデルを組み合わせ、モダンな Swift コードでより扱いやすい `@MainActor` 分離 API を提供します。

安全でない型キャスト、生の `id` 型、そして暗黙的なグローバル状態の処理に苦労する代わりに、IMKSwift は以下を提供します：

- **すべての API に対する明示的な `@MainActor` 分離**
- **生の `id` ポインタの代わりとなる、具体的で Null 許容性が明示された型**
- **完全な Swift 6 並行処理サポート**
- **完全な InputMethodKit サーフェス** と安全な使用法の改善

## 機能

### 型安全性とNull許容性
すべての API には明示的なNull許容性注釈（`_Nullable`、`_Nonnull`）が含まれており、一般的な `id` の代わりに具体的な Objective-C 型（`NSString`、`NSAttributedString`、`NSDictionary`、`NSArray`、`NSEvent` など）を使用します。

### MainActor 分離
すべてのメソッドとプロパティは `@MainActor` でマークされており、コンパイル時の呼び出し元 (Call-site) の安全性を保証し、並行処理コードでのデータ競合 (データレース) を防止します。

### 完全な InputMethodKit カバレッジ
IMKSwift は以下の InputMethodKit コンポーネントを再エクスポートして強化します：

- **IMKCandidates** — 候補パネルの管理と表示
- **IMKServer** — 入力メソッドセッションサーバー
- **IMKInputController** — 入力メソッドのイベント処理とコンポジション (未確定文字列制御)
- **IMKTextInput** — テキスト編集クライアントプロトコル
- **サポートプロトコル** — IMKStateSetting、IMKMouseHandling、IMKServerInput

### Swift 6 対応
Swift 6 の厳密な並行処理モデルを念頭に置いて設計されています。すべての API は適切に分離されており、並行コンテキスト環境においてデータ競合なしに使用できます。

## 要件

- **Swift** 6.2 以降
- **macOS** ご使用の Swift のバージョン次第
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

class MyInputMethodController: IMKInputController {
  override func handle(_ event: NSEvent?, client sender: any IMKTextInput) -> Bool {
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
  panelType: .horizontal,
  styleType: .default
)

candidates.show(.below)
```

### コンポジション管理

```swift
// 属性でコンポジション (未確定文字列) を更新
updateComposition()

// 選択範囲と置換範囲にアクセス
let selRange = selectionRange()
let replaceRange = replacementRange()

// コンポジションを確定
commitComposition(client)
```

## アーキテクチャ

### 構造

- **IMKSwift** — プロトコル定義と拡張機能を含むメイン Swift ライブラリ
- **IMKSwiftModernHeaders** — `@MainActor` 注釈と型の改善を備えたモダンな Objective-C ヘッダー

### 拡張パターン

すべての InputMethodKit クラスは Objective-C のカテゴリを通じて拡張され、Swift でのプロトコル適合を通じて再エクスポートされます。これにより、完全な API 互換性を維持しながら、型安全性と並行処理の保証が追加されます。

## API ドキュメント

### コアプロトコル

#### IMKInputSessionControllerProtocol

以下を含むメインアクター分離のスーパープロトコル：
- `IMKStateSetting` — アクティベーション、非アクティベーション、環境設定
- `IMKInputController` — イベント処理、コンポジション、候補管理
- `IMKMouseHandling` — マウスイベント処理
- `IMKServerInput` — テキストとイベント入力

利用側のコード (Downstream code) は、これらの変更点を適用するために（`IMKInputController`のみではなく）`IMKInputSessionController` をサブクラス化する必要があります。

### 主要メソッド

**状態管理：**
- `activateServer(_:)` — 入力メソッドを有効化
- `deactivateServer(_:)` — 入力メソッドを無効化
- `showPreferences(_:)` — 環境設定ダイアログを表示

**テキスト入力とコンポジション：**
- `inputText(_:key:modifiers:client:)` — キーボード入力を処理
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
| **候補パネル** | 手動セットアップ | 簡略化 API |

## 関連プロジェクト

このライブラリは [vChewing プロジェクト](https://github.com/vChewing) の一部です。これは macOS 用の最新のオープンソース入力メソッド フレームワークです。

## ライセンス

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

詳細は [LICENSE](LICENSE) を参照してください。
