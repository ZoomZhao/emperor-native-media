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

The public MAP reader identifies terrain flags and other auxiliary grids, but does not establish an SG3 archive/image-ID join. The five skipped byte grids are unclassified here. Numerical range, filename order or visual resemblance cannot establish IDs. **Verified MAP→SG3 tile joins remain 0/167**; the original image mapping and screen projection are unknown. Diagnostic bit annotations/crops must not be described as original artwork.

Metric acceptance is gated by path role and complete container independently: city grid/CGImage/crop counts cannot be supplied by an empire path; all discovered containers must complete, empire paths must be explicitly excluded from city parsing, and unknown paths or an empty city corpus fail acceptance. Unset root is SKIP, never a real-data pass. `python3 scripts/map_data.py metrics-test` and `--mode undefined` exercise eight authored cases per mode, including swapped roles, incomplete empire, late city corruption, invalid crop, unclassified path, empty data and unset-root SKIP. The real corpus independently passes city **167/167**, empire exclusion **1/1**, complete containers **168/168**, role errors **0** and crops **167/167** in both modes; correcting the gate did not change these measured numerators.

Final native build: **167/167 city crop selections**, five size captures and crop/raw toggles, **32/32 UTF-8 bit annotations**, unknown-bit presentation, empire failure clearing and normal quit passed. An initial label encoding error was corrected before release. A failed selection left an earlier test instance with a delayed quit; it later exited without a forced signal and is not used as successful final-build quit evidence. Screenshots and private runtime/launcher inventory stay outside Git.
