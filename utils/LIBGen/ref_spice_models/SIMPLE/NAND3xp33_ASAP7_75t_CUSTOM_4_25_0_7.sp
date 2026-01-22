.subckt PM_NAND3xp33_ASAP7_75t_R%8 vss 2 3 1
c1 1 vss 0.000775373f $X=0.162 $Y=0.0675
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.0675 $X2=0.162 $Y2=0.0675
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.0675 $X2=0.162 $Y2=0.0675
.ends

.subckt PM_NAND3xp33_ASAP7_75t_R%B vss 13 5 9 1 4 3
c1 1 vss 0.00182208f $X=0.135 $Y=0.135
c2 3 vss 0.0374584f $X=0.135 $Y=0.0245
c3 4 vss 0.0129561f $X=0.135 $Y=0.098
r1 13 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.136
+ $Y=0.137 $X2=0.135 $Y2=0.098
r2 1 7 6.74717 $w=2e-08 $layer=Gate_1 $thickness=5.2e-08 $X=0.135 $Y=0.135
+ $X2=0.135 $Y2=0.135
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.136 $Y=0.137 $X2=0.135 $Y2=0.135
r4 9 8 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.216 $X2=0.135 $Y2=0.167
r5 7 8 11.0545 $w=2.06875e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.135 $X2=0.135 $Y2=0.167
r6 6 7 6.46262 $w=2.04595e-08 $l=1.85e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.1165 $X2=0.135 $Y2=0.135
r7 5 6 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.0675 $X2=0.135 $Y2=0.1165
r8 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.0675 $X2=0.135 $Y2=0.0245
.ends

.subckt PM_NAND3xp33_ASAP7_75t_R%C vss 15 5 11 3 4
c1 1 vss 0.00498004f $X=0.189 $Y=0.135
c2 3 vss 0.080835f $X=0.189 $Y=0.0245
c3 4 vss 0.0171503f $X=0.189 $Y=0.08
r1 15 4 12.8254 $w=1.3e-08 $l=5.5e-08 $layer=M1 $thickness=3.6e-08 $X=0.191
+ $Y=0.137 $X2=0.189 $Y2=0.08
r2 1 8 6.74717 $w=2e-08 $layer=Gate_1 $thickness=5.2e-08 $X=0.189 $Y=0.135
+ $X2=0.189 $Y2=0.135
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.191 $Y=0.137 $X2=0.189 $Y2=0.135
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.216 $X2=0.189 $Y2=0.173
r5 9 10 7.48304 $w=2.1e-08 $l=2.2e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.151 $X2=0.189 $Y2=0.173
r6 8 9 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.135 $X2=0.189 $Y2=0.151
r7 7 8 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.119 $X2=0.189 $Y2=0.135
r8 6 7 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.1105 $X2=0.189 $Y2=0.119
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.0675 $X2=0.189 $Y2=0.1105
r10 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.0675 $X2=0.189 $Y2=0.0245
.ends

