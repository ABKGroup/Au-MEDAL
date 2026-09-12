#!/usr/bin/env bash
# Pure-C++ batch standard-cell generator (no Python).
#
# Replaces `./run.sh`'s cell loop: iterates every .subckt in the schematic, selects the
# per-cell config exactly as run.sh does, and routes with build/flow.
# Output DB layout matches the Python flow: <SAVE_DIR>/<cell>/<cell>.gds (+ run.log,
# ROUTING_UNSAT.txt). Calibre LVS/DRC mirrors run.sh's post-check (same env vars,
# same CALIBRE_DRC_EXCLUDE default, same calibre_summary.csv row format) so C++-flow
# output is checked and reported the same way as the Python flow.
#
# Usage:  ./run.sh
# Env:    TRACK=7p5t FIN=3F3F BASE_OPTION=SP SAVE_DIR=<dir> MAX_PROC=16 NO_CACHE=0
#         CELL_TIMEOUT=10800 (per-cell solve timeout in seconds; ensure_access_points
#         cells like FAx1 legitimately take 20+ minutes at tolerance 6)
#         ENABLE_CALIBRE_CHECK=1 CALIBRE_CHECK_DIR=... CALIBRE_MODULE=... CALIBRE_DRC_EXCLUDE=...
set -u
cd "$(cd "$(dirname "$0")" && pwd)"   # repo root

TRACK="${TRACK:-7p5t}"
FIN="${FIN:-3F3F}"
BASE_OPTION="${BASE_OPTION:-SP}"
SAVE_DIR="${SAVE_DIR:-gds_flow_${TRACK}_${BASE_OPTION}}"
MAX_PROC="${MAX_PROC:-16}"
NO_CACHE="${NO_CACHE:-0}"
SCHEMATIC_FILE="inputs/schematic/asap7sc${TRACK}.sp"
PLACEMENT_DIR="inputs/placement/Reference_${TRACK}"
DRIVER="build/flow"

# Calibre post-check (same defaults/semantics as run.sh's run_cell()).
ENABLE_CALIBRE_CHECK="${ENABLE_CALIBRE_CHECK:-1}"
CALIBRE_CHECK_DIR="${CALIBRE_CHECK_DIR:-utils/validation}"
# Post-Cell-Gen/run.sh builds log paths as logs/{lvs,drc}/<run-id>_<cell>.log, so a run-id
# containing "/" (a SAVE_DIR containing "/") breaks that path. Sanitize.
CALIBRE_RUN_ID="${CALIBRE_RUN_ID:-$(printf '%s' "${SAVE_DIR}_auto" | tr '/' '_')}"
CALIBRE_MODULE="${CALIBRE_MODULE:-calibre/2022.2_24.16}"
CALIBRE_DRC_EXCLUDE="${CALIBRE_DRC_EXCLUDE:-ACTIVE.LUP.1}"
CALIBRE_SUMMARY_FILE="${SAVE_DIR}/calibre_summary.csv"

[ -x "$DRIVER" ] || { echo "ERROR: $DRIVER not built. Run: ./build.sh"; exit 1; }
[ -f "$SCHEMATIC_FILE" ] || { echo "ERROR: schematic missing: $SCHEMATIC_FILE"; exit 1; }

mkdir -p "$SAVE_DIR/_logs"
[ "$ENABLE_CALIBRE_CHECK" = "1" ] && : > "$CALIBRE_SUMMARY_FILE"
export ENABLE_CALIBRE_CHECK CALIBRE_CHECK_DIR CALIBRE_RUN_ID CALIBRE_MODULE CALIBRE_DRC_EXCLUDE CALIBRE_SUMMARY_FILE
HOME_DIR="$(pwd)"
export HOME_DIR SCHEMATIC_FILE

# Mirrors run.sh's Calibre post-check block. $1=cell $2=gds_file $3=database_dir
calibre_check() {
  local cell="$1" gds_file="$2" database_dir="$3"
  [ "$ENABLE_CALIBRE_CHECK" = "1" ] || return 0
  [ -f "$gds_file" ] || { printf '%-30s SKIP_CALIBRE (missing GDS)\n' "$cell"; return 0; }
  [ -x "${CALIBRE_CHECK_DIR}/run.sh" ] || { printf '%-30s SKIP_CALIBRE (runner missing: %s)\n' "$cell" "${CALIBRE_CHECK_DIR}/run.sh"; return 0; }

  local drc_exclude_args=()
  [ -n "$CALIBRE_DRC_EXCLUDE" ] && drc_exclude_args=(--drc-exclude "$CALIBRE_DRC_EXCLUDE")

  local cal_log="${database_dir}/${cell}.cal.log"
  local cal_rc=0
  if ! (
    cd "$CALIBRE_CHECK_DIR"
    ./run.sh \
        --lvs --drc \
        --layout "${HOME_DIR}/${gds_file}" \
        --top "$cell" \
        --netlist "${HOME_DIR}/${SCHEMATIC_FILE}" \
        "${drc_exclude_args[@]}" \
        --module "$CALIBRE_MODULE" \
        --run-id "$CALIBRE_RUN_ID"
  ) > "$cal_log" 2>&1; then
    cal_rc=$?
  fi

  local lvs_status drc_status
  lvs_status="$(grep -oE 'LVS=[A-Z]+' "$cal_log" | tail -1)"
  drc_status="$(grep -oE 'DRC_RESULTS=[0-9]+' "$cal_log" | tail -1)"
  [ -z "$lvs_status" ] && lvs_status="LVS=UNKNOWN(rc=${cal_rc})"
  [ -z "$drc_status" ] && drc_status="DRC_RESULTS=UNKNOWN"

  (
    flock -x 200
    echo "${cell},GDS_OK,${lvs_status},${drc_status}" >> "$CALIBRE_SUMMARY_FILE"
  ) 200>"${CALIBRE_SUMMARY_FILE}.lock"
}
export -f calibre_check

