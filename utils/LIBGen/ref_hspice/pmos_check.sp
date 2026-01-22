* P-FET METAL WORK FUNCTION ENGINEERING

.inc 7nm_TT.pm

.PARAM
.GLOBAL gnd! vss!
*.OPTION POST
*.OPTION NODE
*.OPTION LIST

.OP

Vvdd vdd! 0 0.7v
Vgnd gnd! 0 0v

*Vgs g gnd! pulse 0 0.7 0 1p 1p 5n 10n

Vsg gnd! g 0v
Vsd gnd! d 0v

*M1 d g vdd! vdd! pmos_slvt W=27n L=20n nfin=1
*M2 d g vdd! vdd! pmos_lvt W=27n L=20n nfin=1
M1 d g vdd! vdd! pmos_rvt W=27n L=20n nfin=1
*M4 d g vdd! vdd! pmos_sram W=27n L=20n nfin=1

*.tran 1f 5n
*.DC Vds 0 0.7 0.7 SWEEP Vgs 0 0.7 0.7

*.PROBE DC i(M1)
.PRINT i(M1)

.end
