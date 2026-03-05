# IMKSwift

A modernized **InputMethodKit** overlay for Swift 6 and later, providing safe, concurrent, and type-safe APIs for building input method engines on macOS.

## Overview

IMKSwift is part of the [vChewing Project](https://github.com/vChewing) and offers a Swift-native replacement for Apple's InputMethodKit framework. It combines Objective-C interoperability with Swift 6's strict concurrency model, delivering `@MainActor`-isolated APIs that are easier to work with in modern Swift code.

InputMethodKit dates back to macOS 10.5 Leopard—predating ARC, Sandboxing, and Swift itself. It is a legacy framework spanning two generations of technology shifts. IMKSwift bridges this gap, allowing modern Swift developers to build input methods without wrestling with ancient Objective-C patterns.

Instead of struggling with unsafe casts, bare `id` types, and implicit global state, IMKSwift provides:

- **Explicit `@MainActor` isolation** on every API
- **Concrete, nullable-annotated types** instead of bare `id` pointers
- **Full Swift 6 concurrency support**
- **Complete InputMethodKit surface** with refinements for safe usage

## Why IMKInputSessionController?

Apple's `IMKInputController` is designed to run on the MainActor, but its Objective-C headers lack proper `@MainActor` annotations. When imported into Swift, this creates a fundamental problem: the original SDK headers expose APIs without actor isolation, causing **unresolvable concurrency checking errors** in Swift 6 strict mode.

**You cannot simply use `IMKInputController` in Swift 6.** Even if you try to override headers directly, Swift still picks up the original Xcode SDK headers, causing API collisions. The only solution is to use a **subclass with a different name** — `IMKInputSessionController` — which inherits all functionality while providing properly `@MainActor`-isolated APIs.

> ⚠️ **Critical:** If you subclass `IMKInputController` directly in Swift 6, you will face compiler errors that can only be "fixed" with ugly pointer manipulation to force objects onto `@MainActor`. Don't do this. Use `IMKInputSessionController` instead.

## Features

### Type Safety & Nullability
All APIs include explicit nullability annotations (`_Nullable`, `_Nonnull`) and use concrete Objective-C types (`NSString`, `NSAttributedString`, `NSDictionary`, `NSArray`, `NSEvent`, etc.) instead of generic `id`.

### MainActor Isolation
Every method and property is marked with `@MainActor`, ensuring call-site safety at compile time and preventing race conditions in concurrent code.

### Complete InputMethodKit Coverage
IMKSwift re-exports and enhances the following InputMethodKit components:

- **IMKCandidates** — Candidate panel management and display
- **IMKServer** — Input method session server
- **IMKInputSessionController** — Input method event handling and composition (the **only** recommended base class for Swift 6+)
- **IMKTextInput** — Text editing client protocols
- **Supporting protocols** — IMKStateSetting, IMKMouseHandling, IMKServerInput

### Swift 6 Ready
Built with Swift 6's strict concurrency model in mind. All APIs are properly isolated and can be used in concurrent contexts without data races.

## Requirements

- **Swift** 6.2 or later
- **macOS** 10.13 or later (depending on Swift version used)
- **Xcode** 16.0 or later

## Installation

### Swift Package Manager

Add IMKSwift to your `Package.swift`:

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.05"),
```

Then add it as a dependency to your target:

```swift
.target(
  name: "MyInputMethod",
  dependencies: [
    .product(name: "IMKSwift", package: "IMKSwift"),
  ]
)
```

## Usage

### Basic Input Method Controller Setup

```swift
import IMKSwift

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  override public func handle(_ event: NSEvent?, client sender: any IMKTextInput) -> Bool {
    // Event handling with full type safety
    guard let event else { return false }
    
    // Process input...
    return true
  }
  
  override func inputText(_ string: String, client sender: any IMKTextInput) -> Bool {
    // Text input handling
    return true
  }
  
  override func candidates(_ sender: any IMKTextInput) -> [Any]? {
    // Return candidate suggestions
    return nil
  }
}
```

### Working with Candidates

```swift
// Create and show a candidate panel
let candidates = IMKCandidates(
  server: server,
  panelType: .horizontal
)

candidates.show(.below)
```

### Composition Management

```swift
// Update composition with attributes
updateComposition()

// Access selection and replacement ranges
let selRange = selectionRange()
let replaceRange = replacementRange()

// Commit composition
commitComposition(sender)
```

## Best Practices

### 1. NSConnection Naming

The `InputMethodConnectionName` key in your input method's `Info.plist` **must** be set to:

```
$(PRODUCT_BUNDLE_IDENTIFIER)_Connection
```

> ⚠️ This naming convention is mandatory since macOS 10.7 Lion. Without it, your input method will fail to load when Sandbox is enabled. You will see NSConnection-related errors in `Console.app`.

### 2. Enable App Sandbox

Always enable the Sandbox if possible. Given that you're forced to use the fragile NSConnection mechanism, without Sandbox enabled, Apple has no way to trust that your input method is safe. Enabling Sandbox is the best security credential you can offer users.

Here's a recommended `entitlements` file:

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

### 3. Don't Hold Strong References in Your Controller

**Your `IMKInputSessionController` subclass should not directly hold business logic objects.** This is critical for handling high-frequency input method switching (e.g., CapsLock toggling between ABC and your IME).

When users switch input methods rapidly, the system creates new `IMKInputController` instances each time. If your controller holds strong references to heavy objects, ARC cleanup causes noticeable lag.

**Recommended Pattern:** Use a weak-key cache (`NSMapTable`) keyed by the client object:

```swift
import IMKSwift

