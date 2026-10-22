#import <AppKit/AppKit.h>
#include "av_audio_backend.hpp"
#include "../app_media.hpp"
#include "../original_pak.hpp"
#include "../movie_session.hpp"
#include "../emperor_assets.hpp"
#include "../sg3_sequence.hpp"
#include "../original_building_view.hpp"
#include "building_candidate_config.hpp"
#include "../resource_root.hpp"
#include "../audio_policy.hpp"
#include "../image_presentation.hpp"
#include "../cg_image_conversion.hpp"
#include "../candidate_import.hpp"
#include "../app_launch.hpp"
#include "../map_geometry.hpp"
#include "../map_viewport.hpp"
#include "../city_simulation.hpp"
#include "../city_state_io.hpp"
#include "city_goal_feedback_adapter.hpp"
#include "../city_game_state.hpp"
#include "city_warehouse_transfer.hpp"
#include "city_warehouse_routes.hpp"
#include "map_surface.hpp"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <memory>

@interface AppMovieSeekSlider : NSSlider {BOOL _mouseTracking;}
@property(nonatomic,readonly) BOOL mouseTracking;
@end
@implementation AppMovieSeekSlider
- (BOOL)mouseTracking {return _mouseTracking;}
- (void)mouseDown:(NSEvent*)event {_mouseTracking=YES;[super mouseDown:event];_mouseTracking=NO;}
@end


static NSImage* imageFromPixels(uint32_t width, uint32_t height, const std::vector<uint32_t>& pixels) {
    CGImageRef image = game_lab::createCGImageFromPremultipliedBGRA(width, height, pixels);
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
@interface EmperorPixelImageView : NSImageView
@end
@implementation EmperorPixelImageView
- (void)drawRect:(NSRect)dirtyRect {
    if(self.imageScaling!=NSImageScaleNone){[super drawRect:dirtyRect];return;}
    if(!self.image)return;
    const NSSize size=self.image.size;
    const NSRect target=NSMakeRect((self.bounds.size.width-size.width)/2,(self.bounds.size.height-size.height)/2,size.width,size.height);
    [self.image drawInRect:target fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1
        respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];
}
@end
static bool readMapCameraNumber(NSTextField* field,double& value) {
    NSString* text=[field.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    const char* raw=text.UTF8String;char* end=nullptr;if(!raw||!*raw)return false;
    value=std::strtod(raw,&end);return end&&end!=raw&&*end=='\0'&&std::isfinite(value);
}
static bool presentPixels(NSImageView* view, uint32_t width, uint32_t height,
                          const std::vector<uint32_t>& pixels) {
    ViewImagePresenter presenter(view);
    return game_lab::presentImage(presenter, width, height, pixels) == game_lab::PresentationResult::Presented;
}

@interface InteractiveMapView : NSImageView {
    std::vector<size_t> _routeRoads, _routeWarehouses;
    size_t _routeFarm;
    BOOL _routeValid, _panMode, _dragging;
    NSPoint _dragPoint;
    game_lab::MapViewport _viewport;
}
- (void)setWarehouseRoute:(const game_lab::CityFarmWarehouseRoute&)route;
@property (nonatomic, copy) void (^onTileClicked)(int tileX, int tileY);
@property (nonatomic, copy) void (^onViewportChanged)(void);
@property (nonatomic) BOOL panMode;
- (void)zoomBy:(double)factor anchorX:(double)x anchorY:(double)y;
- (void)panX:(double)x y:(double)y;
- (void)fullViewport;
- (void)initialViewport;
- (const game_lab::MapViewport&)viewport;
@end

@implementation InteractiveMapView
- (void)setWarehouseRoute:(const game_lab::CityFarmWarehouseRoute&)route {
    _routeValid=route.valid;_routeFarm=route.farmTile;
    _routeRoads=route.roadTiles;_routeWarehouses=route.warehouseTiles;self.needsDisplay=YES;
}
- (const game_lab::MapViewport&)viewport {return _viewport;}
- (BOOL)panMode {return _panMode;}
- (void)setPanMode:(BOOL)value {_panMode=value;_dragging=NO;[self viewportChanged];}
- (void)viewportChanged {self.needsDisplay=YES;if(_onViewportChanged)_onViewportChanged();}
- (void)zoomBy:(double)factor anchorX:(double)x anchorY:(double)y {
    if(_viewport.zoomBy(factor,x,y))[self viewportChanged];
}
- (void)panX:(double)x y:(double)y {if(_viewport.pan(x,y))[self viewportChanged];}
- (void)fullViewport {_viewport.reset();_panMode=NO;_dragging=NO;[self viewportChanged];}
- (void)initialViewport {_viewport.reset();_viewport.zoomBy(4,0,0);_panMode=NO;_dragging=NO;[self viewportChanged];}
- (void)drawRect:(NSRect)dirty {
    (void)dirty;[NSGraphicsContext saveGraphicsState];NSRectClip(self.bounds);
    [NSGraphicsContext currentContext].imageInterpolation=NSImageInterpolationNone;
    [[NSColor colorWithCalibratedRed:.16 green:.2 blue:.16 alpha:1] setFill];NSRectFill(self.bounds);
    const auto& v=_viewport;
    if(self.image)[self.image drawInRect:self.bounds fromRect:NSMakeRect(v.left(),128-v.top()-v.span(),v.span(),v.span())
        operation:NSCompositingOperationCopy fraction:1 respectFlipped:NO hints:nil];
    if(_routeValid) {
        auto rect=[&](size_t i,NSRect& result){game_lab::MapTileRect r;if(!v.tileRect(i%128,i/128,self.bounds.size.width,self.bounds.size.height,r))return false;result=NSMakeRect(self.bounds.origin.x+r.x,self.bounds.origin.y+r.y,r.width,r.height);return true;};
        NSBezierPath* roads=[NSBezierPath bezierPath];NSRect tile;
        for(size_t i:_routeRoads)if(rect(i,tile))[roads appendBezierPathWithRect:NSInsetRect(tile,.25,.25)];
        [[NSColor colorWithCalibratedRed:1 green:.83 blue:.08 alpha:.8] setFill];[roads fill];
        NSBezierPath* warehouses=[NSBezierPath bezierPath];warehouses.lineWidth=1.7;
        for(size_t i:_routeWarehouses)if(rect(i,tile))[warehouses appendBezierPathWithRect:NSInsetRect(tile,-2,-2)];
        [[NSColor cyanColor] setStroke];[warehouses stroke];
        if(rect(_routeFarm,tile)){NSBezierPath* farm=[NSBezierPath bezierPathWithOvalInRect:NSInsetRect(tile,-3,-3)];farm.lineWidth=1.5;[[NSColor whiteColor] setStroke];[farm stroke];}
    }
    [NSGraphicsContext restoreGraphicsState];
}
- (void)mouseDown:(NSEvent*)event {
    NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];
    if(!NSPointInRect(p,self.bounds))return;
    if(_panMode){_dragging=YES;_dragPoint=p;return;}
    int x=0,y=0;
    if(_viewport.pointToTile(p.x-self.bounds.origin.x,p.y-self.bounds.origin.y,self.bounds.size.width,self.bounds.size.height,x,y)&&_onTileClicked)_onTileClicked(x,y);
}
- (void)mouseDragged:(NSEvent*)event {
    if(!_dragging||!_panMode)return;NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];
    if(self.bounds.size.width>0&&self.bounds.size.height>0)
        [self panX:-double(p.x-_dragPoint.x)/self.bounds.size.width*_viewport.span() y:double(p.y-_dragPoint.y)/self.bounds.size.height*_viewport.span()];
    _dragPoint=p;
}
- (void)mouseUp:(NSEvent*)event {(void)event;_dragging=NO;}

@end

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
    NSView* _originalMapView;
    NSPopUpButton* _originalMapPicker;
    NSPopUpButton* _originalMapBitPicker;
    NSButton* _originalMapCrop;
    NSButton* _originalMapSpatial;
    NSPopUpButton* _originalFigureSource;
    NSButton* _originalMapOutlines;
    NSTextField* _originalMapCameraX;
    NSTextField* _originalMapCameraY;
    NSButton* _originalMapCameraApply;
    NSButton* _originalMapCandidates;
    NSTextField* _originalMapInfo;
    NSTextView* _originalMapUnknowns;
    NSTextField* _originalSaveCellX;
    NSTextField* _originalSaveCellY;
    NSButton* _originalSaveCellApply;
    NSButton* _originalBuildingLocate;
    NSTextField* _originalBuildingSlot;
    NSButton* _originalManualAdopt;NSButton* _originalManualVisible;NSButton* _originalManualClear;NSButton* _originalManualSave;
    std::optional<size_t> _originalSelectedSlot;
    std::optional<game_lab::OriginalBuildingCandidate> _originalManualCandidate;
    std::string _originalSourceRelative,_originalManualMessage;uint64_t _originalSaveRevision;
    std::optional<size_t> _spriteStaticIndex;game_lab::DecodedImage _spriteStaticPixels;BOOL _spriteExplicitSelection;

    NSImageView* _originalMapImage;
    std::vector<game_lab::OriginalMapSource> _originalMapSources;
    std::vector<size_t> _originalMapVisibleSources;
    NSTextField* _originalMapFilter;
    std::string _originalMapSourceInfo;
    BOOL _originalMapPackageSummary;
    std::optional<game_lab::OriginalSaveCellLayers> _originalSaveLayers;
    std::optional<game_lab::OriginalBuildingProfile> _originalSaveBuildings;
    game_lab::OriginalMapResult _originalMapResult;
    std::unique_ptr<game_lab::AppMapSurface> _originalMapSurface;
    NSView* _candidateView;
    NSTextView* _candidateReport;
    NSButton* _candidateRetry;
    game_lab::CandidateImportController _candidateImport;
    NSURL* _candidateDirectory;
    
    // Bottom playback bar
    NSTextField* _musicStatusLabel;
    NSButton* _playPauseButton;
    NSButton* _muteButton;
    NSSlider* _volumeSlider;
    
    BOOL _selfAuthoredLaunch;
    // Audio Player
    game_lab::AVAudioFactory _audioFactory;
    std::unique_ptr<game_lab::AppMedia> _media;
    
    // Main Menu Assets
    NSImage* _mainMenuImage;
    NSImageView* _mainMenuImageView;
    
    // Maps & Simulation
    game_lab::CitySimulation _citySim;
    game_lab::CityGoalConfig _cityGoalConfig;
    game_lab::CityGoalMemory _cityGoalMemory;
    NSTextField* _cityGoalLabel;
    game_lab::CityScenarioSession _cityScenario;
    game_lab::CityScenarioAwardBook _cityScenarioReceipts;
    NSPopUpButton* _cityScenarioPicker;
    NSTextField* _cityScenarioLabel;
    game_lab::BuildingType _activeBuildTool;
    int _selectedCursorX;
    int _selectedCursorY;
    NSTimer* _simTimer;
    NSTextField* _cityHUDLabel;
    NSTextField* _cityEventLabel;
    game_lab::CityWarehouseTransferSelection _warehouseTransfer;
    NSButton* _warehouseTransferBegin;
    NSButton* _warehouseTransferConfirm;
    NSButton* _warehouseTransferCancel;
    NSTextField* _warehouseTransferAmount;
    NSTextField* _warehouseTransferStatus;
    NSTextField* _warehouseRouteLabel;
    NSTextField* _mapViewportLabel;
    NSButton* _mapPanToggle;
    NSButton* _mapZoomIn;
    NSButton* _mapZoomOut;
    NSSegmentedControl* _speedControl;
    NSSegmentedControl* _buildToolControl;
    NSArray<NSString*>* _mapFiles;
    NSTableView* _mapTableView;
    NSTextField* _mapInfoLabel;
    InteractiveMapView* _mapCanvasView;
    
    // Sprites & 555
    NSPopUpButton* _spritePackagePopup;
    NSSlider* _spriteSlider;
    NSTextField* _spriteIdField;
    NSButton* _spriteIdApply;
    NSButton* _spriteBaseCanvas;
    NSTextField* _spriteInfoLabel;
    NSImageView* _spriteImageView;
    game_lab::SG3Archive _currentSG3;
    NSTextField* _spriteSequenceIds;
    NSTextField* _spriteSequenceRate;
    NSButton* _spriteSequenceApply;
    NSButton* _spriteSequencePrevious;
    NSButton* _spriteSequenceNext;
    NSButton* _spriteSequencePlay;
    NSButton* _spriteSequenceAlign;
    NSTimer* _spriteSequenceTimer;
    game_lab::SG3Sequence _spriteSequence;
    size_t _spriteSequenceFrame;
    std::vector<std::string> _sg3Packages;
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

    // Movie silent continuous preview & poster
    NSView* _movieView;
    NSTextField* _moviePathField;
    NSTextField* _movieStatusLabel;
    NSTextField* _moviePositionLabel;
    NSImageView* _movieImageView;
    NSButton* _moviePlayButton;
    NSButton* _moviePauseButton;
    NSButton* _movieStopButton;
    AppMovieSeekSlider* _movieSeekSlider;
    NSTimer* _movieTimer;
    BOOL _movieTickQueued;
    NSUInteger _moviePending;
    std::unique_ptr<game_lab::MovieSession> _movieSession;
    std::unique_ptr<ViewImagePresenter> _moviePresenter;
    BOOL _movieTerminating;
}
- (instancetype)initWithResourceRoot:(const game_lab::ResourceRoot&)root selfAuthored:(BOOL)selfAuthored;
- (void)refreshAudioStatus;
- (void)updateMovieControls;
- (void)updateMovieTimer;
- (void)invalidateMovieTimer;
- (void)requestMovieAction:(media_demo::Action)action frame:(uint64_t)frame;
@end

@implementation EmperorAppDelegate

- (instancetype)initWithResourceRoot:(const game_lab::ResourceRoot&)root selfAuthored:(BOOL)selfAuthored {
    self = [super init];
    if (self) {
        _selfAuthoredLaunch = selfAuthored;
        _candidateDirectory = [NSURL fileURLWithPath:[NSString stringWithUTF8String:root.configuredDirectory().c_str()]];
        _media = std::make_unique<game_lab::AppMedia>(root, _audioFactory);
        _originalMapSurface = std::make_unique<game_lab::AppMapSurface>(root);
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

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    [self stopSpriteSequence];
    if (_movieTerminating) return NSTerminateLater;
    [self invalidateMovieTimer];
    if (!_movieSession) return NSTerminateNow;
    _movieTerminating = YES;
    _moviePending = 1;
    __weak EmperorAppDelegate* weakSelf = self;
    // The shared contract owns one deadline, reuses the existing worker, and
    // confirms its idle/resource state. Retain the front until this result.
    const auto status = _movieSession->requestTermination(5000, [weakSelf, sender](const game_lab::TerminationResult& result) {
        EmperorAppDelegate* owner = weakSelf;
        if (!owner) { [sender replyToApplicationShouldTerminate:NO]; return; }
        owner->_moviePending = 0;
        owner->_movieTerminating = NO;
        if (!result.permitsExit()) {
            owner->_movieStatusLabel.stringValue = [NSString stringWithUTF8String:game_lab::formatTerminationResult(result).c_str()];
            [owner updateMovieControls];
            [sender replyToApplicationShouldTerminate:NO];
            return;
        }
        owner->_movieSession.reset();
        [sender replyToApplicationShouldTerminate:YES];
    });
    if (status == game_lab::TerminationStatus::Completed) {
        _moviePending = 0;
        _movieTerminating = NO;
        _movieSession.reset();
        return NSTerminateNow;
    }
    return NSTerminateLater;
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    [self stopSpriteSequence];
    (void)notification;
    if (_simTimer) {
        [_simTimer invalidate];
        _simTimer = nil;
    }
    [self invalidateMovieTimer];
    _movieSession.reset();
    _movieTickQueued = NO;
    _moviePending = 0;
    _media->stopAll();
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    _citySim.reset(game_lab::CityRuleset::StaffedServices);
    _citySim.setSpeed(0);
    (void)notification;
    [self setupWindow];
    [self setupAudio];
    [self setupMainMenuView];
    [self setupMapView];
    [self setupSpriteView];
    [self setupMusicView];
    [self setupTextView];
    [self setupMovieView];
    [self setupCandidateView];
    [self setupOriginalMapView];
    [self showTab:7];
    
    // Start the self-authored city tick timer
    _simTimer = [NSTimer scheduledTimerWithTimeInterval:0.25 target:self selector:@selector(onSimulationStep:) userInfo:nil repeats:YES];
}

- (void)setupWindow {
    NSRect frame = NSMakeRect(100, 100, 1024, 820);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.contentMinSize=NSMakeSize(1024,760);
    _window.title = @"Emperor · 原始数据浏览器";
    [_window center];
    
    NSView* root = _window.contentView;
    
    // Top Navigation Bar
    _navBar = [NSSegmentedControl segmentedControlWithLabels:@[@"主界面", @"城市营建（自写）", @"图素与贴图", @"国风音乐台", @"游戏文本资料库", @"影片音画预览", @"候选存档", @"原始地图"]
                                                trackingMode:NSSegmentSwitchTrackingSelectOne
                                                      target:self
                                                      action:@selector(onTabChanged:)];
    _navBar.frame = NSMakeRect(12, 775, 1000, 32);
    for (NSInteger segment = 0; segment < 8; ++segment) [_navBar setWidth:1000.0 / 8.0 forSegment:segment];
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
    _volumeSlider.accessibilityIdentifier = @"application-volume";
    _muteButton.accessibilityIdentifier = @"application-mute";
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
    if (_movieSession) _movieSession->setAudioGain(_media->audioGain());
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
    _media->setVolume(_volumeSlider.doubleValue);
    _media->setMuted(!_media->muted());
    if (_movieSession) _movieSession->setAudioGain(_media->audioGain());
}

- (void)onVolumeChanged:(id)sender {
    (void)sender;
    _media->setVolume(_volumeSlider.doubleValue);
    if (_movieSession) _movieSession->setAudioGain(_media->audioGain());
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
    NSView* menuOverlay = [[NSView alloc] initWithFrame:NSMakeRect(40, 70, 260, 560)];
    menuOverlay.wantsLayer = YES;
    menuOverlay.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.08 green:0.08 blue:0.1 alpha:0.75].CGColor;
    menuOverlay.layer.cornerRadius = 10;
    [_mainMenuView addSubview:menuOverlay];
    
    NSTextField* titleLabel = [NSTextField labelWithString:@"皇帝：龙之崛起"];
    titleLabel.frame = NSMakeRect(20, 500, 220, 36);
    titleLabel.font = [NSFont boldSystemFontOfSize:22];
    titleLabel.textColor = [NSColor colorWithCalibratedRed:0.95 green:0.85 blue:0.4 alpha:1.0];
    [menuOverlay addSubview:titleLabel];
    
    NSTextField* subLabel = [NSTextField labelWithString:@"媒体资源浏览器 · 2026"];
    if (!_mainMenuImage) subLabel.stringValue = @"背景图片不可读取或转换失败";
    subLabel.frame = NSMakeRect(20, 475, 220, 20);
    subLabel.font = [NSFont systemFontOfSize:12];
    subLabel.textColor = [NSColor colorWithCalibratedWhite:0.75 alpha:1.0];
    [menuOverlay addSubview:subLabel];
    
    NSArray* btnConfigs = @[
        @{@"title": @"🏯 营建城邑", @"action": NSStringFromSelector(@selector(gotoMaps:))},
        @{@"title": @"🖼 图素与贴图", @"action": NSStringFromSelector(@selector(gotoSprites:))},
        @{@"title": @"🎵 国风民乐鉴赏", @"action": NSStringFromSelector(@selector(gotoMusic:))},
        @{@"title": @"📜 游戏文本与对话", @"action": NSStringFromSelector(@selector(gotoTexts:))},
        @{@"title": @"🎬 影片音画预览", @"action": NSStringFromSelector(@selector(gotoMovies:))},
        @{@"title": @"🔔 播放音效 (祈福)", @"action": NSStringFromSelector(@selector(playBlessSFX:))},
        @{@"title": @"❌ 退出应用", @"action": NSStringFromSelector(@selector(terminate:))}
    ];
    
    CGFloat y = 420;
    for (NSDictionary* cfg in btnConfigs) {
        NSButton* btn = [NSButton buttonWithTitle:cfg[@"title"] target:self action:NSSelectorFromString(cfg[@"action"])];
        btn.frame = NSMakeRect(20, y, 220, 36);
        btn.bezelStyle = NSBezelStyleRounded;
        [menuOverlay addSubview:btn];
        y -= 48;
    }
}

