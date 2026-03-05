# IMKSwift

A modernized **InputMethodKit** overlay for Swift 6 and later, providing safe, concurrent, and type-safe APIs for building input method engines on macOS.

## Overview

IMKSwift is part of the [vChewing Project](https://github.com/vChewing) and offers a Swift-native replacement for Apple's InputMethodKit framework. It combines Objective-C interoperability with Swift 6's strict concurrency model, delivering `@MainActor`-isolated APIs that are easier to work with in modern Swift code.

Instead of struggling with unsafe casts, bare `id` types, and implicit global state, IMKSwift provides:

- **Explicit `@MainActor` isolation** on every API
- **Concrete, nullable-annotated types** instead of bare `id` pointers
- **Full Swift 6 concurrency support**
- **Complete InputMethodKit surface** with refinements for safe usage

## Features

### Type Safety & Nullability
All APIs include explicit nullability annotations (`_Nullable`, `_Nonnull`) and use concrete Objective-C types (`NSString`, `NSAttributedString`, `NSDictionary`, `NSArray`, `NSEvent`, etc.) instead of generic `id`.

### MainActor Isolation
Every method and property is marked with `@MainActor`, ensuring call-site safety at compile time and preventing race conditions in concurrent code.

### Complete InputMethodKit Coverage
IMKSwift re-exports and enhances the following InputMethodKit components:

- **IMKCandidates** — Candidate panel management and display
- **IMKServer** — Input method session server
- **IMKInputController** — Input method event handling and composition
- **IMKTextInput** — Text editing client protocols
- **Supporting protocols** — IMKStateSetting, IMKMouseHandling, IMKServerInput

### Swift 6 Ready
Built with Swift 6's strict concurrency model in mind. All APIs are properly isolated and can be used in concurrent contexts without data races.

## Requirements

- **Swift** 6.2 or later
- **macOS** depending on the Swift version you are using.
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

class MyInputMethodController: IMKInputController {
  override func handle(_ event: NSEvent?, client sender: some IMKTextInput) -> Bool {
    // Event handling with full type safety
    guard let event else { return false }
    
    // Process input...
    return true
  }
  
  override func inputText(_ string: String, client sender: some IMKTextInput) -> Bool {
    // Text input handling
    return true
  }
  
  override func candidates(_ sender: some IMKTextInput) -> [Any]? {
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
  panelType: .horizontal,
  styleType: .default
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
commitComposition(client)
```

## Architecture

### Structure

- **IMKSwift** — Main Swift library with protocol definitions and extensions
- **IMKSwiftModernHeaders** — Modernized Objective-C headers with `@MainActor` annotations and type refinements

### Extension Pattern

All InputMethodKit classes are extended via category in the Objective-C headers and re-exported through protocol conformance in Swift. This maintains full API compatibility while adding type safety and concurrency guarantees.

## API Documentation

### Core Protocols

#### IMKInputSessionControllerProtocol

A main-actor-isolated super-protocol that encompasses:
- `IMKStateSetting` — Activation, deactivation, preferences
- `IMKInputController` — Event handling, composition, candidate management
- `IMKMouseHandling` — Mouse event handling
- `IMKServerInput` — Text and event input

Downstream code should subclass `IMKInputSessionController` (not just `IMKInputController`) to pick up these refinements.

### Key Methods

**State Management:**
- `activateServer(_:)` — Activate the input method
- `deactivateServer(_:)` — Deactivate the input method
- `showPreferences(_:)` — Display preferences dialog

**Text & Composition:**
- `inputText(_:key:modifiers:client:)` — Handle keyboard input
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
| **Candidate Panel** | Manual setup | Simplified API |

## Related Projects

This library is part of the [vChewing Project](https://github.com/vChewing), a modern, open-source input method framework for macOS.

## License

```
// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)
```

See [LICENSE](LICENSE) for details.
