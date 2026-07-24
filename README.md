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

**You cannot simply use `IMKInputController` in Swift 6.** Even if you try to override headers directly, Swift still picks up the original Xcode SDK headers, causing API collisions. `IMKInputSessionController` provides a clean, `@MainActor`-isolated API surface.

> ⚠️ **Do NOT subclass `IMKInputSessionController`.** From v26.07.24 onward, IMKSwift uses an **injection-based architecture**. Subclassing would expose the controller's lifecycle (`alloc` / `dealloc` / `retain` / `release`) to Swift ARC, reintroducing the exact MainActor contention that causes CapsLock switching stutter. Instead, all IMK callbacks are injected via class-level static blocks in `applicationDidFinishLaunching`. The controller lifecycle stays entirely within ObjC MRC (`-fno-objc-arc`), and Swift interacts with controllers only through raw `uintptr_t` addresses — never through ARC-managed references.

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
- **Xcode** 16.0 or later
- **macOS**:
    - macOS 10.13 High Sierra or later (depending on your Swift version).
    - The code itself can run on macOS 10.09 Mavericks, but requires the corresponding macOS SDK and libARCLite.

## Installation

### Swift Package Manager

Add IMKSwift to your `Package.swift`:

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.07.24"),
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

### Deployment Architecture Overview

IMKSwift v26.07.24 adopts an **injection-based architecture**. You no longer subclass `IMKInputSessionController`. Instead, all IMK callbacks are injected via **class-level static blocks** — configured once at startup and shared by all controller instances. Each block receives raw `uintptr_t` memory addresses instead of object references — no retain/release is performed.

**Info.plist:** Declare the controller class directly as `IMKInputSessionController` (no custom subclass needed). The following keys must all point to `IMKInputSessionController` to ensure the controller lifecycle never touches Swift ARC:

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

**Build settings:** Your project must compile `IMKSwiftModernHeaders` with `-fno-objc-arc`:

```swift
// Package.swift
.target(
  name: "IMKSwiftModernHeaders",
  cSettings: [.unsafeFlags(["-fno-objc-arc"])]
),
```

**Startup injection:** All configure blocks must be set in `applicationDidFinishLaunching`, **before** the first `IMKServer` is initialized. The blocks are class-level statics — set once, shared forever.

### Available Configure Blocks

| Block | Purpose | Signature |
|-------|---------|-----------|
| `configureActivatingServer` | Controller activated | `(clientAddr, selfAddr) -> Void` |
| `configureDeactivatingServer` | Controller deactivated | `(clientAddr, selfAddr) -> Void` |
| `configureDealloc` | Controller about to dealloc | `(selfAddr) -> Void` |
| `configureShowingPreferences` | Show preferences | `(clientAddr, selfAddr) -> Void` |
| `configureHidingPallettes` | Hide UI panels | `(selfAddr) -> Void` |
| `configureInputControllerWillClose` | Controller will close | `(selfAddr) -> Void` |
| `configureProvidingSelectionRange` | Provide selection range | `(selfAddr) -> NSRange` |
| `configureProvidingIMEMenu` | Build IME menu | `(selfAddr) -> NSMenu?` |
| `configureProvidingComposedString` | Provide composed string | `(clientAddr, selfAddr) -> Any?` |
| `configureAutoCommittingComposition` | Auto-commit composition | `(clientAddr, selfAddr) -> Void` |
| `configureProvidingRecognizedEvents` | Recognized event mask | `(clientAddr, selfAddr) -> NSUInteger` |
| `configureHandlingGivenNullableEvent` | Handle NSEvent | `(nsEventPtr, clientAddr, selfAddr) -> Bool` |
| `configureSettingObjCValue` | Handle `setValue:` | `(valuePtr, tag, clientAddr, selfAddr) -> Void` |

> **Note:** All addresses are `uintptr_t`. Use `UnsafeRawPointer(bitPattern:)` + `Unmanaged<...>.fromOpaque(...).takeUnretainedValue()` to recover objects on the Swift side. The `IMKControllerLifetimeTracker` should verify controller liveness before every `takeUnretainedValue()` call.

### Basic Injection Example

```swift
import IMKSwift

// ── applicationDidFinishLaunching: inject ONCE before IMKServer init ──

IMKInputSessionController.configureActivatingServer { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.activate()
}

IMKInputSessionController.configureDeactivatingServer { clientAddr, selfAddr in
  guard let session = InputSession.session(for: selfAddr) else { return }
  session.deactivate()
}

IMKInputSessionController.configureDealloc { selfAddr in
  // Delayed XPC cleanup + session mapping teardown
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

// ── Business logic (pure Swift, no IMK types inherited) ──

@MainActor
public final class InputSession {
  /// Parity-based double-buffered pool: two singletons, routed by generation parity.
  fileprivate static let sessionEven = InputSession(preallocated: ())
  fileprivate static let sessionOdd  = InputSession(preallocated: ())

  /// Resolve session by controller's generation parity.
  static func session(for ctlAddr: UInt) -> InputSession? {
    let tracker = IMKControllerLifetimeTracker.shared()
    guard tracker.isAddressAlive(ctlAddr) else { return nil }
    let parity = tracker.generationForAddress(ctlAddr) % 2
    return parity == 0 ? sessionEven : sessionOdd
  }

  static func cleanupController(addr: UInt) { /* 3-sec delayed dealloc + XPC teardown */ }

  func handle(event: NSEvent) -> Bool { return true }

  var currentComposedString: Any? { nil }
  static func showPreferences() {}
  static func buildMenu(for addr: UInt) -> NSMenu? { nil }
  func activate() {}
  func deactivate() {}
}
```