- (void)gotoMaps:(id)sender {
    (void)sender;
    [self showTab:1];
}
- (void)gotoSprites:(id)sender { (void)sender; [self showTab:2]; }
- (void)gotoMusic:(id)sender { (void)sender; [self showTab:3]; }
- (void)gotoTexts:(id)sender { (void)sender; [self showTab:4]; }
- (void)gotoMovies:(id)sender { (void)sender; [self showTab:5]; }
- (void)playBlessSFX:(id)sender { (void)sender; [self playSFX:@"H_ancestor_bless.wav"]; }
- (void)terminate:(id)sender { (void)sender; [NSApp terminate:nil]; }

// ----------------------------------------------------
// Tab 1: Self-authored City Building
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
    
    if (_selfAuthoredLaunch) _mapFiles = @[];
    // Left: Table of maps (width 260)
    const CGFloat cityHeight = _mapView.bounds.size.height;
    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(16, 456, 260, cityHeight - 472)];
    scroll.autoresizingMask = NSViewHeightSizable;
    _mapTableView = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"MapName"];
    col.title = _selfAuthoredLaunch ? @"自写空白平原（无需原版资源）" : @"媒体目录地图 (.map)";
    col.width = 240;
    [_mapTableView addTableColumn:col];
    _mapTableView.delegate = self;
    _mapTableView.dataSource = self;
    scroll.documentView = _mapTableView;
    [_mapView addSubview:scroll];
    if (_selfAuthoredLaunch) {
        scroll.hidden=YES;
        NSTextField* firstPlay=[NSTextField wrappingLabelWithString:@"第一次玩：\n1. 选“聚落初成”，点击开始挑战\n2. 选民居，在空地建六户\n3. 点击 +1月，观察人口与目标\n4. 也可新建仓储城，自由营建\n5. 保存城邑，之后载入继续"];
        firstPlay.frame=scroll.frame;firstPlay.autoresizingMask=NSViewHeightSizable;
        firstPlay.maximumNumberOfLines=10;firstPlay.font=[NSFont systemFontOfSize:12];
        firstPlay.accessibilityIdentifier=@"first-play-guide";[_mapView addSubview:firstPlay];
    }
    
    _cityGoalLabel = [NSTextField wrappingLabelWithString:@"自写经营目标"];
    _cityGoalLabel.frame = NSMakeRect(16, 108, 260, 238);
    _cityGoalLabel.maximumNumberOfLines = 18;
    _cityGoalLabel.font = [NSFont systemFontOfSize:11];
    _cityGoalLabel.textColor = [NSColor colorWithCalibratedWhite:0.9 alpha:1.0];
    [_mapView addSubview:_cityGoalLabel];

    _cityScenarioPicker = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(16, 366, 164, 28) pullsDown:NO];
    for (const auto& definition : game_lab::selfAuthoredCityScenarios())
        [_cityScenarioPicker addItemWithTitle:[NSString stringWithUTF8String:definition.title.c_str()]];
    [_cityScenarioPicker addItemWithTitle:[NSString stringWithUTF8String:game_lab::selfAuthoredWarehouseTraining().title.c_str()]];
    [_mapView addSubview:_cityScenarioPicker];
    NSButton* startChallenge = [NSButton buttonWithTitle:@"开始挑战" target:self action:@selector(onStartCityScenario:)];
    startChallenge.frame = NSMakeRect(186, 366, 90, 28);
    startChallenge.toolTip = @"新建空城并开始所选挑战；新建空城按钮回到自由营建。";
    [_mapView addSubview:startChallenge];
    _cityScenarioLabel = [NSTextField wrappingLabelWithString:@"自由营建：可选择限期挑战"];
    _cityScenarioLabel.frame = NSMakeRect(16, 402, 260, 42);
    _cityScenarioLabel.maximumNumberOfLines = 3;
    _cityScenarioLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightMedium];
    [_mapView addSubview:_cityScenarioLabel];

    // Right Top: City HUD Status (Width 730)
    _cityHUDLabel = [NSTextField wrappingLabelWithString:@"📅 城市模拟已就绪"];
    _cityHUDLabel.frame = NSMakeRect(286, cityHeight - 66, 722, 50);
    _cityHUDLabel.maximumNumberOfLines = 2;
    _cityHUDLabel.autoresizingMask = NSViewMinYMargin | NSViewWidthSizable;
    _cityHUDLabel.font = [NSFont boldSystemFontOfSize:13];
    _cityHUDLabel.textColor = [NSColor colorWithCalibratedRed:0.95 green:0.85 blue:0.4 alpha:1.0];
    [_mapView addSubview:_cityHUDLabel];
    
    // Speed Control Bar (Width 240)
    _speedControl = [NSSegmentedControl segmentedControlWithLabels:@[@"⏸ 暂停", @"▶ 1x", @"⏩ 2x", @"⏭ 3x", @"⏩+1月"]
                                                      trackingMode:NSSegmentSwitchTrackingSelectOne
                                                            target:self
                                                            action:@selector(onSpeedChanged:)];
    _speedControl.frame = NSMakeRect(286, cityHeight - 102, 240, 26);
    _speedControl.autoresizingMask = NSViewMinYMargin;
    _speedControl.selectedSegment = 1;
    [_mapView addSubview:_speedControl];

    // Building Tools Palette (Width 470)
    _buildToolControl = [NSSegmentedControl segmentedControlWithLabels:@[
        @"勘查", @"道路", @"民居", @"农庄", @"水井", @"宗庙", @"市肆", @"更楼", @"仓库", @"拆除"
    ]
                                                          trackingMode:NSSegmentSwitchTrackingSelectOne
                                                                target:self
                                                                action:@selector(onBuildToolChanged:)];
    _buildToolControl.frame = NSMakeRect(536, cityHeight - 102, 472, 26);
    _buildToolControl.autoresizingMask = NSViewMinYMargin | NSViewWidthSizable;
    NSArray<NSString*>* toolTips = @[@"勘查地块", @"道路：5贯", @"民居：15贯", @"农庄：40贯", @"水井：10贯", @"宗庙：50贯", @"市肆：30贯", @"更楼：20贯", @"仓库：60贯；须启用仓储规则", @"拆除：2贯"];
    for (NSInteger i = 0; i < _buildToolControl.segmentCount; ++i) {
        [_buildToolControl setToolTip:toolTips[static_cast<NSUInteger>(i)] forSegment:i];
    }
    _buildToolControl.selectedSegment = 0;
    [_mapView addSubview:_buildToolControl];

    // Leave room for the wrapped HUD; track the content height when macOS fits the window to the screen.
    _mapCanvasView = [[InteractiveMapView alloc] initWithFrame:NSMakeRect(360, 52, 532, cityHeight - 326)];
    _mapCanvasView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    _mapCanvasView.imageScaling = NSImageScaleAxesIndependently;
    _mapCanvasView.wantsLayer = YES;
    _mapCanvasView.layer.borderColor = [NSColor colorWithCalibratedRed:0.6 green:0.5 blue:0.3 alpha:1.0].CGColor;
    _mapCanvasView.layer.borderWidth = 2;
    _mapCanvasView.layer.cornerRadius = 4;

    __weak EmperorAppDelegate* weakSelf = self;
    _mapCanvasView.onTileClicked = ^(int tileX, int tileY) {
        [weakSelf handleTileClickAtX:tileX y:tileY];
    };
    [_mapView addSubview:_mapCanvasView];
    _mapCanvasView.onViewportChanged=^{[weakSelf updateMapViewportControls];};
    _mapZoomOut=[NSButton buttonWithTitle:@"−" target:self action:@selector(onMapZoom:)];_mapZoomOut.tag=-1;
    _mapZoomIn=[NSButton buttonWithTitle:@"+" target:self action:@selector(onMapZoom:)];_mapZoomIn.tag=1;
    NSButton* full=[NSButton buttonWithTitle:@"全图" target:self action:@selector(onMapFull:)];
    NSArray<NSButton*>* zoomButtons=@[_mapZoomOut,_mapZoomIn,full];
    const CGFloat zx[]={286,326,366},zw[]={34,34,52};
    for(NSUInteger i=0;i<zoomButtons.count;++i){NSButton* b=zoomButtons[i];b.frame=NSMakeRect(zx[i],cityHeight-262,zw[i],26);b.autoresizingMask=NSViewMinYMargin;[_mapView addSubview:b];}
    NSArray<NSString*>* arrows=@[@"←",@"↑",@"↓",@"→"];
    for(NSUInteger i=0;i<4;++i){NSButton* b=[NSButton buttonWithTitle:arrows[i] target:self action:@selector(onMapPan:)];b.tag=i;b.frame=NSMakeRect(424+i*36,cityHeight-262,30,26);b.autoresizingMask=NSViewMinYMargin;[_mapView addSubview:b];}
    _mapPanToggle=[NSButton buttonWithTitle:@"拖动平移" target:self action:@selector(onMapDrag:)];_mapPanToggle.buttonType=NSButtonTypePushOnPushOff;
    _mapPanToggle.frame=NSMakeRect(574,cityHeight-262,88,26);_mapPanToggle.autoresizingMask=NSViewMinYMargin;
    _mapPanToggle.toolTip=@"开启后拖动地图移动视图，不建设；再次点击退出平移";[_mapView addSubview:_mapPanToggle];
    _mapViewportLabel=[NSTextField labelWithString:@""];
    _mapViewportLabel.frame=NSMakeRect(672,cityHeight-258,336,20);_mapViewportLabel.font=[NSFont systemFontOfSize:11];
    _mapViewportLabel.autoresizingMask=NSViewMinYMargin|NSViewWidthSizable;_mapViewportLabel.accessibilityIdentifier=@"map-viewport-summary";[_mapView addSubview:_mapViewportLabel];
    [_mapCanvasView initialViewport];


    // Status info label below controls
    _mapInfoLabel = [NSTextField wrappingLabelWithString:@""];
    _mapInfoLabel.maximumNumberOfLines = 2;
    _mapInfoLabel.frame = NSMakeRect(286, cityHeight - 148, 722, 40);
    _mapInfoLabel.autoresizingMask = NSViewMinYMargin | NSViewWidthSizable;
    _mapInfoLabel.font = [NSFont systemFontOfSize:12];
    _mapInfoLabel.textColor = [NSColor colorWithCalibratedWhite:0.8 alpha:1.0];
    [_mapView addSubview:_mapInfoLabel];

    _warehouseTransferBegin = [NSButton buttonWithTitle:@"搬到另一仓" target:self action:@selector(onBeginWarehouseTransfer:)];
    _warehouseTransferBegin.frame = NSMakeRect(286, cityHeight-186, 116, 26);
    _warehouseTransferBegin.autoresizingMask = NSViewMinYMargin;
    [_mapView addSubview:_warehouseTransferBegin];
    NSTextField* amountLabel = [NSTextField labelWithString:@"数量(担)"];
    amountLabel.frame=NSMakeRect(410,cityHeight-181,62,18);amountLabel.autoresizingMask=NSViewMinYMargin;[_mapView addSubview:amountLabel];
    _warehouseTransferAmount = [[NSTextField alloc] initWithFrame:NSMakeRect(478,cityHeight-186,68,26)];
    _warehouseTransferAmount.placeholderString=@"1–600";
    _warehouseTransferAmount.accessibilityIdentifier=@"warehouse-transfer-amount";
    _warehouseTransferAmount.autoresizingMask=NSViewMinYMargin;[_mapView addSubview:_warehouseTransferAmount];
    _warehouseTransferConfirm=[NSButton buttonWithTitle:@"搬粮" target:self action:@selector(onCommitWarehouseTransfer:)];
    _warehouseTransferCancel=[NSButton buttonWithTitle:@"取消搬粮" target:self action:@selector(onCancelWarehouseTransfer:)];
    _warehouseTransferConfirm.frame=NSMakeRect(554,cityHeight-186,64,26);
    _warehouseTransferCancel.frame=NSMakeRect(624,cityHeight-186,84,26);
    _warehouseTransferConfirm.autoresizingMask=NSViewMinYMargin;_warehouseTransferCancel.autoresizingMask=NSViewMinYMargin;
    [_mapView addSubview:_warehouseTransferConfirm];[_mapView addSubview:_warehouseTransferCancel];
    _warehouseTransferStatus=[NSTextField labelWithString:@"先勘查有粮官仓"];
    _warehouseTransferStatus.frame=NSMakeRect(714,cityHeight-182,294,20);
    _warehouseTransferStatus.font=[NSFont systemFontOfSize:11];
    _warehouseTransferStatus.autoresizingMask=NSViewMinYMargin|NSViewWidthSizable;[_mapView addSubview:_warehouseTransferStatus];

    _warehouseRouteLabel=[NSTextField wrappingLabelWithString:@"仓储城勘查农庄：黄线为可达道路，青框为官仓"];
    _warehouseRouteLabel.frame=NSMakeRect(286,cityHeight-226,722,34);
    _warehouseRouteLabel.maximumNumberOfLines=2;_warehouseRouteLabel.font=[NSFont systemFontOfSize:11];
    _warehouseRouteLabel.accessibilityIdentifier=@"warehouse-route-summary";
    _warehouseRouteLabel.autoresizingMask=NSViewMinYMargin|NSViewWidthSizable;
    [_mapView addSubview:_warehouseRouteLabel];

    // Bottom: Event Log / Status
    _cityEventLabel = [NSTextField wrappingLabelWithString:@"📜 帝都政务记录"];
    _cityEventLabel.frame = NSMakeRect(286, 12, 722, 34);
    _cityEventLabel.maximumNumberOfLines = 2;
    _cityEventLabel.autoresizingMask = NSViewWidthSizable;
    _cityEventLabel.font = [NSFont systemFontOfSize:12 weight:NSFontWeightMedium];
    _cityEventLabel.textColor = [NSColor colorWithCalibratedWhite:0.85 alpha:1.0];
    [_mapView addSubview:_cityEventLabel];

    _selectedCursorX = -1;
    _selectedCursorY = -1;
    _activeBuildTool = game_lab::BuildingType::None;
    NSButton* saveButton = [NSButton buttonWithTitle:@"保存城邑" target:self action:@selector(onSaveCity:)];
    NSButton* loadButton = [NSButton buttonWithTitle:@"载入城邑" target:self action:@selector(onLoadCity:)];
    NSButton* resetButton = [NSButton buttonWithTitle:@"新建空城" target:self action:@selector(onResetCity:)];
    saveButton.frame = NSMakeRect(16, 64, 82, 28);
    loadButton.frame = NSMakeRect(102, 64, 82, 28);
    resetButton.frame = NSMakeRect(188, 64, 82, 28);
    [_mapView addSubview:saveButton]; [_mapView addSubview:loadButton]; [_mapView addSubview:resetButton];
    NSTextField* saveLabel = [NSTextField labelWithString:@"城邑与挑战进度一起保存"];
    NSButton* warehouseCity = [NSButton buttonWithTitle:@"仓储新城" target:self action:@selector(onNewWarehouseCity:)];
    warehouseCity.frame = NSMakeRect(16, 34, 124, 26);
    warehouseCity.toolTip = @"明确新建仓储自由城：预置400/600担官仓，农庄须用四邻道路通仓。";
    NSButton* convert = [NSButton buttonWithTitle:@"启用仓储" target:self action:@selector(onEnableWarehouses:)];
    convert.frame = NSMakeRect(144, 34, 126, 26);
    convert.toolTip = @"v3自由城：先勘查空陆格，再花60贯新增仓库搬入全部旧粮（最多600）。活动挑战不可转换。";
    [_mapView addSubview:warehouseCity]; [_mapView addSubview:convert];
    saveLabel.frame = NSMakeRect(16, 6, 260, 22);
    [_mapView addSubview:saveLabel];
    [self updateCityHUD];
    [self renderCityGrid];
}

