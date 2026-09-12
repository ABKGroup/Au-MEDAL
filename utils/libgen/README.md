# libgen

Characterizes the routed cells into a Liberty (`.lib`) library with Synopsys SiliconSmart.

## Usage

```bash
cd utils/libgen
python run_libgen.py <database>
```

`<database>` is the name of a cell-results directory at the repo root (for the default
evaluation flow this is `results`). Two inputs must exist beforehand:

- `<repo-root>/<database>/<cell>/area.txt` for every cell, written by
  `utils/run_evaluation.py run --get_gds_area True`.
- `ref_<database>/` next to this script: per-type SPICE netlists
  (`ADDER/AO/INVBUF/OA/PHY/SEQ/SIMPLE` subdirectories), written by
  `utils/run_evaluation.py run --pex True`.

`utils/run_evaluation.py run --lib True` invokes the same entry point automatically.
`siliconsmart` must be on `PATH` with a license.

## What it does

`run_libgen.py <database>` creates the working directory `LIBGEN_<database>/`, changes into
it, and re-invokes itself as `run_libgen.py sm <database>` for the worker step, which

1. copies `ref_lib/*.lib` templates to `./LIB/`, patching each cell's `area` value from the
   database's `area.txt` and renaming the `CUSTOM_4_372_0_70_75T` cell-name token to `R`,
2. copies `ref_hspice/` device models to `./hspice/` and `ref_<database>/` netlists to
   `./spice_models/`, renaming each netlist to `*_ASAP7_75t_R.sp`,
3. generates `LIB_<database>/run_sm.tcl` (per-type cell lists) and `configure_sm.tcl`
   (TT, 0.70 V, 25 C) from the `ref_run.tcl` and `ref_configure.tcl` templates,
4. runs `siliconsmart LIB_<database>/run_sm.tcl`.

Results land in `LIBGEN_<database>/LIB_<database>/models/liberty/*.lib`.

## Contents

- `run_libgen.py`: the whole flow (entry plus the internal `sm` worker subcommand).
- `ref_run.tcl`, `ref_configure.tcl`: SiliconSmart script templates.
- `ref_lib/`: per-type Liberty templates the cell areas are patched into.
- `ref_hspice/`: ASAP7 7 nm device models.
