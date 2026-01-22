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

create_operating_condition __DIR_NAME__
set_opc_temperature __DIR_NAME__ 25
set_opc_default_voltage __DIR_NAME__ 0.7
set_opc_process __DIR_NAME__ [subst {
    { .lib './hspice/7nm_TT.lib' TT}
}]

add_opc_supplies __DIR_NAME__ VDD 0.7
add_opc_grounds __DIR_NAME__ VSS 0

pintype default {
	set logic_high_name VDD
	set logic_low_name VSS
	set logic_high_threshold 0.9
	set logic_low_threshold 0.1
	set prop_delay_inp_level_rise 0.5
	set prop_delay_inp_level_fall 0.5
	set prop_delay_out_level_rise 0.5
	set prop_delay_out_level_fall 0.5
#	set largest_slew 3.2e-10
	set largest_slew 3.2e-8
#	set max_tout 4.15496e-10
	set max_tout 4.15496e-8
}

define_parameters default {
set active_pvts __DIR_NAME__
set power_meas_supplies VDD
set power_meas_grounds VSS
set model_significant_digits 6
set model_significant_digits_area 5
}

