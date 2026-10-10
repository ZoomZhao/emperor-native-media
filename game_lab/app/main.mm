#import <AppKit/AppKit.h>
#include "av_audio_backend.hpp"
#include "../app_media.hpp"
#include "../emperor_assets.hpp"
#include "../resource_root.hpp"
#include "../audio_policy.hpp"
#include "../image_presentation.hpp"
#include "../app_launch.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <memory>


static NSImage* imageFromPixels(uint32_t width, uint32_t height, const std::vector<uint32_t>& pixels) {
    size_t count = 0;
    if (!game_lab::imagePixelCount(width, height, count) || pixels.size() != count) return nil;
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    if (!space) return nil;
    CGContextRef context = CGBitmapContextCreate(const_cast<uint32_t*>(pixels.data()), width, height,
        8, static_cast<size_t>(width) * 4, space,
        kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(space);
    if (!context) return nil;
    CGImageRef image = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    if (!image) return nil;
    NSImage* result = [[NSImage alloc] initWithCGImage:image size:NSMakeSize(width, height)];
    CGImageRelease(image);
    return result;
}

class ViewImagePresenter final : public game_lab::ImagePresenter {
public:
    explicit ViewImagePresenter(NSImageView* view) : view_(view) {}
    void clear() override { view_.image = nil; }
    bool present(uint32_t width, uint32_t height, const std::vector<uint32_t>& pixels) override {
        if (!view_) return false;
        view_.image = imageFromPixels(width, height, pixels);
        return view_.image != nil;
    }
private:
    NSImageView* __weak view_;
};
static bool presentPixels(NSImageView* view, uint32_t width, uint32_t height,
                          const std::vector<uint32_t>& pixels) {
    ViewImagePresenter presenter(view);
    return game_lab::presentImage(presenter, width, height, pixels) == game_lab::PresentationResult::Presented;
}

@interface EmperorAppDelegate : NSObject <NSApplicationDelegate, NSTableViewDelegate, NSTableViewDataSource> {
    NSWindow* _window;
    NSSegmentedControl* _navBar;
    NSView* _contentContainer;
    
    // Sub-views for each tab
    NSView* _mainMenuView;
    NSView* _mapView;
    NSView* _spriteView;
    NSView* _musicView;
    NSView* _textView;
    
    // Bottom playback bar
    NSTextField* _musicStatusLabel;
    NSButton* _playPauseButton;
    NSButton* _muteButton;
    NSSlider* _volumeSlider;
    
    // Audio Player
    game_lab::AVAudioFactory _audioFactory;
    std::unique_ptr<game_lab::AppMedia> _media;
    
    // Main Menu Assets
    NSImage* _mainMenuImage;
    NSImageView* _mainMenuImageView;
    
    // Maps
    NSArray<NSString*>* _mapFiles;
    NSTableView* _mapTableView;
    NSTextField* _mapInfoLabel;
    NSImageView* _mapCanvasView;
    
    // Sprites & 555
    NSPopUpButton* _spritePackagePopup;
    NSSlider* _spriteSlider;
    NSTextField* _spriteInfoLabel;
    NSImageView* _spriteImageView;
    game_lab::SG3Archive _currentSG3;
    std::string _current555Path;
    
    // Music tracks
    NSArray<NSString*>* _musicTracks;
    NSTableView* _musicTableView;
    NSArray<NSString*>* _sfxFiles;
    
    // Text database
    std::vector<std::string> _chineseTexts;
    NSTableView* _textTableView;
    NSSearchField* _textSearchField;
    NSMutableArray<NSString*>* _filteredTexts;
}
- (instancetype)initWithResourceRoot:(const game_lab::ResourceRoot&)root;
- (void)refreshAudioStatus;
@end

@implementation EmperorAppDelegate

- (instancetype)initWithResourceRoot:(const game_lab::ResourceRoot&)root {
    self = [super init];
    if (self) {
        _media = std::make_unique<game_lab::AppMedia>(root, _audioFactory);
        __weak EmperorAppDelegate* weakSelf = self;
        _media->setStatusChangedCallback([weakSelf](const game_lab::PlaybackStatus&) {
            [weakSelf refreshAudioStatus];
        });
    }
    return self;
}

- (void)dealloc {
    if (_media) _media->setStatusChangedCallback({});
}

- (std::string)resourcePath:(NSString*)relativePath {
    if (!relativePath || !relativePath.UTF8String) return {};
    return _media->resourcePath(relativePath.UTF8String);
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;
    _media->stopAll();
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    [self setupWindow];
    [self setupAudio];
    [self setupMainMenuView];
    [self setupMapView];
    [self setupSpriteView];
    [self setupMusicView];
    [self setupTextView];
    [self showTab:0];
    
    // Launch does not create an audio player or start playback.
}

- (void)setupWindow {
    NSRect frame = NSMakeRect(100, 100, 1024, 820);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.title = @"皇帝媒体资源浏览器 · Native macOS";
    [_window center];
    
    NSView* root = _window.contentView;
    
    // Top Navigation Bar
    _navBar = [NSSegmentedControl segmentedControlWithLabels:@[@"主界面", @"城市地图", @"图素与贴图", @"国风音乐台", @"游戏文本资料库"]
                                                trackingMode:NSSegmentSwitchTrackingSelectOne
                                                      target:self
                                                      action:@selector(onTabChanged:)];
    _navBar.frame = NSMakeRect(12, 775, 1000, 32);
    _navBar.selectedSegment = 0;
    _navBar.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    [root addSubview:_navBar];
    
    // Content Container (1024x720)
    _contentContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 48, 1024, 720)];
    _contentContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [root addSubview:_contentContainer];
    
    // Bottom Playback Bar (1024x48)
    NSView* bottomBar = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1024, 48)];
    bottomBar.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
    bottomBar.wantsLayer = YES;
    bottomBar.layer.backgroundColor = [NSColor colorWithCalibratedWhite:0.12 alpha:1.0].CGColor;
    [root addSubview:bottomBar];
    
    _playPauseButton = [NSButton buttonWithTitle:@"▶ 播放" target:self action:@selector(togglePlayPause:)];
    _playPauseButton.frame = NSMakeRect(12, 10, 80, 28);
    [bottomBar addSubview:_playPauseButton];
    
    _muteButton = [NSButton buttonWithTitle:@"🔇 取消静音" target:self action:@selector(toggleMute:)];
    _muteButton.frame = NSMakeRect(100, 10, 80, 28);
    [bottomBar addSubview:_muteButton];
    
    _volumeSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(190, 14, 120, 20)];
    _volumeSlider.minValue = 0.0;
    _volumeSlider.maxValue = 1.0;
    _volumeSlider.doubleValue = 0.8;
    _volumeSlider.target = self;
    _volumeSlider.action = @selector(onVolumeChanged:);
    [bottomBar addSubview:_volumeSlider];
    
    _musicStatusLabel = [NSTextField labelWithString:@"默认静音；取消静音后点击播放"];
    _musicStatusLabel.frame = NSMakeRect(330, 14, 680, 20);
    _musicStatusLabel.textColor = [NSColor colorWithCalibratedWhite:0.85 alpha:1.0];
    _musicStatusLabel.font = [NSFont systemFontOfSize:13 weight:NSFontWeightMedium];
    [bottomBar addSubview:_musicStatusLabel];
    
    [_window makeKeyAndOrderFront:nil];
}

