#import <AppKit/AppKit.h>
#import <AVFoundation/AVFoundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "../emperor_assets.hpp"
#include <vector>
#include <string>
#include <memory>

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
    AVAudioPlayer* _musicPlayer;
    AVAudioPlayer* _sfxPlayer;
    BOOL _isMuted;
    
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
@end

@implementation EmperorAppDelegate

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
    
    // Start background music
    [self playMusicTrack:@"GameData/Audio/Music/MENU1-San_Xuan.mp3"];
}

- (void)setupWindow {
    NSRect frame = NSMakeRect(100, 100, 1024, 820);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.title = @"皇帝：龙之崛起 (Emperor: Rise of the Middle Kingdom) · Native macOS";
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
    
    _playPauseButton = [NSButton buttonWithTitle:@"⏸ 暂停" target:self action:@selector(togglePlayPause:)];
    _playPauseButton.frame = NSMakeRect(12, 10, 80, 28);
    [bottomBar addSubview:_playPauseButton];
    
    _muteButton = [NSButton buttonWithTitle:@"🔊 静音" target:self action:@selector(toggleMute:)];
    _muteButton.frame = NSMakeRect(100, 10, 80, 28);
    [bottomBar addSubview:_muteButton];
    
    _volumeSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(190, 14, 120, 20)];
    _volumeSlider.minValue = 0.0;
    _volumeSlider.maxValue = 1.0;
    _volumeSlider.doubleValue = 0.8;
    _volumeSlider.target = self;
    _volumeSlider.action = @selector(onVolumeChanged:);
    [bottomBar addSubview:_volumeSlider];
    
    _musicStatusLabel = [NSTextField labelWithString:@"♫ 正在载入背景音乐..."];
    _musicStatusLabel.frame = NSMakeRect(330, 14, 680, 20);
    _musicStatusLabel.textColor = [NSColor colorWithCalibratedWhite:0.85 alpha:1.0];
    _musicStatusLabel.font = [NSFont systemFontOfSize:13 weight:NSFontWeightMedium];
    [bottomBar addSubview:_musicStatusLabel];
    
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)setupAudio {
    _isMuted = NO;
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

- (void)playMusicTrack:(NSString*)relativePath {
    NSString* fullPath = [NSString stringWithUTF8String:relativePath.UTF8String];
    NSURL* url = [NSURL fileURLWithPath:fullPath];
    NSError* error = nil;
    _musicPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
    if (_musicPlayer) {
        _musicPlayer.numberOfLoops = -1; // Infinite loop
        _musicPlayer.volume = _isMuted ? 0.0 : _volumeSlider.doubleValue;
        [_musicPlayer play];
        NSString* filename = [relativePath lastPathComponent];
        _musicStatusLabel.stringValue = [NSString stringWithFormat:@"♫ 正在播放: %@ (时长: %.0f 秒)", filename, _musicPlayer.duration];
        _playPauseButton.title = @"⏸ 暂停";
    } else {
        _musicStatusLabel.stringValue = [NSString stringWithFormat:@"无法播放音频: %@", error.localizedDescription];
    }
}

- (void)playSFX:(NSString*)filename {
    NSString* fullPath = [NSString stringWithFormat:@"GameData/Audio/Wavs/%@", filename];
    NSURL* url = [NSURL fileURLWithPath:fullPath];
    NSError* error = nil;
    _sfxPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
    if (_sfxPlayer) {
        _sfxPlayer.volume = _volumeSlider.doubleValue;
        [_sfxPlayer play];
    }
}

- (void)togglePlayPause:(id)sender {
    (void)sender;
    if (!_musicPlayer) return;
    if (_musicPlayer.isPlaying) {
        [_musicPlayer pause];
        _playPauseButton.title = @"▶ 播放";
    } else {
        [_musicPlayer play];
        _playPauseButton.title = @"⏸ 暂停";
    }
}

- (void)toggleMute:(id)sender {
    (void)sender;
    _isMuted = !_isMuted;
    if (_musicPlayer) {
        _musicPlayer.volume = _isMuted ? 0.0 : _volumeSlider.doubleValue;
    }
    _muteButton.title = _isMuted ? @"🔇 取消静音" : @"🔊 静音";
}

- (void)onVolumeChanged:(id)sender {
    (void)sender;
    if (_musicPlayer && !_isMuted) {
        _musicPlayer.volume = _volumeSlider.doubleValue;
    }
}

// ----------------------------------------------------
// Tab 0: Main Menu View
// ----------------------------------------------------
- (void)setupMainMenuView {
    _mainMenuView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _mainMenuView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Load China_FE_MainMenu.jpg
    const auto decoded = game_lab::loadJPEG("GameData/DATA/China_FE_MainMenu.jpg");
    if (decoded.valid()) {
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(
            (void*)decoded.pixelsBGRA.data(),
            decoded.width, decoded.height, 8, decoded.width * 4,
            cs, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst
        );
        CGImageRef cgImg = CGBitmapContextCreateImage(ctx);
        _mainMenuImage = [[NSImage alloc] initWithCGImage:cgImg size:NSMakeSize(decoded.width, decoded.height)];
        CGImageRelease(cgImg);
        CGContextRelease(ctx);
        CGColorSpaceRelease(cs);
    }
    
    _mainMenuImageView = [[NSImageView alloc] initWithFrame:_mainMenuView.bounds];
    _mainMenuImageView.image = _mainMenuImage;
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
    
    NSTextField* subLabel = [NSTextField labelWithString:@"Native Apple Engine · 2026"];
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
    _mapInfoLabel = [NSTextField labelWithString:@"选择左侧地图文件以解压并预览地形"];
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
    std::string path = "GameData/Cities/" + std::string(filename.UTF8String);
    const auto cityMap = game_lab::loadCityMap(path);
    if (!cityMap.valid()) {
        _mapInfoLabel.stringValue = [NSString stringWithFormat:@"无法载入地图: %@", filename];
        return;
    }
    
    _mapInfoLabel.stringValue = [NSString stringWithFormat:@"地图: %@ · 尺寸: 128×128 (共 %zu 地块) · zlib 解压完成",
                                 filename, cityMap.tiles.size()];
    
    // Render 128x128 tile elevation into BGRA8 image
    std::vector<uint32_t> pixels(128 * 128);
    for (size_t i = 0; i < cityMap.tiles.size(); ++i) {
        uint16_t tileId = cityMap.tiles[i];
        uint8_t r = (tileId * 17) % 256;
        uint8_t g = (tileId * 31 + 40) % 200 + 55;
        uint8_t b = (tileId * 7) % 150 + 40;
        pixels[i] = (255u << 24) | (r << 16) | (g << 8) | b;
    }
    
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        pixels.data(), 128, 128, 8, 128 * 4,
        cs, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst
    );
    CGImageRef cgImg = CGBitmapContextCreateImage(ctx);
    _mapCanvasView.image = [[NSImage alloc] initWithCGImage:cgImg size:NSMakeSize(128, 128)];
    CGImageRelease(cgImg);
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
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
    if (sel >= 3) {
        // Fullscreen demo 555 image
        NSString* demoNames[] = {@"China_DEMO1.555", @"China_DEMO2.555", @"China_DEMO3.555"};
        NSString* name = demoNames[sel - 3];
        std::string path = "GameData/DATA/" + std::string(name.UTF8String);
        const auto raw = game_lab::loadRaw555(path, 640, 480);
        if (raw.valid()) {
            CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
            CGContextRef ctx = CGBitmapContextCreate(
                (void*)raw.pixelsBGRA.data(),
                raw.width, raw.height, 8, raw.width * 4,
                cs, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst
            );
            CGImageRef cgImg = CGBitmapContextCreateImage(ctx);
            _spriteImageView.image = [[NSImage alloc] initWithCGImage:cgImg size:NSMakeSize(raw.width, raw.height)];
            CGImageRelease(cgImg);
            CGContextRelease(ctx);
            CGColorSpaceRelease(cs);
            _spriteInfoLabel.stringValue = [NSString stringWithFormat:@"贴图: %@ · 尺寸: 640×480 · 格式: RGB555LE (通过 surface_lab 转 BGRA8)", name];
        }
        _spriteSlider.hidden = YES;
    } else {
        _spriteSlider.hidden = NO;
        NSString* sg3Names[] = {@"China_Elevation", @"China_General", @"China_Bodhidharma"};
        NSString* baseName = sg3Names[sel];
        std::string sg3Path = "GameData/DATA/" + std::string(baseName.UTF8String) + ".sg3";
        _current555Path = "GameData/DATA/" + std::string(baseName.UTF8String) + ".555";
        _currentSG3 = game_lab::loadSG3(sg3Path);
        if (!_currentSG3.sprites.empty()) {
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
    if (idx >= _currentSG3.sprites.size()) return;
    const auto& s = _currentSG3.sprites[idx];
    const auto extracted = game_lab::extractSprite(s, _current555Path);
    if (extracted.valid()) {
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(
            (void*)extracted.pixelsBGRA.data(),
            extracted.width, extracted.height, 8, extracted.width * 4,
            cs, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst
        );
        CGImageRef cgImg = CGBitmapContextCreateImage(ctx);
        _spriteImageView.image = [[NSImage alloc] initWithCGImage:cgImg size:NSMakeSize(extracted.width, extracted.height)];
        CGImageRelease(cgImg);
        CGContextRelease(ctx);
        CGColorSpaceRelease(cs);
    } else {
        _spriteImageView.image = nil;
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
    col.title = @"原版中国传统民乐库 (20首 44.1kHz MP3)";
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
    
    NSTextField* sfxTitle = [NSTextField labelWithString:@"🔊 原版音效触发展示台 (736 WAV音效)"];
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
    
    _chineseTexts = game_lab::loadEmperorText("GameData/EmperorText.txt");
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
    col.title = [NSString stringWithFormat:@"原版中文文本库 (共 %zu 条记录 · GBK转码)", _chineseTexts.size()];
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
        [self playMusicTrack:[NSString stringWithFormat:@"GameData/Audio/Music/%@", track]];
    }
}

@end

int main(int argc, const char * argv[]) {
    (void)argc;
    (void)argv;
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        EmperorAppDelegate* delegate = [[EmperorAppDelegate alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