# Per-cell config selection, identical to run.sh.
config_for() {
  local cell="$1" opt="$BASE_OPTION"
  case "$cell" in
    FAx1_ASAP7_75t_R)    opt="SP_for_FAx1" ;;    # disable MOL routing
    BUFx24_ASAP7_75t_R)  opt="SP_for_BUFx24" ;;  # coarse routing grid
  esac
  echo "inputs/configs/${TRACK}_${FIN}_${opt}.json"
}
export -f config_for
export TRACK FIN BASE_OPTION SAVE_DIR PLACEMENT_DIR DRIVER NO_CACHE

route_one() {
  local cell="$1"
  # Cells excluded by run.sh (known-unroutable / scan flops out of scope).
  case "$cell" in
    DFFASRHQNx1_ASAP7_75t_R|SDFHx1_ASAP7_75t_R|SDFLx1_ASAP7_75t_R)
      printf '%-30s SKIP (excluded)\n' "$cell"; return 0 ;;
  esac
  local cfg pl log nc=() database_dir gds_file
  cfg="$(config_for "$cell")"
  pl="$PLACEMENT_DIR/$cell.json"
  log="$SAVE_DIR/_logs/$cell.log"
  database_dir="$SAVE_DIR/$cell"
  gds_file="$database_dir/$cell.gds"
  [ -f "$pl" ] || { printf '%-30s NO_PLACEMENT (%s)\n' "$cell" "$pl"; return 0; }
  [ "$NO_CACHE" = "1" ] && nc=(--no_cache)
  timeout "${CELL_TIMEOUT:-10800}" "$DRIVER" --save_dir "$SAVE_DIR" --cell_name "$cell" \
      --config "$cfg" --placement_file "$pl" "${nc[@]}" > "$log" 2>&1
  local rc=$?
  local st
  st="$(grep -oE 'status: (SAT|UNSAT)[^]]*' "$log" | head -1)"
  [ "$rc" = "124" ] && st="TIMEOUT"
  printf '%-30s rc=%-3s %s\n' "$cell" "$rc" "$st"
  [ "$rc" = "0" ] && [[ "$st" == status:\ SAT* ]] && calibre_check "$cell" "$gds_file" "$database_dir"
}
export -f route_one

# Cell list from the schematic subckts (same source as run.sh).
grep -i '.subckt' "$SCHEMATIC_FILE" | awk '{print $2}' \
  | xargs -P"$MAX_PROC" -I{} bash -c 'route_one "$@"' _ {} \
  | sort > "$SAVE_DIR/_summary.txt"

echo "=== GENERATION SUMMARY ($SAVE_DIR) ==="
total=$(grep -cvE 'SKIP|NO_PLACEMENT' "$SAVE_DIR/_summary.txt")
sat=$(grep -c 'status: SAT' "$SAVE_DIR/_summary.txt")
unsat=$(grep -c 'status: UNSAT' "$SAVE_DIR/_summary.txt")
to=$(grep -c 'TIMEOUT' "$SAVE_DIR/_summary.txt")
gds=$(find "$SAVE_DIR" -name '*.gds' | wc -l)
echo "attempted=$total SAT=$sat UNSAT=$unsat TIMEOUT=$to gds_files=$gds"
echo "=== non-SAT / skipped ==="
grep -vE 'status: SAT' "$SAVE_DIR/_summary.txt"

if [ "$ENABLE_CALIBRE_CHECK" = "1" ] && [ -f "$CALIBRE_SUMMARY_FILE" ]; then
  echo "=== CALIBRE SUMMARY ($CALIBRE_SUMMARY_FILE) ==="
  checked=$(wc -l < "$CALIBRE_SUMMARY_FILE")
  lvs_ok=$(grep -c 'LVS=CORRECT' "$CALIBRE_SUMMARY_FILE")
  drc_clean=$(grep -c 'DRC_RESULTS=0$' "$CALIBRE_SUMMARY_FILE")
  echo "checked=$checked LVS_CORRECT=$lvs_ok DRC_CLEAN=$drc_clean"
  echo "=== non-clean Calibre rows ==="
  grep -vE 'LVS=CORRECT,DRC_RESULTS=0$' "$CALIBRE_SUMMARY_FILE" || echo "(none)"
fi