- (void)setupAudio {
    _musicTracks = @[
        @"MENU1-San_Xuan.mp3",
        @"BG01-Lonely_Er_Hu.mp3",
        @"BG02-Temple_of_Zhong.mp3",
        @"BG03-Happy_Qu_Di.mp3",
        @"BG04-Di_Zi_Feeling.mp3",
        @"BG05-Dance_of_the_Great_Wall.mp3",
        @"BG06-Bo_Luo_Vo_La.mp3",
        @"BG07-Journey_of_the_Gu_Qin.mp3",
        @"BG08-Buddha's_Dream.mp3",
        @"BG09-Guan_Zi_Festival.mp3",
        @"BG10-Pi_Pa_Village.mp3",
        @"BG11-Sunset_Ceremony.mp3",
        @"BG12-Himalayan_Echos.mp3",
        @"MISSION-The_Rising_Sun.mp3",
        @"VIC1-The_Jolly_Emperor.mp3",
        @"VIC2-Village_of_the_Victors.mp3",
        @"BAT1-Battle_of_the_Tang_Gu.MP3",
        @"BAT2-Xiao_Luo_Conflict.mp3",
        @"BAT3-War_of_the_Elephants.MP3",
        @"BAT4-Attack_of_the_Feng_Luo.mp3"
    ];
    
    _sfxFiles = @[
        @"Fire.wav",
        @"D_earthquake.wav",
        @"D_flood.wav",
        @"Halberd_attack.wav",
        @"Pheasant_hit.wav",
        @"H_ancestor_bless.wav",
        @"H_buddhist_bless.wav",
        @"H_daoist_bless.wav",
        @"H_confucian_bless.wav"
    ];
}

