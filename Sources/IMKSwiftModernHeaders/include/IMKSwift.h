// (c) 2026 and onwards The vChewing Project (MIT License).
// ====================
// This code is released under the MIT license (SPDX-License-Identifier: MIT)

// IMKSwift — Modernized InputMethodKit overlay for Swift 6 concurrency.
//
// This header re-declares the InputMethodKit surface with:
//  • @MainActor isolation on every API.
//  • Explicit nullability annotations (_Nullable / _Nonnull).
//  • Concrete ObjC types in lieu of bare `id` where the SDK intends a specific
//    type (NSString, NSAttributedString, NSDictionary, NSArray, NSEvent …).
//
// All enumerations, typedefs, extern constants, and protocols from
// InputMethodKit and Carbon/HIToolbox are re-exported transitively through
// the `#import <InputMethodKit/InputMethodKit.h>` below.
//
// Downstream Swift modules should `import IMKSwift` instead of
// `import InputMethodKit` to pick up these refinements.

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <InputMethodKit/InputMethodKit.h>

// clang-format off

// ==========================================================================
// MARK: - @MainActor Scope for All Declarations Below
// ==========================================================================
//
// The pragma pushes @MainActor onto every ObjC method declared within the
// scope.  Enums, typedefs, and extern constants from the SDK imports above
// are unaffected.

#pragma clang attribute push(                                                  \
    __attribute__((swift_attr("@MainActor"))),                                 \
    apply_to = any(objc_method, objc_property))

// ==========================================================================
#pragma mark - IMKCandidates
// ==========================================================================

@class IMKServer;

@interface IMKCandidates (IMKSwift)

// MARK: Initializers

- (nonnull instancetype)init __attribute__((unavailable("Please use those constructors intentionally exposed to Swift.")));

- (nonnull instancetype)initWithServer:(nonnull IMKServer *)server
                             panelType:(IMKCandidatePanelType)panelType;

- (nonnull instancetype)initWithServer:(nonnull IMKServer *)server
                             panelType:(IMKCandidatePanelType)panelType
                             styleType:(IMKStyleType)style;

// MARK: Panel Configuration

- (IMKCandidatePanelType)panelType;
- (void)setPanelType:(IMKCandidatePanelType)panelType;

// MARK: Visibility

- (void)show:(IMKCandidatesLocationHint)locationHint;
- (void)hide;
- (BOOL)isVisible;

// MARK: Candidate Data

- (void)updateCandidates;

- (void)showAnnotation:(nullable NSAttributedString *)annotationString;

- (void)showSublist:(nonnull NSArray *)candidates
    subListDelegate:(nonnull id)delegate;

- (NSRect)candidateFrame;

// MARK: Appearance

- (void)setSelectionKeys:(nonnull NSArray<NSNumber *> *)keyCodes;
- (nonnull NSArray<NSNumber *> *)selectionKeys;

- (void)setSelectionKeysKeylayout:(nonnull TISInputSourceRef)layout;
- (nonnull TISInputSourceRef)selectionKeysKeylayout;

- (void)setAttributes:(nonnull NSDictionary<NSString *, id> *)attributes;
- (nonnull NSDictionary<NSString *, id> *)attributes;

// MARK: Dismissal

- (void)setDismissesAutomatically:(BOOL)flag;
- (BOOL)dismissesAutomatically;

// MARK: Selection & Identification (10.7+)

- (NSInteger)selectedCandidate API_AVAILABLE(macosx(10.07));

- (void)setCandidateFrameTopLeft:(NSPoint)point API_AVAILABLE(macosx(10.07));

- (void)showChild API_AVAILABLE(macosx(10.07));
- (void)hideChild API_AVAILABLE(macosx(10.07));

- (void)attachChild:(nonnull IMKCandidates *)child
        toCandidate:(NSInteger)candidateIdentifier
               type:(IMKStyleType)theType API_AVAILABLE(macosx(10.07));

- (void)detachChild:(NSInteger)candidateIdentifier API_AVAILABLE(macosx(10.07));

- (void)setCandidateData:(nonnull NSArray *)candidatesArray
    API_AVAILABLE(macosx(10.07));

- (BOOL)selectCandidateWithIdentifier:(NSInteger)candidateIdentifier
    API_AVAILABLE(macosx(10.07));

- (void)selectCandidate:(NSInteger)candidateIdentifier;

- (void)showCandidates API_AVAILABLE(macosx(10.07));

- (NSInteger)candidateStringIdentifier:(nonnull id)candidateString
    API_AVAILABLE(macosx(10.07));

- (nullable NSAttributedString *)selectedCandidateString
    API_AVAILABLE(macosx(10.07));

- (NSInteger)candidateIdentifierAtLineNumber:(NSInteger)lineNumber
    API_AVAILABLE(macosx(10.07));

- (NSInteger)lineNumberForCandidateWithIdentifier:(NSInteger)candidateIdentifier
    API_AVAILABLE(macosx(10.07));

- (void)clearSelection API_AVAILABLE(macosx(10.07));

// MARK: Window Level & Font (10.14+, Force-Exposed)

