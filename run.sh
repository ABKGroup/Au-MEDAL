#!/bin/sh

TRACK="7p5t" # "7p5t" or "6t"
PLACEMENT="Reference"
FIN="3F3F" # "3F3F" or "2F2F" or "2F3F" ...

BASE_OPTION="SP" # "SP" or "MP" or "SP_for_FAx1" or "SP_for_BUFx24" ...
SAVE_DIR="${PLACEMENT}_${TRACK}_${BASE_OPTION}"
SCHEMATIC_FILE="inputs/schematic/asap7sc${TRACK}.sp"


CELLS=$(grep -i .subckt ${SCHEMATIC_FILE} | awk '{print $2}')

MAX_JOBS=2 # For Multi Processing

for CELL in $CELLS; do
    if [[ "$CELL" = "DFFASRHQNx1_ASAP7_75t_R" || "$CELL" = "SDFHx1_ASAP7_75t_R" || "$CELL" = "SDFLx1_ASAP7_75t_R" ]]; then
        continue
    fi

    OPTION=${BASE_OPTION}
    if [[ "$CELL" = "FAx1_ASAP7_75t_R" ]]; then
        OPTION="SP_for_FAx1" # disable MOL routing
    elif [[ "$CELL" = "BUFx24_ASAP7_75t_R" ]]; then
        OPTION="SP_for_BUFx24" # Enable coarse routing grid spacings
    fi

    CONFIG_FILE="inputs/configs/${TRACK}_${FIN}_${OPTION}.json"
    PLACEMENT_DIR="./inputs/placement/${PLACEMENT}_${TRACK}"

    options="--save_dir ${SAVE_DIR} --cell_name ${CELL} --schematic ${SCHEMATIC_FILE} --config ${CONFIG_FILE}"

    if [ ${PLACEMENT} != "" ]; then
        echo "Cell name: ${CELL}, place: ${PLACEMENT}"
        PLACEMENT_FILE="${PLACEMENT_DIR}/${CELL}.txt"
        options="$options --placement_file ${PLACEMENT_FILE}"
    else
        echo "Cell name: ${CELL}"
    fi

    python3 main.py ${options} &

    echo "CPU Usage: `jobs -p | wc -l`"
    # Limit the number of concurrent jobs
    while [ $(jobs -p | wc -l) -ge $MAX_JOBS ]; do
        sleep 1
    done
done

wait