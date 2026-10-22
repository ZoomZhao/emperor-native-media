#import <AppKit/AppKit.h>
#include "game_lab/app/movie_quit_menu.hpp"
#include "game_lab/app_media.hpp"
#include "game_lab/movie_session.hpp"
#include "stream_lab/movie_io.hpp"
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <mach-o/dyld.h>

// Regression: Cmd+Q while a movie is playing must terminate the App.
// Cmd+Q is an application-menu key equivalent; before the fix the App had no
// main menu, so NSApplication never asked its delegate and never ran the movie
// termination contract at all. The same item, action and key equivalent used by
// the App are built by game_lab::makeApplicationMainMenu.
using namespace game_lab;
using namespace std::chrono;

static size_t checks = 0;
static void need(bool value, const char* why) {
    ++checks;
    if (!value) throw std::runtime_error(why);
}

class NoAudio : public game_lab::AudioFactory {
public:
    std::unique_ptr<game_lab::AudioPlayback> create(const std::vector<uint8_t>&) override {
        throw std::runtime_error("device audio requested");
    }
};

static std::string currentExecutable() {
    char raw[4096];
    uint32_t size = sizeof(raw);
    if (_NSGetExecutablePath(raw, &size) != 0) return {};
    char resolved[4096];
    if (::realpath(raw, resolved)) return resolved;
    return raw;
}

static NSEvent* quitKeyEvent(NSString* characters = @"q",
                            NSEventModifierFlags flags = NSEventModifierFlagCommand) {
    return [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
                      modifierFlags:flags timestamp:0 windowNumber:0 context:nil
                           characters:characters charactersIgnoringModifiers:characters
                             isARepeat:NO keyCode:12];
}

@interface QuitProbe : NSObject<NSApplicationDelegate>
@property(nonatomic) NSInteger asked;
@end
@implementation QuitProbe
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    (void)sender;
    _asked += 1;
    return NSTerminateCancel;
}
@end

// ---------------------------------------------------------------- child process
struct ChildState {
    std::unique_ptr<game_lab::MovieSession> session;
    std::shared_ptr<const game_lab::MovieSessionStats> stats;
    bool playing = false;
    int asked = 0;
};

@interface ChildDelegate : NSObject<NSApplicationDelegate>
@property(nonatomic) ChildState* state;
@end

@implementation ChildDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    ChildState* state = self.state;
    state->asked += 1;
    if (!state->playing) {
        std::fprintf(stdout, "quit-before-playback=1\n");
        std::fflush(stdout);
        return NSTerminateCancel;
    }
    // Mirror the App: request the shared bounded termination contract, reply
    // only after its result, and never block the main thread while doing it.
    const auto status = state->session->requestTermination(5000, [state, sender](const TerminationResult& r) {
        const bool permits = r.permitsExit();
        const bool zero = state->stats->resourcesZero.load();
        const bool idle = state->stats->workerFinished.load() && state->stats->active == 0 &&
                          state->stats->pending == 0 && state->stats->delivery == 0;
        const bool noController = !state->stats->hasController.load();
        std::fprintf(stdout, "quit-result permits_exit=%d resources_zero=%d worker_idle=%d no_controller=%d\n",
                     permits ? 1 : 0, zero ? 1 : 0, idle ? 1 : 0, noController ? 1 : 0);
        std::fflush(stdout);
        [sender replyToApplicationShouldTerminate:(permits && zero && idle && noController) ? YES : NO];
    });
    std::fprintf(stdout, "quit-request-pending=%d\n", status == TerminationStatus::Pending ? 1 : 0);
    std::fflush(stdout);
    return NSTerminateLater;
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;
    std::fprintf(stdout, "will-terminate=1\n");
    std::fflush(stdout);
    self.state->session.reset();
}
@end