- (unsigned long long)windowLevel API_AVAILABLE(macosx(10.14));
- (void)setWindowLevel:(unsigned long long)level API_AVAILABLE(macosx(10.14));
- (void)setFontSize:(double)fontSize API_AVAILABLE(macosx(10.14));

@end

// ==========================================================================
#pragma mark - IMKServer
// ==========================================================================

@class IMKInputController;
@protocol IMKServerProxy;

@interface IMKServer (IMKSwift)

- (nonnull instancetype)init __attribute__((unavailable("Please use those constructors intentionally exposed to Swift.")));

- (nonnull instancetype)initWithName:(nonnull NSString *)name
                    bundleIdentifier:(nonnull NSString *)bundleIdentifier;

- (nonnull instancetype)initWithName:(nonnull NSString *)name
                     controllerClass:(nonnull Class)controllerClassID
                       delegateClass:(nullable Class)delegateClassID;

- (nonnull NSBundle *)bundle;

- (BOOL)paletteWillTerminate API_AVAILABLE(macosx(10.07));
- (BOOL)lastKeyEventWasDeadKey API_AVAILABLE(macosx(10.07));

@end

// ==========================================================================
#pragma mark - IMKInputSessionController
// ==========================================================================

@interface IMKInputSessionController : IMKInputController

// MARK: Initializer

- (nonnull instancetype)init __attribute__((unavailable("Please use those constructors intentionally exposed to Swift.")));

- (nonnull instancetype)initWithServer:(nonnull IMKServer *)server
                              delegate:(nullable id)delegate
                                client:(nonnull id<IMKTextInput>)inputClient;

// MARK: Composition

- (void)updateComposition;
- (void)cancelComposition;

- (nonnull NSMutableDictionary *)compositionAttributesAtRange:(NSRange)range;

- (NSRange)selectionRange;
- (NSRange)replacementRange;

- (nonnull NSDictionary *)markForStyle:(NSInteger)style atRange:(NSRange)range;

// MARK: Commands & Palettes

- (void)doCommandBySelector:(nonnull SEL)aSelector
         commandDictionary:(nonnull NSDictionary *)infoDictionary;

- (void)hidePalettes;

- (nullable NSMenu *)menu;

// MARK: Delegate & Server

- (nullable id)delegate;
- (void)setDelegate:(nullable id)newDelegate;

- (nonnull IMKServer *)server;
- (nullable id<IMKTextInput>)client;

// MARK: Lifecycle (10.7+)

- (void)inputControllerWillClose API_AVAILABLE(macosx(10.07));

// MARK: Candidate Callbacks

- (void)annotationSelected:(nullable NSAttributedString *)annotationString
              forCandidate:(nullable NSAttributedString *)candidateString;

- (void)candidateSelectionChanged:(nullable NSAttributedString *)candidateString;

- (void)candidateSelected:(nullable NSAttributedString *)candidateString;

// MARK: IMKStateSetting

- (void)activateServer:(nonnull id<IMKTextInput>)sender;
- (void)deactivateServer:(nonnull id<IMKTextInput>)sender;

- (nullable id)valueForTag:(long)tag client:(nonnull id<IMKTextInput>)sender;
- (void)setValue:(nullable id)value forTag:(long)tag client:(nonnull id<IMKTextInput>)sender;

- (nullable NSDictionary *)modes:(nonnull id<IMKTextInput>)sender;

- (NSUInteger)recognizedEvents:(nonnull id<IMKTextInput>)sender;

- (void)showPreferences:(nullable id<IMKTextInput>)sender;

// MARK: IMKMouseHandling

- (BOOL)mouseDownOnCharacterIndex:(NSUInteger)index
                       coordinate:(NSPoint)point
                     withModifier:(NSUInteger)flags
                 continueTracking:(nonnull BOOL *)keepTracking
                           client:(nonnull id<IMKTextInput>)sender;

- (BOOL)mouseUpOnCharacterIndex:(NSUInteger)index
                     coordinate:(NSPoint)point
                   withModifier:(NSUInteger)flags
                         client:(nonnull id<IMKTextInput>)sender;

- (BOOL)mouseMovedOnCharacterIndex:(NSUInteger)index
                        coordinate:(NSPoint)point
                      withModifier:(NSUInteger)flags
                            client:(nonnull id<IMKTextInput>)sender;

// MARK: IMKServerInput (Informal Protocol)

- (BOOL)inputText:(nonnull NSString *)string
              key:(NSInteger)keyCode
        modifiers:(NSUInteger)flags
           client:(nonnull id<IMKTextInput>)sender;

- (BOOL)inputText:(nonnull NSString *)string client:(nonnull id<IMKTextInput>)sender;

- (BOOL)handleEvent:(nullable NSEvent *)event client:(nonnull id<IMKTextInput>)sender;

- (BOOL)didCommandBySelector:(nonnull SEL)aSelector client:(nonnull id<IMKTextInput>)sender;

- (nullable id)composedString:(nonnull id<IMKTextInput>)sender;

- (nullable NSAttributedString *)originalString:(nonnull id<IMKTextInput>)sender;

- (void)commitComposition:(nonnull id<IMKTextInput>)sender;

- (nullable NSArray *)candidates:(nonnull id<IMKTextInput>)sender;

@end

#pragma clang attribute pop

// clang-format on
