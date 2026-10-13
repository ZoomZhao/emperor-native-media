# Original MAP terrain: verified scope

The original-map path is independent of the frozen CitySimulation/EMP1 model and the old 128×128 preview. It reads only the user-selected data root. It does not restore save objects, buildings, units or rules.

## Corpus and roles

The measured local corpus contains **168 MAP files**: **167 city maps** under `Cities/` and **1 empire map** at `Campaigns/UNTITLED/UNTITLEDP.map`. The official [Campaign Creator manual](https://sierrachest.com/gfx/games/Emperor/box/02_CampaignCreator.pdf), page 38, describes `Cities/*.MAP`, campaign `.set` settings, the campaign `P.map` empire map and the playable `.pak` package. A legally supplied local copy, `EmperorCampaignCreator.pdf`, was used because the public host returned HTTP 403.

The previous loader accepted each first compressed block as a hardcoded 128×128 preview. **168/168 first-block acceptance was not complete parsing.** The new loader consumes and verifies the complete container before publishing any terrain.

## Verified binary facts

All measured files use little-endian outer marker `0xfedcbaaa` and a single sequence of compressed records through exact EOF. Each record has marker `0xabcdefff`, compressed length, decoded length, then a zlib stream. Every decoded chunk is at most 32,768 bytes; the decoded length range for this corpus is 471,164–1,918,465 bytes. Raw records and multipart trailer layouts are not supported by this implementation.

| City logical payload field | Verified layout |
|---|---|
| Signature | `05 00 fe ca 00 00 02 00` |
| Active-side field | little-endian uint32 at offset 84 |
| Terrain flags | offset 261,455; 51,984 little-endian uint32 values, row-major 228×228 backing grid |
| Active-side distribution | 84:1, 112:16, 140:67, 170:72, 226:11 |

The public modern [Emperor format reader](https://github.com/bvschaik/citybuilding-mappers/blob/master/emperorfile.cpp), [framing reader](https://github.com/bvschaik/citybuilding-mappers/blob/master/zlibfile.cpp) and [integer-grid reader](https://github.com/bvschaik/citybuilding-mappers/blob/master/gamefile.cpp) establish these format facts. That project is GPL-2.0-or-later. **Its implementation, palettes and coordinate projection were not copied or ported.** This repository keeps its MIT license; the bounded sequential decoder and numerical palette are independently authored. No original executable, disassembly, translated library or old reverse-engineering analysis was used.

The active-side value is distinct from backing capacity. The centered activity box is verified below; original screen projection remains unverified. The full backing grid remains available for diagnosis. The empire map has a different logical header; its internal layout and dimensions remain unsupported.

## Results and verification

Release and UBSan agree: **168/168 complete compressed containers**, **167/167 city terrain grids** and **167/167 CGImages**; **1/1 empire role rejected explicitly**, no malformed-container failures. CGImage creation alone does not prove window display or original-renderer fidelity. The native window passed **167/167 city selections**, five active-side cases and **32/32 bit planes**; the unsupported empire selection cleared the old image and normal quit passed. Selection checks wait for matching filename/plane image identity, rather than accepting a stale accessibility read. Window captures were inspected locally and are excluded from Git; original-renderer fidelity remains unmeasured. `STATUS.md` records these separately.

`make map-test` and `make map-test-undefined` use only authored fixtures. They verify complete multi-block consumption, row-major bounds, bit planes, late checksum failure, trailing bytes, invalid sizes, short grids and unsupported headers/containers. Real metrics use `EMPEROR_GAMEDATA_ROOT` exclusively; when unset they return successful SKIP before building. Reports contain aggregate counts, no decoded grid or pixel exports. Input limit is 4 MiB, decoded limit 8 MiB, at most 256 blocks.

A recursive case-insensitive search of the same supplied corpus found **0 `ChinaMapProj.555` candidates**. No download or substitute was used; the 192 SG3 records requiring that companion remain blocked.

Next MAP work needs evidence for the remaining flag meanings and a verifiable terrain-to-SG3 image connection. Empire-map format needs separate evidence. Complete world parsing remains **unmeasured/168**; diagnostic terrain display is not a playable city or proof of original graphical agreement.

## Verified activity box and partial flag annotations

The same public modern reader identifies a centered activity interval and `0x00080000` as off-map. Independent byte checks establish a safe **rectangular crop**, without porting its minimap projection, palette, branch priorities or renderer. For side S, each axis begins at `(228-S)/2`; the image remains in storage-grid coordinates. Only cells explicitly carrying the off-map bit are transparent. All other values, including unknown flags, are preserved.

| S | Crop origin (both axes) | Corpus maps |
|---|---|---|
| 84 | 72 | 1 |
| 112 | 58 | 16 |
| 140 | 44 | 67 |
| 170 | 29 | 72 |
| 226 | 1 | 11 |

Release/UBSan: **167/167 valid crops and crop CGImages**, preserving **2,107,836 non-off-map cells**, with **0 outside the crop**. The public reader's ideal diamond excludes **56 non-off-map cells across 17 maps**; this implementation preserves them and does not apply a guessed diamond mask. Unlisted sizes or any non-off-map cell outside the proposed crop fail crop validation; the raw grid remains available. This is not proof of original playability or original border rendering.

The bit picker has partial public annotations, not mutually exclusive terrain categories. Primitive field labels are independently recorded; combinations retain their raw values. No GPL implementation or palette is included. Public-source annotations are: tree/bamboo bit0, rock/ore bit1, water bit2, building-related bit3, garden bit5, road bit6, fertility bit7, flood-related bit8, elevation bit9, irrigation bit11, ruins-related bit12, wall bit14, beach bit16, quarry bit17, salt marsh bit18, off-map bit19, mineral/ruins combinations bits20–21, pinnacle bit25, deep-water-related bit26, canal/monument-related bit28 and sand-related bit31. These observations come from [the terrain annotations](https://github.com/bvschaik/citybuilding-mappers/blob/master/emperorfile.cpp#L1056); compound-state behavior and original colours are not ported.

Observed **unannotated bits: 6**, covering **77,341/2,107,836 non-off-map cells**; union mask `0x60c08400`. Counts overlap when several bits occur in one cell:

| Unknown bit | Cell occurrences |
|---|---|
| 10 | 2,807 |
| 15 | 835 |
| 22 | 70,828 |
| 23 | 3,995 |
| 29 | 58,572 |
| 30 | 5 |

The public MAP reader identifies terrain flags and other auxiliary grids, but does not establish an SG3 archive/image-ID join. The skipped auxiliary area is 5×51,984 bytes; the public reader annotates its last byte grid as edges. The remaining layout is unclassified here, rather than assumed to be five separate byte grids or an image-ID grid. Numerical range, filename order or visual resemblance cannot establish IDs. **Verified MAP→SG3 tile joins remain 0/167**; the original image mapping and screen projection are unknown. Diagnostic bit annotations/crops must not be described as original artwork.

Metric acceptance is gated by path role and complete container independently: city grid/CGImage/crop counts cannot be supplied by an empire path; all discovered containers must complete, empire paths must be explicitly excluded from city parsing, and unknown paths or an empty city corpus fail acceptance. Unset root is SKIP, never a real-data pass. `python3 scripts/map_data.py metrics-test` and `--mode undefined` exercise eight authored cases per mode, including swapped roles, incomplete empire, late city corruption, invalid crop, unclassified path, empty data and unset-root SKIP. The real corpus independently passes city **167/167**, empire exclusion **1/1**, complete containers **168/168**, role errors **0** and crops **167/167** in both modes; correcting the gate did not change these measured numerators.

Final native build: **167/167 city crop selections**, five size captures and crop/raw toggles, **32/32 UTF-8 bit annotations**, unknown-bit presentation, empire failure clearing and normal quit passed. An initial label encoding error was corrected before release. A failed selection left an earlier test instance with a delayed quit; it later exited without a forced signal and is not used as successful final-build quit evidence. Screenshots and private runtime/launcher inventory stay outside Git.

## Original-program observation and current startup baseline

Six separately approved agent launches used the existing unsigned bundled Wine installation and the `EmperorShared` launcher. All six produced a named window, five yielded verified main-menu pixels, and none established a city or matching MAP: **named windows 6/6, menu pixels 5/6, city/same-city identity 0/6**. Screenshots and detailed process evidence stay local, outside Git. These are observation metrics, not native-renderer or playability passes.

| 2026-10-13 start (UTC) | Scope | Menu pixels | Exit evidence |
|---|---|---|---|
| 05:58:58 | 180s, original profile | No usable capture | Normal closure at limit, exit0 |
| 07:13:11.747 | 180s, original profile | Verified | Normal closure at limit, exit0 |
| 09:12:50.298 | 20min cap | Verified | ESC preceded normal exit0 at09:15:14.725 |
| 12:12:40.943 | 20min cap | Verified | Normal AX closure, exit0 at12:21:04.828 |
| 12:29:11.834 | 20min cap, crisp | Verified | AX close at12:49:08.976, exit0 at12:49:09.209 |
| 14:05:14.845 | 20min cap, crisp | Verified | Exit0 recorded14:08:44.681, before any close request |

Each launch's PID disappeared and its game windows were zero after exit, without forced signals. Two existing saves retained size/mtime, with none added or removed. Run6 ended about210s after startup; its deadline was14:25:14.845. No mouse event or Command+Return was sent. The controller remained alive, and its fallback found the game already ended. The exit cause is **unknown**: neither a deadline closure nor a model-capacity termination is evidenced. Its603 volume-label warnings and IMK/TSM messages do not establish causation.

### Proven normal window from the user-opened application

The user independently opened `/Applications/皇帝龙之崛起.app` at about14:43:21 UTC. This is a separate existing session, excluded from the six agent-launch denominator. Read-only observation found PID51379, named window23247, frame(211,60,1024,796) points, normal titlebar/borders and main-menu pixels. SCK enumerated one app window; AX ownership/frame and frontmost PID matched. The selected CG window was **layer0, alpha1**, on a1440×900-point display; `CGShieldingWindowID(display)` returned0. This ordinary window is the working startup baseline. The observer did not launch, close or restart it.

| Startup property | Prior shared launcher | User's normal application |
|---|---|---|
| Entry | `EmperorShared/launch.sh`, through the desktop wrapper | `/Applications/皇帝龙之崛起.app`, `WineskinLauncher` |
| Wine prefix | `/Users/zoomzhao/Library/Application Support/EmperorShared/prefix` | `/Applications/皇帝龙之崛起.app/Contents/Resources` |
| Working directory | `EmperorShared/龙之崛起` | `Contents/Resources/drive_c/EmperorRotMK[ZeaS]` in that app |
| Windows executable | `Emperor.exe` | `C:\EmperorRotMK[ZeaS]\Emperor[CH].exe` |
| Extra ddraw setup | Local `ddraw.dll`; explicit `WINEDLLOVERRIDES=ddraw=n,b`; crisp selects `ddraw-crisp.ini` | No `ddraw.dll`, `ddraw.ini` or `ddraw-crisp.ini` in this game directory; inspected registry override blocks contain no relevant ddraw/d3d8/d3d9 override |
| Observed host window | Main at2147483630, peer at2147483628, display-covering geometry | One normal bordered window, layer0 |

Both game executables have SHA256 `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`. The user prefix's directory device/inode matches its wineserver directory identity and differs from the shared prefix. Matching executable bytes do not imply matching runtime/configuration. File/configuration evidence does not prove which DLL was dynamically loaded or which difference caused the windowing behavior. The old profile's `windowed=true`/`border=true` fields proved intent only. No config, registry or security setting was changed. Future user-directed observation should preserve the already-open normal application and avoid repeating the shared launch route.

At15:03:56 UTC, Apple's System Events executed a process-targeted `click at {572.472,424.568}` on the verified single-player button. It returned exit0 and the game window reference; the following window-only image still showed the main menu. Therefore **visible submenu transitions 0/1**, city/MAP identity not established. Command completion is not successful game interaction. Observation/input stopped at the20min cap measured from the first current-window observation at14:47:05, and the user's game was left open. The delayed answer “可以手动检查” belonged to the12:49 question; it does not establish a manual click or a city. No screenshot polling or further click experiment is pending.

A newly authorized focused observation at15:31:18 UTC found the existing PID51379/window23247 at(201,40,1024,796). Its first scoped image, taken before this turn's input, showed **player-account selection1/1** with an account selected and a visible Continue button. The source of that transition is unknown; it is not attributed to automation or a reported manual success. One identity-checked System Events Return command completed0 at15:33:27, but the second image still showed the account page: **Return-triggered transitions0/1**, scoped captures2/2, new game launches0 and no user-game closure. The180s input scope was not extended. Initial sandbox capture preflight failed; the approved unsandboxed readonly observer succeeded without changing permissions. Submenu visibility is now established; actual automated entry, city identity and MAP match remain unverified.

The approved alternative runtime uses official UTM4.7.5 (installer SHA256, code signature and final notarization accepted) and a SHA512-verified Debian12 amd64 cloud image. Actual UTM AppleScript create/configure/start/input calls succeeded after Automation approval; no permission popup is pending. The same owned VM is reused after reconnection. Firmware `help` had a visible keyboard response **1/1** and scoped screenshots succeeded, but a submitted firmware mouse command did not establish a visible control transition. The initial Debian boot diagnosis found that legacy BIOS/VGA exposed **IO-APIC timer kernel panic**, while the prior UEFI boot stayed at kernel/initrd loading. Standard `qemu64,-apic` then passed that panic point and produced kernel/initramfs progress in both serial and VGA output; that17:57 observation alone did not establish full boot or successful cloud-init. Runtime is x86_64 TCG on arm64 host,3GiB RAM/1vCPU; the imported image initially remained3GiB despite the size request. Official UTM resize with its second24GiB confirmation then produced **25,769,803,776 virtual bytes (24GiB)**, verified from the exported QCOW2 header; host image bytes2,395,471,872 remain sparse. Guest root partition **25,635,569,152B** and root filesystem **25,149,710,336B** are separately verified, with21,928,210,432B available. Recovery cloud-init completed; Xorg/Openbox and scoped Xev/xterm pixels are verified. UTM guest mouse at(100,150) produced Xev ButtonPress0→1 and ButtonRelease0→1, observed root(99,149): **real mouse event pair1/1**. Terminal keyboard marker appeared **1/1**. These standard-component passes do not establish original game compatibility. Linux tty1→tty2 keyboard transition is separately verified **1/1**. Existing installation/checkpoints are preserved; a local console recovery seed omits package download/installation and has restarted the same VM for service diagnosis. Disks and seed optical media are confirmed open by the owned QEMU process; no host directory share or port forward is enabled. Wine is not installed and guest game launches remain0. The existing initramfs-tools half-configuration and pending triggers are being completed with standard gzip/dep initramfs settings before the approved official-repository Wine/i386 install. These are runtime diagnostics, not native-data/gameplay progress. The Microsoft official download interface rejected the Windows ISO request; that route was not retried or bypassed. The user-opened Mac game remains preserved.

All167/167 parsed same-name city MAP files previously matched the installed copies byte-for-byte, but no actual city was selected. This proves available data equivalence, not current-scene identity. **Verified MAP→SG3 tile joins remain0/167**; comparable native city region, original projection and image-difference metrics remain unmeasured. Numerical grids, menu images and observed input submission do not substitute for city pixels.

## Scoped input diagnosis and verified limits

The authored driver is `input_lab/scoped_click.hpp`, shared by the CLI and regression. It requires exact PID/UID/birth/executable identity, explicit window ID, unchanged point frame, active/frontmost app, AX owner/frame, matching front CG candidate, no held left button and0.5–1200s remaining. It never launches/activates an app, posts keys, requests permissions or reads game data. A submitted down is balanced by an up after250ms. Focus cannot be atomically locked during that interval, so observed behavior remains necessary.

Run6 captured the missing layer evidence: main23187 at **2147483630/alpha1**, lower same-PID peer23208 at **2147483628/alpha1**, AX on the main, public shield ID0. The previous code skipped every window at the numeric cursor level, including the legal game window, then falsely selected the lower peer. **A numeric level is not a window role.** The fix retains application windows at cursor level and resolves front candidates across all owners at or above the selected layer. Same-owner and foreign front covers remain blocked; lower background windows do not replace the selected window. A narrow cursor exception requires the protected WindowServer executable path, a different system UID, cursor level, bounded small geometry and containment of the actual pointer. Unknown owner identity grants no exception; an API-certified display-shield label still grants no dispatch permission.

The stronger front-candidate check conservatively refuses the user's layer0 window because it sees a Dock-owned1440×900 rectangle at layer20/alpha1, despite AX hitting the game. Global alpha1 does not prove per-pixel opacity or input interception. No transparency/shape witness was measured, so Dock is not blanket-whitelisted. This is a **local guard limitation**, not an OS permission rejection. No raw CG mouse event was posted to the user's game.

Release/UBSan final headless checks each pass **29/29 no-send rejections,3/3 occlusion cases,7/7 layer regressions,5/5 shield diagnostics and5/5 malformed CLI requests**, with0 UI events. The layer regressions cover a legal cursor-level app, a narrowly identified system cursor, a same-level app impostor, a large system overlay, a foreign front cover despite an own AX hit, a same-owner high cover and a lower background window. The earlier live Release fixture for this layer-selection fix uses the observed cursor-level main window and a cover one level above:4/4 refused clicks emit0 events, shared CLI/session action1/1 with2/2 routed mouse events, AX action1/1, readonly inspection passes and normal exit0 without forced signals. Final metadata-type checking was verified headlessly; this live check was not repeated and there is no new UBSan live result. These are authored platform checks, not Wine or original-data progress.

Earlier standard-level authored diagnostics remain historical evidence: Retina screenshot mapping agreed with the independent AX button centre; three PID-event variants gave0/3 actions, while guarded session posting gave1/1 and AX activation1/1. System Events activated a native button1/1 but logged **zero mouse down/up events**. Neither result proves custom game-canvas mouse delivery. Tool discovery found no first-class general macOS mouse tool or cliclick/peekaboo; System Events' first no-prompt preflight was -600 while the service was absent, then0 after its normal startup. There was no execution-permission denial or security-setting change.

[Apple's display-capture documentation](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/QuartzDisplayServicesConceptual/Articles/DisplayCapture.html) describes exclusive drawing and warns normal windows may be invisible over a captured display. [Upstream Wine5.0](https://github.com/wine-mirror/wine/blob/wine-5.0/dlls/winemac.drv/cocoa_window.m#L1091-L1117) raises captured-display windows above shield level. These support a possible rendering explanation, not an OS security rejection or proof of the installed unsigned fork. Exact public `CGShieldingWindowID` matches are required for the shield role; the latest observed ID was0. [Wine's default modifier mapping](https://github.com/wine-mirror/wine/blob/wine-5.0/dlls/winemac.drv/cocoa_window.m#L132-L149) maps Command to Windows Alt; previous Option+Return attempts did not prove a Windows Alt+Enter action. No additional mode-switch experiment is required for the already-normal user window.

Use `make input-test` and `make input-test-undefined` for headless authored checks. `make input-window-test` opts into an authored window; it is not needed while preserving the user's game. Evidence/binaries/captures remain in temporary local directories. `python3 input_lab/run_tests.py --request /absolute/local/request.json --inspect` reads owners/frames, layer/alpha, candidate owner and public display-shield ID without sending input. The explicit request schema is `pid`, `uid`, `birth_seconds`, `birth_micros`, `executable`, `window_id`, `frame_x`, `frame_y`, `frame_width`, `frame_height`, `normalized_x`, `normalized_y`, `deadline_unix_seconds`; values must come from the actual authorized process and scoped frame. Without `--inspect`, that request submits at most one checked click and always leaves `visible_transition_verified=false` until separately observed. There is no pending generic UI-automation expansion.

Next comparison requires a visibly entered city, its matching MAP and original pixels. The user approved replacing the unavailable Windows image route with a local Debian amd64 VM plus official-repository Wine; guest-scoped screenshot and mouse input must pass before game interaction is claimed. No city selector path is assumed. CitySimulation/EMP1 stay frozen; no original binary analysis, original-fragment/translated-library execution or legacy Swift route is resumed.