static int runQuitChild(bool withMenu, const std::string& root) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        if (withMenu) game_lab::installApplicationMainMenu(@"QuitChild");
        else app.mainMenu = nil;
        auto state = std::make_unique<ChildState>();
        ChildDelegate* delegate = [[ChildDelegate alloc] init];
        delegate.state = state.get();
        app.delegate = delegate;
        NoAudio audio;
        game_lab::AppMedia media(game_lab::ResourceRoot::fromAbsolute(root), audio);
        state->session = media.createMovieSession();
        state->stats = state->session->stats();
        state->session->openAndPlay("Movies/own.mov", [state = state.get()](const game_lab::MovieDelivery& delivery) {
            if (!delivery.accepted) return;
            const auto& snapshot = delivery.snapshot;
            if (snapshot.state != media_demo::State::Playing || snapshot.readers != 1) return;
            if (state->playing) return;
            state->playing = true;
            std::fprintf(stdout, "playback-live=1 reads=%zu\n", size_t(snapshot.readers));
            std::fflush(stdout);
            // A real key press arrives through the event queue and is matched
            // against the main menu before any window sees it.
            [NSApp postEvent:quitKeyEvent() atStart:NO];
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 4 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            std::fprintf(stdout, "watchdog=1 still-running\n");
            std::fflush(stdout);
            _exit(4);
        });
        [app run];
        if (state->asked == 0) {
            std::fprintf(stdout, "delegate-never-asked=1\n");
            std::fflush(stdout);
            return 5;
        }
        return 0;
    }
}

struct ChildRun {
    bool exited = false;
    bool timedOut = false;
    int code = -1;
    std::string output;
};

static ChildRun spawnChild(const std::string& executable, const std::vector<std::string>& args,
                           const std::filesystem::path& log, double seconds) {
    ChildRun run;
    const int fd = ::open(log.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (fd < 0) throw std::runtime_error("cannot create child log");
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(executable.c_str()));
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    const pid_t pid = ::fork();
    if (pid == 0) {
        ::dup2(fd, STDOUT_FILENO);
        ::dup2(fd, STDERR_FILENO);
        ::close(fd);
        ::execv(executable.c_str(), argv.data());
        ::_exit(127);
    }
    ::close(fd);
    if (pid < 0) throw std::runtime_error("cannot fork quit child");
    const auto start = steady_clock::now();
    int status = 0;
    for (;;) {
        const pid_t done = ::waitpid(pid, &status, WNOHANG);
        if (done == pid) { run.exited = true; break; }
        if (duration<double>(steady_clock::now() - start).count() > seconds) {
            ::kill(pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            run.timedOut = true;
            break;
        }
        std::this_thread::sleep_for(milliseconds(10));
    }
    if (!run.timedOut) {
        if (WIFEXITED(status)) run.code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) run.code = 128 + WTERMSIG(status);
    }
    std::ifstream in(log);
    run.output.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return run;
}

