// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
static NSString* outputDirectory;
static void emit(NSDictionary* value) {
    NSData* data = [NSJSONSerialization dataWithJSONObject:value options:0 error:nil];
    NSFileHandle* handle = [NSFileHandle fileHandleForWritingAtPath:
        [outputDirectory stringByAppendingPathComponent:@"events.jsonl"]];
    [handle seekToEndOfFile]; [handle writeData:data];
    [handle writeData:[@"\n" dataUsingEncoding:NSUTF8StringEncoding]]; [handle closeFile];
}
@interface InputFixture : NSObject<NSApplicationDelegate>
@property(strong) NSWindow* window;
@property(strong) NSWindow* overlay;
@property(strong) id monitor;
@property NSUInteger actions;
@end
@implementation InputFixture
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(220,210,640,420)
        styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"Codex Authored Input Fixture";
    self.window.releasedWhenClosed = NO;
    self.window.level = CGWindowLevelForKey(kCGCursorWindowLevelKey);
    NSButton* button = [NSButton buttonWithTitle:@"Scoped click test" target:self action:@selector(clicked:)];
    button.frame = NSMakeRect(180,160,260,80); button.accessibilityIdentifier = @"probe-button";
    [self.window.contentView addSubview:button]; [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:NO];
    __weak InputFixture* weak = self;
    self.monitor = [NSEvent addLocalMonitorForEventsMatchingMask:
        NSEventMaskLeftMouseDown|NSEventMaskLeftMouseUp handler:^NSEvent*(NSEvent* event) {
        if (weak) emit(@{@"kind":@"mouse", @"event_type":@(event.type),
            @"window_number":@(event.windowNumber), @"click_count":@(event.clickCount),
            @"location_in_window":@[@(event.locationInWindow.x),@(event.locationInWindow.y)],
            @"flags":@(event.modifierFlags), @"active":@(NSApp.active),
            @"combined_button_down":@(CGEventSourceButtonState(
                kCGEventSourceStateCombinedSessionState,kCGMouseButtonLeft)),
            @"timestamp":@(event.timestamp)});
        return event;
    }];
    [NSTimer scheduledTimerWithTimeInterval:.05 target:self selector:@selector(poll:) userInfo:nil repeats:YES];
    [NSTimer scheduledTimerWithTimeInterval:60 target:self selector:@selector(stop:) userInfo:nil repeats:NO];
    emit(@{@"kind":@"ready", @"window_number":@(self.window.windowNumber)});
}
- (void)clicked:(id)sender {
    (void)sender; ++self.actions;
    emit(@{@"kind":@"action", @"actions":@(self.actions), @"active":@(NSApp.active)});
}
- (void)poll:(NSTimer*)timer {
    (void)timer;
    NSString* command = [NSString stringWithContentsOfFile:
        [outputDirectory stringByAppendingPathComponent:@"command.txt"]
        encoding:NSUTF8StringEncoding error:nil];
    if ([command isEqual:@"stop"]) {[self stop:nil]; return;}
    if ([command isEqual:@"cover"] && !self.overlay) {
        self.overlay = [[NSWindow alloc] initWithContentRect:self.window.frame
            styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];
        self.overlay.releasedWhenClosed = NO;
        self.overlay.level = self.window.level+1;
        [self.overlay orderFront:nil];
        emit(@{@"kind":@"cover",@"window_number":@(self.overlay.windowNumber)});
    }
    if ([command isEqual:@"clear"] && self.overlay) {
        [self.overlay close]; self.overlay = nil; emit(@{@"kind":@"clear"});
    }
}
- (void)stop:(id)sender {
    (void)sender; [NSEvent removeMonitor:self.monitor];
    [self.overlay close]; [self.window close]; [NSApp terminate:nil];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)application {
    (void)application; return YES;
}
@end
int main(int argc, char** argv) { @autoreleasepool {
    if (argc != 2) return 2;
    outputDirectory = [NSString stringWithUTF8String:argv[1]];
    [NSApplication.sharedApplication setActivationPolicy:NSApplicationActivationPolicyRegular];
    InputFixture* delegate = [InputFixture new]; NSApp.delegate = delegate; [NSApp run];
    return 0;
}}