- (void)updateCityHUD {
    const auto& s = _citySim.stats();
    const bool warehouses = _citySim.ruleset() == game_lab::CityRuleset::RoadWarehouses;
    const auto rule = warehouses ? game_lab::CityHarvestRule::RoadWarehousesV1 :
        _citySim.ruleset() == game_lab::CityRuleset::StaffedServices ?
        game_lab::CityHarvestRule::StaffedServicesV1 : game_lab::CityHarvestRule::LaborLimitedIntegerV1;
    [_buildToolControl setEnabled:warehouses forSegment:8];
    [self updateWarehouseTransferControls];
    const auto goal = game_lab::evaluateCityGoals(game_lab::makeCityFeedbackSnapshot(_citySim, rule), _cityGoalConfig, _cityGoalMemory);
    _cityGoalMemory = goal.nextMemory;
    const bool training = _cityScenario.active() && game_lab::isCityWarehouseTrainingSpec(_cityScenario.checkpoint().spec);
    const auto challenge = training ? game_lab::observeCityWarehouseTraining(_cityScenario,_citySim,_cityScenarioReceipts) :
        _cityScenario.observe({s.year,s.month},goal,_cityScenarioReceipts);
    std::string challengeText = "自由营建：可选择限期挑战\n开始挑战会新建空城";
    if (challenge.active) {
        std::string title = "自定义挑战";
        for (const auto& definition : game_lab::selfAuthoredCityScenarios())
            if (definition.spec.id == _cityScenario.checkpoint().spec.id) title = definition.title;
        if (training) title = game_lab::selfAuthoredWarehouseTraining().title;
        challengeText = title + " · ";
        if (!challenge.ok) challengeText += challenge.message;
        else if (challenge.status == game_lab::CityScenarioStatus::Won) challengeText += "胜利：目标已按期达成\n可继续营建，或另开新挑战";
        else if (challenge.status == game_lab::CityScenarioStatus::Lost) challengeText += "失败：到期未达成\n可继续营建，或另开新挑战";
        else challengeText += "已过" + std::to_string(challenge.elapsedMonths) + "月 / 剩余" +
            std::to_string(challenge.remainingMonths) + "月\n暂停不计时，+1月计入期限";
    }
    _cityScenarioLabel.stringValue = [NSString stringWithUTF8String:challengeText.c_str()];
    const auto selected = _cityScenarioPicker.indexOfSelectedItem;
    if (selected >= 0 && selected < 3)
        _cityScenarioPicker.toolTip = [NSString stringWithUTF8String:game_lab::selfAuthoredCityScenarios()[size_t(selected)].instructions.c_str()];
    else if (selected == 3)
        _cityScenarioPicker.toolTip = [NSString stringWithUTF8String:game_lab::selfAuthoredWarehouseTraining().instructions.c_str()];
    std::string goalText = (warehouses ? "自写经营目标（仓储运输）\n" :
        _citySim.ruleset() == game_lab::CityRuleset::StaffedServices ?
        "自写经营目标（服务需工）\n" : "自写经营目标（旧存档服务规则）\n") + goal.status;
    if (goal.valid) {
        const char* labels[] = {"人口", "存粮", "国库", "人口月度收支"};
        for (size_t i = 0; i < goal.goals.size(); ++i) {
            const auto& item = goal.goals[i];
            goalText += "\n" + std::string(labels[i]) + ": " + std::to_string(item.current) + "/" + std::to_string(item.target);
            if (!item.met) goalText += "（差 " + std::to_string(item.remaining) + "）";
        }
        if (goal.nextMonth.known) {
            const auto& p = goal.nextMonth;
            goalText += "\n下月" + std::to_string(p.month) + (warehouses ? "月：入仓" : "月：新粮") + std::to_string(p.harvest) + " / 耗粮" + std::to_string(p.consumption) +
                        "\n余粮" + std::to_string(p.foodAfter) + " / 迁入" + std::to_string(p.incoming) +
                        "\n税" + std::to_string(p.tax) + " − 维护" + std::to_string(p.upkeep) + " = " + std::to_string(p.balance);
        }
        const game_lab::CityFeedbackIssue* primary = nullptr;
        auto priority = [](game_lab::CityFeedbackSeverity level) { return level == game_lab::CityFeedbackSeverity::Critical ? 3 : level == game_lab::CityFeedbackSeverity::Warning ? 2 : level == game_lab::CityFeedbackSeverity::Info ? 1 : 0; };
        std::string details = goal.rulesNote + "\n" + goal.progressText + "\n" + goal.previewText;
        for (const auto& issue : goal.issues) {
            if (priority(issue.severity) > 0) details += "\n" + issue.message;
            if (!primary || priority(issue.severity) > priority(primary->severity)) primary = &issue;
        }
        if (primary && priority(primary->severity) > 0) goalText += "\n" + primary->message;
        _cityGoalLabel.toolTip = [NSString stringWithUTF8String:details.c_str()];
    }
    if (training) {
        const auto guide = game_lab::evaluateCityWarehouseTraining(game_lab::makeCityWarehouseTrainingSnapshot(_citySim),_cityGoalConfig);
        std::string allHints;
        for (const auto& hint : guide.hints) allHints += hint.text + "\n";
        if (!guide.hints.empty()) goalText += "\n" + guide.hints.front().text;
        _cityGoalLabel.toolTip = [_cityGoalLabel.toolTip stringByAppendingFormat:@"\n%@",[NSString stringWithUTF8String:allHints.c_str()]];
    }
    _cityGoalLabel.stringValue = [NSString stringWithUTF8String:goalText.c_str()];
    NSString* yearStr = s.year < 0 ? [NSString stringWithFormat:@"公元前 %lld 年", -static_cast<long long>(s.year)] : [NSString stringWithFormat:@"公元 %d 年", s.year];
    _cityHUDLabel.stringValue = [NSString stringWithFormat:@"📅 %@ %d月 [%@]  |  💰 国库: %d贯  |  👥 户籍: %d人 (容积: %d)  |  🛠 就业: %d%% (%d/%d工)  |  🌾 存粮: %d担  |  🕊 繁荣: %d%%",
                                 yearStr, s.month, [NSString stringWithUTF8String:s.rankTitle.c_str()], s.treasury, s.population, s.housingCapacity, s.employmentPercent, s.availableWorkers, s.neededWorkers, s.foodSupply, s.favor];
    if (warehouses) {
        const auto current = _citySim.logisticsOverview(); const auto& last = _citySim.lastLogisticsMonth();
        std::string text = "官仓" + std::to_string(current.warehouses) + "座 · 存粮" + std::to_string(current.stored) +
            "/" + std::to_string(current.capacity) + "担 · 农庄通仓" + std::to_string(current.connectedFarms) +
            "/" + std::to_string(current.connectedFarms+current.disconnectedFarms) + "；四邻道路连接，斜角不算";
        if (last.valid) text += "\n上月生产" + std::to_string(last.produced) + " / 入仓" + std::to_string(last.delivered) +
            " / 断路损耗" + std::to_string(last.noRouteDiscard) + " / 满仓损耗" + std::to_string(last.fullDiscard) +
            " / 耗粮" + std::to_string(last.consumed) + "担";
        else text += "\n先建住宅增加工人；勘查仓库位置，用连续四邻道路接到农庄";
        _mapInfoLabel.stringValue = [NSString stringWithUTF8String:text.c_str()];
    }
    if (!_citySim.events().empty()) {
        _cityEventLabel.stringValue = [NSString stringWithFormat:@"📜 帝都政务记录: %@", [NSString stringWithUTF8String:_citySim.events().back().c_str()]];
    }
}

- (void)updateWarehouseTransferControls {
    const bool warehouseCity=_citySim.ruleset()==game_lab::CityRuleset::RoadWarehouses;
    const bool source=warehouseCity && _selectedCursorX>=0 && _selectedCursorY>=0 &&
        _citySim.buildingAt(_selectedCursorX,_selectedCursorY)==game_lab::BuildingType::Warehouse &&
        _citySim.warehouseStockAt(_selectedCursorX,_selectedCursorY)>0;
    _warehouseTransferBegin.enabled=source && !_warehouseTransfer.active();
    _warehouseTransferAmount.enabled=_warehouseTransfer.ready();
    _warehouseTransferConfirm.enabled=_warehouseTransfer.ready() && _warehouseTransfer.maximum(_citySim)>0;
    _warehouseTransferCancel.enabled=_warehouseTransfer.active();
    if (_warehouseTransfer.ready())
        _warehouseTransferStatus.stringValue=[NSString stringWithFormat:@"(%u,%u)→(%u,%u)，最多%d担",_warehouseTransfer.sourceX(),_warehouseTransfer.sourceY(),_warehouseTransfer.targetX(),_warehouseTransfer.targetY(),_warehouseTransfer.maximum(_citySim)];
    else _warehouseTransferStatus.stringValue=_warehouseTransfer.active()?@"请选择另一座官仓":(warehouseCity?@"先勘查有粮官仓":@"搬粮须使用仓储城");
}
- (void)onBeginWarehouseTransfer:(id)sender {
    (void)sender;std::string message;
    if (_selectedCursorX<0 || _selectedCursorY<0) message="请先勘查一座有粮的官仓";
    else _warehouseTransfer.begin(_citySim,_selectedCursorX,_selectedCursorY,message);
    _warehouseTransferAmount.stringValue=@"";[self updateWarehouseTransferControls];
    _cityEventLabel.stringValue=[NSString stringWithUTF8String:message.c_str()];
}
- (void)onCancelWarehouseTransfer:(id)sender {
    (void)sender;_warehouseTransfer.cancel();_warehouseTransferAmount.stringValue=@"";
    [self updateWarehouseTransferControls];_cityEventLabel.stringValue=@"已取消搬粮，库存与城市保持";
}
- (void)onCommitWarehouseTransfer:(id)sender {
    (void)sender;std::string message;
    NSString* input=[_warehouseTransferAmount.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    const bool moved=_warehouseTransfer.commit(_citySim,input.UTF8String?input.UTF8String:"",message);
    if(moved) {[self updateCityHUD];[self renderCityGrid];_warehouseTransferAmount.stringValue=@"";}
    [self updateWarehouseTransferControls];_cityEventLabel.stringValue=[NSString stringWithUTF8String:message.c_str()];
}
- (void)updateMapViewportControls {
    const auto& v=[_mapCanvasView viewport];
    _mapViewportLabel.stringValue=[NSString stringWithUTF8String:v.summary().c_str()];
    _mapPanToggle.state=_mapCanvasView.panMode?NSControlStateValueOn:NSControlStateValueOff;
    _mapZoomIn.enabled=v.zoom()<8;_mapZoomOut.enabled=v.zoom()>1;
}
- (void)onMapZoom:(NSButton*)sender {
    const auto& v=[_mapCanvasView viewport];double x=v.left()+v.span()/2,y=v.top()+v.span()/2;
    if(_selectedCursorX>=v.left()&&_selectedCursorX<v.left()+v.span()&&_selectedCursorY>=v.top()&&_selectedCursorY<v.top()+v.span())
        {x=_selectedCursorX+.5;y=_selectedCursorY+.5;}
    [_mapCanvasView zoomBy:sender.tag>0?2:.5 anchorX:x anchorY:y];
}
- (void)onMapPan:(NSButton*)sender {
    const double step=[_mapCanvasView viewport].span()/4;
    [_mapCanvasView panX:sender.tag==0?-step:sender.tag==3?step:0 y:sender.tag==1?-step:sender.tag==2?step:0];
}
- (void)onMapFull:(id)sender {(void)sender;[_mapCanvasView fullViewport];}
- (void)onMapDrag:(NSButton*)sender {_mapCanvasView.panMode=sender.state==NSControlStateValueOn;}
- (void)renderCityGrid {
    game_lab::CityFarmWarehouseRoute route;
    if(_activeBuildTool==game_lab::BuildingType::None && !_warehouseTransfer.active() &&
       _selectedCursorX>=0 && _selectedCursorY>=0)
        route=_citySim.farmWarehouseRoute(_selectedCursorX,_selectedCursorY);
    [_mapCanvasView setWarehouseRoute:route];
    _warehouseRouteLabel.stringValue=[NSString stringWithUTF8String:game_lab::describeCityFarmWarehouseRoute(route).c_str()];
    const auto pixels = _citySim.renderGridPixels(_selectedCursorX, _selectedCursorY);
    if (!presentPixels(_mapCanvasView, 128, 128, pixels)) {
        _mapCanvasView.image = nil;
        _mapInfoLabel.stringValue = @"地图画面转换或呈现失败";
    }
}

- (void)onSimulationStep:(NSTimer*)timer {
    (void)timer;
    const int oldMonth = _citySim.stats().month;
    _citySim.tick();
    if (_citySim.stats().month != oldMonth) {
        [self updateCityHUD];
        [self renderCityGrid];
    }
}

- (void)onSpeedChanged:(NSSegmentedControl*)sender {
    NSInteger seg = sender.selectedSegment;
    if (seg == 4) {
        _citySim.advanceMonth();
        [self updateCityHUD];
        [self renderCityGrid];
        sender.selectedSegment = _citySim.stats().speed;
    } else {
        _citySim.setSpeed((int)seg);
        [self updateCityHUD];
    }
}

- (void)onBuildToolChanged:(NSSegmentedControl*)sender {
    _mapCanvasView.panMode=NO;
    _warehouseTransfer.cancel();_warehouseTransferAmount.stringValue=@"";
    switch (sender.selectedSegment) {
        case 0: _activeBuildTool = game_lab::BuildingType::None; break;
        case 1: _activeBuildTool = game_lab::BuildingType::Road; break;
        case 2: _activeBuildTool = game_lab::BuildingType::House; break;
        case 3: _activeBuildTool = game_lab::BuildingType::Farm; break;
        case 4: _activeBuildTool = game_lab::BuildingType::Well; break;
        case 5: _activeBuildTool = game_lab::BuildingType::Temple; break;
        case 6: _activeBuildTool = game_lab::BuildingType::Market; break;
        case 7: _activeBuildTool = game_lab::BuildingType::Watchtower; break;
        case 8: _activeBuildTool = game_lab::BuildingType::Warehouse; break;
        case 9: _activeBuildTool = static_cast<game_lab::BuildingType>(99); break; // Demolish
        default: _activeBuildTool = game_lab::BuildingType::None; break;
    }
    [self updateWarehouseTransferControls];[self renderCityGrid];
}

- (void)handleTileClickAtX:(int)x y:(int)y {
    _selectedCursorX = x;
    _selectedCursorY = y;

    if (_warehouseTransfer.active()) {
        std::string message;
        if(_warehouseTransfer.choose(_citySim,x,y,message))
            _warehouseTransferAmount.stringValue=[NSString stringWithFormat:@"%d",_warehouseTransfer.maximum(_citySim)];
        [self updateWarehouseTransferControls];
        _cityEventLabel.stringValue=[NSString stringWithUTF8String:message.c_str()];
    } else if (_activeBuildTool == game_lab::BuildingType::None) {
        const std::string info = _citySim.inspectTile(x, y);
        _cityEventLabel.stringValue = [NSString stringWithUTF8String:info.c_str()];
        [self updateWarehouseTransferControls];
    } else {
        std::string msg;
        if (static_cast<int>(_activeBuildTool) == 99) {
            if (_citySim.demolish(x, y, msg)) {
                [self playSFX:@"Fire.wav"];
            }
        } else {
            if (_citySim.build(x, y, _activeBuildTool, msg)) {
                if (_activeBuildTool == game_lab::BuildingType::Temple) {
                    [self playSFX:@"H_ancestor_bless.wav"];
                } else if (_activeBuildTool == game_lab::BuildingType::Well) {
                    [self playSFX:@"D_flood.wav"];
                } else {
                    [self playSFX:@"Halberd_attack.wav"];
                }
            }
        }
        [self updateCityHUD];
        _cityEventLabel.stringValue = [NSString stringWithFormat:@"📜 帝都政务记录: %@", [NSString stringWithUTF8String:msg.c_str()]];
    }
    [self renderCityGrid];
}

- (void)onResetCity:(id)sender {
    (void)sender;
    _citySim.reset(game_lab::CityRuleset::StaffedServices);
    _warehouseTransfer.cancel();_warehouseTransferAmount.stringValue=@"";
    _cityScenario.disable();
    _cityGoalConfig = {};
    _cityGoalMemory = {};
    _selectedCursorX = _selectedCursorY = -1; [_mapCanvasView initialViewport];
    _activeBuildTool = game_lab::BuildingType::None;
    _buildToolControl.selectedSegment = 0;
    _speedControl.selectedSegment = _citySim.stats().speed;
    [self updateCityHUD]; [self renderCityGrid];
    _mapInfoLabel.stringValue = @"自写空城 · 服务需工 · 自由营建";
}

- (void)onNewWarehouseCity:(id)sender {
    (void)sender;
    _citySim.reset(game_lab::CityRuleset::RoadWarehouses); _citySim.setSpeed(0);
    _warehouseTransfer.cancel();_warehouseTransferAmount.stringValue=@"";
    _cityScenario.disable(); _cityGoalConfig = {}; _cityGoalMemory = {};
    _selectedCursorX = _selectedCursorY = -1; [_mapCanvasView initialViewport]; _activeBuildTool = game_lab::BuildingType::None;
    _buildToolControl.selectedSegment = 0; _speedControl.selectedSegment = 0;
    [self updateCityHUD]; [self renderCityGrid];
}
- (void)onEnableWarehouses:(id)sender {
    (void)sender;
    if (_cityScenario.active()) { _cityEventLabel.stringValue = @"挑战保留原规则；请另建仓储自由城。"; return; }
    const uint32_t x = _selectedCursorX >= 0 ? uint32_t(_selectedCursorX) : game_lab::CityWarehouseRules::initialX;
    const uint32_t y = _selectedCursorY >= 0 ? uint32_t(_selectedCursorY) : game_lab::CityWarehouseRules::initialY;
    std::string message;
    if (!_citySim.migrateToRoadWarehouses(x,y,message)) {
        _cityEventLabel.stringValue = [NSString stringWithUTF8String:message.c_str()]; return;
    }
    _cityGoalMemory = {}; [self updateCityHUD]; [self renderCityGrid];
    _cityEventLabel.stringValue = [NSString stringWithUTF8String:message.c_str()];
}

- (void)onStartCityScenario:(id)sender {
    (void)sender;
    const auto index = _cityScenarioPicker.indexOfSelectedItem;
    if (index < 0 || index >= 4) return;
    NSError* directoryError = nil;
    NSURL* support = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
        inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:&directoryError];
    NSURL* profile = [support URLByAppendingPathComponent:@"EmperorNativeMedia" isDirectory:YES];
    if (!profile || ![[NSFileManager defaultManager] createDirectoryAtURL:profile withIntermediateDirectories:YES attributes:nil error:&directoryError]) {
        _cityEventLabel.stringValue = @"无法创建本 App 挑战记录目录；现有城市保持。"; return;
    }
    const char* path = [profile URLByAppendingPathComponent:@"RunCounter.ecr"].fileSystemRepresentation;
    std::string error; uint64_t runId = 0;
    if (!path || !game_lab::allocateCityRunId(path,
        game_lab::assignedCityRunFloor(_cityScenario,_cityScenarioReceipts),runId,error)) {
        _cityEventLabel.stringValue = [NSString stringWithUTF8String:error.c_str()]; return;
    }
    const bool training = index == 3;
    game_lab::CitySimulation candidate(training ? game_lab::CityRuleset::RoadWarehouses : game_lab::CityRuleset::StaffedServices);
    candidate.setSpeed(0);
    game_lab::CityScenarioSession challenge;
    const auto& definition = training ? game_lab::selfAuthoredWarehouseTraining() : game_lab::selfAuthoredCityScenarios()[size_t(index)];
    const auto begun = training ? game_lab::beginCityWarehouseTraining(challenge,candidate,runId,_cityScenarioReceipts) :
        challenge.begin(definition.spec,runId,{candidate.stats().year,candidate.stats().month},_cityScenarioReceipts);
    if (!begun.ok) { _cityEventLabel.stringValue = [NSString stringWithUTF8String:begun.message.c_str()]; return; }
    _warehouseTransfer.cancel();_warehouseTransferAmount.stringValue=@"";
    _citySim = std::move(candidate); _cityScenario = std::move(challenge);
    _cityGoalConfig = definition.spec.goals; _cityGoalMemory = {};
    _selectedCursorX = _selectedCursorY = -1; [_mapCanvasView initialViewport]; _activeBuildTool = game_lab::BuildingType::None;
    _buildToolControl.selectedSegment = 0; _speedControl.selectedSegment = 0;
    [self updateCityHUD]; [self renderCityGrid];
    _mapInfoLabel.stringValue = [NSString stringWithUTF8String:definition.instructions.c_str()];
}