int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            if (argc == 4 && std::string(argv[1]) == "--quit-child")
                return runQuitChild(std::string(argv[2]) == "menu", argv[3]);
            need(argc == 2, "fresh fixture directory argument");
            std::filesystem::path fixtures(argv[1]);
            need(fixtures.is_absolute(), "fixture path must be absolute");
            std::filesystem::create_directories(fixtures / "quit-root" / "Movies");
            stream_lab::writeSyntheticMovie((fixtures / "quit-root" / "Movies/own.mov").string());

            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

            // The App's own menu construction must bind Cmd+Q to terminate:.
            NSMenu* mainMenu = game_lab::makeApplicationMainMenu(@"QuitTest");
            need(mainMenu != nil && mainMenu.numberOfItems >= 1, "no application main menu item");
            NSMenu* appMenu = mainMenu.itemArray.firstObject.submenu;
            need(appMenu != nil, "application menu item has no submenu");
            NSMenuItem* quitItem = nil;
            for (NSMenuItem* item in appMenu.itemArray)
                if (item.action == @selector(terminate:)) { quitItem = item; break; }
            need(quitItem != nil, "application menu has no terminate: item");
            need([quitItem.keyEquivalent isEqualToString:game_lab::applicationQuitKeyEquivalent()],
                 "quit key equivalent is not q");
            need(quitItem.keyEquivalentModifierMask == NSEventModifierFlagCommand,
                 "quit key equivalent is not Command-only");

            // Baseline: the pre-fix App had no main menu. Cmd+Q is then not a key
            // equivalent at all and the delegate is never asked to terminate.
            QuitProbe* probe = [[QuitProbe alloc] init];
            NSApp.delegate = probe;
            NSApp.mainMenu = nil;
            [NSApp sendEvent:quitKeyEvent()];
            need(probe.asked == 0, "Cmd+Q reached the delegate without a main menu");

            // Installed menu: the same event now enters the App termination path.
            game_lab::installApplicationMainMenu(@"QuitTest");
            need(NSApp.mainMenu != nil && NSApp.mainMenu.numberOfItems >= 1, "menu not installed");
            [NSApp sendEvent:quitKeyEvent()];
            need(probe.asked == 1, "Cmd+Q did not reach applicationShouldTerminate:");
            [NSApp sendEvent:quitKeyEvent(@"q", 0)];
            need(probe.asked == 1, "plain q was treated as quit");

            // End-to-end: a live synthetic movie, a real Cmd+Q event through the
            // event queue, the production menu, the shared termination contract
            // and a real AppKit reply that ends the process.
            std::string executable = currentExecutable();
            need(!executable.empty(), "cannot resolve the test executable");
            const auto root = (fixtures / "quit-root").string();
            const auto baseline = spawnChild(executable, {"--quit-child", "baseline", root},
                                             fixtures / "quit-baseline.log", 20);
            need(baseline.exited && !baseline.timedOut, "baseline child did not stop");
            need(baseline.code == 4, "pre-fix configuration unexpectedly quit on Cmd+Q");
            need(baseline.output.find("playback-live=1") != std::string::npos,
                 "baseline child was not playing before Cmd+Q");
            need(baseline.output.find("quit-before-playback=1") == std::string::npos,
                 "baseline Cmd+Q was routed with a live movie");
            need(baseline.output.find("quit-result") == std::string::npos,
                 "baseline termination contract ran without a menu");
            need(baseline.output.find("watchdog=1") != std::string::npos,
                 "baseline child was not stopped by the bounded watchdog");

            const auto start = steady_clock::now();
            const auto fixed = spawnChild(executable, {"--quit-child", "menu", root},
                                          fixtures / "quit-menu.log", 20);
            const auto elapsed = duration<double>(steady_clock::now() - start).count();
            need(fixed.exited && !fixed.timedOut, "fixed child did not exit");
            need(fixed.code == 0, "fixed child did not exit through AppKit termination");
            need(fixed.output.find("playback-live=1") != std::string::npos, "fixed child never played");
            need(fixed.output.find("quit-request-pending=1") != std::string::npos,
                 "Cmd+Q did not start the asynchronous movie termination");
            need(fixed.output.find("quit-result permits_exit=1 resources_zero=1 worker_idle=1 no_controller=1") !=
                     std::string::npos,
                 "movie termination did not confirm clean audio/video release");
            need(fixed.output.find("will-terminate=1") != std::string::npos,
                 "applicationWillTerminate was not reached");
            need(elapsed < 10.0, "quit took longer than the bounded deadline");

            std::ofstream(fixtures / "movie-quit-observations.json")
                << "{\"checks\":" << checks
                << ",\"baseline_exit\":" << baseline.code
                << ",\"fixed_exit\":" << fixed.code
                << ",\"fixed_elapsed_seconds\":" << elapsed
                << ",\"cmd_q_routed_without_menu\":false"
                << ",\"cmd_q_routed_with_menu\":true"
                << ",\"live_playback_termination\":true}\n";
            std::cout << "{\"passed\":true,\"checks\":" << checks
                      << ",\"UI\":false,\"audio_devices\":false,\"app_launched\":false"
                      << ",\"live_playback_termination\":true}\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << "\n";
            return 1;
        }
    }
}