.subckt PM_NAND3xp33_ASAP7_75t_R%Y vss 23 18 36 39 40 10 1 11 15 12
c1 1 vss 0.00660267f $X=0.054 $Y=0.0675
c2 3 vss 0.00773604f $X=0.162 $Y=0.216
c3 10 vss 0.0371275f $X=0.056 $Y=0.0675
c4 11 vss 0.0351504f $X=0.056 $Y=0.216
c5 12 vss 0.00369905f $X=0.1475 $Y=0.216
c6 13 vss 0.00562215f $X=0.0405 $Y=0.036
c7 14 vss 0.00264861f $X=0.027 $Y=0.234
c8 15 vss 0.00419091f $X=0.027 $Y=0.0575
c9 16 vss 0.0123305f $X=0.0405 $Y=0.234
r1 40 38 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.216 $X2=0.1765 $Y2=0.216
r2 3 38 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.162 $Y=0.216 $X2=0.1765 $Y2=0.216
r3 12 3 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1475 $Y=0.216 $X2=0.162 $Y2=0.216
r4 39 12 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.216 $X2=0.1475 $Y2=0.216
r5 36 35 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.216 $X2=0.0685 $Y2=0.216
r6 11 35 0.231482 $w=5.4e-08 $l=1.25e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.216 $X2=0.0685 $Y2=0.216
r7 3 33 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.162 $Y=0.216 $X2=0.162 $Y2=0.234
r8 11 26 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.056 $Y=0.216 $X2=0.054 $Y2=0.234
r9 32 33 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1485
+ $Y=0.234 $X2=0.162 $Y2=0.234
r10 31 32 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.234 $X2=0.1485 $Y2=0.234
r11 30 31 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.108
+ $Y=0.234 $X2=0.135 $Y2=0.234
r12 29 30 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.234 $X2=0.108 $Y2=0.234
r13 28 29 2.56509 $w=1.3e-08 $l=1.1e-08 $layer=M1 $thickness=3.6e-08 $X=0.07
+ $Y=0.234 $X2=0.081 $Y2=0.234
r14 27 28 1.04935 $w=1.3e-08 $l=4.5e-09 $layer=M1 $thickness=3.6e-08 $X=0.0655
+ $Y=0.234 $X2=0.07 $Y2=0.234
r15 26 27 2.68168 $w=1.3e-08 $l=1.15e-08 $layer=M1 $thickness=3.6e-08 $X=0.054
+ $Y=0.234 $X2=0.0655 $Y2=0.234
r16 16 26 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.0405
+ $Y=0.234 $X2=0.054 $Y2=0.234
r17 14 25 3.36447 $w=1.71023e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.234 $X2=0.027 $Y2=0.2125
r18 14 16 1.49895 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.234 $X2=0.0405 $Y2=0.234
r19 24 25 9.50248 $w=1.3e-08 $l=4.08e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.1717 $X2=0.027 $Y2=0.2125
r20 23 24 6.70421 $w=1.3e-08 $l=2.87e-08 $layer=M1 $thickness=3.6e-08 $X=0.028
+ $Y=0.143 $X2=0.027 $Y2=0.1717
r21 23 22 8.56972 $w=1.3e-08 $l=3.68e-08 $layer=M1 $thickness=3.6e-08 $X=0.028
+ $Y=0.143 $X2=0.027 $Y2=0.1062
r22 15 21 3.83327 $w=1.5093e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.0575 $X2=0.027 $Y2=0.036
r23 15 22 11.368 $w=1.3e-08 $l=4.87e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.0575 $X2=0.027 $Y2=0.1062
r24 13 19 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.0405
+ $Y=0.036 $X2=0.054 $Y2=0.036
r25 13 21 1.96775 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.0405 $Y=0.036 $X2=0.027 $Y2=0.036
r26 1 19 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.054 $Y=0.0675 $X2=0.054
+ $Y2=0.036
r27 18 17 0.0308642 $w=8.1e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.0675 $X2=0.0685 $Y2=0.0675
r28 10 17 0.154321 $w=8.1e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.0675 $X2=0.0685 $Y2=0.0675
r29 1 10 1e-05 $l=2e-09 $X=0.054 $Y=0.0675 $X2=0.056 $Y2=0.0675
.ends

.subckt PM_NAND3xp33_ASAP7_75t_R%7 vss 2 3 1
c1 1 vss 0.000728757f $X=0.108 $Y=0.0675
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.0675 $X2=0.108 $Y2=0.0675
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1253 $Y=0.0675 $X2=0.108 $Y2=0.0675
.ends

.subckt PM_NAND3xp33_ASAP7_75t_R%A vss 15 5 11 3 4 1
c1 1 vss 0.00204702f $X=0.081 $Y=0.135
c2 3 vss 0.0421048f $X=0.081 $Y=0.0245
c3 4 vss 0.0118747f $X=0.081 $Y=0.098
r1 15 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.083
+ $Y=0.137 $X2=0.081 $Y2=0.098
r2 1 8 6.74717 $w=2e-08 $layer=Gate_1 $thickness=5.2e-08 $X=0.081 $Y=0.135
+ $X2=0.081 $Y2=0.135
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.083 $Y=0.137 $X2=0.081 $Y2=0.135
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.216 $X2=0.081 $Y2=0.173
r5 9 10 7.48304 $w=2.1e-08 $l=2.2e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.151 $X2=0.081 $Y2=0.173
r6 8 9 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.151
r7 7 8 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.119 $X2=0.081 $Y2=0.135
r8 6 7 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.1105 $X2=0.081 $Y2=0.119
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0675 $X2=0.081 $Y2=0.1105
r10 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0675 $X2=0.081 $Y2=0.0245
.ends


* End of included file NAND3xp33_ASAP7_75t_R.pex.sp.pex



