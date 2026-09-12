##################################################################
# Portions Copyright 2022 Synopsys, Inc. All rights reserved.
# Portions of these TCL scripts are proprietary to and owned
# by Synopsys, Inc. and may only be used for internal use by
# educational institutions (including United States government
# labs, research institutes and federally funded research and
# development centers) on Synopsys tools for non-profit research,
# development, instruction, and other non-commercial uses or as
# otherwise specifically set forth by written agreement with
# Synopsys. All other use, reproduction, modification, or
# distribution of these TCL scripts is strictly prohibited.
##################################################################

set cellsAO {}
set cellsOA {}
set cellsINVBUF {}
set cellsSIMPLE {}
set cellsSEQ {}
set cellsADDER {}
set cellsPHY {}

set charpoint __DIR_NAME__
create $charpoint
set_log_file ${charpoint}.log

exec cp ${charpoint}/configure_sm.tcl ${charpoint}/config/configure.tcl
set_location $charpoint 

#set simulator hspice_embedded
set simulator finesim_embedded
set simulator_default_options 1

set_config_opt job_scheduler standalone
set_config_opt run_list_maxsize 8
set_config_opt normal_queue {bnoraml -R rusage[mem=4000]}

## liberty files ##

import -fast -liberty LIB/asap7sc7p5t_AO_RVT_TT_nldm.lib -extension .sp -netlist_dir spice_models/AO $cellsAO
configure -fast -timing -power $cellsAO
characterize $cellsAO
model -timing -power -output AO $cellsAO

import -fast -liberty LIB/asap7sc7p5t_OA_RVT_TT_nldm.lib -extension .sp -netlist_dir spice_models/OA $cellsOA
configure -fast -timing -power $cellsOA
characterize $cellsOA
model -timing -power -output OA $cellsOA

import -fast -liberty LIB/asap7sc7p5t_INVBUF_RVT_TT_nldm.lib -extension .sp -netlist_dir spice_models/INVBUF $cellsINVBUF
configure -fast -timing -power $cellsINVBUF
characterize $cellsINVBUF
model -timing -power -output INVBUF $cellsINVBUF

import -fast -liberty LIB/asap7sc7p5t_SEQ_RVT_TT_nldm.lib -extension .sp -netlist_dir spice_models/SEQ $cellsSEQ
configure -fast -timing -power $cellsSEQ
characterize $cellsSEQ
model -timing -power -output SEQ $cellsSEQ

import -fast -liberty LIB/asap7sc7p5t_SIMPLE_RVT_TT_nldm.lib -extension .sp -netlist_dir spice_models/SIMPLE $cellsSIMPLE
configure -fast -timing -power $cellsSIMPLE
characterize -fast $cellsSIMPLE
model -timing -power -output SIMPLE $cellsSIMPLE

import -skeleton -fast -liberty LIB/asap7sc7p5t_PHY_RVT_TT_nldm.lib -extension .sp -netlist_dir spice_models/PHY $cellsPHY
configure -fast -timing -power $cellsPHY
characterize -fast $cellsPHY
model -power -output PHY $cellsPHY


set_config_opt -type delay table_dimensions 3
import -fast -liberty LIB/asap7sc7p5t_ADDER_RVT_TT_nldm.lib -extension .sp -netlist_dir spice_models/ADDER $cellsADDER
configure -fast -timing -power $cellsADDER
characterize $cellsADDER
model -timing -power -output ADDER $cellsADDER