- (void)refreshAudioStatus {
    _musicStatusLabel.stringValue = [NSString stringWithUTF8String:_media->status().message.c_str()];
    _playPauseButton.title = _media->musicPlaying() ? @"⏸ 暂停" : @"▶ 播放";
    _muteButton.title = _media->muted() ? @"🔇 取消静音" : @"🔊 静音";
}

- (void)playMusicTrack:(NSString*)relativePath {
    _media->selectMusic(relativePath.UTF8String ? relativePath.UTF8String : "");
}

- (void)playSFX:(NSString*)filename {
    const auto relative = [NSString stringWithFormat:@"Audio/Wavs/%@", filename];
    _media->playSfx(relative.UTF8String ? relative.UTF8String : "");
}

- (void)togglePlayPause:(id)sender {
    (void)sender;
    _media->toggleMusic();
}

- (void)toggleMute:(id)sender {
    (void)sender;
    _media->setMuted(!_media->muted());
}

- (void)onVolumeChanged:(id)sender {
    (void)sender;
    _media->setVolume(_volumeSlider.doubleValue);
}

// ----------------------------------------------------
// Tab 0: Main Menu View
// ----------------------------------------------------
- (void)setupMainMenuView {
    _mainMenuView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _mainMenuView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Load China_FE_MainMenu.jpg
    const auto decoded = _media->jpeg("DATA/China_FE_MainMenu.jpg");
    _mainMenuImageView = [[NSImageView alloc] initWithFrame:_mainMenuView.bounds];
    presentPixels(_mainMenuImageView, decoded.width, decoded.height, decoded.pixelsBGRA);
    _mainMenuImage = _mainMenuImageView.image;
    _mainMenuImageView.imageScaling = NSImageScaleAxesIndependently;
    _mainMenuImageView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [_mainMenuView addSubview:_mainMenuImageView];
    
    // Classical styled menu buttons panel on top of the background
    NSView* menuOverlay = [[NSView alloc] initWithFrame:NSMakeRect(40, 120, 260, 480)];
    menuOverlay.wantsLayer = YES;
    menuOverlay.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.08 green:0.08 blue:0.1 alpha:0.75].CGColor;
    menuOverlay.layer.cornerRadius = 10;
    [_mainMenuView addSubview:menuOverlay];
    
    NSTextField* titleLabel = [NSTextField labelWithString:@"皇帝：龙之崛起"];
    titleLabel.frame = NSMakeRect(20, 420, 220, 36);
    titleLabel.font = [NSFont boldSystemFontOfSize:22];
    titleLabel.textColor = [NSColor colorWithCalibratedRed:0.95 green:0.85 blue:0.4 alpha:1.0];
    [menuOverlay addSubview:titleLabel];
    
    NSTextField* subLabel = [NSTextField labelWithString:@"媒体资源浏览器 · 2026"];
    if (!_mainMenuImage) subLabel.stringValue = @"背景图片不可读取或转换失败";
    subLabel.frame = NSMakeRect(20, 395, 220, 20);
    subLabel.font = [NSFont systemFontOfSize:12];
    subLabel.textColor = [NSColor colorWithCalibratedWhite:0.75 alpha:1.0];
    [menuOverlay addSubview:subLabel];
    
    NSArray* btnConfigs = @[
        @{@"title": @"🏯 浏览城市地图", @"action": NSStringFromSelector(@selector(gotoMaps:))},
        @{@"title": @"🖼 图素与贴图", @"action": NSStringFromSelector(@selector(gotoSprites:))},
        @{@"title": @"🎵 国风民乐鉴赏", @"action": NSStringFromSelector(@selector(gotoMusic:))},
        @{@"title": @"📜 游戏文本与对话", @"action": NSStringFromSelector(@selector(gotoTexts:))},
        @{@"title": @"🔔 播放音效 (祈福)", @"action": NSStringFromSelector(@selector(playBlessSFX:))},
        @{@"title": @"❌ 退出应用", @"action": NSStringFromSelector(@selector(terminate:))}
    ];
    
    CGFloat y = 330;
    for (NSDictionary* cfg in btnConfigs) {
        NSButton* btn = [NSButton buttonWithTitle:cfg[@"title"] target:self action:NSSelectorFromString(cfg[@"action"])];
        btn.frame = NSMakeRect(20, y, 220, 36);
        btn.bezelStyle = NSBezelStyleRounded;
        [menuOverlay addSubview:btn];
        y -= 48;
    }
}