- (void)onSaveCity:(id)sender {
    (void)sender;
    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.nameFieldStringValue = @"city.emp";
    panel.message = @"保存自写城邑及当前挑战进度。";
    if ([panel runModal] != NSModalResponseOK) return;
    std::string error;
    const char* path = panel.URL.fileSystemRepresentation;
    [self updateCityHUD];
    const bool saved = game_lab::saveCityGameState(_citySim,_cityScenario,_cityScenarioReceipts,path ? path : "",error);
    _cityEventLabel.stringValue = saved ? @"城邑已保存" :
        [NSString stringWithUTF8String:error.c_str()];
}

- (void)onLoadCity:(id)sender {
    (void)sender;
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES; panel.canChooseDirectories = NO; panel.allowsMultipleSelection = NO;
    panel.message = @"选择自写城邑或挑战存档。";
    if ([panel runModal] != NSModalResponseOK) return;
    std::string error;
    const char* path = panel.URL.fileSystemRepresentation;
    if (!game_lab::loadCityGameState(_citySim,_cityScenario,_cityScenarioReceipts,path ? path : "",error)) {
        _cityEventLabel.stringValue = [NSString stringWithUTF8String:error.c_str()];
        return;
    }
    _warehouseTransfer.cancel();_warehouseTransferAmount.stringValue=@"";
    _cityGoalConfig = _cityScenario.active() ? _cityScenario.checkpoint().spec.goals : game_lab::CityGoalConfig{};
    _cityGoalMemory = {};
    _selectedCursorX = _selectedCursorY = -1; [_mapCanvasView initialViewport];
    _activeBuildTool = game_lab::BuildingType::None;
    _buildToolControl.selectedSegment = 0;
    _speedControl.selectedSegment = _citySim.stats().speed;
    [self updateCityHUD]; [self renderCityGrid];
    if (_citySim.ruleset() != game_lab::CityRuleset::RoadWarehouses)
        _mapInfoLabel.stringValue = @"已载入自写城邑存档";
}

- (void)loadAndRenderMap:(NSString*)filename {
    _mapCanvasView.image = nil;
    _selectedCursorX = -1;
    _selectedCursorY = -1;
    [_mapCanvasView initialViewport];
    if (!filename || !filename.UTF8String) {
        [self updateCityHUD];
        [self renderCityGrid];
        _mapInfoLabel.stringValue = @"未指定地图文件";
        _cityEventLabel.stringValue = @"📜 地图载入失败：未指定文件。";
        return;
    }
    const auto relative = [NSString stringWithFormat:@"Cities/%@", filename];
    const auto cityMap = _media->cityMap(relative.UTF8String);
    if (!cityMap.valid()) {
        [self updateCityHUD];
        [self renderCityGrid];
        _mapInfoLabel.stringValue = [NSString stringWithFormat:@"无法载入地图: %@", filename];
        _cityEventLabel.stringValue = [NSString stringWithFormat:@"📜 地图载入失败: %@", filename];
        return;
    }
    
    auto freshCity = game_lab::CitySimulation(game_lab::CityRuleset::StaffedServices);
    if (!freshCity.loadFromMap(cityMap)) {
        _cityEventLabel.stringValue = @"地图无效，现有城市保持。";
        return;
    }
    _citySim = std::move(freshCity);
    _warehouseTransfer.cancel();_warehouseTransferAmount.stringValue=@"";
    _cityScenario.disable();
    _cityGoalConfig = {};
    _cityGoalMemory = {};
    _mapInfoLabel.stringValue = [NSString stringWithFormat:@"地图: %@ · 128×128 地块已载入 · ⚠️ 自写城市营建", filename];
    [self updateCityHUD];
    [self renderCityGrid];
}


// ----------------------------------------------------
// Tab 2: Sprites & 555 Textures View
// ----------------------------------------------------
- (void)setupSpriteView {
    _spriteView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _spriteView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    NSTextField* title = [NSTextField labelWithString:@"图素与贴图浏览器 (SG3 / 555)"];
    title.frame = NSMakeRect(20, _spriteView.bounds.size.height - 38, 900, 26);
    title.font = [NSFont boldSystemFontOfSize:18];
    [_spriteView addSubview:title];
    
    _spritePackagePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(20, _spriteView.bounds.size.height - 80, 520, 30) pullsDown:NO];
    _sg3Packages = _media->sg3Packages();
    for (const auto& relative : _sg3Packages)
        [_spritePackagePopup addItemWithTitle:[NSString stringWithUTF8String:std::filesystem::path(relative).filename().string().c_str()]];
    if (_sg3Packages.empty()) [_spritePackagePopup addItemWithTitle:@"目录内没有 SG3；请用 --data-root 指定 GameData"];
    // Start with an existing internal companion. Missing archives remain listed
    // and show an explicit failure when selected, without keeping an old image.
    for (size_t i = 0; i < _sg3Packages.size(); ++i) {
        auto path = std::filesystem::path(_sg3Packages[i]); path.replace_extension(".555");
        if (!_media->resourcePath(path.generic_string()).empty()) {
            [_spritePackagePopup selectItemAtIndex:static_cast<NSInteger>(i)]; break;
        }
    }
    _spritePackagePopup.accessibilityIdentifier = @"sg3-package";
    title.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    _spritePackagePopup.autoresizingMask = NSViewMinYMargin | NSViewWidthSizable;
    _spritePackagePopup.target = self;
    _spritePackagePopup.action = @selector(onSpritePackageChanged:);
    [_spriteView addSubview:_spritePackagePopup];
    
    _spriteSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(560, _spriteView.bounds.size.height - 74, 420, 24)];
    _spriteSlider.accessibilityIdentifier = @"sg3-image-index";
    _spriteSlider.minValue = 0;
    _spriteSlider.maxValue = 100;
    _spriteSlider.autoresizingMask = NSViewMinYMargin;
    _spriteSlider.target = self;
    _spriteSlider.action = @selector(onSpriteSliderChanged:);
    [_spriteView addSubview:_spriteSlider];
    
    NSTextField* idLabel=[NSTextField labelWithString:@"物理 ID（1起）"];
    idLabel.frame=NSMakeRect(20,_spriteView.bounds.size.height-114,130,24);idLabel.autoresizingMask=NSViewMinYMargin;[_spriteView addSubview:idLabel];
    _spriteIdField=[[NSTextField alloc] initWithFrame:NSMakeRect(156,_spriteView.bounds.size.height-114,90,24)];
    _spriteIdField.target=self;_spriteIdField.action=@selector(onSpriteIdChanged:);_spriteIdField.autoresizingMask=NSViewMinYMargin;
    _spriteIdField.accessibilityIdentifier=@"sg3-image-id";[_spriteView addSubview:_spriteIdField];
    _spriteIdApply=[NSButton buttonWithTitle:@"显示" target:self action:@selector(onSpriteIdChanged:)];
    _spriteIdApply.frame=NSMakeRect(258,_spriteView.bounds.size.height-116,80,28);_spriteIdApply.autoresizingMask=NSViewMinYMargin;
    _spriteIdApply.accessibilityIdentifier=@"sg3-id-apply";[_spriteView addSubview:_spriteIdApply];
    _spriteBaseCanvas=[NSButton checkboxWithTitle:@"单格基底居中对照（1逻辑像素=1点）" target:self action:@selector(onSpriteSliderChanged:)];
    _spriteBaseCanvas.frame=NSMakeRect(360,_spriteView.bounds.size.height-114,620,24);_spriteBaseCanvas.autoresizingMask=NSViewMinYMargin;
    _spriteBaseCanvas.accessibilityIdentifier=@"sg3-base-canvas";[_spriteView addSubview:_spriteBaseCanvas];

    NSTextField* sequenceLabel=[NSTextField labelWithString:@"明确帧ID列表"];
    sequenceLabel.frame=NSMakeRect(20,_spriteView.bounds.size.height-150,130,24);sequenceLabel.autoresizingMask=NSViewMinYMargin;[_spriteView addSubview:sequenceLabel];
    _spriteSequenceIds=[[NSTextField alloc] initWithFrame:NSMakeRect(156,_spriteView.bounds.size.height-150,640,24)];
    _spriteSequenceIds.placeholderString=@"物理ID，用逗号分隔；不自动推断帧链";_spriteSequenceIds.accessibilityIdentifier=@"sg3-sequence-ids";
    _spriteSequenceIds.autoresizingMask=NSViewMinYMargin;_spriteSequenceIds.target=self;_spriteSequenceIds.action=@selector(onSpriteSequenceApply:);[_spriteView addSubview:_spriteSequenceIds];
    _spriteSequenceApply=[NSButton buttonWithTitle:@"应用帧列表" target:self action:@selector(onSpriteSequenceApply:)];
    _spriteSequenceApply.frame=NSMakeRect(810,_spriteView.bounds.size.height-152,160,28);_spriteSequenceApply.autoresizingMask=NSViewMinYMargin;_spriteSequenceApply.accessibilityIdentifier=@"sg3-sequence-apply";[_spriteView addSubview:_spriteSequenceApply];
    _spriteSequencePrevious=[NSButton buttonWithTitle:@"上一帧" target:self action:@selector(onSpriteSequenceStep:)];
    _spriteSequenceNext=[NSButton buttonWithTitle:@"下一帧" target:self action:@selector(onSpriteSequenceStep:)];
    _spriteSequencePlay=[NSButton buttonWithTitle:@"播放" target:self action:@selector(onSpriteSequencePlay:)];
    NSArray* sequenceButtons=@[_spriteSequencePrevious,_spriteSequenceNext,_spriteSequencePlay];
    NSArray* sequenceNames=@[@"sg3-sequence-previous",@"sg3-sequence-next",@"sg3-sequence-play"];
    for(NSUInteger i=0;i<sequenceButtons.count;++i){NSButton* b=sequenceButtons[i];b.frame=NSMakeRect(20+100*i,_spriteView.bounds.size.height-188,90,28);b.autoresizingMask=NSViewMinYMargin;b.accessibilityIdentifier=sequenceNames[i];[_spriteView addSubview:b];}
    NSTextField* rateLabel=[NSTextField labelWithString:@"展示 FPS"];
    rateLabel.frame=NSMakeRect(324,_spriteView.bounds.size.height-186,80,24);rateLabel.autoresizingMask=NSViewMinYMargin;[_spriteView addSubview:rateLabel];
    _spriteSequenceRate=[[NSTextField alloc] initWithFrame:NSMakeRect(410,_spriteView.bounds.size.height-186,60,24)];
    _spriteSequenceRate.stringValue=@"8";_spriteSequenceRate.accessibilityIdentifier=@"sg3-sequence-rate";_spriteSequenceRate.autoresizingMask=NSViewMinYMargin;_spriteSequenceRate.target=self;_spriteSequenceRate.action=@selector(onSpriteSequenceRateChanged:);[_spriteView addSubview:_spriteSequenceRate];
    _spriteSequenceAlign=[NSButton checkboxWithTitle:@"减去原始X/Y偏移对齐（预览约定）" target:self action:@selector(onSpriteSequenceApply:)];
    _spriteSequenceAlign.frame=NSMakeRect(490,_spriteView.bounds.size.height-186,490,24);_spriteSequenceAlign.state=NSControlStateValueOn;_spriteSequenceAlign.autoresizingMask=NSViewMinYMargin;_spriteSequenceAlign.accessibilityIdentifier=@"sg3-sequence-align";[_spriteView addSubview:_spriteSequenceAlign];

    _spriteInfoLabel = [NSTextField labelWithString:@"图素信息: 请选择图素包"];
    _spriteInfoLabel.frame = NSMakeRect(20, _spriteView.bounds.size.height - 280, 984, 84);
    _spriteInfoLabel.autoresizingMask = NSViewMinYMargin | NSViewWidthSizable;
    _spriteInfoLabel.maximumNumberOfLines = 4;
    _spriteInfoLabel.accessibilityIdentifier = @"sg3-image-info";
    [_spriteView addSubview:_spriteInfoLabel];
    
    _spriteImageView = [[EmperorPixelImageView alloc] initWithFrame:NSMakeRect(20, 20, 984, _spriteView.bounds.size.height - 314)];
    _spriteImageView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    _spriteImageView.accessibilityIdentifier = @"sg3-native-image";
    _spriteImageView.imageScaling = NSImageScaleProportionallyUpOrDown;
    _spriteImageView.wantsLayer = YES;
    _spriteImageView.layer.borderColor = [NSColor darkGrayColor].CGColor;
    _spriteImageView.layer.borderWidth = 1;
    [_spriteView addSubview:_spriteImageView];
    
    [self onSpritePackageChanged:nil];
}

- (void)onSpritePackageChanged:(id)sender {
    (void)sender;
    [self stopSpriteSequence];_spriteSequence={};_spriteSequenceIds.stringValue=@"";
    _spriteSequencePrevious.enabled=NO;_spriteSequenceNext.enabled=NO;_spriteSequencePlay.enabled=NO;
    NSInteger sel = _spritePackagePopup.indexOfSelectedItem;
    _spriteStaticIndex.reset();_spriteStaticPixels={};_spriteExplicitSelection=NO;_originalManualAdopt.enabled=NO;
    _currentSG3 = {};
    _current555Path.clear();
    _spriteImageView.image = nil;
    _spriteSlider.enabled = NO;
    _spriteIdField.enabled=NO;_spriteIdApply.enabled=NO;_spriteBaseCanvas.enabled=NO;
    _spriteImageView.accessibilityLabel=@"没有显示SG3图像";
    _spriteInfoLabel.stringValue = @"资源不可读取或图素布局不受支持";
    if (sel < 0 || static_cast<size_t>(sel) >= _sg3Packages.size()) return;
    const auto& relative = _sg3Packages[static_cast<size_t>(sel)];
    auto companion = std::filesystem::path(relative); companion.replace_extension(".555");
    _current555Path = companion.generic_string();
    _currentSG3 = _media->sg3(relative);
    _spriteSlider.hidden = NO;
    if (!_currentSG3.sprites.empty()) {
        _spriteSlider.enabled = YES;
        _spriteIdField.enabled=YES;_spriteIdApply.enabled=YES;
        _spriteSlider.maxValue = _currentSG3.sprites.size() - 1;
        _spriteSlider.doubleValue = 0;
        [self renderSpriteAtIndex:0];
    } else _spriteInfoLabel.stringValue = @"SG3 索引无效或不受支持；未显示旧图像";

}

- (void)onSpriteSliderChanged:(id)sender {
    (void)sender;_spriteExplicitSelection=YES;[self stopSpriteSequence];_spriteSequence={};
    _spriteSequencePrevious.enabled=NO;_spriteSequenceNext.enabled=NO;_spriteSequencePlay.enabled=NO;
    size_t idx = static_cast<size_t>(_spriteSlider.doubleValue);
    [self renderSpriteAtIndex:idx];
}

