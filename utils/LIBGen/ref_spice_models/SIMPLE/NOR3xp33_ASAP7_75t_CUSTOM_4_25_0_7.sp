.subckt PM_NOR3xp33_ASAP7_75t_R%8 vss 2 3 1
c1 1 vss 0.000777245f $X=0.162 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.2025 $X2=0.162 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.2025 $X2=0.162 $Y2=0.2025
.ends

.subckt PM_NOR3xp33_ASAP7_75t_R%A vss 15 5 11 3 4 1
c1 1 vss 0.00210755f $X=0.081 $Y=0.135
c2 3 vss 0.042102f $X=0.081 $Y=0.0245
c3 4 vss 0.0121144f $X=0.081 $Y=0.098
r1 15 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.083
+ $Y=0.133 $X2=0.081 $Y2=0.098
r2 1 8 6.74717 $w=2e-08 $layer=Gate_1 $thickness=5.2e-08 $X=0.081 $Y=0.135
+ $X2=0.081 $Y2=0.135
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.083 $Y=0.133 $X2=0.081 $Y2=0.135
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.2025 $X2=0.081 $Y2=0.1595
r5 9 10 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.151 $X2=0.081 $Y2=0.1595
r6 8 9 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.151
r7 7 8 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.119 $X2=0.081 $Y2=0.135
r8 6 7 7.48304 $w=2.1e-08 $l=2.2e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.097 $X2=0.081 $Y2=0.119
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.054 $X2=0.081 $Y2=0.097
r10 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.054 $X2=0.081 $Y2=0.0245
.ends

.subckt PM_NOR3xp33_ASAP7_75t_R%C vss 15 5 11 3 4
c1 1 vss 0.00485572f $X=0.189 $Y=0.135
c2 3 vss 0.0809244f $X=0.189 $Y=0.0245
c3 4 vss 0.0172137f $X=0.189 $Y=0.098
r1 15 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.191
+ $Y=0.133 $X2=0.189 $Y2=0.098
r2 1 8 6.74717 $w=2e-08 $layer=Gate_1 $thickness=5.2e-08 $X=0.189 $Y=0.135
+ $X2=0.189 $Y2=0.135
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.191 $Y=0.133 $X2=0.189 $Y2=0.135
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.2025 $X2=0.189 $Y2=0.1595
r5 9 10 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.151 $X2=0.189 $Y2=0.1595
r6 8 9 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.135 $X2=0.189 $Y2=0.151
r7 7 8 5.61228 $w=2.0375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.119 $X2=0.189 $Y2=0.135
r8 6 7 7.48304 $w=2.1e-08 $l=2.2e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.097 $X2=0.189 $Y2=0.119
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.054 $X2=0.189 $Y2=0.097
r10 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.054 $X2=0.189 $Y2=0.0245
.ends

