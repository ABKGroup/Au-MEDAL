.subckt PM_INVxp33_ASAP7_75t_R%A vss 23 8 14 3 7
c1 1 vss 0.0047681f $X=0.072 $Y=0.135
c2 3 vss 0.0599411f $X=0.081 $Y=0.0245
c3 4 vss 0.00101579f $X=0.027 $Y=0.135
c4 5 vss 0.00970256f $X=0.027 $Y=0.0855
c5 6 vss 0.00970256f $X=0.027 $Y=0.1465
c6 7 vss 0.00169275f $X=0.0455 $Y=0.135
r1 23 6 0.699569 $w=1.3e-08 $l=3e-09 $layer=M1 $thickness=3.6e-08 $X=0.022
+ $Y=0.1495 $X2=0.027 $Y2=0.1465
r2 4 7 3.1337 $w=1.54324e-08 $l=1.85e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.135 $X2=0.0455 $Y2=0.135
r3 4 5 10.3626 $w=1.39091e-08 $l=4.95e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.135 $X2=0.027 $Y2=0.0855
r4 4 6 1.50137 $w=1.6913e-08 $l=1.15e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.135 $X2=0.027 $Y2=0.1465
r5 7 20 4.31401 $w=1.3e-08 $l=1.85e-08 $layer=M1 $thickness=3.6e-08 $X=0.0455
+ $Y=0.135 $X2=0.064 $Y2=0.135
r6 18 20 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.063 $Y=0.135 $X2=0.064 $Y2=0.135
r7 1 16 2.6116 $w=2.2e-08 $l=1e-08 $layer=LIG $thickness=4.8e-08 $X=0.072
+ $Y=0.135 $X2=0.082 $Y2=0.135
r8 1 18 4.98695 $w=1.60444e-08 $l=9e-09 $layer=LIG $thickness=4.8e-08 $X=0.072
+ $Y=0.135 $X2=0.063 $Y2=0.135
r9 14 13 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.2295 $X2=0.081 $Y2=0.1865
r10 12 13 11.9048 $w=2.1e-08 $l=3.5e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.1515 $X2=0.081 $Y2=0.1865
r11 11 12 5.79936 $w=2.03333e-08 $l=1.65e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.1515
r12 11 16 6.27904 $w=2.09e-08 $l=1e-09 $layer=Gate_1 $thickness=5.24e-08
+ $X=0.081 $Y=0.135 $X2=0.082 $Y2=0.135
r13 10 11 5.79936 $w=2.03333e-08 $l=1.65e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.1185 $X2=0.081 $Y2=0.135
r14 9 10 11.9048 $w=2.1e-08 $l=3.5e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.0835 $X2=0.081 $Y2=0.1185
r15 8 9 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0405 $X2=0.081 $Y2=0.0835
r16 8 3 5.44221 $w=2.1e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0405 $X2=0.081 $Y2=0.0245
.ends

.subckt PM_INVxp33_ASAP7_75t_R%Y vss 17 13 25 8 11 7
c1 1 vss 0.00664287f $X=0.108 $Y=0.0405
c2 2 vss 0.00662323f $X=0.108 $Y=0.2295
c3 7 vss 0.0184478f $X=0.0935 $Y=0.0405
c4 8 vss 0.0184441f $X=0.0935 $Y=0.2295
c5 9 vss 0.00650506f $X=0.0965 $Y=0.036
c6 10 vss 0.00651929f $X=0.135 $Y=0.234
c7 11 vss 0.00862817f $X=0.135 $Y=0.0795
r1 8 2 0.462963 $w=2.7e-08 $l=1.25e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.0935 $Y=0.2295 $X2=0.108 $Y2=0.2295
r2 25 8 0.0925926 $w=2.7e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.2295 $X2=0.0935 $Y2=0.2295
r3 2 22 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.108 $Y=0.2295 $X2=0.108 $Y2=0.234
r4 22 23 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.108
+ $Y=0.234 $X2=0.1215 $Y2=0.234
r5 10 20 10.3626 $w=1.39091e-08 $l=4.95e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.135 $Y=0.234 $X2=0.135 $Y2=0.1845
r6 10 23 1.96775 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.135 $Y=0.234 $X2=0.1215 $Y2=0.234
r7 19 20 11.5429 $w=1.3e-08 $l=4.95e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.135 $X2=0.135 $Y2=0.1845
r8 18 19 3.38125 $w=1.3e-08 $l=1.45e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.1205 $X2=0.135 $Y2=0.135
r9 17 18 1.39914 $w=1.3e-08 $l=6e-09 $layer=M1 $thickness=3.6e-08 $X=0.1355
+ $Y=0.1145 $X2=0.135 $Y2=0.1205
r10 17 11 8.16164 $w=1.3e-08 $l=3.5e-08 $layer=M1 $thickness=3.6e-08 $X=0.1355
+ $Y=0.1145 $X2=0.135 $Y2=0.0795
r11 11 16 8.96345 $w=1.40345e-08 $l=4.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.135 $Y=0.0795 $X2=0.135 $Y2=0.036
r12 15 16 1.96775 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.1215 $Y=0.036 $X2=0.135 $Y2=0.036
r13 14 15 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.108
+ $Y=0.036 $X2=0.1215 $Y2=0.036
r14 9 14 2.68168 $w=1.3e-08 $l=1.15e-08 $layer=M1 $thickness=3.6e-08 $X=0.0965
+ $Y=0.036 $X2=0.108 $Y2=0.036
r15 1 14 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.108 $Y=0.0405 $X2=0.108
+ $Y2=0.036
r16 7 1 0.462963 $w=2.7e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.0935 $Y=0.0405 $X2=0.108 $Y2=0.0405
r17 13 7 0.0925926 $w=2.7e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.0405 $X2=0.0935 $Y2=0.0405
.ends


* End of included file INVxp33_ASAP7_75t_R.pex.sp.pex



*
.subckt INVxp33_ASAP7_75t_CUSTOM_4_25_0_7 VSS VDD A Y
*
* VSS VSS
* VDD VDD
* A A
* Y Y
*
*

M0 N_M0_d N_M0_g VSS VSS nmos_rvt L=2e-08 W=2.7e-08 nfin=1 $X=0.071 $Y=0.027
M1 N_M1_d N_M1_g VDD VDD pmos_rvt L=2e-08 W=2.7e-08 nfin=1 $X=0.071 $Y=0.216


* .include "INVxp33_ASAP7_75t_R.pex.sp.pxi"

* Start of included file INVxp33_ASAP7_75t_R.pex.sp.pxi
x_PM_INVxp33_ASAP7_75t_R%A vss A N_M0_g N_M1_g N_A_3 N_A_7
+ PM_INVxp33_ASAP7_75t_R%A
x_PM_INVxp33_ASAP7_75t_R%Y vss Y N_M0_d N_M1_d N_Y_8 N_Y_11 N_Y_7
+ PM_INVxp33_ASAP7_75t_R%Y
cc_1 N_Y_8 N_A_3 0.00650245f $X=0.0935 $Y=0.2295
cc_2 N_Y_11 N_A_7 0.00300733f $X=0.135 $Y=0.0795
cc_3 N_Y_7 N_A_3 0.0290551f $X=0.0935 $Y=0.0405


* End of included file INVxp33_ASAP7_75t_R.pex.sp.pxi
.ends