- (void)gotoMaps:(id)sender { (void)sender; [self showTab:1]; }
- (void)gotoSprites:(id)sender { (void)sender; [self showTab:2]; }
- (void)gotoMusic:(id)sender { (void)sender; [self showTab:3]; }
- (void)gotoTexts:(id)sender { (void)sender; [self showTab:4]; }
- (void)playBlessSFX:(id)sender { (void)sender; [self playSFX:@"H_ancestor_bless.wav"]; }
- (void)terminate:(id)sender { (void)sender; [NSApp terminate:nil]; }

// ----------------------------------------------------
// Tab 1: City Maps View
// ----------------------------------------------------
- (void)setupMapView {
    _mapView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _mapView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    _mapFiles = @[
        @"Chang-an Han.map", @"Chang-an Sui.map", @"Chang-an Zhou.map",
        @"Anyang.map", @"Badaling.map", @"Badaling_S.map",
        @"Banpo.map", @"Baoji.map", @"Bo.map", @"Chengdu.map",
        @"ASSR-Baoji.map", @"ASSR-Jiayuguan.map", @"ASSR-Kashgar.map",
        @"ASSR-Liangzhou.map", @"ASSR-LopNor.map", @"ASSR-Loulan.map"
    ];
    
    // Left: Table of maps
    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 20, 300, 680)];
    _mapTableView = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"MapName"];
    col.title = @"原版城市地图 (.map)";
    col.width = 280;
    [_mapTableView addTableColumn:col];
    _mapTableView.delegate = self;
    _mapTableView.dataSource = self;
    scroll.documentView = _mapTableView;
    [_mapView addSubview:scroll];
    
    // Right: Map preview and info
    _mapInfoLabel = [NSTextField labelWithString:@"选择左侧文件以预览地块编号颜色"];
    _mapInfoLabel.frame = NSMakeRect(340, 650, 660, 40);
    _mapInfoLabel.font = [NSFont boldSystemFontOfSize:16];
    [_mapView addSubview:_mapInfoLabel];
    
    _mapCanvasView = [[NSImageView alloc] initWithFrame:NSMakeRect(340, 80, 560, 560)];
    _mapCanvasView.imageScaling = NSImageScaleAxesIndependently;
    _mapCanvasView.wantsLayer = YES;
    _mapCanvasView.layer.borderColor = [NSColor grayColor].CGColor;
    _mapCanvasView.layer.borderWidth = 1;
    [_mapView addSubview:_mapCanvasView];
}

- (void)loadAndRenderMap:(NSString*)filename {
    _mapCanvasView.image = nil;
    if (!filename || !filename.UTF8String) {
        _mapInfoLabel.stringValue = @"未指定地图文件";
        return;
    }
    const auto relative = [NSString stringWithFormat:@"Cities/%@", filename];
    const auto cityMap = _media->cityMap(relative.UTF8String);
    if (!cityMap.valid()) {
        _mapInfoLabel.stringValue = [NSString stringWithFormat:@"无法载入地图: %@", filename];
        return;
    }
    
    // Preview tile IDs with arbitrary colors; this is not terrain simulation.
    std::vector<uint32_t> pixels(128 * 128);
    for (size_t i = 0; i < cityMap.tiles.size(); ++i) {
        uint16_t tileId = cityMap.tiles[i];
        uint8_t r = (tileId * 17) % 256;
        uint8_t g = (tileId * 31 + 40) % 200 + 55;
        uint8_t b = (tileId * 7) % 150 + 40;
        pixels[i] = (255u << 24) | (r << 16) | (g << 8) | b;
    }
    
    if (!presentPixels(_mapCanvasView, 128, 128, pixels)) {
        _mapInfoLabel.stringValue = @"地图图片转换失败";
        return;
    }
    _mapInfoLabel.stringValue = [NSString stringWithFormat:@"地图: %@ · 尺寸: 128×128 (共 %zu 地块) · zlib 解压完成",
                                 filename, cityMap.tiles.size()];
}

