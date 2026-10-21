# IMKSwift

Swift 6 以降向けにモダナイズされた **InputMethodKit** オーバーレイ。macOS 上で安全、並行処理対応、かつ型安全な入力メソッドエンジンを構築できます。

## 概要

IMKSwift は [vChewing プロジェクト](https://github.com/vChewing) の一部であり、Apple の InputMethodKit フレームワークに対する Swift ネイティブのラッパーを提供します。本パッケージは二つのことを同時に行います。

第一に、IMK の Objective-C インターフェースを Swift 6 の言葉で宣言し直します。`id` の代わりに具体的な型、明示的な Null 許容性、そしてすべての入口に `@MainActor` 隔離を付けます。第二に、IMKServer が 10.5 以来抱え続けてきたコントローラのライフタイム欠陥を修復します。これはヘッダを何度書き換えても届かない欠陥です——欠陥は IMKServer の私有状態の中にあり、宣言の中にはないからです。

InputMethodKit の歴史は macOS 10.5 Leopard にまで遡ります——ARC、Sandbox、そして Swift の登場より前です。二世代の技術的転換をまたぐ遺産的フレームワークであり、IMKSwift はその溝を埋め、現代の Swift 開発者が古い Objective-C の流儀と格闘せずに入力メソッドを構築できるようにします。

安全でない型変換、裸の `id` 型、暗黙のグローバル状態と格闘する代わりに、IMKSwift は以下を提供します：

- すべての API における**明示的な `@MainActor` 隔離**
- 裸の `id` ポインタではなく、**具体的で Null 許容性を注記した型**
- **完全な Swift 6 並行性サポート**
- **InputMethodKit の完全なサーフェス**と、安全な利用のための改良
- `-fno-objc-arc` でコンパイルされた**修復済みのコントローラ・ライフタイム**：アドレス索引による生存追跡、孤児コントローラの剪定、遅延解体、クライアント接続の終了、そして診断専用の 4 つの kill switch

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

### コントローラ・ライフタイムの修復

本パッケージには Objective-C 層（`IMKSwiftModernHeaders`、`-fno-objc-arc` でコンパイル）も同梱されます。ヘッダの書き換えでは決して届かない領域を扱うためです：

- 生存中のコントローラはすべて、生のアドレスで `IMKControllerLifetimeTracker` に登録されます；
- 孤児となったコントローラは IMKServer の私有 `_controllers` 辞書から剪定され、それが保持するクライアント接続ラッパーも終了させられます；
- `deactivateServer:` の後、コントローラの解体は 3 秒遅延されます。頻繁な CapsLock 切り替えで、まさに再利用されようとしている接続を壊さないためです；
- 診断専用の 4 つのスイッチと安定したログ語彙により、現場の事故を特製ビルドなしで二分探索できます。

### Swift 6 対応
Swift 6 の厳密な並行処理モデルを念頭に置いて設計されています。すべての API は適切に分離されており、並行コンテキスト環境においてデータ競合なしに使用できます。

## 要件

- **Swift** 6.2 以降
- **Xcode** 16.0 以降（現行の私有面ヘッダは 26.x で検証しています）
- **macOS**：
    - macOS 10.13 High Sierra 以降（Swift Concurrency のランタイム下限であり、実際の下限は Swift のバージョンに依存します）。
    - コード自体は、対応する macOS SDK と libARCLite を用いれば macOS 10.09 Mavericks 上でも動作します。

## インストール

### Swift Package Manager

`Package.swift` に IMKSwift を追加します：

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.07"),
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

### 3. コントローラでオブジェクトを強参照しない——そしてクライアント束縛はイベントごとに張り直す

二つの鉄則は、いずれも失敗から学んだものです。

**第一に、`IMKInputSessionController` のサブクラスは業務ロジックのオブジェクトを強参照してはいけません。** CapsLock の切り替えやクライアントの切り替えのたびに、新しいコントローラ複製が作られ得ます。切り替えのたびに大きなオブジェクトグラフを解体する ARC churn は、そのまま入力の遅延として現れます。

**第二に、アクティベート時に記録したクライアントのアドレスを不変の事実と見なしてはいけません。** IMKServer は私有のコントローラ表をクライアント・プロキシのアドレスで索引しており、クライアントの脈絡が変われば新しいプロキシが渡されます。「自分のクライアントは 0x…9d0 だ」と覚えているセッションは、すでに退役したコントローラへ送り続けます。症状は静かです。モード切り替えの表示は出るのに、キー入力が届かなくなります。

セッションはコントローラ自身のアドレスをキーとする小さなプールから払い出し、イベントのたびにクライアントを結び直してください：

```swift
@MainActor
final class SessionPool {
  static let shared = SessionPool(capacity: 4)
  private var slots: [UInt: InputSession] = [:]

  func session(for ctlAddr: UInt) -> InputSession { /* ... */ }
  func reap(using tracker: IMKControllerLifetimeTracker) { /* コントローラが死んだスロットを回収 */ }
}

// イベントコールバック内、ディスパッチの前に：
session.adoptClientAddr(ctlAddr)  // 権威はイベントの送り手であり、アクティベート時の記録ではない
```

頭に入れておくべき二つの系：

- **`-dealloc` を待ってはいけません。** IMKServer はコントローラを孤児にしますが、決して解放しません。`sessionFinished:` はクライアント・プロキシごとに配送されるため、孤児には決して届きません。セッションの回収は、ライフタイムトラッカーに「そのコントローラのアドレスがまだ生きているか」を問い合わせて行います。
- **バンドルが差し替えられる前に、入力メソッドのプロセスを殺してはいけません。** IMK は登録済みバンドルから必要に応じて入力メソッドを再起動します。インストールの途中で終了させても、古いコードが新しい PID で戻ってくるだけです。差し替えの後に PID を捕捉し、終了を要求し、必要なら `SIGKILL` まで引き上げて、最後に検証してください（§7 参照）。

「退役した複製」の危険について一言。二つのコントローラが同じセッションスロットを共有していた場合、先に退役したほうの時点でそのスロットのクライアント束縛は消され、もう一方はそのまま送り続けます。これは「世代の奇偶で対にしたダブルバッファ」がまさに起こす不具合であり、本設計がアドレスをキーとし、束縛をイベントごとに張り直す理由です。

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

### 7. IMKServer の私有面を、観測点のあるブラックボックスとして扱う

IMKServer を直すことはできませんし、ブレークポイントでデバッグすることもできません。できるのは、その振る舞いを観測可能にし、自分自身の干渉を一つずつ切れるようにすることです。

IMKSwift の Objective-C 層は、そのために小さく安定した語彙だけを出力します：

| 語彙 | 意味 |
|---|---|
| `ControllerInit:` | コントローラ複製が生成された（アドレスと世代を含む） |
| `Prune:` | 私有の `_controllers` 表が検査された、または孤児が排除された |
| `ActivateServer:` / `DeactivateServer:` | クライアントの有効化と退役 |
| `DelayedDealloc:` | 遅延解体が予約・省略・実行された |
| `EventEntry:` | キーイベントがどのコントローラに届いたか、そのクライアントが生存していたか |
| `guard fail:` | イベントは入力メソッドに届いたが、使用可能なクライアントプロキシが無かった |

さらに、診断専用の 4 つのスイッチ（クラスメソッドで設定し、vChewing では「開発道場」パネルに接続しています）により、各干渉を個別に無効化できます：

| スイッチ | 無効化されるもの |
|---|---|
| `configureServerControllerPruningDisabled(_:)` | KVC による剪定 |
| `configureEvictedClientInvalidationDisabled(_:)` | 排除されたコントローラのクライアント接続の無効化（排除のうち攻撃的な半分だけ） |
| `configureClientWrapperTerminationDisabled(_:)` | ラッパーへの `+terminateForClient*`（dealloc の簿記は残す） |
| `configureControllerDelayedDeallocationDisabled(_:)` | 遅延解体そのもの |

各ブロックは判断の時点で評価され、キャッシュされることはありません。実行中のプロセスで対応する `_IMKSwift_disable…` 設定を変えれば、次回から効きます。

生の 16 通りの組み合わせは、実際には 9 通りの区別可能な挙動に潰れます。二つのスイッチが他の二つに支配されるからです。「スクリーンショットツールを走らせた後、修飾キーで切り替わらなくなった」という報告を受けたら、上記のログ語彙と 4 つのスイッチの状態を聞くだけで十分です。ログは、イベントが届いていないのか、死んだクライアントに届いたのか、誤ったクライアントに届いたのかを教えてくれます——三つの異なる病気に三つの異なる処方があり、観測点が無ければ一つも見分けられません。

## アーキテクチャ

### 構造

- **IMKSwift** — メインの Swift ライブラリ：プロトコル定義、`@MainActor` 拡張、そして Swift 側のセッション配管
- **IMKSwiftModernHeaders** — 現代化された Objective-C ヘッダ、および MRC（`-fno-objc-arc`）の私有面層：`IMKSwift.m`（剪定・遅延解放・クライアント接続終了のフック）と `IMKControllerLifetimeTracker.m`（アドレス生存と世代の簿記）
- **利用側** — あなたの `IMKInputSessionController` サブクラス、およびアドレスの変動を生き延びるセッション・プール

### 三つの層、二つの言語

- **宣言層。** `IMKInputSessionController` は `IMKInputController` の具象サブクラスであり、その存在理由は Swift と Objective-C の相互運用上の制約を回避することだけです。元の SDK ヘッダは `@MainActor` 隔離なしで API を公開しており、API 衝突なしに再宣言することはできません。したがって、名前を変えたサブクラスが唯一の現実解です。`#pragma clang attribute` によって `@MainActor` を付与し、Swift 側では `IMKInputSessionControllerProtocol` でその全サーフェスを鏡映します。
- **修復層。** 欠陥は IMKServer の私有状態にあるため、修正は Objective-C でしか書けません。`_private._controllers` への KVC アクセス、世代簿記付きの剪定、dealloc 時のフック、クライアント接続ラッパー・クラスへの `+terminateForClient*` 呼び出しです。このコードはあなたの入力メソッドのプロセス内で、`-fno-objc-arc` でコンパイルされます。コントローラ複製の retain／release churn が入力遅延として現れないようにするためです。
- **方針層。** どのコントローラがどのセッションを受け取るか、セッションがいつクライアントへ結び直されるか、いつ退役するかは、あなたの入力メソッドが決めることです。IMKSwift はアドレス、生存確認、フック点を提供しますが、あなたのセッション・モデルは意図的に持ちません。

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

| 項目 | InputMethodKit | IMKSwift |
|---------|----------------|----------|
| **並行性** | 隔離なし | 完全な `@MainActor` 隔離 |
| **型安全性** | 裸の `id` 型 | 具体的で名前のある型 |
| **Null 許容性** | 暗黙 | 明示的な注記 |
| **Swift 対応** | 基本的なブリッジ | 完全な Swift 6 統合 |
| **Swift 6 の基底クラス** | `IMKInputController`（使用不可） | `IMKInputSessionController`（使用可） |
| **コントローラ・ライフタイム** | CapsLock の切り替えごとに孤児コントローラを漏らす。`sessionFinished:` は決して来ない | アドレス索引の追跡、KVC 剪定、遅延解体、ラッパー終了 |
| **クライアント束縛** | 暗黙で、プロキシ変動後に失効 | イベントごとに再確立 |
| **可観測性** | 沈黙 | 安定したログ語彙と、診断専用の 4 つのスイッチ |

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
