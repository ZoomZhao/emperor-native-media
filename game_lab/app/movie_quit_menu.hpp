#pragma once
#import <AppKit/AppKit.h>

namespace game_lab {

// Cmd+Q is an application-menu key equivalent, not a window key event. An
// NSApplication that has no main menu never consults its delegate: the key
// press is handed to the key window (and to nothing when no window wants it).
// The movie view therefore cannot "swallow" Cmd+Q; it was never routed at all.
// Keep the menu construction here so the App and the regression test use the
// same item, action and key equivalent.
inline NSString* applicationQuitKeyEquivalent() { return @"q"; }

inline NSMenu* makeApplicationMainMenu(NSString* applicationName) {
    NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* applicationItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [mainMenu addItem:applicationItem];
    NSMenu* applicationMenu = [[NSMenu alloc] initWithTitle:applicationName];
    applicationItem.submenu = applicationMenu;
    [applicationMenu addItemWithTitle:[@"关于 " stringByAppendingString:applicationName]
                               action:@selector(orderFrontStandardAboutPanel:)
                        keyEquivalent:@""];
    [applicationMenu addItem:[NSMenuItem separatorItem]];
    // Target stays nil: AppKit walks the responder chain to -[NSApplication terminate:].
    [applicationMenu addItemWithTitle:[@"退出 " stringByAppendingString:applicationName]
                               action:@selector(terminate:)
                        keyEquivalent:applicationQuitKeyEquivalent()];
    return mainMenu;
}

inline void installApplicationMainMenu(NSString* applicationName) {
    NSApp.mainMenu = makeApplicationMainMenu(applicationName);
}

} // namespace game_lab