// ----------------------------------------------------
// Tab 2: Sprites & 555 Textures View
// ----------------------------------------------------
- (void)setupSpriteView {
    _spriteView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _spriteView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    NSTextField* title = [NSTextField labelWithString:@"图素与贴图浏览器 (SG3 / 555)"];
    title.frame = NSMakeRect(20, 670, 400, 30);
    title.font = [NSFont boldSystemFontOfSize:18];
    [_spriteView addSubview:title];
    
    _spritePackagePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(20, 630, 320, 30) pullsDown:NO];
    [_spritePackagePopup addItemsWithTitles:@[
        @"China_Elevation (372 精灵)",
        @"China_General (2941 精灵)",
        @"China_Bodhidharma (601 精灵)",
        @"China_DEMO1.555 (640×480 全屏贴图)",
        @"China_DEMO2.555 (640×480 全屏贴图)",
        @"China_DEMO3.555 (640×480 全屏贴图)"
    ]];
    _spritePackagePopup.target = self;
    _spritePackagePopup.action = @selector(onSpritePackageChanged:);
    [_spriteView addSubview:_spritePackagePopup];
    
    _spriteSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(360, 634, 400, 24)];
    _spriteSlider.minValue = 0;
    _spriteSlider.maxValue = 100;
    _spriteSlider.target = self;
    _spriteSlider.action = @selector(onSpriteSliderChanged:);
    [_spriteView addSubview:_spriteSlider];
    
    _spriteInfoLabel = [NSTextField labelWithString:@"图素信息: 请选择图素包"];
    _spriteInfoLabel.frame = NSMakeRect(20, 590, 740, 30);
    [_spriteView addSubview:_spriteInfoLabel];
    
    _spriteImageView = [[NSImageView alloc] initWithFrame:NSMakeRect(20, 40, 640, 520)];
    _spriteImageView.imageScaling = NSImageScaleProportionallyUpOrDown;
    _spriteImageView.wantsLayer = YES;
    _spriteImageView.layer.borderColor = [NSColor darkGrayColor].CGColor;
    _spriteImageView.layer.borderWidth = 1;
    [_spriteView addSubview:_spriteImageView];
    
    [self onSpritePackageChanged:nil];
}

- (void)onSpritePackageChanged:(id)sender {
    (void)sender;
    NSInteger sel = _spritePackagePopup.indexOfSelectedItem;
    _currentSG3 = {};
    _current555Path.clear();
    _spriteImageView.image = nil;
    _spriteSlider.enabled = NO;
    _spriteInfoLabel.stringValue = @"资源不可读取或图素布局不受支持";
    if (sel < 0 || sel >= 6) return;
    if (sel >= 3) {
        // Fullscreen demo 555 image
        NSString* demoNames[] = {@"China_DEMO1.555", @"China_DEMO2.555", @"China_DEMO3.555"};
        NSString* name = demoNames[sel - 3];
        const auto relative = [NSString stringWithFormat:@"DATA/%@", name];
        const auto raw = _media->raw555(relative.UTF8String, 640, 480);
        if (raw.valid()) {
            const bool presented = presentPixels(_spriteImageView, raw.width, raw.height, raw.pixelsBGRA);
            _spriteInfoLabel.stringValue = presented ?
                [NSString stringWithFormat:@"贴图: %@ · 尺寸: 640×480 · 格式: RGB555LE", name] :
                @"贴图转换失败";
        }
        _spriteSlider.hidden = YES;
    } else {
        _spriteSlider.hidden = NO;
        NSString* sg3Names[] = {@"China_Elevation", @"China_General", @"China_Bodhidharma"};
        NSString* baseName = sg3Names[sel];
        const auto relativeSg3 = [NSString stringWithFormat:@"DATA/%@.sg3", baseName];
        _current555Path = [NSString stringWithFormat:@"DATA/%@.555", baseName].UTF8String;
        _currentSG3 = _media->sg3(relativeSg3.UTF8String);
        if (!_currentSG3.sprites.empty() && !_current555Path.empty()) {
            _spriteSlider.enabled = YES;
            _spriteSlider.maxValue = _currentSG3.sprites.size() - 1;
            _spriteSlider.doubleValue = 0;
            [self renderSpriteAtIndex:0];
        }
    }
}

- (void)onSpriteSliderChanged:(id)sender {
    (void)sender;
    size_t idx = static_cast<size_t>(_spriteSlider.doubleValue);
    [self renderSpriteAtIndex:idx];
}

