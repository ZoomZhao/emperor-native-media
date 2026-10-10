# Native map input contract

`game_lab/map_geometry.hpp` maps finite view coordinates to grid indices without
depending on a city model, assets, AppKit, or a device. It rejects NaN, infinity,
nonpositive dimensions and points outside `[0, width) × [0, height)`, leaving both
output values unchanged. Valid coordinates use a bottom-left view origin and a
top-left grid origin; the bottom edge maps to the last row.

The previous implementation multiplied a coordinate by the grid dimension before
dividing by the view width. A finite `DBL_MAX` width and its half-width coordinate
overflowed that intermediate product and then converted infinity to `int`.
The shared component normalizes before scaling and checks finite inputs.

Run the standalone synthetic contract test from any working directory:

```sh
python3 -I -B /absolute/repo/scripts/map_geometry_runner.py --mode release --output /absolute/fresh/results
python3 -I -B /absolute/repo/scripts/map_geometry_runner.py --mode undefined --output /absolute/fresh/results
python3 -I -B /absolute/repo/scripts/map_geometry_runner.py --verify /absolute/results/map-input-undefined-RUN_ID
```

Release and UBSan with `float-cast-overflow` each passed 34 cases: the original
large-width/NaN/infinity defects, four edges, finite largest/smallest dimensions,
the maximum `int` grid dimension, and failure output preservation. Each compiler
and test process has a 60-second limit. Reports bind the three explicit authored
sources, binary, process output and result hashes. No city model, GameData,
original game, GUI or audio device is executed.

This verifies the coordinate component. It does not establish original terrain
semantics, faithful city rules, App interaction, rendering or original save
compatibility. The separate local city prototype has no demonstrated original
rule source and is outside this component's publication.
