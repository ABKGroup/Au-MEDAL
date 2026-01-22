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

set cellsAO { A2O1A1Ixp33_ASAP7_75t_R A2O1A1O1Ixp25_ASAP7_75t_R AO211x2_ASAP7_75t_R AO21x1_ASAP7_75t_R AO21x2_ASAP7_75t_R AO221x1_ASAP7_75t_R AO221x2_ASAP7_75t_R AO222x2_ASAP7_75t_R AO22x1_ASAP7_75t_R AO22x2_ASAP7_75t_R AO31x2_ASAP7_75t_R AO322x2_ASAP7_75t_R AO32x1_ASAP7_75t_R AO32x2_ASAP7_75t_R AO331x1_ASAP7_75t_R AO331x2_ASAP7_75t_R AO332x1_ASAP7_75t_R AO332x2_ASAP7_75t_R AO333x1_ASAP7_75t_R AO333x2_ASAP7_75t_R AO33x2_ASAP7_75t_R AOI211x1_ASAP7_75t_R AOI211xp5_ASAP7_75t_R AOI21x1_ASAP7_75t_R AOI21xp33_ASAP7_75t_R AOI21xp5_ASAP7_75t_R AOI221x1_ASAP7_75t_R AOI221xp5_ASAP7_75t_R AOI222xp33_ASAP7_75t_R AOI22x1_ASAP7_75t_R AOI22xp33_ASAP7_75t_R AOI22xp5_ASAP7_75t_R AOI311xp33_ASAP7_75t_R AOI31xp33_ASAP7_75t_R AOI31xp67_ASAP7_75t_R AOI321xp33_ASAP7_75t_R AOI322xp5_ASAP7_75t_R AOI32xp33_ASAP7_75t_R AOI331xp33_ASAP7_75t_R AOI332xp33_ASAP7_75t_R AOI333xp33_ASAP7_75t_R AOI33xp33_ASAP7_75t_R }
set cellsOA { O2A1O1Ixp33_ASAP7_75t_R O2A1O1Ixp5_ASAP7_75t_R OA211x2_ASAP7_75t_R OA21x2_ASAP7_75t_R OA221x2_ASAP7_75t_R OA222x2_ASAP7_75t_R OA22x2_ASAP7_75t_R OA31x2_ASAP7_75t_R OA331x1_ASAP7_75t_R OA331x2_ASAP7_75t_R OA332x1_ASAP7_75t_R OA332x2_ASAP7_75t_R OA333x1_ASAP7_75t_R OA333x2_ASAP7_75t_R OA33x2_ASAP7_75t_R OAI211xp5_ASAP7_75t_R OAI21x1_ASAP7_75t_R OAI21xp33_ASAP7_75t_R OAI21xp5_ASAP7_75t_R OAI221xp5_ASAP7_75t_R OAI222xp33_ASAP7_75t_R OAI22x1_ASAP7_75t_R OAI22xp33_ASAP7_75t_R OAI22xp5_ASAP7_75t_R OAI311xp33_ASAP7_75t_R OAI31xp33_ASAP7_75t_R OAI31xp67_ASAP7_75t_R OAI321xp33_ASAP7_75t_R OAI322xp33_ASAP7_75t_R OAI32xp33_ASAP7_75t_R OAI331xp33_ASAP7_75t_R OAI332xp33_ASAP7_75t_R OAI333xp33_ASAP7_75t_R OAI33xp33_ASAP7_75t_R }
set cellsINVBUF { BUFx10_ASAP7_75t_R BUFx12_ASAP7_75t_R BUFx12f_ASAP7_75t_R BUFx16f_ASAP7_75t_R BUFx24_ASAP7_75t_R BUFx2_ASAP7_75t_R BUFx3_ASAP7_75t_R BUFx4_ASAP7_75t_R BUFx4f_ASAP7_75t_R BUFx5_ASAP7_75t_R BUFx6f_ASAP7_75t_R BUFx8_ASAP7_75t_R HB1xp67_ASAP7_75t_R HB2xp67_ASAP7_75t_R HB3xp67_ASAP7_75t_R HB4xp67_ASAP7_75t_R INVx11_ASAP7_75t_R INVx13_ASAP7_75t_R INVx1_ASAP7_75t_R INVx2_ASAP7_75t_R INVx3_ASAP7_75t_R INVx4_ASAP7_75t_R INVx5_ASAP7_75t_R INVx6_ASAP7_75t_R INVx8_ASAP7_75t_R INVxp33_ASAP7_75t_R INVxp67_ASAP7_75t_R }
set cellsSIMPLE { AND2x2_ASAP7_75t_R AND2x4_ASAP7_75t_R AND2x6_ASAP7_75t_R AND3x1_ASAP7_75t_R AND3x2_ASAP7_75t_R AND3x4_ASAP7_75t_R AND4x1_ASAP7_75t_R AND4x2_ASAP7_75t_R AND5x1_ASAP7_75t_R AND5x2_ASAP7_75t_R MAJIxp5_ASAP7_75t_R MAJx2_ASAP7_75t_R MAJx3_ASAP7_75t_R NAND2x1_ASAP7_75t_R NAND2x1p5_ASAP7_75t_R NAND2x2_ASAP7_75t_R NAND2xp33_ASAP7_75t_R NAND2xp5_ASAP7_75t_R NAND2xp67_ASAP7_75t_R NAND3x1_ASAP7_75t_R NAND3x2_ASAP7_75t_R NAND3xp33_ASAP7_75t_R NAND4xp25_ASAP7_75t_R NAND4xp75_ASAP7_75t_R NAND5xp2_ASAP7_75t_R NOR2x1_ASAP7_75t_R NOR2x1p5_ASAP7_75t_R NOR2x2_ASAP7_75t_R NOR2xp33_ASAP7_75t_R NOR2xp67_ASAP7_75t_R NOR3x1_ASAP7_75t_R NOR3x2_ASAP7_75t_R NOR3xp33_ASAP7_75t_R NOR4xp25_ASAP7_75t_R NOR4xp75_ASAP7_75t_R NOR5xp2_ASAP7_75t_R OR2x2_ASAP7_75t_R OR2x4_ASAP7_75t_R OR2x6_ASAP7_75t_R OR3x1_ASAP7_75t_R OR3x2_ASAP7_75t_R OR3x4_ASAP7_75t_R OR4x1_ASAP7_75t_R OR4x2_ASAP7_75t_R OR5x1_ASAP7_75t_R OR5x2_ASAP7_75t_R XNOR2x1_ASAP7_75t_R XNOR2x2_ASAP7_75t_R XNOR2xp5_ASAP7_75t_R XOR2x1_ASAP7_75t_R XOR2x2_ASAP7_75t_R XOR2xp5_ASAP7_75t_R }
set cellsSEQ { DFFASRHQNx1_ASAP7_75t_R DFFHQNx1_ASAP7_75t_R DFFHQNx2_ASAP7_75t_R DFFHQNx3_ASAP7_75t_R DFFHQx4_ASAP7_75t_R DFFLQNx1_ASAP7_75t_R DFFLQNx2_ASAP7_75t_R DFFLQNx3_ASAP7_75t_R DFFLQx4_ASAP7_75t_R DHLx1_ASAP7_75t_R DHLx2_ASAP7_75t_R DHLx3_ASAP7_75t_R DLLx1_ASAP7_75t_R DLLx2_ASAP7_75t_R DLLx3_ASAP7_75t_R }
set cellsADDER { FAx1_ASAP7_75t_R HAxp5_ASAP7_75t_R }
set cellsPHY { }

set charpoint LIB_results
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
