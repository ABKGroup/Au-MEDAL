# Au-MEDAL on IHP SG13G2

This branch routes standard cells for the open [IHP SG13G2](https://github.com/IHP-GmbH/IHP-Open-PDK)
PDK with the Au-MEDAL SMT router, and carries the cells it produced. The router itself, the
ASAP7 study it was published with, and the evaluation and characterisation scripts are on the
`ASAP7` branch, which also documents the input formats and every config key.

If you use Au-MEDAL in published work, please cite [\[1\]](https://vlsicad.ucsd.edu/Publications/Conferences/420/c420.pdf).
This work was enabled by the academic support of *Cadence Design Systems*, *Synopsys* and
*Siemens EDA*, whose full acknowledgement is on `ASAP7`.

## Build

```bash
./build.sh                  # the router, into build/flow, with an existing z3 (-DZ3_ROOT=<dir>)
./build.sh --with-z3        # downloads and builds z3 into build/z3 first
```

Build on Linux with a C++17 compiler, CMake 3.15 or newer, and the Z3 headers and shared
library. Use `-DZ3_ROOT=<dir>` to select an existing Z3 installation. `--with-z3` builds
the pinned `z3-4.15.4` tag with its own compiler requirements. Set `CXX` to select the compiler.

## Route a cell

```bash
build/flow --save_dir out --cell_name sg13g2_nand2_4 \
  --config inputs/configs/sg13g2.json \
  --placement_file inputs/placement/sg13g2/sg13g2_nand2_4.json \
  --ports Y,A,B,VDD,VSS
```

Keep `--ports` in the CDL subcircuit order. The run writes
`<save_dir>/<cell>/<cell>.gds`. To route another cell, supply a placement JSON in the same
format as the examples in `inputs/placement/sg13g2/`. The DP placer on `ASAP7` produces this format.

`--no_cache` forces a new solve. `--regen_gds_only` replays a validated routing cache with
unchanged config, placement and ports, including after an emitter rebuild. Cache reuse
checks input, executable and output identities. Legacy caches require a new solve.
After an emission failure, retry with the retained routing cache. Required pin failures
stop GDS emission, and unresolved geometry produces `UNDECIDED`.

## CDL, SPICE and Xschem views

Generate CDL, SPICE, schematic and symbol files in a separate output directory:

```bash
python3 tools/generate_ihp_views.py \
  outputs/sg13g2_cells/cells/sg13g2_nand2_4/sg13g2_nand2_4.cdl \
  --output-dir out/views/sg13g2_nand2_4
```

The input is one flat IHP subcircuit with explicit total `w`, `l` and `ng`.
The generator preserves these values and port order. It accepts a legacy neutral
`m=1` but omits the multiplier from every generated CDL, SPICE and schematic view;
other multipliers require an explicit W/ng specification rather than an inferred
conversion. Fresh output paths are required.

Schematics group series devices into vertical stacks and draw PMOS above NMOS.
Each embeds [multiplier-free IHP transistor symbols](tools/assets/ihp_xschem/README.md),
including their drawing, pin order and W/L/ng properties. Their defaults, display
text and netlisting formats cannot restore `m`. Keep Xschem's standard `devices`
directory in its symbol search path; a separately installed IHP symbol revision is
not needed to open these schematics. All 30 were netlisted and rendered with
Xschem 3.4.8RC, preserving the original 220 devices' connections and W/L/ng values.
The multiplier-free CDL files also match the retained GDS in native LVS.

## The cells

The 30 supplied cells pass cell-local DRC, antenna and LVS checks with the IHP decks.
Use the combined GDS, CDL and LEF in `outputs/sg13g2_cells/` for library integration, or
the individual GDS, netlist and Xschem views under `outputs/sg13g2_cells/cells/<cell>/`.

Apply global density checks to the integrated chip. Isolated cells retain global
minimum-density violations. `sg13g2_inv_32` and `sg13g2_buf_32` also trigger `AFil.g1`
at 57.0248% and 56.8108% Active density. The [IHP rule table](https://ihp-open-pdk-docs.readthedocs.io/en/latest/verification/drc/02_main_rules.html)
sets a 55% global maximum. Chip-level density compliance depends on the integrated layout.

Metal1 pins use layer 8/2, full rectangular VDD/VSS rails, and signal access on the
0.48 x 0.42 um grid. Contacts have at least 0.05 um Metal1 enclosure on all sides.
The solver requires a connected M1 grid witness for each signal port; the writer
derives actual pin rectangles from the routed metal, even without external-label
hints. Its legal M1-extension paths and invalid-input guards have separate tests.
Optional two-contact additions use guarded vertical and horizontal candidates within
the existing Active/GatPoly geometry. Some diffusion and GatPoly components retain one
contact. Review contact redundancy for the intended application.

## Config

`inputs/configs/sg13g2.json` is the config the delivered cells were made with. It carries
`design_rules`, `design_specs` and `design_options`, derived from the PDK rule schema. The key
by key reference is on `ASAP7`.

## Regression tests

```bash
python3 -m unittest discover -s tests -v
```

Run the tests on Linux with Python's standard library and a C++17 compiler. Set
`AUMEDAL_FLOW` to a compiled `flow` executable to include the CLI cache regression.
The suite covers cache invalidation, required pins, contact candidates, geometry acceptance,
and netlist/schematic round trips.

## LEF

The LEF comes from the KLayout flow at
[ABKGroup/GDS-to-LEF](https://github.com/ABKGroup/GDS-to-LEF), which carries a config for IHP
SG13G2. Use `outputs/sg13g2_cells/lef/sg13g2_cells.lef` for the supplied library.

## References

\[1\] A. B. Kahng, S. Kang, S. Kim, J. Lee and D. Yoon, "Au-MEDAL: Adaptable Grid Router with Metal Edge Detection And Layer Integration", in Proc. Asia and South Pacific Design Automation Conference (ASP-DAC) (2026). \[[link](https://vlsicad.ucsd.edu/Publications/Conferences/420/c420.pdf)\]<br>
\[2\] AutoCellGen (the original DP-Placer code) \[[GitHub](https://github.com/The-OpenROAD-Project/AutoCellGen)\]<br>
\[3\] IHP Open PDK \[[GitHub](https://github.com/IHP-GmbH/IHP-Open-PDK)\]