- (void)onSpriteIdChanged:(id)sender {
    (void)sender;_spriteExplicitSelection=YES;[self stopSpriteSequence];_spriteSequence={};
    _spriteSequencePrevious.enabled=NO;_spriteSequenceNext.enabled=NO;_spriteSequencePlay.enabled=NO;
    NSString* text=[_spriteIdField.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    const char* raw=text.UTF8String;char* end=nullptr;const auto id=raw?std::strtoull(raw,&end,10):0;
    if(!raw||!*raw||!end||end==raw||*end!='\0'||!id||id>_currentSG3.sprites.size()) {
        _spriteStaticIndex.reset();_spriteStaticPixels={};_originalManualAdopt.enabled=NO;
        _spriteImageView.image=nil;_spriteImageView.accessibilityLabel=@"没有显示SG3图像";
        _spriteInfoLabel.stringValue=@"物理ID须在当前包使用中范围内；旧图像已清空";return;
    }
    _spriteSlider.doubleValue=double(id-1);[self renderSpriteAtIndex:size_t(id-1)];
}

- (void)renderSpriteAtIndex:(size_t)idx {
    _spriteStaticIndex.reset();_spriteStaticPixels={};_originalManualAdopt.enabled=NO;
    _spriteImageView.image=nil;_spriteImageView.accessibilityLabel=@"没有显示SG3图像";
    if (idx >= _currentSG3.sprites.size()) {
        _spriteImageView.image = nil;
        _spriteInfoLabel.stringValue = @"图素序号无效";
        return;
    }
    const auto& s = _currentSG3.sprites[idx];
    const auto delivery = _media->sprite(_currentSG3, idx, _current555Path);
    const auto& extracted = delivery.image;
    _spriteIdField.stringValue=[NSString stringWithFormat:@"%zu",idx+1];
    const auto geometry=game_lab::sg3SingleCellGeometry(_currentSG3,idx);
    _spriteBaseCanvas.enabled=geometry.has_value();
    const bool baseCanvas=geometry&&_spriteBaseCanvas.state==NSControlStateValueOn;
    const auto displayed=baseCanvas?game_lab::renderSG3SingleCellCanvas(extracted,{}):extracted;
    _spriteImageView.imageScaling=baseCanvas?NSImageScaleNone:NSImageScaleProportionallyUpOrDown;
    if (!presentPixels(_spriteImageView, displayed.width, displayed.height, displayed.pixelsBGRA)) {
        _spriteImageView.image = nil;
        if (delivery.status == game_lab::SG3DecodeStatus::EmptyRecord)
            _spriteInfoLabel.stringValue = @"空图像记录：没有像素可显示；不计入解码成功，未显示旧图像";
        else if (delivery.status == game_lab::SG3DecodeStatus::MissingDependency)
            _spriteInfoLabel.stringValue = @"像素文件依赖缺失；未显示旧图像";
        else _spriteInfoLabel.stringValue = @"图像记录/载荷无效、格式不受支持或转换失败；未显示旧图像";
        return;
    }
    
    if(extracted.width<=2048&&extracted.height<=2048){_spriteStaticPixels=extracted;_spriteStaticIndex=idx;_originalManualAdopt.enabled=_spriteExplicitSelection&&_originalSelectedSlot.has_value();}
    _spriteInfoLabel.stringValue = [NSString stringWithFormat:@"图像 ID: %zu / %zu（使用中）· SG3 v%u · 类型: %u · 尺寸: %dx%d\n镜像: %@ · Alpha: %@ · 已原生显示；与原版视觉一致性未测",
        idx + 1, _currentSG3.sprites.size(), _currentSG3.version, s.type, extracted.width, extracted.height,
        delivery.mirrored ? @"是" : @"否", delivery.alphaApplied ? @"是" : @"否"];
    if(baseCanvas)_spriteInfoLabel.stringValue=[_spriteInfoLabel.stringValue stringByAppendingString:@"\n手动单格画布按底部78×40基底居中，越界裁剪；MAP选图与原版绘制锚点未知"];
    _spriteImageView.accessibilityLabel=[NSString stringWithFormat:@"%@，图像ID %zu，原始尺寸 %u×%u，%@",_spritePackagePopup.titleOfSelectedItem,idx+1,extracted.width,extracted.height,baseCanvas?@"单格基底居中对照，500×240，MAP选图未知":@"原始画布预览"];

}

- (void)stopSpriteSequence {
    [_spriteSequenceTimer invalidate];_spriteSequenceTimer=nil;_spriteSequencePlay.title=@"播放";
}
- (void)onSpriteSequenceRateChanged:(id)sender {(void)sender;[self stopSpriteSequence];[self renderSpriteSequence];}
- (void)onSpriteSequenceApply:(id)sender {
    _spriteExplicitSelection=NO;_originalManualAdopt.enabled=NO;
    (void)sender;[self stopSpriteSequence];_spriteSequence={};_spriteImageView.image=nil;
    _spriteImageView.accessibilityLabel=@"没有显示SG3图像";
    _spriteSequencePrevious.enabled=NO;_spriteSequenceNext.enabled=NO;_spriteSequencePlay.enabled=NO;
    const auto ids=game_lab::parseSG3SequenceIds(_spriteSequenceIds.stringValue.UTF8String,_currentSG3.sprites.size());
    const NSInteger selected=_spritePackagePopup.indexOfSelectedItem;
    if(!ids||selected<0||size_t(selected)>=_sg3Packages.size()){_spriteInfoLabel.stringValue=@"须提供当前包内1..256个明确物理ID；无效列表已清空预览";return;}
    const auto path=_media->resourcePath(_sg3Packages[size_t(selected)]);
    auto companion=std::filesystem::path(path);companion.replace_extension(".555");
    if(!path.empty())_spriteSequence=game_lab::prepareSG3Sequence(_currentSG3,*ids,companion.string(),_spriteSequenceAlign.state==NSControlStateValueOn);
    if(!_spriteSequence.valid()){_spriteInfoLabel.stringValue=[NSString stringWithFormat:@"序列未发布：%s；失败帧不跳过，旧图像已清空",_spriteSequence.error.c_str()];return;}
    _spriteSequenceFrame=0;_spriteSequencePrevious.enabled=YES;_spriteSequenceNext.enabled=YES;_spriteSequencePlay.enabled=YES;
    [self renderSpriteSequence];
}
- (void)renderSpriteSequence {
    if(!_spriteSequence.valid()||_spriteSequenceFrame>=_spriteSequence.frames.size())return;
    const auto& image=_spriteSequence.frames[_spriteSequenceFrame];const auto id=_spriteSequence.physicalIds[_spriteSequenceFrame];const auto& record=_currentSG3.sprites[id-1];
    _spriteImageView.imageScaling=NSImageScaleProportionallyUpOrDown;
    if(!presentPixels(_spriteImageView,image.width,image.height,image.pixelsBGRA)){[self stopSpriteSequence];_spriteImageView.image=nil;_spriteImageView.accessibilityLabel=@"没有显示SG3图像";_spriteInfoLabel.stringValue=@"序列画布转换失败，播放已停止";return;}
    _spriteSlider.doubleValue=double(id-1);_spriteIdField.stringValue=[NSString stringWithFormat:@"%u",id];
    const char* bitmap=record.bitmapId<_currentSG3.bitmaps.size()?_currentSG3.bitmaps[record.bitmapId].filename.c_str():"unknown";
    _spriteInfoLabel.stringValue=[NSString stringWithFormat:@"明确序列帧 %zu/%zu · 物理ID %u（vector %u）· %@ · 展示循环，非原版计时\nbitmap %s · 尺寸 %d×%d · count %u · offset %d,%d · mirror %d · reverse %u · speedID %u（未解释）\n%@；不推断方向/帧链，不绑定建筑底图或城市。原始解码像素，原版阴影合成未验证。",_spriteSequenceFrame+1,_spriteSequence.frames.size(),id,id-1,_spriteSequenceTimer?@"播放中":@"已暂停",bitmap,record.width,record.height,record.animationSpriteCount,record.animationOffsetX,record.animationOffsetY,record.mirrorOffset,unsigned(record.animationReversible),unsigned(record.animationSpeedId),_spriteSequence.offsetAligned?@"减去公开原始偏移，共用固定画布（预览约定，非原版锚点）":@"原始画布左上对齐，共用固定画布"];
    _spriteImageView.accessibilityLabel=[NSString stringWithFormat:@"%@，明确序列帧 %zu/%zu，物理ID %u，公共画布 %u×%u，非原版计时",_spritePackagePopup.titleOfSelectedItem,_spriteSequenceFrame+1,_spriteSequence.frames.size(),id,image.width,image.height];
}
- (void)onSpriteSequenceStep:(id)sender {
    [self stopSpriteSequence];if(!_spriteSequence.valid())return;
    _spriteSequenceFrame=game_lab::sg3SequenceStep(_spriteSequenceFrame,_spriteSequence.frames.size(),sender==_spriteSequencePrevious);[self renderSpriteSequence];
}
- (void)onSpriteSequencePlay:(id)sender {
    (void)sender;if(_spriteSequenceTimer){[self stopSpriteSequence];[self renderSpriteSequence];return;}
    if(!_spriteSequence.valid())return;
    double rate=0;NSScanner* scan=[NSScanner scannerWithString:_spriteSequenceRate.stringValue];
    if(![scan scanDouble:&rate]||!scan.isAtEnd||!game_lab::validSG3PreviewRate(rate)){_spriteInfoLabel.stringValue=@"展示FPS须为1..30的有限数值；没有解释原版speed ID";return;}
    __weak EmperorAppDelegate* weakSelf=self;
    _spriteSequenceTimer=[NSTimer timerWithTimeInterval:1.0/rate repeats:YES block:^(NSTimer* timer){
        (void)timer;EmperorAppDelegate* owner=weakSelf;if(!owner)return;
        owner->_spriteSequenceFrame=game_lab::sg3SequenceStep(owner->_spriteSequenceFrame,owner->_spriteSequence.frames.size());[owner renderSpriteSequence];
    }];
    [NSRunLoop.mainRunLoop addTimer:_spriteSequenceTimer forMode:NSRunLoopCommonModes];_spriteSequencePlay.title=@"暂停";[self renderSpriteSequence];
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
    
    _filteredTexts = [NSMutableArray array];
    
    _textSearchField = [[NSSearchField alloc] initWithFrame:NSMakeRect(20, _textView.bounds.size.height - 45, 400, 30)];
    _textSearchField.autoresizingMask = NSViewMinYMargin;
    _textSearchField.placeholderString = @"搜索游戏文本（如：新建、建造、粮食、战役）";
    _textSearchField.target = self;
    _textSearchField.action = @selector(onTextSearch:);
    [_textView addSubview:_textSearchField];
    NSButton* reload = [NSButton buttonWithTitle:@"重新加载" target:self action:@selector(onReloadText:)];
    reload.frame = NSMakeRect(440, _textView.bounds.size.height - 45, 110, 30);
    reload.autoresizingMask = NSViewMinYMargin;
    [_textView addSubview:reload];
    
    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:
        NSMakeRect(20, 20, _textView.bounds.size.width - 40, _textView.bounds.size.height - 75)];
    scroll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    _textTableView = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"TextLine"];
    col.title = @"文本资源待载入";
    col.width = 960;
    [_textTableView addTableColumn:col];
    _textTableView.delegate = self;
    _textTableView.dataSource = self;
    scroll.documentView = _textTableView;
    [_textView addSubview:scroll];
    [self onReloadText:nil];
}