.subckt PM_NOR3xp33_ASAP7_75t_R%Y vss 29 18 34 35 40 12 2 10 15 11
c1 2 vss 0.00659568f $X=0.054 $Y=0.2025
c2 3 vss 0.00772786f $X=0.162 $Y=0.054
c3 10 vss 0.035141f $X=0.056 $Y=0.054
c4 11 vss 0.00369759f $X=0.1475 $Y=0.054
c5 12 vss 0.0369241f $X=0.056 $Y=0.2025
c6 13 vss 0.00264596f $X=0.027 $Y=0.036
c7 14 vss 0.00561768f $X=0.027 $Y=0.234
c8 15 vss 0.00421974f $X=0.027 $Y=0.0575
c9 16 vss 0.0123181f $X=0.0405 $Y=0.036
r1 40 39 0.0308642 $w=8.1e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.2025 $X2=0.0685 $Y2=0.2025
r2 12 39 0.154321 $w=8.1e-08 $l=1.25e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.2025 $X2=0.0685 $Y2=0.2025
r3 2 37 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.054 $Y=0.2025 $X2=0.054 $Y2=0.234
r4 36 37 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.0405
+ $Y=0.234 $X2=0.054 $Y2=0.234
r5 14 31 3.83327 $w=1.5093e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.234 $X2=0.027 $Y2=0.2125
r6 14 36 1.96775 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.234 $X2=0.0405 $Y2=0.234
r7 35 33 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.054 $X2=0.1765 $Y2=0.054
r8 3 33 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.162 $Y=0.054 $X2=0.1765 $Y2=0.054
r9 11 3 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1475 $Y=0.054 $X2=0.162 $Y2=0.054
r10 34 11 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.054 $X2=0.1475 $Y2=0.054
r11 30 31 11.368 $w=1.3e-08 $l=4.88e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.1637 $X2=0.027 $Y2=0.2125
r12 29 30 8.56972 $w=1.3e-08 $l=3.67e-08 $layer=M1 $thickness=3.6e-08 $X=0.028
+ $Y=0.127 $X2=0.027 $Y2=0.1637
r13 29 28 6.70421 $w=1.3e-08 $l=2.88e-08 $layer=M1 $thickness=3.6e-08 $X=0.028
+ $Y=0.127 $X2=0.027 $Y2=0.0982
r14 15 28 9.50248 $w=1.3e-08 $l=4.07e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.0575 $X2=0.027 $Y2=0.0982
r15 3 26 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.162 $Y=0.054 $X2=0.162 $Y2=0.036
r16 13 16 1.49895 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.036 $X2=0.0405 $Y2=0.036
r17 13 15 3.36447 $w=1.71023e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.036 $X2=0.027 $Y2=0.0575
r18 25 26 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1485
+ $Y=0.036 $X2=0.162 $Y2=0.036
r19 24 25 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.036 $X2=0.1485 $Y2=0.036
r20 23 24 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.108
+ $Y=0.036 $X2=0.135 $Y2=0.036
r21 22 23 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.036 $X2=0.108 $Y2=0.036
r22 21 22 2.56509 $w=1.3e-08 $l=1.1e-08 $layer=M1 $thickness=3.6e-08 $X=0.07
+ $Y=0.036 $X2=0.081 $Y2=0.036
r23 20 21 1.04935 $w=1.3e-08 $l=4.5e-09 $layer=M1 $thickness=3.6e-08 $X=0.0655
+ $Y=0.036 $X2=0.07 $Y2=0.036
r24 19 20 2.68168 $w=1.3e-08 $l=1.15e-08 $layer=M1 $thickness=3.6e-08 $X=0.054
+ $Y=0.036 $X2=0.0655 $Y2=0.036
r25 16 19 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.0405
+ $Y=0.036 $X2=0.054 $Y2=0.036
r26 10 19 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.056 $Y=0.054 $X2=0.054
+ $Y2=0.036
r27 18 17 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.054 $X2=0.0685 $Y2=0.054
r28 10 17 0.231482 $w=5.4e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.054 $X2=0.0685 $Y2=0.054
r29 2 12 1e-05 $l=2e-09 $X=0.054 $Y=0.2025 $X2=0.056 $Y2=0.2025
.ends

.subckt PM_NOR3xp33_ASAP7_75t_R%7 vss 2 3 1
c1 1 vss 0.000743083f $X=0.108 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.2025 $X2=0.108 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1253 $Y=0.2025 $X2=0.108 $Y2=0.2025
.ends

.subckt PM_NOR3xp33_ASAP7_75t_R%B vss 13 5 9 1 4 3
c1 1 vss 0.00183906f $X=0.135 $Y=0.135
c2 3 vss 0.037482f $X=0.135 $Y=0.0245
c3 4 vss 0.013032f $X=0.135 $Y=0.098
r1 13 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.136
+ $Y=0.133 $X2=0.135 $Y2=0.098
r2 1 7 6.74717 $w=2e-08 $layer=Gate_1 $thickness=5.2e-08 $X=0.135 $Y=0.135
+ $X2=0.135 $Y2=0.135
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.136 $Y=0.133 $X2=0.135 $Y2=0.135
r4 9 8 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.2025 $X2=0.135 $Y2=0.1535
r5 7 8 6.46262 $w=2.04595e-08 $l=1.85e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.135 $X2=0.135 $Y2=0.1535
r6 6 7 11.0545 $w=2.06875e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.103 $X2=0.135 $Y2=0.135
r7 5 6 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.054 $X2=0.135 $Y2=0.103
r8 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.054 $X2=0.135 $Y2=0.0245
.ends


* End of included file NOR3xp33_ASAP7_75t_R.pex.sp.pex



