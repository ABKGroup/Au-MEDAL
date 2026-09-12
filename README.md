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

g++ 8.5 or newer (C++17) and cmake 3.15 or newer. The pinned z3 tag needs a newer compiler
than 8.5 (g++ 12 verified), so point `CXX` at one when using `--with-z3`.

## Route a cell

```bash
build/flow --save_dir out --cell_name sg13g2_nand2_4 \
  --config inputs/configs/sg13g2.json \
  --placement_file inputs/placement/sg13g2/sg13g2_nand2_4.json \
  --ports A,B,VDD,VSS,Y
```

Those four flags are all that is required, with `--ports` in the library order. The run writes
`<save_dir>/<cell>/<cell>.gds`, and that is how every delivered cell was made. A cell not listed
in `inputs/placement/sg13g2/` needs its own placement JSON in the same shape, which the DP placer
on `ASAP7` writes.

## The cells

`outputs/sg13g2_cells/` holds 30 routed cells, 28 the PDK library does not have and 2 it also
provides. Every cell has its layout, its netlists in the library form, the xschem views, a
rendering and every report a verdict was read from, under `cells/<cell>/`, and the library is
also collected into one GDS, one CDL and one LEF.

Every cell passes the PDK's own DRC runner at its defaults with no item outside the density
rules, its antenna deck with no item, and LVS against the CDL here, each on the cell's own GDS
and again on the cell taken out of the collected library. Rows of these cells flipped against
each other, and mixed with rows of library cells, give no item outside the density rules either.

The GDS writer in `src/` is the one that emitted those layouts, from routing results earlier
builds of this router produced. `inputs/configs/sg13g2.json` is the config they were
made with and `inputs/placement/sg13g2/` holds the placements they were routed from.

## Config

`inputs/configs/sg13g2.json` is the config the delivered cells were made with. It carries
`design_rules`, `design_specs` and `design_options`, derived from the PDK rule schema. The key
by key reference is on `ASAP7`.

## Repository layout

- `src/`: the C++ router, with `flow.cpp` as the `flow` entry point and the headers in `src/include/`.
- `CMakeLists.txt`, `build.sh`: the build, which produces `build/flow`.
- `inputs/configs/`, `inputs/placement/`: the config and the placements.
- `outputs/sg13g2_cells/`: the delivered cells.

## LEF

The LEF comes from the KLayout flow at
[ABKGroup/GDS-to-LEF](https://github.com/ABKGroup/GDS-to-LEF), which carries a config for IHP
SG13G2 and needs no commercial EDA tool. The library LEF of the delivered cells was made that
way and is in `outputs/sg13g2_cells/lef/sg13g2_cells.lef`.

## References

\[1\] A. B. Kahng, S. Kang, S. Kim, J. Lee and D. Yoon, "Au-MEDAL: Adaptable Grid Router with Metal Edge Detection And Layer Integration", in Proc. Asia and South Pacific Design Automation Conference (ASP-DAC) (2026). \[[link](https://vlsicad.ucsd.edu/Publications/Conferences/420/c420.pdf)\]<br>
\[2\] AutoCellGen (the original DP-Placer code) \[[GitHub](https://github.com/The-OpenROAD-Project/AutoCellGen)\]<br>
\[3\] IHP Open PDK \[[GitHub](https://github.com/IHP-GmbH/IHP-Open-PDK)\]