- (void)renderSpriteAtIndex:(size_t)idx {
    if (idx >= _currentSG3.sprites.size()) {
        _spriteImageView.image = nil;
        _spriteInfoLabel.stringValue = @"图素序号无效";
        return;
    }
    const auto& s = _currentSG3.sprites[idx];
    const auto extracted = _media->sprite(s, _current555Path);
    if (!presentPixels(_spriteImageView, extracted.width, extracted.height, extracted.pixelsBGRA)) {
        _spriteInfoLabel.stringValue = @"图素布局不受支持或转换失败";
        return;
    }
    
    _spriteInfoLabel.stringValue = [NSString stringWithFormat:@"精灵序号: %zu / %zu · 偏移: %u · 长度: %u · 类型: %u · 尺寸: %dx%d",
                                    idx, _currentSG3.sprites.size(), s.offset, s.length, s.type, s.width, s.height];
}

// ----------------------------------------------------
// Tab 3: Music & SFX Jukebox View
// ----------------------------------------------------
- (void)setupMusicView {
    _musicView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _musicView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Left: Music tracks table
    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 20, 480, 680)];
    _musicTableView = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"MusicName"];
    col.title = @"音乐资源列表 (20 个候选文件)";
    col.width = 460;
    [_musicTableView addTableColumn:col];
    _musicTableView.delegate = self;
    _musicTableView.dataSource = self;
    scroll.documentView = _musicTableView;
    [_musicView addSubview:scroll];
    
    // Right: Sound effects panel
    NSView* sfxPanel = [[NSView alloc] initWithFrame:NSMakeRect(520, 20, 480, 680)];
    sfxPanel.wantsLayer = YES;
    sfxPanel.layer.backgroundColor = [NSColor colorWithCalibratedWhite:0.15 alpha:1.0].CGColor;
    sfxPanel.layer.cornerRadius = 8;
    [_musicView addSubview:sfxPanel];
    
    NSTextField* sfxTitle = [NSTextField labelWithString:@"🔊 音效资源列表 (9 个候选文件)"];
    sfxTitle.frame = NSMakeRect(20, 640, 440, 25);
    sfxTitle.font = [NSFont boldSystemFontOfSize:16];
    sfxTitle.textColor = [NSColor colorWithCalibratedRed:0.95 green:0.85 blue:0.4 alpha:1.0];
    [sfxPanel addSubview:sfxTitle];
    
    CGFloat y = 580;
    for (NSString* sfx in _sfxFiles) {
        NSButton* btn = [NSButton buttonWithTitle:[NSString stringWithFormat:@"▶ 播放音效: %@", sfx]
                                           target:self
                                           action:@selector(onPlaySFXButton:)];
        btn.frame = NSMakeRect(20, y, 440, 32);
        btn.identifier = sfx;
        btn.bezelStyle = NSBezelStyleRounded;
        [sfxPanel addSubview:btn];
        y -= 44;
    }
}

- (void)onPlaySFXButton:(NSButton*)sender {
    [self playSFX:sender.identifier];
}

// ----------------------------------------------------
// Tab 4: Game Text View
// ----------------------------------------------------
- (void)setupTextView {
    _textView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _textView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    _chineseTexts = _media->text("EmperorText.txt");
    _filteredTexts = [NSMutableArray array];
    for (const auto& line : _chineseTexts) {
        [_filteredTexts addObject:[NSString stringWithUTF8String:line.c_str()]];
    }
    
    _textSearchField = [[NSSearchField alloc] initWithFrame:NSMakeRect(20, 675, 400, 30)];
    _textSearchField.placeholderString = @"搜索游戏文本（如：新建、建造、粮食、战役）";
    _textSearchField.target = self;
    _textSearchField.action = @selector(onTextSearch:);
    [_textView addSubview:_textSearchField];
    
    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 20, 984, 645)];
    _textTableView = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"TextLine"];
    col.title = [NSString stringWithFormat:@"原版中文文本库 (共 %zu 条非空文本行 · GB18030 转码)", _chineseTexts.size()];
    col.width = 960;
    [_textTableView addTableColumn:col];
    _textTableView.delegate = self;
    _textTableView.dataSource = self;
    scroll.documentView = _textTableView;
    [_textView addSubview:scroll];
}