- (void)onReloadText:(id)sender {
    (void)sender;
    const auto result = _media->textWithStatus("EmperorText.txt");
    _chineseTexts = result.lines; // Failed replacement clears old rows before filtering.
    NSTableColumn* column = _textTableView.tableColumns.firstObject;
    column.title = [NSString stringWithUTF8String:game_lab::formatTextLoadResult(result).c_str()];
    [self onTextSearch:_textSearchField];
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

- (void)setupOriginalMapView {
    _originalMapView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _originalMapView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    const CGFloat height = _originalMapView.bounds.size.height;
    NSTextField* title = [NSTextField labelWithString:@"原始城市数据 · MAP / PAK 地表图素 · SAV 数值"];
    title.frame = NSMakeRect(20, height - 38, 570, 26);
    title.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    title.font = [NSFont boldSystemFontOfSize:17]; [_originalMapView addSubview:title];
    _originalMapPicker = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(20, height - 80, 560, 30) pullsDown:NO];
    const auto catalog = _media->originalMapCatalog();
    _originalMapSources = catalog.sources;
    _originalMapVisibleSources=game_lab::originalMapSourceMatches(_originalMapSources,"");
    for(const auto& source : _originalMapSources) [_originalMapPicker addItemWithTitle:[NSString stringWithUTF8String:source.label.c_str()]];
    if(_originalMapSources.empty()) [_originalMapPicker addItemWithTitle:catalog.error.empty()?@"数据目录没有 MAP 或战役包":[NSString stringWithUTF8String:catalog.error.c_str()]];
    for(size_t i = 0; i < _originalMapSources.size(); ++i) if(_originalMapSources[i].kind==game_lab::OriginalMapSourceKind::MapFile&&_originalMapSources[i].relativePath.rfind("Cities/", 0) == 0) {
        [_originalMapPicker selectItemAtIndex:static_cast<NSInteger>(i)]; break;
    }
    _originalMapPicker.target = self; _originalMapPicker.action = @selector(onOriginalMapChanged:);
    _originalMapPicker.autoresizingMask = NSViewMinYMargin;
    _originalMapPicker.accessibilityIdentifier = @"original-map-file"; [_originalMapView addSubview:_originalMapPicker];
    _originalMapFilter=[[NSTextField alloc] initWithFrame:NSMakeRect(620,height-40,270,26)];
    _originalMapFilter.placeholderString=@"筛选来源名称或包内城市";_originalMapFilter.target=self;_originalMapFilter.action=@selector(onOriginalMapFilterChanged:);
    _originalMapFilter.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;_originalMapFilter.accessibilityIdentifier=@"original-map-source-filter";[_originalMapView addSubview:_originalMapFilter];
    NSButton* filter=[NSButton buttonWithTitle:@"筛选来源" target:self action:@selector(onOriginalMapFilterChanged:)];
    filter.frame=NSMakeRect(900,height-40,100,26);filter.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;filter.accessibilityIdentifier=@"original-map-filter-apply";[_originalMapView addSubview:filter];
    _originalMapBitPicker = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(600, height - 80, 400, 30) pullsDown:NO];
    _originalMapBitPicker.menu.autoenablesItems=NO;
    [_originalMapBitPicker addItemWithTitle:@"完整标志值（诊断色，非原版调色板）"];
    for(int bit = 0; bit < 32; ++bit) {
        NSString* annotation = [NSString stringWithUTF8String:game_lab::originalMapFlagLabel(bit).c_str()];
        [_originalMapBitPicker addItemWithTitle:[NSString stringWithFormat:@"位 %d · 0x%08x · %@", bit, uint32_t(1) << bit, annotation]];
    }
    for(unsigned field=0;field<5;++field){
        [_originalMapBitPicker addItemWithTitle:[NSString stringWithUTF8String:game_lab::originalSaveFieldLabel(field).c_str()]];
        _originalMapBitPicker.lastItem.enabled=NO;
    }
    [_originalMapBitPicker addItemWithTitle:@"存储地表图素（MAP / PAK 城市）"];
    [_originalMapBitPicker selectItemAtIndex:38];
    _originalMapBitPicker.target = self; _originalMapBitPicker.action = @selector(onOriginalMapPlaneChanged:);
    _originalMapBitPicker.autoresizingMask = NSViewMinYMargin;
    _originalMapBitPicker.accessibilityIdentifier = @"original-map-plane"; [_originalMapView addSubview:_originalMapBitPicker];
    _originalMapCrop = [NSButton checkboxWithTitle:@"已验证中心活动框裁剪（按地图外标志留空，保留边缘格）" target:self action:@selector(onOriginalMapPlaneChanged:)];
    _originalMapCrop.frame = NSMakeRect(20, height - 114, 560, 24);
    _originalMapCrop.state = NSControlStateValueOn;
    _originalMapCrop.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    _originalMapCrop.accessibilityIdentifier = @"original-map-crop"; [_originalMapView addSubview:_originalMapCrop];
    _originalMapSpatial=[NSButton checkboxWithTitle:@"等距空间（SAV 含已绑定建筑基座）" target:self action:@selector(onOriginalMapPlaneChanged:)];
    _originalMapSpatial.frame=NSMakeRect(620,height-114,380,24);_originalMapSpatial.autoresizingMask=NSViewMinYMargin;
    _originalMapSpatial.accessibilityIdentifier=@"original-map-spatial";[_originalMapView addSubview:_originalMapSpatial];
    _originalFigureSource=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(620,height-82,380,24) pullsDown:NO];
    [_originalFigureSource addItemsWithTitles:@[@"Saved snapshot",@"post-load k=1"]];
    _originalFigureSource.target=self;_originalFigureSource.action=@selector(onOriginalFigureSourceChanged:);
    _originalFigureSource.autoresizingMask=NSViewMinYMargin;_originalFigureSource.enabled=NO;
    _originalFigureSource.accessibilityIdentifier=@"original-figure-render-source";[_originalMapView addSubview:_originalFigureSource];
    NSTextField* cameraLabel=[NSTextField labelWithString:@"相机中心 · 活动格 X / Y"];
    cameraLabel.frame=NSMakeRect(20,height-146,240,24);cameraLabel.autoresizingMask=NSViewMinYMargin;[_originalMapView addSubview:cameraLabel];
    _originalMapCameraX=[[NSTextField alloc] initWithFrame:NSMakeRect(264,height-148,104,26)];
    _originalMapCameraY=[[NSTextField alloc] initWithFrame:NSMakeRect(382,height-148,104,26)];
    for(NSTextField* field in @[_originalMapCameraX,_originalMapCameraY]){field.target=self;field.action=@selector(onOriginalMapPlaneChanged:);field.autoresizingMask=NSViewMinYMargin;[_originalMapView addSubview:field];}
    _originalMapCameraX.accessibilityIdentifier=@"original-map-camera-x";_originalMapCameraY.accessibilityIdentifier=@"original-map-camera-y";
    _originalMapCameraApply=[NSButton buttonWithTitle:@"应用相机" target:self action:@selector(onOriginalMapPlaneChanged:)];
    _originalMapCameraApply.frame=NSMakeRect(506,height-148,100,26);_originalMapCameraApply.autoresizingMask=NSViewMinYMargin;
    _originalMapCameraApply.accessibilityIdentifier=@"original-map-camera-apply";[_originalMapView addSubview:_originalMapCameraApply];
    _originalMapOutlines=[NSButton checkboxWithTitle:@"显示格轮廓" target:self action:@selector(onOriginalMapPlaneChanged:)];
    _originalMapOutlines.frame=NSMakeRect(620,height-146,300,24);_originalMapOutlines.state=NSControlStateValueOn;
    _originalMapOutlines.autoresizingMask=NSViewMinYMargin;_originalMapOutlines.accessibilityIdentifier=@"original-map-outlines";[_originalMapView addSubview:_originalMapOutlines];
    _originalMapCandidates=[NSButton checkboxWithTitle:@"候选补全（原版放置未验证）" target:self action:@selector(onOriginalMapPlaneChanged:)];
    _originalMapCandidates.frame=NSMakeRect(620,height-146,380,24);_originalMapCandidates.state=NSControlStateValueOn;
    _originalMapCandidates.autoresizingMask=NSViewMinYMargin;_originalMapCandidates.accessibilityIdentifier=@"original-map-candidate-preview";[_originalMapView addSubview:_originalMapCandidates];
    _originalMapInfo = [NSTextField wrappingLabelWithString:@"等待地图载入"];
    _originalMapInfo.frame = NSMakeRect(20, height - 244, 980, 88);
    _originalMapInfo.maximumNumberOfLines = 6; _originalMapInfo.font = [NSFont systemFontOfSize:12];
    _originalMapInfo.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    _originalMapInfo.accessibilityIdentifier = @"original-map-info"; [_originalMapView addSubview:_originalMapInfo];
    NSTextField* cellLabel=[NSTextField labelWithString:@"SAV 查格 · 0基活动坐标 X / Y"];
    cellLabel.frame=NSMakeRect(20,height-278,240,24);cellLabel.autoresizingMask=NSViewMinYMargin;[_originalMapView addSubview:cellLabel];
    _originalSaveCellX=[[NSTextField alloc] initWithFrame:NSMakeRect(264,height-280,104,26)];
    _originalSaveCellY=[[NSTextField alloc] initWithFrame:NSMakeRect(382,height-280,104,26)];
    for(NSTextField* field in @[_originalSaveCellX,_originalSaveCellY]){field.stringValue=@"0";field.target=self;field.action=@selector(onOriginalSaveCellChanged:);field.autoresizingMask=NSViewMinYMargin;[_originalMapView addSubview:field];}
    _originalSaveCellX.accessibilityIdentifier=@"original-save-cell-x";_originalSaveCellY.accessibilityIdentifier=@"original-save-cell-y";
    _originalSaveCellApply=[NSButton buttonWithTitle:@"查看六个原值" target:self action:@selector(onOriginalSaveCellChanged:)];
    _originalSaveCellApply.frame=NSMakeRect(506,height-280,160,26);_originalSaveCellApply.autoresizingMask=NSViewMinYMargin;_originalSaveCellApply.accessibilityIdentifier=@"original-save-cell-apply";[_originalMapView addSubview:_originalSaveCellApply];
    _originalBuildingLocate=[NSButton buttonWithTitle:@"定位建筑记录" target:self action:@selector(onOriginalBuildingLocate:)];
    NSTextField* slotLabel=[NSTextField labelWithString:@"建筑槽"];slotLabel.frame=NSMakeRect(680,height-278,48,24);slotLabel.autoresizingMask=NSViewMinYMargin;[_originalMapView addSubview:slotLabel];
    _originalBuildingSlot=[[NSTextField alloc]initWithFrame:NSMakeRect(732,height-280,58,26)];_originalBuildingSlot.accessibilityIdentifier=@"original-building-slot";_originalBuildingSlot.target=self;_originalBuildingSlot.action=@selector(onOriginalBuildingLocate:);_originalBuildingSlot.autoresizingMask=NSViewMinYMargin;[_originalMapView addSubview:_originalBuildingSlot];
    _originalBuildingLocate.frame=NSMakeRect(800,height-280,180,26);_originalBuildingLocate.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;
    _originalBuildingLocate.accessibilityIdentifier=@"original-building-locate";_originalBuildingLocate.enabled=NO;[_originalMapView addSubview:_originalBuildingLocate];
    _originalManualAdopt=[NSButton buttonWithTitle:@"使用 SG3 当前图素（人工候选）" target:self action:@selector(onOriginalManualAdopt:)];
    _originalManualVisible=[NSButton checkboxWithTitle:@"显示人工候选 · 锚点未验" target:self action:@selector(onOriginalMapPlaneChanged:)];
    _originalManualClear=[NSButton buttonWithTitle:@"清除" target:self action:@selector(onOriginalManualClear:)];
    _originalManualSave=[NSButton buttonWithTitle:@"保存本地候选" target:self action:@selector(onOriginalManualSave:)];
    NSButton* manualLoad=[NSButton buttonWithTitle:@"载入本地候选" target:self action:@selector(onOriginalManualLoad:)];
    NSArray* buttons=@[_originalManualAdopt,_originalManualVisible,_originalManualClear,_originalManualSave,manualLoad];NSArray* ids=@[@"original-manual-adopt",@"original-manual-visible",@"original-manual-clear",@"original-manual-save",@"original-manual-load"];
    const CGFloat bx[]={20,320,590,680,820},bw[]={290,260,80,130,160};for(NSUInteger i=0;i<buttons.count;++i){NSButton* b=buttons[i];b.frame=NSMakeRect(bx[i],height-318,bw[i],26);b.autoresizingMask=NSViewMinYMargin;b.accessibilityIdentifier=ids[i];[_originalMapView addSubview:b];}
    _originalManualAdopt.enabled=NO;_originalManualSave.enabled=NO;_originalManualClear.enabled=NO;
    NSScrollView* unknownScroll=[[NSScrollView alloc] initWithFrame:NSMakeRect(20,height-428,980,100)];
    unknownScroll.hasVerticalScroller=YES;unknownScroll.borderType=NSBezelBorder;
    unknownScroll.autoresizingMask=NSViewWidthSizable|NSViewMinYMargin;
    _originalMapUnknowns=[[NSTextView alloc] initWithFrame:unknownScroll.bounds];
    _originalMapUnknowns.editable=NO;_originalMapUnknowns.selectable=YES;
    _originalMapUnknowns.verticallyResizable=YES;_originalMapUnknowns.horizontallyResizable=NO;
    _originalMapUnknowns.font=[NSFont systemFontOfSize:12];
    _originalMapUnknowns.textContainer.widthTracksTextView=YES;
    _originalMapUnknowns.autoresizingMask=NSViewWidthSizable;
    _originalMapUnknowns.accessibilityIdentifier=@"original-map-unknown-cells";
    _originalMapUnknowns.accessibilityLabel=@"地表未覆盖格或SAV查格原值";
    unknownScroll.documentView=_originalMapUnknowns;[_originalMapView addSubview:unknownScroll];
    _originalMapImage = [[NSImageView alloc] initWithFrame:NSMakeRect(20, 20, 980, height - 464)];
    _originalMapImage.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    _originalMapImage.imageScaling = NSImageScaleProportionallyUpOrDown;
    _originalMapImage.accessibilityIdentifier = @"original-map-grid"; [_originalMapView addSubview:_originalMapImage];
    [self onOriginalMapChanged:nil];
}
- (void)onOriginalMapFilterChanged:(id)sender {
    (void)sender;const NSInteger previous=_originalMapPicker.indexOfSelectedItem;
    const size_t old=previous>=0&&size_t(previous)<_originalMapVisibleSources.size()?_originalMapVisibleSources[size_t(previous)]:_originalMapSources.size();
    _originalMapVisibleSources=game_lab::originalMapSourceMatches(_originalMapSources,_originalMapFilter.stringValue.UTF8String);
    [_originalMapPicker removeAllItems];NSInteger selected=0;
    for(size_t i=0;i<_originalMapVisibleSources.size();++i){const auto source=_originalMapVisibleSources[i];[_originalMapPicker addItemWithTitle:[NSString stringWithUTF8String:_originalMapSources[source].label.c_str()]];if(source==old)selected=NSInteger(i);}
    if(_originalMapVisibleSources.empty())[_originalMapPicker addItemWithTitle:@"没有匹配的来源"];
    [_originalMapPicker selectItemAtIndex:selected];[self onOriginalMapChanged:nil];
}
- (void)onOriginalMapChanged:(id)sender {
    (void)sender;const bool priorSurface=_originalMapSurface&&_originalMapSurface->available(); _originalMapResult = {}; _originalMapImage.image = nil;
    _originalMapUnknowns.string=@"地图来源已切换；等待新地表结果，不沿用旧坐标。";
    _originalMapImage.accessibilityLabel = @"没有显示地形网格";
    const NSInteger index = _originalMapPicker.indexOfSelectedItem;
    _originalManualCandidate.reset();_originalSelectedSlot.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualAdopt.enabled=NO;_originalManualSave.enabled=NO;_originalManualClear.enabled=NO;_originalManualMessage.clear();_originalSaveRevision=0;_originalSourceRelative.clear();_originalBuildingSlot.stringValue=@"";
    _originalMapSourceInfo.clear();_originalMapPackageSummary=NO;_originalSaveLayers.reset();_originalSaveBuildings.reset();_originalBuildingLocate.enabled=NO;
    [_originalFigureSource selectItemAtIndex:0];_originalFigureSource.enabled=NO;
    _originalSaveCellX.stringValue=@"0";_originalSaveCellY.stringValue=@"0";
    _originalSaveCellX.enabled=NO;_originalSaveCellY.enabled=NO;_originalSaveCellApply.enabled=NO;
    _originalMapSurface->clear();
    [_originalMapBitPicker itemAtIndex:38].enabled=NO;
    for(NSInteger field=33;field<38;++field)[_originalMapBitPicker itemAtIndex:field].enabled=NO;
    if(index < 0 || static_cast<size_t>(index) >= _originalMapVisibleSources.size()) {
        [_originalMapBitPicker selectItemAtIndex:0];_originalMapInfo.stringValue=@"没有匹配的地图来源";_originalMapUnknowns.string=@"没有匹配来源；图像和坐标已清空。";
        _originalMapCandidates.enabled=NO;_originalMapCandidates.hidden=YES;_originalMapCameraX.enabled=NO;_originalMapCameraY.enabled=NO;_originalMapCameraApply.enabled=NO;return;
    }
    const size_t sourceIndex=_originalMapVisibleSources[static_cast<size_t>(index)];
    _originalSourceRelative=_originalMapSources[sourceIndex].relativePath;
    auto selection = _media->originalMapSource(_originalMapSources[sourceIndex]);
    _originalSaveRevision=selection.saveRevision;
    _originalMapResult = std::move(selection.map);
    _originalMapSourceInfo = std::move(selection.sourceInfo);
    _originalMapPackageSummary = selection.packageSummary;
    _originalSaveLayers=std::move(selection.saveLayers);_originalSaveBuildings=std::move(selection.saveBuildings);
    _originalBuildingLocate.enabled=_originalSaveBuildings&&!_originalSaveBuildings->footprints.empty();
    _originalBuildingSlot.enabled=_originalBuildingLocate.enabled;if(_originalBuildingLocate.enabled)_originalBuildingSlot.stringValue=[NSString stringWithFormat:@"%zu",_originalSaveBuildings->footprints.front().slot];
    if(_originalSaveLayers&&_originalSaveLayers->valid()){for(NSInteger field=33;field<38;++field)[_originalMapBitPicker itemAtIndex:field].enabled=YES;}
    else if(_originalMapBitPicker.indexOfSelectedItem>=33&&_originalMapBitPicker.indexOfSelectedItem<38)[_originalMapBitPicker selectItemAtIndex:0];
    const auto& source=_originalMapSources[sourceIndex];
    if(source.kind==game_lab::OriginalMapSourceKind::SaveFile&&_originalSaveBuildings){
        _originalFigureSource.enabled=_originalSaveLayers&&_originalSaveLayers->figures.has_value();
        if(!_originalMapSurface->selectSavedBuildings(_originalMapResult.terrain,*_originalSaveBuildings,
            _originalSaveLayers&&_originalSaveLayers->ground?&*_originalSaveLayers->ground:nullptr,0,
            _originalSaveLayers&&_originalSaveLayers->figures?&*_originalSaveLayers->figures:nullptr))
            _originalMapSourceInfo+="；"+_originalMapSurface->error();
        [_originalMapBitPicker selectItemAtIndex:0];_originalMapSpatial.state=NSControlStateValueOn;
    }
    const bool surface=(source.kind==game_lab::OriginalMapSourceKind::MapFile||source.kind==game_lab::OriginalMapSourceKind::PakCity)&&
        _originalMapSurface->select(source,_originalMapResult.terrain);
    [_originalMapBitPicker itemAtIndex:38].enabled=surface;
    if(surface&&!priorSurface)[_originalMapBitPicker selectItemAtIndex:38];
    if((source.kind==game_lab::OriginalMapSourceKind::MapFile||source.kind==game_lab::OriginalMapSourceKind::PakCity)&&!surface&&!_originalMapSurface->error().empty())
        _originalMapSourceInfo+="；存储地表不可用："+_originalMapSurface->error();
    if(!surface&&_originalMapBitPicker.indexOfSelectedItem==38)[_originalMapBitPicker selectItemAtIndex:0];
    if(_originalMapResult.terrain.valid()&&!game_lab::analyzeOriginalMap(_originalMapResult.terrain).cropValid()) _originalMapCrop.state=NSControlStateValueOff;
    const double center=double(_originalMapResult.terrain.activeSide)/2;
    _originalMapCameraX.stringValue=[NSString stringWithFormat:@"%.2f",center];_originalMapCameraY.stringValue=[NSString stringWithFormat:@"%.2f",center];
    if(source.kind==game_lab::OriginalMapSourceKind::SaveFile&&_originalSaveBuildings&&!_originalSaveBuildings->footprints.empty()){
        const auto& f=_originalSaveBuildings->footprints.front();
        _originalMapCameraX.stringValue=[NSString stringWithFormat:@"%.2f",double(f.x)+(double(game_lab::originalFootprintWidth(f))-1)/2];
        _originalMapCameraY.stringValue=[NSString stringWithFormat:@"%.2f",double(f.y)+(double(game_lab::originalFootprintHeight(f))-1)/2];
    }
    [self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalFigureSourceChanged:(id)sender {
    (void)sender;
    if(!_originalSaveBuildings||!_originalSaveLayers||!_originalSaveLayers->figures)return;
    bool selected=false;
    if(_originalFigureSource.indexOfSelectedItem==1){
        const auto save=game_lab::loadOriginalSave(_media->resourcePath(_originalSourceRelative));
        std::vector<uint8_t> raw;
        if(save.readStable&&save.snapshotRevision==_originalSaveRevision&&game_lab::detail::readFile(_media->resourcePath(_originalSourceRelative),4194304,raw)){
            const auto decoded=game_lab::decodeOriginalMapContainer(raw);
            const auto check=game_lab::decodeOriginalSave(raw);
            if(decoded.complete&&check.snapshotRevision==save.snapshotRevision)selected=_originalMapSurface->selectPostLoadK1(save,decoded.payload);
        }
    }else selected=_originalMapSurface->selectSavedBuildings(_originalMapResult.terrain,*_originalSaveBuildings,
        _originalSaveLayers->ground?&*_originalSaveLayers->ground:nullptr,0,&*_originalSaveLayers->figures);
    if(!selected){_originalMapSourceInfo+="；所选人物来源无法完成；请重新选择存档";_originalMapSurface->clear();}
    [self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalMapPlaneChanged:(id)sender {
    (void)sender; _originalMapImage.image = nil;
    _originalMapUnknowns.string=@"当前为数值视图；不提供已验证的原版地表覆盖。";
    _originalMapImage.accessibilityLabel = @"没有显示地形网格";
    const auto& result = _originalMapResult;
    const bool surface=_originalMapBitPicker.indexOfSelectedItem==38;
    const bool candidatePreview=surface&&_originalMapCandidates.state==NSControlStateValueOn;
    const bool savedField=_originalMapBitPicker.indexOfSelectedItem>=33&&!surface;
    _originalMapSpatial.enabled=!savedField&&!surface;
    if(savedField)_originalMapSpatial.state=NSControlStateValueOff;
    const bool spatial=surface||(!savedField&&_originalMapSpatial.state==NSControlStateValueOn);
    const bool buildingOverlay=spatial&&!surface&&_originalSaveBuildings;
    if(!buildingOverlay){_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage.clear();}
    _originalManualSave.enabled=_originalManualCandidate.has_value();_originalManualClear.enabled=_originalManualCandidate.has_value();
    _originalManualAdopt.enabled=buildingOverlay&&_originalSelectedSlot.has_value()&&_spriteExplicitSelection&&_spriteStaticIndex.has_value()&&_spriteStaticPixels.valid()&&_spriteSequence.frames.empty();
    _originalMapCrop.enabled=!spatial;
    _originalMapCameraX.enabled=spatial;_originalMapCameraY.enabled=spatial;_originalMapCameraApply.enabled=spatial;_originalMapOutlines.enabled=spatial&&!surface;
    _originalMapOutlines.hidden=surface;_originalMapCandidates.hidden=!surface;_originalMapCandidates.enabled=surface;
    std::string text = (_originalManualMessage.empty()?"":_originalManualMessage+"\n")+_originalMapSourceInfo;
    if(!_originalMapPackageSummary){
        if(_originalSaveLayers)text += "SAV 地形网格：存储228×228，活动边长字段 "+std::to_string(result.terrain.activeSide)+"；只读已保存字段，无城市规则绑定";
        else text += game_lab::describeOriginalMap(result);
    }
    if(result.terrain.valid()) {
        const int bit = static_cast<int>(_originalMapBitPicker.indexOfSelectedItem) - 1;
        const bool crop = _originalMapCrop.state == NSControlStateValueOn;
        const auto analysis = game_lab::analyzeOriginalMap(result.terrain);
        game_lab::OriginalMapCamera camera;camera.outlines=_originalMapOutlines.state==NSControlStateValueOn;
        if(buildingOverlay)camera=game_lab::originalBuildingFocusCamera(_originalSaveBuildings->footprints,camera);
        if(buildingOverlay&&_originalManualCandidate&&_originalManualVisible.state==NSControlStateValueOn)camera=game_lab::originalCandidateCamera(*_originalManualCandidate,camera);
        game_lab::DecodedImage image;
        const bool cameraNumbers=readMapCameraNumber(_originalMapCameraX,camera.centerX)&&readMapCameraNumber(_originalMapCameraY,camera.centerY);
        if(surface){
            _originalMapUnknowns.string=@"相机无效或地表结果不可用；未刷新坐标，不沿用旧列表。";
            if(cameraNumbers){
                game_lab::MapImageGridResult rendered;game_lab::AppSurfaceCandidateStats candidate;
                if(candidatePreview){auto preview=_originalMapSurface->renderPreview(camera);rendered.image=std::move(preview.image);rendered.stats=std::move(preview.verified);candidate=preview.candidate;}
                else rendered=_originalMapSurface->render(camera);
                image=std::move(rendered.image);
                const auto& s=rendered.stats;
                std::string diagnostic;
                if(candidatePreview)diagnostic="候选补全 "+std::to_string(candidate.decodedCells)+"/"+std::to_string(candidate.structuralCells)+" 格，"+std::to_string(candidate.decodedGroups)+"/"+std::to_string(candidate.structuralGroups)+" 组已解码；原路径＋候选可绘制 "+std::to_string(s.decodedCells+candidate.decodedCells)+"/"+std::to_string(s.nonzeroCells)+" 格。当前视口候选绘制 "+std::to_string(candidate.drawnCells)+" 格 / "+std::to_string(candidate.drawnGroups)+" 组，写入 "+std::to_string(candidate.paintedPixels)+" 像素。原版放置和遮挡未验证。\n以下仍为原路径未知格，候选显示不移除未知诊断。\n";
                diagnostic+=game_lab::describeAppSurfaceDiagnostics(_originalMapSurface->diagnostics(s));
                if(image.valid())_originalMapUnknowns.string=[NSString stringWithUTF8String:diagnostic.c_str()];
                text+="\n存储 surface："+std::to_string(s.decodedCells)+"/"+std::to_string(s.nonzeroCells)+" 非零格已解码；视口绘制 "+std::to_string(s.drawnCells)+" 格";
                text+="；未支持几何 "+std::to_string(s.geometryUnsupported)+"，缺档案 "+std::to_string(s.archiveUnavailable)+"，未解析索引 "+std::to_string(s.unresolvedIndices)+"，解码失败 "+std::to_string(s.decodeFailures);
            }
            text+="\n按 MAP 保存的图素编号绘制；透明处未知。覆盖率不是原版画面像素一致率，不代表城市可玩。";
        }else if(savedField){
            if(_originalSaveLayers){auto values=game_lab::originalSaveFieldValues(*_originalSaveLayers,unsigned(_originalMapBitPicker.indexOfSelectedItem-33));image=game_lab::renderOriginalMapValues(result.terrain,values,-1,crop);}
            text+="；存档原始字段数值视图，字段名不证明运行时用途";
        }else if(spatial){
            if(buildingOverlay){
                _originalMapUnknowns.string=@"占地视口无效；没有保留旧占地坐标或图像。";
                if(cameraNumbers){auto preview=game_lab::renderOriginalBuildingFootprints(result.terrain,_originalSaveBuildings->footprints,camera,bit);image=std::move(preview.image);
                    std::string detail="数值建筑占地："+std::to_string(preview.valid)+" 个已验证记录，当前视口轮廓 "+std::to_string(preview.visible)+" 个。琥珀线标注原始记录占地，背景来自同一 SAV 的数值地形。\n";
                    if(_originalManualCandidate&&_originalManualVisible.state==NSControlStateValueOn){auto manual=game_lab::renderOriginalBuildingCandidate(result.terrain,_originalSaveBuildings->footprints,camera,_originalSourceRelative,_originalSaveRevision,*_originalManualCandidate,bit);
                        if(manual.accepted){image=std::move(manual.image);detail="人工候选 · 锚点未验 · confirmed 0；"+_originalManualCandidate->archive+" / 物理ID "+std::to_string(_originalManualCandidate->physicalId)+" → 表槽 "+std::to_string(_originalManualCandidate->target.slot)+"；绘制 "+std::to_string(manual.paintedPixels)+" 像素。约定：画布底边中心→占地中心，非原版锚点。\n"+detail;}
                        else{_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage="候选来源/记录/画布校验失败，旧候选已清除。";}
                    }
                    if(!_originalManualCandidate||_originalManualVisible.state!=NSControlStateValueOn){
                        auto scene=_originalMapSurface->renderSavedBuildings(camera,bit);
                        if(scene.image.valid()){
                            image=std::move(scene.image);
                            detail="SAV 建筑基座及静态对象 "+std::to_string(scene.placedRecords)+"/"+std::to_string(scene.liveRecords)+" 记录已放置；保留数值占地/标记 "+std::to_string(scene.withheldRecords)+" 记录（"+std::to_string(scene.withheldOutlines)+" 占地，"+std::to_string(scene.unshapedMarkers)+" 未知形状坐标标记）。\n";
                            detail+="地面重建 "+std::to_string(scene.groundCells)+" 格；选图未知 "+std::to_string(scene.groundUnknown)+"，未写入 "+std::to_string(scene.groundNoWrite)+"，索引未解 "+std::to_string(scene.groundUnresolved)+"；未绑定建筑占格 "+std::to_string(scene.unboundBuildingCells)+"。\n";
                            detail+="地面及建筑格解码 "+std::to_string(scene.bases.decodedCells)+"/"+std::to_string(scene.bases.nonzeroCells)+"；失败 "+std::to_string(scene.bases.decodeFailures)+"；当前视口绘制 "+std::to_string(scene.bases.drawnCells)+" 格。\n";
                            detail+="对象层解码 "+std::to_string(scene.overlayDecoded)+"/"+std::to_string(scene.overlayObjects)+"；失败 "+std::to_string(scene.overlayDecodeFailures+scene.overlayInvalid)+"；当前视口绘制 "+std::to_string(scene.overlayDrawn)+" 个。\n";
                            detail+="建筑动画条件通过 "+std::to_string(scene.animationEnabled)+"/"+std::to_string(scene.animationCandidates)+"；帧解码 "+std::to_string(scene.animationDecoded)+"/"+std::to_string(scene.animationSelected)+"；条件未知 "+std::to_string(scene.animationUnknown)+"，无效 "+std::to_string(scene.animationInvalid)+"。时间输入 tick=0，使用 SAV 保存的格子相位。\n";
                            detail+="人物放置 "+std::to_string(scene.figurePlaced)+"/"+std::to_string(scene.figureLive)+"；图像解码 "+std::to_string(scene.figureDecoded)+"；隐藏 "+std::to_string(scene.figureHidden)+"，图像为零 "+std::to_string(scene.figureZeroImage)+"，不支持 "+std::to_string(scene.figureUnsupported)+"。\n";
                            if(scene.timelineCombined)detail+="合并时序："+std::to_string(*scene.timelineUpdates)+" 次获准更新 / "+std::to_string(scene.timelineFrames)+" 次显式绘制；城市时钟、农夫墙钟与 CRT 状态由调用方显式提供，缺少必需输入时拒绝继续。\n";
                            else if(scene.timelineUpdates)detail+="post-load timeline："+std::to_string(*scene.timelineUpdates)+" 次获准更新 / "+std::to_string(scene.timelineFrames)+" 次显式绘制。k=1 预览使用 speed 80、时钟从 0 开始、37 ms 首次绘制；原版时刻未测。CRT 随机状态由原版墙钟播种、不在 SAV 中；本预览未提供，遇到 rand() 将拒绝。\n";
                            else detail+=scene.figurePostLoadK1?"post-load k=1：一次显式人物更新；未指定建筑绘制时钟。\n":"Saved snapshot：人物使用 tick=0 保存图像。\n";
                            if(scene.figureRemoved)detail+="本次更新后移除 "+std::to_string(scene.figureRemoved)+" 人物。\n";
                            detail+="地面与建筑来自同一 SAV；按 view0 角格及对角线顺序绘制。未知处保留数值地形。阴影、高程遮挡及原版画面对照未验证；不是运行中的城市。\n";
                            text+="；基座及静态对象 "+std::to_string(scene.placedRecords)+"/"+std::to_string(scene.liveRecords)+"，未绑定 "+std::to_string(scene.withheldRecords);
                        }
                    }
                    for(const auto& f:_originalSaveBuildings->footprints)detail+=std::string(game_lab::originalBuildingName(f.type))+"(type"+std::to_string(f.type)+") 活动坐标("+std::to_string(f.x)+","+std::to_string(f.y)+")，占地"+std::to_string(game_lab::originalFootprintWidth(f))+"×"+std::to_string(game_lab::originalFootprintHeight(f))+"（记录size"+std::to_string(f.size)+"），表槽"+std::to_string(f.slot)+"。\n";
                    if(image.valid())_originalMapUnknowns.string=[NSString stringWithUTF8String:detail.c_str()];
                    text+="；记录占地 "+std::to_string(preview.visible)+"/"+std::to_string(preview.valid)+" 在视口内";
                }
            }else if(cameraNumbers)image=game_lab::renderOriginalMapSpatialFlags(result.terrain,camera,bit);
        }
        else image=game_lab::renderOriginalMapFlags(result.terrain,bit,crop);
        if(!presentPixels(_originalMapImage,image.width,image.height,image.pixelsBGRA))
            text+=spatial?"；相机无效、活动框未验证或空间画面预算超限":crop?"；裁剪验证未通过，关闭裁剪可查看完整诊断":"；画面转换失败";
        else {
            text+="；已原生显示";
            if(surface){
                _originalMapImage.accessibilityLabel=[NSString stringWithFormat:@"%@，存储地表图素 %u×%u，相机 %.2f,%.2f，覆盖不等于原版画面对照%@",_originalMapPicker.titleOfSelectedItem,image.width,image.height,camera.centerX,camera.centerY,candidatePreview?@"；含候选补全，原版放置未验证":@""];
            }else if(spatial){
                char cameraText[256];std::snprintf(cameraText,sizeof(cameraText),"；平面等距诊断，相机活动格(%.2f,%.2f)，格轮廓%s。",camera.centerX,camera.centerY,camera.outlines?"开":"关");text+=cameraText;
                _originalMapImage.accessibilityLabel=[NSString stringWithFormat:@"%@，等距空间诊断 %u×%u，相机 %.2f,%.2f，%@，格轮廓%@",_originalMapPicker.titleOfSelectedItem,image.width,image.height,camera.centerX,camera.centerY,_originalMapBitPicker.titleOfSelectedItem,camera.outlines?@"开":@"关"];
                if(buildingOverlay)_originalMapImage.accessibilityLabel=[_originalMapImage.accessibilityLabel stringByAppendingString:@"；SAV 静态建筑基座与未绑定占地/标记；数值地形背景，原版画面对照未验"];
                if(buildingOverlay&&_originalManualCandidate&&_originalManualVisible.state==NSControlStateValueOn)_originalMapImage.accessibilityLabel=[_originalMapImage.accessibilityLabel stringByAppendingFormat:@"；人工候选，锚点未验，confirmed 0，表槽%zu，%@，物理ID%u",_originalManualCandidate->target.slot,[NSString stringWithUTF8String:_originalManualCandidate->archive.c_str()],_originalManualCandidate->physicalId];
            }else _originalMapImage.accessibilityLabel=[NSString stringWithFormat:@"%@，%@ %u×%u，%@，%@",_originalMapPicker.titleOfSelectedItem,savedField?@"原始字段数值网格":@"地形标志网格",image.width,image.height,_originalMapBitPicker.titleOfSelectedItem,crop?@"中心活动框裁剪":@"完整存储网格"];
        }
        char details[256];
        std::snprintf(details, sizeof(details), "\n中心活动框起点(%u,%u)，边长%u；非地图外格%zu，框外%zu；未知位格%zu，未知mask 0x%08x。", analysis.cropOrigin, analysis.cropOrigin, analysis.cropSide, analysis.activeCells, analysis.outsideCropCells, analysis.unknownFlagCells, analysis.unknownBits);
        text += details;
        text += "\n完整消费 " + std::to_string(result.frames) + " 个压缩块，逻辑数据 " + std::to_string(result.decodedBytes) + " 字节；只读原始网格，无城市状态绑定。";
    }
    text += surface?"\n仅静态存储地表；重建选图准确度与原版画面对照另计，建筑/单位/规则未恢复。":_originalSaveLayers?"\n只读存档快照，非当前内存；空间视图显示重建地面、已绑定建筑与保存相位的人物；人物移动、规则、完整高程遮挡和原版画面对照尚未恢复。":"\n数值配色不等于原版外观；等距对照使用规范平面格坐标，不含高程/遮挡规则；SG3选图未知，不载入城市/建筑/单位。";
    _originalMapInfo.stringValue = [NSString stringWithUTF8String:text.c_str()];
    const bool inspect=_originalSaveLayers&&_originalSaveLayers->valid()&&result.terrain.valid()&&!buildingOverlay;
    _originalSaveCellX.enabled=inspect;_originalSaveCellY.enabled=inspect;_originalSaveCellApply.enabled=inspect;
    if(inspect)[self onOriginalSaveCellChanged:nil];
}
- (void)onOriginalBuildingLocate:(id)sender {
    (void)sender;
    if(!_originalSaveBuildings||_originalSaveBuildings->footprints.empty()){_originalSelectedSlot.reset();_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage="没有已验证建筑记录；旧人工候选已清除。";[self onOriginalMapPlaneChanged:nil];return;}
    const auto slot=game_lab::parseOriginalCandidateSlot(_originalBuildingSlot.stringValue.UTF8String);
    auto found=std::find_if(_originalSaveBuildings->footprints.begin(),_originalSaveBuildings->footprints.end(),[&](const auto& f){return slot&&f.slot==*slot;});
    if(!slot||found==_originalSaveBuildings->footprints.end()){_originalSelectedSlot.reset();_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage="建筑槽不在当前已验证占地记录中；旧人工候选已清除。";[self onOriginalMapPlaneChanged:nil];return;}
    if(!_originalSelectedSlot||*_originalSelectedSlot!=*slot){_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage.clear();}_originalSelectedSlot=*slot;
    const auto& f=*found;
    [_originalMapBitPicker selectItemAtIndex:0];_originalMapSpatial.state=NSControlStateValueOn;
    _originalMapCameraX.stringValue=[NSString stringWithFormat:@"%.2f",double(f.x)+(double(game_lab::originalFootprintWidth(f))-1)/2];
    _originalMapCameraY.stringValue=[NSString stringWithFormat:@"%.2f",double(f.y)+(double(game_lab::originalFootprintHeight(f))-1)/2];
    [self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalManualClear:(id)sender {
    (void)sender;_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage="人工候选已清除；confirmed 0。";[self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalManualAdopt:(id)sender {
    (void)sender;_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;
    [self onOriginalBuildingLocate:nil];
    if(!_originalSaveBuildings||!_originalSelectedSlot||!_spriteExplicitSelection||!_spriteStaticIndex||!_spriteStaticPixels.valid()||!_spriteSequence.frames.empty()){
        _originalManualMessage="先定位已验证的建筑表槽，并在 SG3 页显式选择有效静态图像；旧候选已清除。";[self onOriginalMapPlaneChanged:nil];return;
    }
    auto target=std::find_if(_originalSaveBuildings->footprints.begin(),_originalSaveBuildings->footprints.end(),[&](const auto& f){return f.slot==*_originalSelectedSlot;});
    const NSInteger package=_spritePackagePopup.indexOfSelectedItem;if(target==_originalSaveBuildings->footprints.end()||package<0||size_t(package)>=_sg3Packages.size())return;
    game_lab::OriginalBuildingCandidate c;c.source=_originalSourceRelative;c.sourceRevision=_originalSaveRevision;c.target=*target;c.archive=_sg3Packages[size_t(package)];c.physicalId=uint32_t(*_spriteStaticIndex+1);c.pixels=_spriteStaticPixels;c.imageRevision=game_lab::originalCandidateImageRevision(c.pixels);
    if(!game_lab::validOriginalCandidate(c)){_originalManualMessage="图素候选超出画布预算或像素校验失败，未显示旧候选。";[self onOriginalMapPlaneChanged:nil];return;}
    _originalManualCandidate=std::move(c);_originalManualVisible.state=NSControlStateValueOn;_originalManualMessage="人工候选 · 锚点未验 · confirmed 0。图像底边中心对齐占地中心仅为预览约定。";[self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalManualSave:(id)sender {
    (void)sender;if(!_originalManualCandidate)return;NSDictionary* object=game_lab::encodeOriginalCandidate(*_originalManualCandidate);if(!object)return;
    NSSavePanel* panel=[NSSavePanel savePanel];panel.nameFieldStringValue=@"building-candidate.json";panel.message=@"仅保存人工候选引用与校验信息，不保存像素；须在 GameData 和 Git 目录之外。";
    if([panel runModal]!=NSModalResponseOK)return;const char* path=panel.URL.fileSystemRepresentation;
    if(!path||!game_lab::originalCandidateConfigPath(path,_candidateDirectory.fileSystemRepresentation)){_originalManualMessage="候选配置路径被拒绝：须为本地 JSON，位于 GameData 和 Git 目录之外。";[self onOriginalMapPlaneChanged:nil];return;}
    NSData* data=[NSJSONSerialization dataWithJSONObject:object options:NSJSONWritingPrettyPrinted error:nil];
    const bool saved=data&&data.length<=16384&&[data writeToURL:panel.URL options:NSDataWritingAtomic error:nil];_originalManualMessage=saved?"已本地保存人工候选配置；无像素、无 confirmed 绑定。":"候选配置保存失败。";[self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalManualLoad:(id)sender {
    (void)sender;if(!_originalSaveBuildings||_originalSaveBuildings->footprints.empty()){_originalMapInfo.stringValue=@"先选择含已验证建筑记录的 SAV，再载入本地人工候选。";return;}
    NSOpenPanel* panel=[NSOpenPanel openPanel];panel.canChooseFiles=YES;panel.canChooseDirectories=NO;panel.allowsMultipleSelection=NO;panel.message=@"选择本地人工候选 JSON；重新校验当前 SAV 和原图像，不载入像素文件。";
    if([panel runModal]!=NSModalResponseOK)return;
    _originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage="配置不兼容、来源/记录/像素已变化或资源缺失；旧人工候选已清除。";
    const char* path=panel.URL.fileSystemRepresentation;
    if(!path||!game_lab::originalCandidateConfigPath(path,_candidateDirectory.fileSystemRepresentation)){[self onOriginalMapPlaneChanged:nil];return;}
    auto input=game_lab::readOriginalCandidateFile(path);if(!input){[self onOriginalMapPlaneChanged:nil];return;}
    NSData* data=[NSData dataWithBytes:input->data() length:input->size()];
    auto c=game_lab::decodeOriginalCandidate([NSJSONSerialization JSONObjectWithData:data options:0 error:nil]);
    if(!c||c->source!=_originalSourceRelative||c->sourceRevision!=_originalSaveRevision){[self onOriginalMapPlaneChanged:nil];return;}
    auto target=std::find_if(_originalSaveBuildings->footprints.begin(),_originalSaveBuildings->footprints.end(),[&](const auto& f){return game_lab::sameOriginalCandidateTarget(f,c->target);});
    if(target==_originalSaveBuildings->footprints.end()){[self onOriginalMapPlaneChanged:nil];return;}
    auto archive=_media->sg3(c->archive);if(!c->physicalId||c->physicalId>archive.sprites.size()){[self onOriginalMapPlaneChanged:nil];return;}
    auto companion=std::filesystem::path(c->archive);companion.replace_extension(".555");auto decoded=_media->sprite(archive,c->physicalId-1,companion.generic_string());c->pixels=std::move(decoded.image);
    if(!game_lab::validOriginalCandidate(*c)){[self onOriginalMapPlaneChanged:nil];return;}
    _originalBuildingSlot.stringValue=[NSString stringWithFormat:@"%zu",c->target.slot];[self onOriginalBuildingLocate:nil];
    _originalManualCandidate=std::move(*c);_originalManualVisible.state=NSControlStateValueOn;_originalManualMessage="已本地载入并复核人工候选；锚点未验，confirmed 0。";[self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalSaveCellChanged:(id)sender {
    (void)sender;
    _originalMapUnknowns.string=@"SAV 查格不可用；没有沿用旧来源的原值。";
    if(!_originalSaveLayers||!_originalMapResult.terrain.valid())return;
    long long x=0,y=0;
    NSScanner* sx=[NSScanner scannerWithString:_originalSaveCellX.stringValue];
    NSScanner* sy=[NSScanner scannerWithString:_originalSaveCellY.stringValue];
    if(![sx scanLongLong:&x]||!sx.isAtEnd||![sy scanLongLong:&y]||!sy.isAtEnd){_originalMapUnknowns.string=@"SAV 坐标须为范围内的整数；未显示旧原值。";return;}
    const auto cell=game_lab::originalSaveCell(_originalMapResult.terrain,*_originalSaveLayers,x,y);
    if(!cell){_originalMapUnknowns.string=@"SAV 坐标越界或活动框未验证；未显示旧原值。";return;}
    const std::string text=std::string(_originalMapPicker.titleOfSelectedItem.UTF8String)+"；活动坐标("+std::to_string(x)+","+std::to_string(y)+")\n"+game_lab::describeOriginalSaveCell(*cell);
    _originalMapUnknowns.string=[NSString stringWithUTF8String:text.c_str()];
}

- (void)setupCandidateView {
    _candidateView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _candidateView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    NSTextField* title = [NSTextField labelWithString:@"候选存档只读检查"];
    title.frame = NSMakeRect(20, _candidateView.bounds.size.height - 38, 900, 24);
    title.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    [_candidateView addSubview:title];
    NSButton* choose = [NSButton buttonWithTitle:@"选择候选文件" target:self action:@selector(onChooseCandidate:)];
    choose.frame = NSMakeRect(20, _candidateView.bounds.size.height - 80, 140, 30);
    choose.autoresizingMask = NSViewMinYMargin;
    [_candidateView addSubview:choose];
    _candidateRetry = [NSButton buttonWithTitle:@"重新检查候选" target:self action:@selector(onRetryCandidate:)];
    _candidateRetry.frame = NSMakeRect(170, _candidateView.bounds.size.height - 80, 140, 30);
    _candidateRetry.autoresizingMask = NSViewMinYMargin;
    [_candidateView addSubview:_candidateRetry];
    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:
        NSMakeRect(20, 20, _candidateView.bounds.size.width - 40, _candidateView.bounds.size.height - 120)];
    scroll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scroll.hasVerticalScroller = YES;
    _candidateReport = [[NSTextView alloc] initWithFrame:scroll.contentView.bounds];
    _candidateReport.editable = NO;
    _candidateReport.richText = NO;
    _candidateReport.font = [NSFont systemFontOfSize:14];
    _candidateReport.autoresizingMask = NSViewWidthSizable;
    _candidateReport.textContainer.widthTracksTextView = YES;
    scroll.documentView = _candidateReport;
    [_candidateView addSubview:scroll];
    [self refreshCandidateReport];
}
- (void)refreshCandidateReport {
    _candidateReport.string = [NSString stringWithUTF8String:game_lab::formatCandidateImport(_candidateImport).c_str()];
    _candidateRetry.enabled = !_candidateImport.selectedPath().empty() &&
        _candidateImport.result().state != game_lab::CandidateState::Selecting;
}
- (void)onChooseCandidate:(id)sender {
    (void)sender;
    _candidateImport.beginSelection();
    [self refreshCandidateReport];
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    panel.canCreateDirectories = NO;
    panel.resolvesAliases = NO;
    panel.directoryURL = _candidateDirectory;
    panel.message = @"只读检查候选文件；不会载入城市或写回存档。";
    panel.prompt = @"只读检查";
    if ([panel runModal] != NSModalResponseOK) _candidateImport.cancelSelection();
    else {
        const char* path = panel.URL.path.UTF8String;
        _candidateImport.inspectFile(path ? path : "");
    }
    [self refreshCandidateReport];
}
- (void)onRetryCandidate:(id)sender {
    (void)sender;
    _candidateImport.retry();
    [self refreshCandidateReport];
}

// ----------------------------------------------------
// Tab 5: Native Bink audio/video (external FFmpeg) and legacy MOV preview
// ----------------------------------------------------
- (void)setupMovieView {
    _movieView = [[NSView alloc] initWithFrame:_contentContainer.bounds];
    _movieView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    NSTextField* title = [NSTextField labelWithString:@"影片音画预览"];
    title.frame = NSMakeRect(20, 670, 600, 30);
    title.font = [NSFont boldSystemFontOfSize:18];
    [_movieView addSubview:title];

    NSTextField* desc = [NSTextField labelWithString:@"Bink 视频使用本机已安装的 FFmpeg；支持播放、暂停和跳转，原生音频跟随底部静音与音量设置。"];
    desc.frame = NSMakeRect(20, 642, 600, 20);
    desc.font = [NSFont systemFontOfSize:12];
    desc.textColor = [NSColor colorWithCalibratedWhite:0.75 alpha:1.0];
    [_movieView addSubview:desc];

    NSTextField* pathLabel = [NSTextField labelWithString:@"相对影片路径:"];
    pathLabel.frame = NSMakeRect(20, 602, 110, 22);
    pathLabel.font = [NSFont systemFontOfSize:13 weight:NSFontWeightMedium];
    [_movieView addSubview:pathLabel];

    _moviePathField = [[NSTextField alloc] initWithFrame:NSMakeRect(135, 600, 360, 26)];
    _moviePathField.placeholderString = @"Binks/…/*.bik";
    _moviePathField.font = [NSFont systemFontOfSize:13];
    _moviePathField.accessibilityIdentifier = @"movie-path";
    [_movieView addSubview:_moviePathField];

    NSButton* loadBtn = [NSButton buttonWithTitle:@"加载首帧" target:self action:@selector(onLoadMoviePoster:)];
    loadBtn.frame = NSMakeRect(505, 598, 90, 30);
    loadBtn.bezelStyle = NSBezelStyleRounded;
    [_movieView addSubview:loadBtn];

    NSButton* clearBtn = [NSButton buttonWithTitle:@"清空" target:self action:@selector(onClearMoviePoster:)];
    clearBtn.frame = NSMakeRect(600, 598, 70, 30);
    clearBtn.bezelStyle = NSBezelStyleRounded;
    [_movieView addSubview:clearBtn];

    _moviePlayButton = [NSButton buttonWithTitle:@"播放" target:self action:@selector(onMoviePlay:)];
    _moviePlayButton.frame = NSMakeRect(20, 558, 75, 30);
    _moviePlayButton.bezelStyle = NSBezelStyleRounded;
    _moviePlayButton.accessibilityIdentifier = @"movie-play";
    [_movieView addSubview:_moviePlayButton];

    _moviePauseButton = [NSButton buttonWithTitle:@"暂停" target:self action:@selector(onMoviePause:)];
    _moviePauseButton.frame = NSMakeRect(100, 558, 75, 30);
    _moviePauseButton.bezelStyle = NSBezelStyleRounded;
    _moviePauseButton.accessibilityIdentifier = @"movie-pause";
    [_movieView addSubview:_moviePauseButton];

    _movieStopButton = [NSButton buttonWithTitle:@"停止" target:self action:@selector(onMovieStop:)];
    _movieStopButton.frame = NSMakeRect(180, 558, 75, 30);
    _movieStopButton.bezelStyle = NSBezelStyleRounded;
    _movieStopButton.accessibilityIdentifier = @"movie-stop";
    [_movieView addSubview:_movieStopButton];

    _moviePositionLabel = [NSTextField labelWithString:@"尚未加载影片"];
    _moviePositionLabel.frame = NSMakeRect(265, 563, 140, 22);
    _moviePositionLabel.font = [NSFont monospacedDigitSystemFontOfSize:13 weight:NSFontWeightRegular];
    _moviePositionLabel.accessibilityIdentifier = @"movie-position";
    [_movieView addSubview:_moviePositionLabel];

    _movieSeekSlider = [[AppMovieSeekSlider alloc] initWithFrame:NSMakeRect(410, 560, 260, 26)];
    _movieSeekSlider.minValue = 0;
    _movieSeekSlider.maxValue = 0;
    _movieSeekSlider.doubleValue = 0;
    _movieSeekSlider.continuous = NO;
    _movieSeekSlider.target = self;
    _movieSeekSlider.action = @selector(onMovieSeek:);
    [_movieView addSubview:_movieSeekSlider];

    _movieStatusLabel = [NSTextField labelWithString:@"待机 · 点击加载首帧或播放开始音画预览"];
    _movieStatusLabel.frame = NSMakeRect(20, 526, 700, 22);
    _movieStatusLabel.font = [NSFont systemFontOfSize:12];
    _movieStatusLabel.textColor = [NSColor colorWithCalibratedWhite:0.8 alpha:1.0];
    _movieStatusLabel.accessibilityIdentifier = @"movie-status";
    [_movieView addSubview:_movieStatusLabel];

    _movieImageView = [[NSImageView alloc] initWithFrame:NSMakeRect(20, 220, 384, 288)];
    _movieImageView.imageScaling = NSImageScaleProportionallyUpOrDown;
    _movieImageView.wantsLayer = YES;
    _movieImageView.layer.borderColor = [NSColor grayColor].CGColor;
    _movieImageView.layer.borderWidth = 1;
    [_movieView addSubview:_movieImageView];

    // Keep the fixed movie controls below navigation when the window is fitted to a shorter screen.
    for (NSView* control in _movieView.subviews) {
        NSRect rect = control.frame;
        rect.origin.y += _movieView.bounds.size.height - 720;
        control.frame = rect;
        control.autoresizingMask |= NSViewMinYMargin;
    }

    _moviePresenter = std::make_unique<ViewImagePresenter>(_movieImageView);
    [self updateMovieControls];
}

- (void)updateMovieControls {
    // ResourceRoot resolution is cached on this owner context; never wait on a
    // decoding worker before a stop/seek request can cancel its metadata load.
    const BOOL hasSession = (_movieSession && !_movieSession->resolvedPath().empty());
    media_demo::State state = _movieSession ? _movieSession->lastSnapshot().state : media_demo::State::Ready;
    const BOOL canPlay = hasSession && state != media_demo::State::Playing && !_moviePending;
    const BOOL canPause = hasSession && state == media_demo::State::Playing && !_moviePending;
    const BOOL canStop = hasSession && (state != media_demo::State::Ready || _moviePending);
    const BOOL canSeek = hasSession && !_moviePending;

    _moviePlayButton.enabled = !_movieTerminating && (canPlay || !hasSession); // Allow Play to open & play
    _moviePauseButton.enabled = !_movieTerminating && canPause;
    _movieStopButton.enabled = !_movieTerminating && canStop;
    _movieSeekSlider.enabled = !_movieTerminating && canSeek;
}

- (void)invalidateMovieTimer {
    if (_movieTimer) {
        [_movieTimer invalidate];
        _movieTimer = nil;
    }
}

- (void)updateMovieTimer {
    if (!_movieSession || _moviePending || _movieSession->lastSnapshot().state != media_demo::State::Playing) {
        [self invalidateMovieTimer];
        return;
    }
    if (_movieTimer) return;
    __weak EmperorAppDelegate* weakSelf = self;
    const auto& meta=_movieSession->lastSnapshot();
    const double frameRate=double(meta.fpsNumerator)/std::max(1u,meta.fpsDenominator);
    const double timerRate=std::max(30.0,std::min(120.0,frameRate*2));
    _movieTimer = [NSTimer timerWithTimeInterval:1.0 / timerRate repeats:YES block:^(NSTimer* timer) {
        (void)timer;
        EmperorAppDelegate* owner = weakSelf;
        if (owner && !owner->_movieTickQueued && !owner->_moviePending) {
            owner->_movieTickQueued = YES;
            [owner requestMovieAction:media_demo::Action::Tick frame:0];
        }
    }];
    [NSRunLoop.mainRunLoop addTimer:_movieTimer forMode:NSRunLoopCommonModes];
}

- (void)handleMovieDelivery:(const game_lab::MovieDelivery&)delivery tick:(BOOL)tick {
    if (tick) {
        _movieTickQueued = NO;
    } else if (_moviePending) {
        --_moviePending;
    }

    if (!delivery.accepted) {
        [self updateMovieControls];
        return;
    }

    const auto& snap = delivery.snapshot;
    if (!_movieSeekSlider.mouseTracking) {
        _movieSeekSlider.doubleValue = double(snap.frame);
    }
    _movieSeekSlider.maxValue=double(snap.videoFrames?snap.videoFrames-1:0);
    _moviePositionLabel.stringValue = [NSString stringWithFormat:@"%.2f / %.2f 秒",snap.positionSeconds,snap.durationSeconds];
    _movieStatusLabel.stringValue = [NSString stringWithUTF8String:game_lab::formatMovieStatus(delivery).c_str()];
    [self updateMovieControls];
    [self updateMovieTimer];
}

- (void)requestMovieAction:(media_demo::Action)action frame:(uint64_t)frame {
    if (_movieTerminating) return;
    const BOOL tick = (action == media_demo::Action::Tick);
    if (!tick) {
        _moviePending = 1;
        _movieTickQueued = NO;
        [self invalidateMovieTimer];
        [self updateMovieControls];
    }
    __weak EmperorAppDelegate* weakSelf = self;
    _movieSession->request(action, frame, [weakSelf, tick](const game_lab::MovieDelivery& delivery) {
        EmperorAppDelegate* owner = weakSelf;
        if (owner) {
            [owner handleMovieDelivery:delivery tick:tick];
        }
    }, _moviePresenter.get());
}

- (void)onMoviePlay:(id)sender {
    if (_movieTerminating) return;
    (void)sender;
    NSString* pathStr = [_moviePathField stringValue];
    const std::string rel = pathStr.UTF8String ? pathStr.UTF8String : "";
    if (!_movieSession) _movieSession = _media->createMovieSession();
    if (_movieSession->currentRelativePath() != rel || _movieSession->resolvedPath().empty() || !_movieSession->hasActiveController()) {
        [self invalidateMovieTimer];
        _movieTickQueued = NO;
        _moviePending = 1;
        _movieStatusLabel.stringValue = @"正在读取影片…";
        __weak EmperorAppDelegate* weakSelf = self;
        _movieSession->openAndPlay(rel, [weakSelf](const game_lab::MovieDelivery& delivery) {
            EmperorAppDelegate* owner = weakSelf;
            if (owner) [owner handleMovieDelivery:delivery tick:NO];
        }, _moviePresenter.get());
        [self updateMovieControls];
        return;
    }
    [self requestMovieAction:media_demo::Action::Play frame:0];
}

- (void)onMoviePause:(id)sender {
    if (_movieTerminating) return;
    (void)sender;
    if (!_movieSession) return;
    [self requestMovieAction:media_demo::Action::Pause frame:0];
}

- (void)onMovieStop:(id)sender {
    if (_movieTerminating) return;
    (void)sender;
    [self invalidateMovieTimer];
    if (!_movieSession) return;
    [self requestMovieAction:media_demo::Action::Stop frame:0];
}

- (void)onMovieSeek:(AppMovieSeekSlider*)sender {
    if (_movieTerminating) return;
    if (!_movieSession) return;
    const uint64_t targetFrame = static_cast<uint64_t>(std::max(0.0, sender.doubleValue));
    [self requestMovieAction:media_demo::Action::Seek frame:targetFrame];
}

- (void)onLoadMoviePoster:(id)sender {
    if (_movieTerminating) return;
    (void)sender;
    [self invalidateMovieTimer];
    if (!_movieSession) _movieSession = _media->createMovieSession();
    _movieTickQueued = NO;
    _moviePending = 1;
    NSString* pathStr = [_moviePathField stringValue];
    _movieStatusLabel.stringValue = @"正在读取首帧…";
    __weak EmperorAppDelegate* weakSelf = self;
    _movieSession->requestPoster(pathStr.UTF8String ? pathStr.UTF8String : "", [weakSelf](const game_lab::MovieDelivery& delivery) {
        EmperorAppDelegate* owner = weakSelf;
        if (owner) [owner handleMovieDelivery:delivery tick:NO];
    }, _moviePresenter.get());
    [self updateMovieControls];
}

- (void)onClearMoviePoster:(id)sender {
    if (_movieTerminating) return;
    (void)sender;
    [self invalidateMovieTimer];
    _movieTickQueued = NO;
    _moviePending = _movieSession ? 1 : 0;
    if (_moviePresenter) _moviePresenter->clear();
    _moviePositionLabel.stringValue = @"尚未加载影片";
    _movieSeekSlider.doubleValue = 0;
    _movieStatusLabel.stringValue = @"正在清空…";
    if (_movieSession) {
        __weak EmperorAppDelegate* weakSelf = self;
        _movieSession->closeAsync([weakSelf](bool zero, const std::string& error) {
            EmperorAppDelegate* owner = weakSelf;
            if (!owner) return;
            owner->_moviePending = 0;
            owner->_movieStatusLabel.stringValue = zero && error.empty() ? @"画面已清空" :
                [NSString stringWithUTF8String:("关闭失败：" + error).c_str()];
            [owner updateMovieControls];
        });
    } else _movieStatusLabel.stringValue = @"画面已清空";
    [self updateMovieControls];
}

// ----------------------------------------------------
// Tab Switching
// ----------------------------------------------------
- (void)onTabChanged:(NSSegmentedControl*)sender {
    [self showTab:sender.selectedSegment];
}

- (void)showTab:(NSInteger)idx {
    if(idx!=2){[self stopSpriteSequence];[self renderSpriteSequence];}
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
        case 5: [_contentContainer addSubview:_movieView]; break;
        case 6: [_contentContainer addSubview:_candidateView]; break;
        case 7: [_contentContainer addSubview:_originalMapView]; break;
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
                                 "Supply original files via --data-root or EMPEROR_GAMEDATA_ROOT; otherwise choose a directory.\n");
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
        EmperorAppDelegate* delegate = [[EmperorAppDelegate alloc] initWithResourceRoot:plan.root selfAuthored:plan.bundledSelfAuthored];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