*
.subckt NAND3xp33_ASAP7_75t_CUSTOM_4_25_0_7 VSS VDD A B C Y
*
* VSS VSS
* VDD VDD
* A A
* B B
* C C
* Y Y
*
*

M0 N_M0_d N_M0_g N_M0_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.071 $Y=0.027
M1 N_M1_d N_M1_g N_M1_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.125 $Y=0.027
M2 VSS N_M2_g N_M2_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.179 $Y=0.027
M3 VDD N_M3_g N_M3_s VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.071 $Y=0.189
M4 N_M4_d N_M4_g VDD VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.125 $Y=0.189
M5 VDD N_M5_g N_M5_s VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.179 $Y=0.189


* .include "NAND3xp33_ASAP7_75t_R.pex.sp.pxi"

* Start of included file NAND3xp33_ASAP7_75t_R.pex.sp.pxi
x_PM_NAND3xp33_ASAP7_75t_R%8 vss N_M1_d N_M2_s N_8_1 PM_NAND3xp33_ASAP7_75t_R%8
cc_1 N_8_1 N_B_3 0.0170139f $X=0.162 $Y=0.0675
cc_2 N_8_1 N_C_3 0.0168337f $X=0.162 $Y=0.0675
x_PM_NAND3xp33_ASAP7_75t_R%B vss B N_M1_g N_M4_g N_B_1 N_B_4 N_B_3
+ PM_NAND3xp33_ASAP7_75t_R%B
cc_3 N_B_1 N_A_3 0.00150637f $X=0.135 $Y=0.135
cc_4 N_B_4 N_A_4 0.00532977f $X=0.135 $Y=0.098
cc_5 N_B_3 N_A_3 0.00800972f $X=0.135 $Y=0.0245
x_PM_NAND3xp33_ASAP7_75t_R%C vss C N_M2_g N_M5_g N_C_3 N_C_4
+ PM_NAND3xp33_ASAP7_75t_R%C
cc_6 N_C_3 N_B_1 0.00151739f $X=0.189 $Y=0.0245
cc_7 N_C_4 N_B_4 0.00641687f $X=0.189 $Y=0.08
cc_8 N_C_3 N_B_3 0.00807868f $X=0.189 $Y=0.0245
x_PM_NAND3xp33_ASAP7_75t_R%Y vss Y N_M0_s N_M3_s N_M4_d N_M5_s N_Y_10 N_Y_1
+ N_Y_11 N_Y_15 N_Y_12 PM_NAND3xp33_ASAP7_75t_R%Y
cc_9 N_Y_10 N_A_1 0.0021644f $X=0.056 $Y=0.0675
cc_10 N_Y_10 N_A_4 0.0010071f $X=0.056 $Y=0.0675
cc_11 N_Y_1 N_A_3 0.00159994f $X=0.054 $Y=0.0675
cc_12 N_Y_11 N_A_3 0.0106954f $X=0.056 $Y=0.216
cc_13 N_Y_15 N_A_4 0.00771436f $X=0.027 $Y=0.0575
cc_14 N_Y_10 N_A_3 0.0519277f $X=0.056 $Y=0.0675
cc_15 N_Y_12 N_B_1 0.000358961f $X=0.1475 $Y=0.216
cc_16 N_Y_12 N_B_4 0.00104655f $X=0.1475 $Y=0.216
cc_17 N_Y_1 N_B_4 0.00211156f $X=0.054 $Y=0.0675
cc_18 N_Y_12 N_B_3 0.0270177f $X=0.1475 $Y=0.216
cc_19 N_Y_12 N_C_4 0.00119079f $X=0.1475 $Y=0.216
cc_20 N_Y_12 N_C_3 0.0273801f $X=0.1475 $Y=0.216
x_PM_NAND3xp33_ASAP7_75t_R%7 vss N_M0_d N_M1_s N_7_1 PM_NAND3xp33_ASAP7_75t_R%7
cc_21 N_7_1 N_A_3 0.0168795f $X=0.108 $Y=0.0675
cc_22 N_7_1 N_B_3 0.0170475f $X=0.108 $Y=0.0675
x_PM_NAND3xp33_ASAP7_75t_R%A vss A N_M0_g N_M3_g N_A_3 N_A_4 N_A_1
+ PM_NAND3xp33_ASAP7_75t_R%A


* End of included file NAND3xp33_ASAP7_75t_R.pex.sp.pxi
.ends