@objc(MyInputMethodController)
public final class MyInputMethodController: IMKInputSessionController {
  // Use weak reference to session
  weak var session: InputSession?
  
  override public init(server: IMKServer, delegate: Any?, client inputClient: any IMKTextInput) {
    super.init(server: server, delegate: delegate, client: inputClient)
    
    // Initialize or retrieve cached session
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

### 4. Structure as Swift Package Libraries

macOS input methods cannot be debugged with breakpoints—they freeze the client applications and your entire desktop. The only viable approach is unit testing with mocked clients.

Structure your input method as a Swift Package:
- **Core library** — Business logic, testable with unit tests
- **Input method target** — Thin wrapper that connects IMKSwift to your core library

This also allows you to write a standard AppKit app for simulating typing and detecting memory leaks with Instruments.

### 5. Avoid IMKCandidates

The system-provided `IMKCandidates` has significant issues, especially on macOS 26 where LiquidGlass rendering causes visual glitches (white text on transparent backgrounds). **Consider implementing your own candidate panel** using SwiftUI or AppKit.

> Even Apple's own NumberInput sample avoids `IMKCandidates`.

### 6. Memory Management

User memory is precious. While macOS 26's AppKit inefficiency may cause your input method to consume 80–200 MB:

- **Monitor memory usage** in `activateServer()` and self-terminate if exceeding 1024 MB (notify the user via `NSNotification`)
- **Minimize NSWindow count** — Beginning with macOS 26, NSWindow memory is never reclaimed. Consolidate panels (tooltips, candidate windows, etc.) into a single `NSPanel` where possible

## Architecture

### Structure

- **IMKSwift** — Main Swift library with protocol definitions and extensions
- **IMKSwiftModernHeaders** — Modernized Objective-C headers with `@MainActor` annotations and type refinements

### The IMKInputSessionController Solution

`IMKInputSessionController` is a concrete subclass of `IMKInputController` that exists solely to work around Swift/Objective-C interoperability limitations.

**The Problem:**
- `IMKInputController`'s original SDK headers expose APIs without `@MainActor` isolation
- Swift imports these as non-isolated, causing strict concurrency errors
- You cannot override headers for existing classes without API collisions

**The Solution:**
- Create a new subclass (`IMKInputSessionController`) with the same API surface
- Apply `@MainActor` isolation via `#pragma clang attribute` in Objective-C
- Swift sees a fresh class with properly isolated methods

**What You Get:**
- `@MainActor` isolation on all methods
- Explicit nullability annotations (`_Nonnull` / `_Nullable`)
- Concrete Objective-C types instead of bare `id`

In the companion Swift module, `IMKInputSessionController` is extended to conform to `IMKInputSessionControllerProtocol`, a Swift-native `@MainActor` protocol that mirrors its full API surface.

## API Documentation

### Core Protocol

#### IMKInputSessionControllerProtocol

A main-actor-isolated protocol that encompasses:
- `IMKStateSetting` — Activation, deactivation, preferences
- `IMKMouseHandling` — Mouse event handling
- `IMKServerInput` — Text and event input
- Composition management — `updateComposition()`, `commitComposition(_:)`, etc.

### Key Methods

**State Management:**
- `activateServer(_:)` — Activate the input method
- `deactivateServer(_:)` — Deactivate the input method
- `showPreferences(_:)` — Display preferences dialog

**Text & Composition:**
- `inputText(_:key:modifiers:client:)` — Handle keyboard input with detailed parameters
- `inputText(_:client:)` — Handle keyboard input (simplified)
- `handle(_:client:)` — Handle raw `NSEvent`
- `updateComposition()` — Update current composition
- `cancelComposition()` — Cancel ongoing composition
- `commitComposition(_:)` — Finalize composition

**Candidate Management:**
- `candidates(_:)` — Provide candidate suggestions
- `candidateSelected(_:)` — Handle candidate selection
- `candidateSelectionChanged(_:)` — Handle selection changes

**Mouse Handling:**
- `mouseDown(onCharacterIndex:coordinate:withModifier:continueTracking:client:)`
- `mouseUp(onCharacterIndex:coordinate:withModifier:client:)`
- `mouseMoved(onCharacterIndex:coordinate:withModifier:client:)`

## Differences from Original InputMethodKit

| Feature | InputMethodKit | IMKSwift |
|---------|----------------|----------|
| **Concurrency** | No isolation | Full `@MainActor` isolation |
| **Type Safety** | Bare `id` types | Concrete, named types |
| **Nullability** | Implicit | Explicit annotations |
| **Swift Support** | Basic bridging | Full Swift 6 integration |
| **Base Class for Swift 6** | `IMKInputController` (broken) | `IMKInputSessionController` (working) |

## Related Projects

This library is part of the [vChewing Project](https://github.com/vChewing). It is the fastest phonetic Chinese input method for macOS, based on the Dachen (大千) consonant-vowel simultaneously-clapped keystroke typing principle and DAG-DP sentence formation technology, with native support for both Simplified and Traditional Chinese.

The vChewing Project offers this package to society and welcomes donations. For more information, please visit the [vChewing Input Method homepage](https://vchewing.github.io/README.html).

## License

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

See [LICENSE](LICENSE) for details.