- (void)onTextSearch:(NSSearchField*)sender {
    NSString* query = sender.stringValue;
    [_filteredTexts removeAllObjects];
    for (const auto& line : _chineseTexts) {
        NSString* nsLine = [NSString stringWithUTF8String:line.c_str()];
        if (query.length == 0 || [nsLine localizedCaseInsensitiveContainsString:query]) {
            [_filteredTexts addObject:nsLine];
        }
    }
    [_textTableView reloadData];
}

// ----------------------------------------------------
// Tab Switching
// ----------------------------------------------------
- (void)onTabChanged:(NSSegmentedControl*)sender {
    [self showTab:sender.selectedSegment];
}

- (void)showTab:(NSInteger)idx {
    [_navBar setSelectedSegment:idx];
    for (NSView* sub in [_contentContainer.subviews copy]) {
        [sub removeFromSuperview];
    }
    switch (idx) {
        case 0: [_contentContainer addSubview:_mainMenuView]; break;
        case 1: [_contentContainer addSubview:_mapView]; break;
        case 2: [_contentContainer addSubview:_spriteView]; break;
        case 3: [_contentContainer addSubview:_musicView]; break;
        case 4: [_contentContainer addSubview:_textView]; break;
    }
}

// ----------------------------------------------------
// NSTableView DataSource & Delegate
// ----------------------------------------------------
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    if (tableView == _mapTableView) return _mapFiles.count;
    if (tableView == _musicTableView) return _musicTracks.count;
    if (tableView == _textTableView) return _filteredTexts.count;
    return 0;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    (void)tableColumn;
    NSTextField* cell = [tableView makeViewWithIdentifier:@"Cell" owner:self];
    if (!cell) {
        cell = [NSTextField labelWithString:@""];
        cell.identifier = @"Cell";
    }
    if (tableView == _mapTableView) {
        cell.stringValue = [NSString stringWithFormat:@"🏯 %@", _mapFiles[row]];
    } else if (tableView == _musicTableView) {
        cell.stringValue = [NSString stringWithFormat:@"🎵 %@", _musicTracks[row]];
    } else if (tableView == _textTableView) {
        cell.stringValue = [NSString stringWithFormat:@"%4ld: %@", (long)row + 1, _filteredTexts[row]];
    }
    return cell;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSTableView* tv = notification.object;
    NSInteger row = tv.selectedRow;
    if (row < 0) return;
    
    if (tv == _mapTableView) {
        [self loadAndRenderMap:_mapFiles[row]];
    } else if (tv == _musicTableView) {
        NSString* track = _musicTracks[row];
        [self playMusicTrack:[NSString stringWithFormat:@"Audio/Music/%@", track]];
    }
}

@end

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        const char* environment = std::getenv("EMPEROR_GAMEDATA_ROOT");
        std::vector<std::string> arguments;
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        const auto plan = game_lab::prepareLaunch(arguments, environment ? environment : "", [] {
            NSApplication* app = [NSApplication sharedApplication];
            [app setActivationPolicy:NSApplicationActivationPolicyRegular];
            NSOpenPanel* chooser = [NSOpenPanel openPanel];
            chooser.canChooseFiles = NO;
            chooser.canChooseDirectories = YES;
            chooser.allowsMultipleSelection = NO;
            chooser.canCreateDirectories = NO;
            chooser.message = @"请选择包含 DATA、Audio 等子目录的媒体资源目录";
            chooser.prompt = @"选择";
            if ([chooser runModal] != NSModalResponseOK) return std::string{};
            const char* selected = chooser.URL.path.UTF8String;
            return selected ? std::string(selected) : std::string{};
        });
        if (plan.action == game_lab::LaunchAction::Help) {
            std::fprintf(stdout, "Usage: EmperorApp [--data-root /absolute/path/to/GameData]\n"
                                 "Without a root argument or EMPEROR_GAMEDATA_ROOT, choose a resource directory.\n");
            return 0;
        }
        if (plan.action == game_lab::LaunchAction::ArgumentError) {
            std::fprintf(stderr, "Expected --data-root with an absolute resource directory.\n");
            return 2;
        }
        if (plan.action == game_lab::LaunchAction::RootError) {
            std::fprintf(stderr, "Set --data-root or EMPEROR_GAMEDATA_ROOT to a readable absolute directory.\n");
            return 2;
        }
        if (plan.action == game_lab::LaunchAction::Cancelled) return 0;
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        EmperorAppDelegate* delegate = [[EmperorAppDelegate alloc] initWithResourceRoot:plan.root];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
