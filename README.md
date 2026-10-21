# IMKSwift

A modernized **InputMethodKit** overlay for Swift 6 and later, providing safe, concurrent, and type-safe APIs for building input method engines on macOS.

## Overview

IMKSwift is part of the [vChewing Project](https://github.com/vChewing) and offers a Swift-native overlay for Apple's InputMethodKit framework. It does two things at once.

First, it re-declares IMK's Objective-C surface in Swift 6 terms: concrete types instead of `id`, explicit nullability, `@MainActor` isolation on every entry point. Second, it repairs the controller-lifetime defects that IMKServer has carried since 10.5 — defects that no header rewrite can reach, because they live in IMKServer's private state rather than in its declarations.

InputMethodKit dates back to macOS 10.5 Leopard—predating ARC, Sandboxing, and Swift itself. It is a legacy framework spanning two generations of technology shifts. IMKSwift bridges this gap, allowing modern Swift developers to build input methods without wrestling with ancient Objective-C patterns.

Instead of struggling with unsafe casts, bare `id` types, and implicit global state, IMKSwift provides:

- **Explicit `@MainActor` isolation** on every API
- **Concrete, nullable-annotated types** instead of bare `id` pointers
- **Full Swift 6 concurrency support**
- **Complete InputMethodKit surface** with refinements for safe usage
- **A repaired controller lifetime** built with `-fno-objc-arc`: address-keyed liveness tracking, orphan pruning, deferred teardown, client-wrapper termination, and four diagnostics-only kill switches

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

### Controller Lifetime Repair

The package also ships an Objective-C layer (`IMKSwiftModernHeaders`, compiled with `-fno-objc-arc`) that addresses what no header rewrite can:

- every live controller is tracked by raw address in `IMKControllerLifetimeTracker`;
- orphaned controllers are pruned out of IMKServer's private `_controllers` dictionary, and the client wrappers they hold are terminated;
- controller teardown is deferred by three seconds after `deactivateServer:`, so rapid CapsLock toggling does not destroy a connection that is about to be reused;
- four diagnostics-only switches and a stable log grammar make field incidents bisectable without shipping a custom build.

### Swift 6 Ready
Built with Swift 6's strict concurrency model in mind. All APIs are properly isolated and can be used in concurrent contexts without data races.

## Requirements

- **Swift** 6.2 or newer
- **Xcode** 16.0 or newer (26.x is what the current private-surface headers are exercised against)
- **macOS**:
    - macOS 10.13 High Sierra or newer (this is the Swift Concurrency runtime floor; the exact floor depends on the Swift version).
    - The code itself runs on macOS 10.9 Mavericks when built against a matching SDK with libARCLite.

## Installation

### Swift Package Manager

Add IMKSwift to your `Package.swift`:

```swift
.package(url: "https://github.com/vChewing/IMKSwift.git", from: "26.03.07"),
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

### 3. Don't Hold Strong References in Your Controller — and Re-Bind the Client on Every Event

Two rules, both learned the hard way.

**Rule one: your `IMKInputSessionController` subclass must not strongly hold business-logic objects.** Every CapsLock toggle or client switch may create a new controller copy; the ARC churn of tearing down a large object graph on each toggle shows up as input latency.

**Rule two: never treat the client address captured at activation as permanent.** IMKServer keys its private controller table by the client proxy's address, and the system hands you a fresh proxy whenever the client context churns. A session that remembers "my client is 0x…9d0" keeps dispatching to a controller that has already retired. The symptom is quiet: the mode-switch feedback still appears, but keystrokes never arrive.

Dispatch sessions from a small pool keyed by the controller's own address, and re-adopt the client on every event:

```swift
@MainActor
final class SessionPool {
  static let shared = SessionPool(capacity: 4)
  private var slots: [UInt: InputSession] = [:]

  func session(for ctlAddr: UInt) -> InputSession { /* ... */ }
  func reap(using tracker: IMKControllerLifetimeTracker) { /* drop slots whose controller is gone */ }
}

// In the event callback, before dispatching:
session.adoptClientAddr(ctlAddr)  // the sender is the authority, not the activation record
```

Two corollaries worth internalizing:

- **Do not wait for `-dealloc`.** IMKServer orphans controllers without ever releasing them, and `sessionFinished:` is dispatched per client proxy, so it never reaches the orphan. Reap sessions by asking the lifetime tracker whether the controller address is still alive.
- **Do not kill the input method process before its bundle has been replaced.** IMK relaunches the IME on demand from the registered bundle; terminating it mid-install only brings the old code back under a new PID. Capture, terminate, escalate to `SIGKILL`, then verify — after the swap (see §7).

The retired-copy hazard is worth one concrete note: if two controllers ever share a session slot, and the first one retires, the slot's client binding is cleared while the second keeps dispatching through it. That is exactly the failure mode of a parity-based double buffer, and it is why the pool is keyed by address and the binding is refreshed per event.

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

### 7. Treat IMKServer's Private Surface as a Black Box with Observation Points

You cannot fix IMKServer, and you cannot debug it with breakpoints. What you *can* do is make its behaviour observable, and make each of your own interventions individually switchable.

IMKSwift's Objective-C layer therefore emits a small, stable vocabulary:

| Token | Meaning |
|---|---|
| `ControllerInit:` | a controller copy was created, with its address and generation |
| `Prune:` | the private `_controllers` table was inspected, or an orphan was evicted |
| `ActivateServer:` / `DeactivateServer:` | client activation and retirement |
| `DelayedDealloc:` | deferred teardown scheduled, skipped, or fired |
| `EventEntry:` | which controller a key event was delivered to, and whether its client was still alive |
| `guard fail:` | the event reached the IME, but no usable client proxy existed |

On top of that, four diagnostics-only switches — installed through class methods, and wired by vChewing to its DevZone pane — let each intervention be disabled in isolation:

| Switch | Disables |
|---|---|
| `configureServerControllerPruningDisabled(_:)` | the KVC prune |
| `configureEvictedClientInvalidationDisabled(_:)` | invalidating the evicted controller's client entry (only the aggressive half of the eviction) |
| `configureClientWrapperTerminationDisabled(_:)` | `+terminateForClient*` on the wrapper, keeping the dealloc bookkeeping |
| `configureControllerDelayedDeallocationDisabled(_:)` | the whole deferred teardown |

Each block is evaluated at the decision point and never cached, so flipping the corresponding `_IMKSwift_disable…` preference of a running process takes effect on the next occasion.

The sixteen raw combinations collapse to nine distinguishable behaviours, because two of the switches are dominated by others. When a user reports "the modifier key stopped switching after a screenshot tool ran", you ask for those log tokens plus the four switch states; the log then tells you whether the event was never delivered, was delivered to a dead client, or was delivered to the wrong client — three different bugs with three different fixes, and no way to tell them apart without instrumentation.

## Architecture

### Structure

- **IMKSwift** — Main Swift library: protocol definitions, `@MainActor` extensions, and the Swift-side session plumbing
- **IMKSwiftModernHeaders** — Modernized Objective-C headers, plus the MRC (`-fno-objc-arc`) private-surface layer: `IMKSwift.m` (hooks for pruning, delayed deallocation, and client-wrapper termination) and `IMKControllerLifetimeTracker.m` (address liveness and generation bookkeeping)
- **Consumer side** — your `IMKInputSessionController` subclass, plus the session pool that survives address churn

### Three Layers, Two Languages

- **Declarations.** `IMKInputSessionController` is a concrete subclass of `IMKInputController` that exists solely to work around Swift/Objective-C interoperability limitations. The original SDK headers expose APIs without `@MainActor` isolation and cannot be redeclared without collisions, so a differently-named subclass is the only workable answer. It is `@MainActor`-annotated through `#pragma clang attribute`, and mirrors its full surface in the Swift-native `IMKInputSessionControllerProtocol`.
- **Repairs.** Because the defects live in IMKServer's private state, the fixes have to be Objective-C: KVC access to `_private._controllers`, generation-bookkept pruning, a dealloc-time hook, and `+terminateForClient*` calls on the client wrapper classes. This code runs inside your IME process, compiled with `-fno-objc-arc` so that the retain/release churn of controller copies does not surface as input latency.
- **Policy.** Which session each controller receives, when a session is rebound to a client, and when a session is retired are decisions your input method makes. IMKSwift supplies the addresses, the liveness checks, and the hook points; it deliberately does not own your session model.

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
| **Controller Lifetime** | Leaks an orphaned controller per CapsLock toggle; `sessionFinished:` never arrives | Address-keyed tracking, KVC pruning, deferred teardown, wrapper termination |
| **Client Binding** | Implicit, and stale after proxy churn | Re-established on every event |
| **Observability** | Silent | Stable log vocabulary plus four diagnostics-only switches |

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
