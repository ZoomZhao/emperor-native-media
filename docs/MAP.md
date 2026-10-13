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

The active-side value is distinct from backing capacity. Its placement/cropping and original screen projection remain unverified; the native view displays the full backing grid. The empire map has a different logical header; its internal layout and dimensions remain unsupported.

## Results and verification

Release and UBSan agree: **168/168 complete compressed containers**, **167/167 city terrain grids** and **167/167 CGImages**; **1/1 empire role rejected explicitly**, no malformed-container failures. CGImage creation alone does not prove window display or original-renderer fidelity. The native window passed **167/167 city selections**, five active-side cases and **32/32 bit planes**; the unsupported empire selection cleared the old image and normal quit passed. Selection checks wait for matching filename/plane image identity, rather than accepting a stale accessibility read. Window captures were inspected locally and are excluded from Git; original-renderer fidelity remains unmeasured. `STATUS.md` records these separately.

`make map-test` and `make map-test-undefined` use only authored fixtures. They verify complete multi-block consumption, row-major bounds, bit planes, late checksum failure, trailing bytes, invalid sizes, short grids and unsupported headers/containers. Real metrics use `EMPEROR_GAMEDATA_ROOT` exclusively; when unset they return successful SKIP before building. Reports contain aggregate counts, no decoded grid or pixel exports. Input limit is 4 MiB, decoded limit 8 MiB, at most 256 blocks.

A recursive case-insensitive search of the same supplied corpus found **0 `ChinaMapProj.555` candidates**. No download or substitute was used; the 192 SG3 records requiring that companion remain blocked.

Next MAP work needs independently verifiable terrain flag meanings and screen/crop mapping. Empire-map format needs separate evidence. Complete world parsing remains **unmeasured/168**; diagnostic terrain display is not a playable city or proof of original graphical agreement.
