# Au-MEDAL: Adaptable Grid Router with Metal Edge Detection And Layer Integration

Au-MEDAL is an SMT-based standard cell router that handles various design rules and specifications at the nanometer scale for bidirectional routing. It incorporates techniques such as metal edge detection, inter-layer design rule checking, off-grid-aware handling of pre-placed objects, flexible routing grid spacing, and pin accessibility-aware routing. These capabilities enable Au-MEDAL to generate DRC-clean layouts in ASAP7 and fully integrate middle-of-line (MOL) layers into the BEOL routing.

This work was enabled by the generous academic support of *Cadence Design Systems*, *Synopsys*, and *Siemens EDA*.
We gratefully acknowledge their provision of EDA tools and technologies used in this research.

Any indicators of correlation or performance presented herein are **not** and should **not** be construed as benchmarking of any commercial EDA tool, product, or vendor. Results are provided solely to enable reproducible academic research.

If you use Au-MEDAL in any published work, we would greatly appreciate it if you could cite this paper [\[1\]](https://vlsicad.ucsd.edu/Publications/Conferences/420/c420.pdf).
Reference citation: A. B. Kahng, S. Kang, S. Kim, J. Lee and D. Yoon, "Au-MEDAL: Adaptable Grid Router with Metal Edge Detection And Layer Integration", ASP-DAC 2026.

## Environment Setup

Toolchain (verified):

- g++ 8.5.0 or newer (C++17), cmake 3.15+
- z3 C++ API ([Z3Prover/z3](https://github.com/Z3Prover/z3)): built automatically by
  `./build.sh --with-z3`, or pass `-DZ3_ROOT=<dir with include/ and lib/>` for an
  existing install.

Build:

```bash
./build.sh                # the router (build/flow), with an existing z3 (-DZ3_ROOT=<dir>)
./build.sh --with-z3      # downloads and builds z3 into build/z3 first
./build.sh --with-placer  # also builds the DP placer into build/dp_placer
```

`--with-z3` needs a compiler recent enough for the pinned z3 tag (g++ 8.5 is too old,
g++ 12 verified). Point `CXX` at one if needed: `CXX=/path/to/g++-12 ./build.sh --with-z3`.

## Quick Start

```bash
build/flow --save_dir INVx1 --cell_name INVx1_ASAP7_75t_R \
  --config inputs/configs/7p5t_3F3F_SP.json \
  --placement_file inputs/placement/Reference_7p5t/INVx1_ASAP7_75t_R.json
```

Pre-generated placement JSONs for the full library are under `inputs/placement/`.

### Optional: from CDL (`--with-placer`)

With the placer built, a cell goes from CDL netlist to routed GDS in two commands:

```bash
build/dp_placer/placement -c INVx1_ASAP7_75t_R -i inputs/schematic/asap7sc7p5t.sp \
  -d third_party/DP-placer/DATA/input/Au-MEDAL.style -o INVx1_placement

build/flow --save_dir INVx1 --cell_name INVx1_ASAP7_75t_R \
  --config inputs/configs/7p5t_3F3F_SP.json \
  --placement_file INVx1_placement/INVx1_ASAP7_75t_R_w3.json
```

The placer writes `<cell>_w<N>.json` per candidate width.

## CLI Reference (`build/flow`)

Required arguments:

- `--save_dir`: output directory root
- `--cell_name`: target subckt/cell name
- `--config`: config JSON path
- `--placement_file`: routing input JSON path (routing-only)

Common arguments:

- `--no_cache`: re-route even when the output GDS already exists
- `--log_file`: explicit log file path (default `<save_dir>/<cell>/run.log`)

`build/flow` is routing-only. Placement JSON comes from `build/dp_placer/placement`
(see Quick Start) or from the pre-generated files under `inputs/placement/`.

## Input File Formats

### A) Routing Input JSON (`--placement_file`) [routing-only mode]

In routing-only mode, placement JSON is required.

Example (`inputs/placement/Reference_7p5t/INVx1_ASAP7_75t_R.json` style):

```json
{
  "cell": "INVx1_ASAP7_75t_R",
  "width": 1,
  "columns": [
    {
      "column": 1,
      "nmos": {"name": "M0", "fin": 3, "nets": ["VSS", "A", "Y"]},
      "pmos": {"name": "M1", "fin": 3, "nets": ["VDD", "A", "Y"]}
    }
  ]
}
```

Field definitions:

- `cell`: target cell name.
- `width`: number of placement columns.
- `columns`: ordered transistor columns.
- `column`: 1-based column index.
- `nmos`, `pmos`: transistor objects.
- `name`: transistor instance name.
- `fin`: fin count.
- `nets`: `[drain, gate, source]`.

### B) Schematic SPICE

- SPICE netlist containing `.subckt` definitions: `inputs/schematic/asap7sc7p5t.sp`.
- Used as the placer's CDL input, by the batch driver for the cell list, and by the Calibre
  LVS check as the source netlist.

### C) Config JSON (`--config`)

Top-level keys used at runtime:

- `design_rules`
- `design_specs`
- `design_options`

## Output Artifacts

For `--save_dir <SAVE_DIR> --cell_name <CELL>`:

- `<SAVE_DIR>/<CELL>/<CELL>.gds`: final GDS output.
- `<SAVE_DIR>/<CELL>/run.log`: execution log.
- `<SAVE_DIR>/<CELL>/<CELL>.smt2`: SMT2 model dump (only with `emit_debug_artifacts`).
- `<SAVE_DIR>/<CELL>/ROUTING_UNSAT.txt`: created when no SAT solution is found.

An existing GDS acts as the cache: rerunning the same cell skips the solve unless `--no_cache`
is given (an existing `ROUTING_UNSAT.txt` also forces a re-solve).

Example tree:

```text
INVx1/
  INVx1_ASAP7_75t_R/
    INVx1_ASAP7_75t_R.gds
    run.log
```

## Configuration Reference

Au-MEDAL config has two required top-level blocks plus an optional runtime-options block:

- `design_rules` (required)
- `design_specs` (required)
- the runtime-options block, read from `design_specs.design_option` (also accepted at top-level
  `design_options`). Optional. A missing block uses all defaults.

All distance/width/spacing values are interpreted in `nm` units.

### `design_rules` (technology rules)

| Key | Type | Role | Tuning impact |
| --- | --- | --- | --- |
| `extension` | `layer -> nm` | Metal extension along the routing direction. | Larger helps tip legality, risks congestion. |
| `max_tip_len` | `layer -> nm` | Threshold classifying short metal ends as tips. | Smaller means stricter tip checks. |
| `min_area` | `layer -> nm^2` | Minimum polygon area per layer. | Higher suppresses tiny islands. |
| `min_width` | `layer -> nm` | Minimum legal metal width. | Higher tightens legal geometry. |
| `min_spacing` | nested (`S2S`,`S2T`,`T2T`,`C2C`) | Edge-type aware spacing rules. | Higher adds margin, harder routing. |
| `min_enclosure` | `via -> layer -> nm` | Metal enclosure around via cuts. | Larger is safer, consumes area. |
| `forbidden_layer_overlaps` | array | Same-height overlap bans. | Extra overlap restrictions. |
| `contact_over_active_gate` | boolean | Allow contact over active gate. Read from `design_specs` first, then `design_rules` (default `false`). | `false` is the conservative choice. |

### `design_specs` (stack and physical setup)

| Key | Type | Role | Tuning impact |
| --- | --- | --- | --- |
| `layer_map` | `layer -> gds#` | Logical-to-GDS layer mapping. | Wrong map shifts output layers. |
| `power_layer` | layer array | Power-rail layers. | Rail behavior, graph edge allowances. |
| `ext_pin_layer` | layer array | External pin-access layers. | Where block-level access is expected. |
| `cell_height` | nm | Standard-cell row height. | Vertical routing geometry. |
| `gate_contact_layer` | string | Gate-contact layer name. | Must match the stack. |
| `active_contact_layer` | string | Source/drain contact layer name. | Active-contact track injection. |
| `power_width` | `layer -> nm` | Power-rail width per layer. | Wider rails, less free space. |
| `routing_layers` | array of layer objects | Routing stack definition. | Search space, legal transitions. |
| `vias` | array of via objects | Via connectivity between layers. | Missing entries break layer hops. |
| `max_fins` | `pmos`/`nmos` | Fin count limits. | Stronger devices vs congestion. |
| `diffusion_break` | number | Diffusion-break policy input. | Placement clustering style. |
| `offset` | nested map | Inter-layer geometric offsets. | Misalignment causes systematic DRC. |
| `routing_track_x_unit` | number or `"Nnm"` | Base X track unit. | Larger is faster, coarser. |
| `routing_track_y_unit` | number or `"Nnm"` | Base Y track unit. | Smaller is finer, slower. |
| `gds_database_unit_nm` | number, default `0.25` | GDS database unit (`0.25` = ASAP7). | Wrong value mis-scales the GDS. |

### `design_option` (runtime behavior toggles)

**Block location.** The C++ router reads this block from `design_specs.design_option` (singular,
nested). For convenience the loader also accepts the block at top-level `design_options` (plural)
and at `design_specs.design_options`, and merges them with the nested singular winning on conflict.
A key placed in any of those locations now takes effect, so the keys below are honored regardless of
which spelling a config uses.

The keys the router reads:

| Key | Type | Role | Tuning impact |
| --- | --- | --- | --- |
| `minimum_pin_length` | nm | Minimum external pin length. | Higher aids access, risks congestion. |
| `add_hor_tracks_for_pin` | boolean | Two extra horizontal pin-access Y tracks. | Better hard-pin access, more runtime. |
| `pin_tracks_on_lower_layers` | boolean | Pin-access tracks on lower layers. | Wider access, larger search. |
| `pin_tracks_on_upper_layers` | boolean | Pin-access tracks on upper layers. | Wider access, larger search. |
| `active_contact_tracks_from_pin_geometry` | boolean | Extra active-contact Y tracks from pin geometry. | Better access, more runtime. |
| `pin_stretch_aware` | boolean | Pin-stretch-aware objective preference. | Boundary-pin quality vs wirelength. |
| `metal_optimization_order` | `BIDIRECTION`/`VERTICAL`/`HORIZONTAL` | Objective direction priority per layer. | Biases congestion relief. |
| `allow_below_min_track` | boolean | Loosen near-gate Y-track pruning. | May rescue hard cells, verify DRC. |
| `low_resolution_routing` | boolean | Coarse fast search mode. | Faster runs, lower quality. |
| `ensure_access_points` | boolean | Access-oriented masking of the metal set. | Better block-level access. |
| `max_tolerance` | integer | Tolerance sweep upper bound. | Larger is more robust, slower. |
| `tolerance_step` | integer `>0` | Tolerance sweep step (default 3). | Smaller is exhaustive, slower. |
| `top_layer_candidates` | layer list | Top layers tried in order (default `["M1","M2"]`). | Restrict or extend the search. |
| `top_layer` | string | Force one top layer (empty = off). | One deterministic stack. |
| `pin_label_texttype` | integer | GDS TEXTTYPE for labels (default 251). | Match the PDK convention. |
| `ext_pin_horizontal_edges` | boolean | Horizontal edges at ext-pin track points. | Access vs search size. |
| `ext_pin_via_edges` | boolean | Via edges at ext-pin track points. | Reachability vs complexity. |
| `enable_via_enc_blockage` | boolean | Via-enclosure spacing vs pre-placed polygons (default off). | Can over-constrain dense cells. |
| `default_max_tip_len` | integer nm | Fallback `max_tip_len` (default 36). | Tip handling without a rule. |
| `power_net_name` | string | Power-rail net name (default `VDD`). | Must match the placement nets. |
| `ground_net_name` | string | Ground-rail net name (default `VSS`). | Must match the placement nets. |
| `z3_threads` | integer | z3 threads (default 0 = z3 default), objective-preserving. | Faster hard cells. |
| `pin_edge_lower_bound_cut` | boolean | Optimum-preserving lower-bound cut (default off). | Faster proof, same result. |
| `pin_edge_lb_source_only` | boolean | The cut on the source pin only (default off). | Fewer added clauses. |
| `emit_debug_artifacts` | boolean | Dump SMT2 to `<save_dir>/<cell>/<cell>.smt2` (default off). | Debug only. |
| `log_level` | `DEBUG`/`INFO`/`WARNING`/`ERROR` | Verbosity (default `INFO`). Above `INFO` quiets `run.log`. | Raise for long batch runs. |

### Runtime validation and derived values

- Unknown keys inside the option block are ignored (not rejected), so a typo silently falls back to
  the default. Check spelling against the table above.
- Required top-level keys are `design_rules` and `design_specs` (the option block is optional, and a
  missing block uses all defaults).
- `routing_layers` and `vias` are required keys (a missing key throws at load). Each element
  must be an object with the documented fields. An empty array is not rejected at load time
  and only fails later in the flow.
- Derived runtime fields include:
  `pitch`, `x_unit`, `y_unit`, `x_offset`, `y_offset`, `np_offset`, `num_track`, plus normalized layer/via connectivity maps.

## Batch Flow (`run.sh`)

Route every `.subckt` in the schematic with per-cell config selection. Three known-unroutable
cells (`DFFASRHQNx1_ASAP7_75t_R`, `SDFHx1_ASAP7_75t_R`, `SDFLx1_ASAP7_75t_R`) are skipped:

```bash
./run.sh
```

Environment overrides:

- `TRACK`, `FIN`, `BASE_OPTION`
- `SAVE_DIR` (default `gds_flow_<TRACK>_<BASE_OPTION>`)
- `MAX_PROC` (parallel workers, default 16)
- `NO_CACHE` (1 = re-route existing GDS)
- `CELL_TIMEOUT` (per-cell solve timeout in seconds, default 10800; some cells
  legitimately take well over an hour)
- `CALIBRE_CHECK_DIR`: optional external LVS/DRC runner invoked per routed cell
  (skipped when absent), results in `<SAVE_DIR>/calibre_summary.csv`.

## Repository Layout

- `src/`: the C++ router (`flow.cpp` is the `flow` CLI entrypoint, headers in
  `src/include/`).
- `CMakeLists.txt`, `build.sh`: build (produces `build/flow`).
- `third_party/DP-placer`: the DP placement submodule
  ([AutoCellGen](https://github.com/The-OpenROAD-Project/AutoCellGen) fork, pinned to the
  paper-era commit). Fetch with `git submodule update --init`.
- `run.sh`: multi-cell batch script with per-cell Calibre check.
- `inputs/configs/`: configuration JSON files.
- `inputs/schematic/`: SPICE netlists.
- `inputs/placement/`: placement JSON files.
- `utils/`: Python evaluation utilities: `run_evaluation.py` (subcommands `run`, `metric`,
  `pin_extension`) and the `libgen/` SiliconSmart characterization flow (`run_libgen.py`).
  Both need a Python environment with `gdspy`/`gdstk`.

## Paper-to-Code Feature Mapping

Main feature-to-function mapping (one main entry per feature):

- Metal edge detection + edge-type aware spacing:
  `src/smt.cpp` (`tip_helper`/`side_helper`/`corner_helper`) + `src/smt_spacing.cpp`
  These build the geometric helper variables (tip/side/corner/via/direction) that the
  edge-type-aware spacing constraints use for bidirectional routing.
- Integrated MOL+BEOL routing over one stack:
  `src/graph.cpp` -> `build_routing_graph()`
  Builds one routing graph across the configured layer stack, so MOL and BEOL connectivity
  are solved in a single routing model.
- Variable routing-grid spacing and per-layer track generation:
  `src/placement.cpp` -> `get_y_points()`
  Generates layer-wise Y tracks using per-layer resolution and access-related options.
- Pin accessibility constraints (minimum external pin length):
  `src/smt_net.cpp` -> `SmtModel::add_minimum_pin_length()`
  Adds SMT constraints forcing external-pin routes to meet the configured minimum pin length.
- Pre-placed/fixed-object aware blockage handling in SMT:
  `src/smt_net.cpp` -> `SmtModel::add_pre_layout_blockage()` (+ `src/prelayout_blockage.cpp`)
  Queries pre-layout polygons and injects blocking constraints during solving.
- Top-layer/tolerance search policy:
  `src/solve.cpp` -> `solve_router()`
  Drives the search loop over top-layer candidates and the tolerance sweep.

(The equivalent Python entry points are preserved at tag `v1.0-python`.)

## Evaluation
Design evaluation can be performed using the script `utils/run_evaluation.py`.

This script supports multi-step evaluation utilities (e.g., Liberty generation, GDS merge, and metric collection) around generated cells.

`python3 utils/run_evaluation.py run --save_dir {save_dir} --lib {True/False} --merge_gds {True/False} ...`

## Metric Extraction
The `metric` subcommand of `utils/run_evaluation.py` performs cell-level analysis based on LEF and GDS inputs. It extracts several layout-related metrics and summarizes them into a CSV file, including pin length, obstructive length, M2 wire length, and the number of vias.

`python3 utils/run_evaluation.py metric --gds_path {gds_path} --lef_path {lef_path}`

## LEF Generation

To generate the LEF file from the routed GDS, use the KLayout-based flow at
[ABKGroup/GDS-to-LEF](https://github.com/ABKGroup/GDS-to-LEF) (ASAP7 config included,
no commercial EDA tool required).

<!-- Due to licensing restrictions, we are unable to provide the *Calibre LVS/DRC/PEX* rule files. Kindly request them directly from the ASAP7 website. -->
<!-- The evaluation includes the *Synopsys SiliconSmart* script. To ensure smooth use of *SiliconSmart*, please download the **7nm_TT.pm** file, the **reference .lib** file, and the **reference .cdl** (= ./asap7sc7p5t.sp) file from the publicly available ASAP7 repository. -->

## Paper Experimental Conditions

ASAP7 allows off-grid routing for LISD, LIG, and M1–M3 layers due to its single patterning EUV process *ASAP7_manual_pdf* [\[3\]](https://github.com/The-OpenROAD-Project/asap7_pdk_r1p7/blob/main/docs/asap7_drm_201207a.pdf). Based on this, the following conditions are applied to our block-level evaluation:

1. Off-grid design rule violations on M1–M3 layers are ignored in all baseline block-level designs. According to the ASAP7 reference DRC manual and Calibre DRC rule files, off-grid constraints apply to M4–M5 layers, but not to M1–M3.

2. Pins are extended to the maximum possible length at off-grid locations through the `pin_extension` subcommand of `utils/run_evaluation.py`. Since routers in standard EDA tools tend to operate on grid-aligned tracks, off-grid pins are often less accessible. To improve routability, we enlarge pins to increase the likelihood of on-grid contact.  
This subcommand takes a LEF file and a GDS file as input, and extends the pins to the maximum length allowed by ASAP7 design rules without causing violations.

`python3 utils/run_evaluation.py pin_extension --gds_path {gds_path} --lef_path {lef_path}`


## References
\[1\] A. B. Kahng, S. Kang, S. Kim, J. Lee and D. Yoon, "Au-MEDAL: Adaptable Grid Router with Metal Edge Detection And Layer Integration", in Proc. Asia and South Pacific Design Automation Conference (ASP-DAC) (2026). \[[link](https://vlsicad.ucsd.edu/Publications/Conferences/420/c420.pdf)\]<br>
\[2\] AutoCellGen (Original code of DP-Placer) \[[GitHub](https://github.com/The-OpenROAD-Project/AutoCellGen)\]<br>
\[3\] ASAP7 Manual PDF \[[GitHub](https://github.com/The-OpenROAD-Project/asap7_pdk_r1p7/blob/main/docs/asap7_drm_201207a.pdf)\]