*
.subckt NOR3xp33_ASAP7_75t_CUSTOM_4_25_0_7 VSS VDD A B C Y
*
* VSS VSS
* VDD VDD
* A A
* B B
* C C
* Y Y
*
*

M0 VSS N_M0_g N_M0_s VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.071 $Y=0.027
M1 N_M1_d N_M1_g VSS VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.125 $Y=0.027
M2 VSS N_M2_g N_M2_s VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.179 $Y=0.027
M3 N_M3_d N_M3_g N_M3_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.071 $Y=0.162
M4 N_M4_d N_M4_g N_M4_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.125 $Y=0.162
M5 VDD N_M5_g N_M5_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.179 $Y=0.162


* .include "NOR3xp33_ASAP7_75t_R.pex.sp.pxi"

* Start of included file NOR3xp33_ASAP7_75t_R.pex.sp.pxi
x_PM_NOR3xp33_ASAP7_75t_R%8 vss N_M4_d N_M5_s N_8_1 PM_NOR3xp33_ASAP7_75t_R%8
cc_1 N_8_1 N_B_3 0.0170299f $X=0.162 $Y=0.2025
cc_2 N_8_1 N_C_3 0.0168488f $X=0.162 $Y=0.2025
x_PM_NOR3xp33_ASAP7_75t_R%A vss A N_M0_g N_M3_g N_A_3 N_A_4 N_A_1
+ PM_NOR3xp33_ASAP7_75t_R%A
x_PM_NOR3xp33_ASAP7_75t_R%C vss C N_M2_g N_M5_g N_C_3 N_C_4
+ PM_NOR3xp33_ASAP7_75t_R%C
cc_3 N_C_3 N_B_1 0.00149744f $X=0.189 $Y=0.0245
cc_4 N_C_4 N_B_4 0.00641724f $X=0.189 $Y=0.098
cc_5 N_C_3 N_B_3 0.00809067f $X=0.189 $Y=0.0245
x_PM_NOR3xp33_ASAP7_75t_R%Y vss Y N_M0_s N_M1_d N_M2_s N_M3_s N_Y_12 N_Y_2
+ N_Y_10 N_Y_15 N_Y_11 PM_NOR3xp33_ASAP7_75t_R%Y
cc_6 N_Y_12 N_A_1 0.00216167f $X=0.056 $Y=0.2025
cc_7 N_Y_12 N_A_4 0.00100599f $X=0.056 $Y=0.2025
cc_8 N_Y_2 N_A_3 0.00159856f $X=0.054 $Y=0.2025
cc_9 N_Y_10 N_A_3 0.0106374f $X=0.056 $Y=0.054
cc_10 N_Y_15 N_A_4 0.00770312f $X=0.027 $Y=0.0575
cc_11 N_Y_12 N_A_3 0.051724f $X=0.056 $Y=0.2025
cc_12 N_Y_11 N_B_1 0.000358233f $X=0.1475 $Y=0.054
cc_13 N_Y_11 N_B_4 0.0010456f $X=0.1475 $Y=0.054
cc_14 N_Y_2 N_B_4 0.00210955f $X=0.054 $Y=0.2025
cc_15 N_Y_11 N_B_3 0.0269856f $X=0.1475 $Y=0.054
cc_16 N_Y_11 N_C_4 0.00118972f $X=0.1475 $Y=0.054
cc_17 N_Y_11 N_C_3 0.0274353f $X=0.1475 $Y=0.054
x_PM_NOR3xp33_ASAP7_75t_R%7 vss N_M3_d N_M4_s N_7_1 PM_NOR3xp33_ASAP7_75t_R%7
cc_18 N_7_1 N_A_3 0.0168197f $X=0.108 $Y=0.2025
cc_19 N_7_1 N_B_3 0.0169849f $X=0.108 $Y=0.2025
x_PM_NOR3xp33_ASAP7_75t_R%B vss B N_M1_g N_M4_g N_B_1 N_B_4 N_B_3
+ PM_NOR3xp33_ASAP7_75t_R%B
cc_20 N_B_1 N_A_3 0.00148357f $X=0.135 $Y=0.135
cc_21 N_B_4 N_A_4 0.00533665f $X=0.135 $Y=0.098
cc_22 N_B_3 N_A_3 0.00799711f $X=0.135 $Y=0.0245


* End of included file NOR3xp33_ASAP7_75t_R.pex.sp.pxi
.ends