### Working with Candidates

```swift
let candidates = IMKCandidates(server: server, panelType: .horizontal)
candidates.show(.below)
```

### Composition Management

```swift
updateComposition()
let selRange = selectionRange()
let replaceRange = replacementRange()
commitComposition(sender)
```

### Setup Checklist

Before shipping, ensure all items below are addressed:

- [ ] **Info.plist:** `InputMethodServerControllerClass` = `IMKInputSessionController` (not a custom subclass).
- [ ] **Package.swift:** `IMKSwiftModernHeaders` target has `cSettings: [.unsafeFlags(["-fno-objc-arc"])]`.
- [ ] **Injection timing:** All `configure*` blocks are set in `applicationDidFinishLaunching`, **before** `IMKServer.init(name:bundleIdentifier:)`.
- [ ] **KVC prune path:** Maintain a monotonically increasing controller generation counter; when tracked controllers exceed 2, prune the oldest inactive one via `[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]`.
- [ ] **Session routing:** Route controllers to sessions by `generation % 2` parity (not by client address), using exactly two preallocated static singletons.
- [ ] **Delayed dealloc:** After `deactivateServer:`, schedule a 3-second timer. On fire: release retained blocks, call `+terminateForClient*` private class methods on the client wrapper, and clear controller↔session mappings. Cancel the timer if the controller is reactivated within the window.
- [ ] **XPC cleanup:** On dealloc, call `+terminateForClientXPCConn:` / `+terminateForClientDOProxy:` on the client wrapper class. Use a `respondsToSelector:` fallback chain: `_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper`. (macOS 15 Sequoia onward split the wrapper class; macOS ≤ 10.15 uses the unsuffixed name.)
- [ ] **Dangling pointer protection:** Before every `takeUnretainedValue()` on a raw controller address, verify liveness via `IMKControllerLifetimeTracker.shared().isAddressAlive(addr)`. IMKSwift provides this tracker as part of the package — it automatically tracks every controller allocated through `IMKInputSessionController` and unregisters them on dealloc via an associated-object sentinel. Use `generationForAddress(_:)` for parity-based session routing.
- [ ] **IME menu injection:** Use `class_addMethod` + `imp_implementationWithBlock` to register menu actions on `IMKInputSessionController` at runtime. No `@objc` selector exposure is needed.
- [ ] **Memory metrics:** Read anonymous private memory via `task_vm_info.internal` (not `phys_footprint` — the latter includes GPU/neural engine shared memory on Apple Silicon). Call `purgeMallocZones()` before reading to flush allocator caches.

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

### 3. Use MRC and Parity-Based Session Pool

**Compile your IMKInputSessionController subclass with `-fno-objc-arc`.** ARC's retain/release churn on the MainActor during CapsLock switching causes perceptible stutter. Disabling ARC eliminates the stutter completely, but requires you to handle cleanup manually.

Reverse-engineering of IMKServer across macOS 10.9–27 revealed a fundamental flaw: `_private._controllers` is keyed by client proxy address, and CapsLock toggles create **new** proxies with different addresses. Old controllers become permanently orphaned — `sessionFinished:` never fires for them because IMK only dispatches to the current (new) proxy. Each CapsLock toggle leaks an IMKInputController plus its NSXPCConnection.

**Recommended Architecture (vChewing "Phase 115"):**

- **KVC-based orphaned controller pruning:** Assign a monotonically increasing generation number at init. When `_controllers` exceeds 2 entries, remove the oldest inactive controller via KVC (`[server valueForKeyPath:@"_private._controllers"] removeObjectForKey:oldKey]`).

- **Parity-based double-buffered session pool:** Use `generation % 2` to route controllers to exactly two static `InputSession` singletons (even → session A, odd → session B). This eliminates LRU caching, dynamic allocation, and eviction logic entirely.

- **Delayed dealloc with XPC cleanup:** Schedule a 3-second timer after `deactivateServer:`. On fire: release blocks, call private `+terminateForClient*` on the client wrapper (try `_IPMDServerClientWrapperModern` → `_IPMDServerClientWrapperLegacy` → `IPMDServerClientWrapper` for macOS 27 compatibility), clear mappings. Cancel if reactivated.

- **IME menu injection:** Use `class_addMethod` + `imp_implementationWithBlock` to dynamically build the IME menu without `@objc` selector exposure. Inject live metrics: controller count and anonymous private memory (`task_vm_info.internal`), purging malloc zones before reading.

```swift
// IMKInputSessionController (.m file, compiled with -fno-objc-arc):
// Class-level static blocks dispatch to Swift via raw uintptr_t addresses.
// +IMKSwift_configureWithActivatingServer: etc. set once at startup.

// Swift side — parity-based session routing:
public final class InputSession {
  fileprivate static let sessionEven = InputSession(preallocated: ())
  fileprivate static let sessionOdd  = InputSession(preallocated: ())

  static func session(for ctlAddr: UInt) -> InputSession? {
    let parity = IMKControllerLifetimeTracker.shared().generation(for: ctlAddr) % 2
    return parity == 0 ? sessionEven : sessionOdd
  }
}
```

> **Note:** The parity-based pool is intentionally not keyed by client address. It accepts that CapsLock creates new proxies with different addresses, using the stable controller generation counter instead.

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
