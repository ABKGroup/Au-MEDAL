.subckt PM_AOI21xp5_ASAP7_75t_R%A2 vss 21 8 14 3
c1 1 vss 0.00722719f $X=0.081 $Y=0.135
c2 3 vss 0.14069f $X=0.081 $Y=0.0245
c3 4 vss 0.000955649f $X=0.027 $Y=0.13
c4 5 vss 0.00855306f $X=0.027 $Y=0.0975
c5 6 vss 0.00568288f $X=0.027 $Y=0.1715
c6 7 vss 0.00295131f $X=0.056 $Y=0.134
r1 6 22 7.00306 $w=1.35469e-08 $l=3.2e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.1715 $X2=0.027 $Y2=0.1395
r2 21 22 0.408178 $w=1.8e-08 $l=4e-09 $layer=M1 $thickness=3.6e-08 $X=0.0305
+ $Y=0.1355 $X2=0.027 $Y2=0.1395
r3 21 4 0.561244 $w=1.8e-08 $l=5.5e-09 $layer=M1 $thickness=3.6e-08 $X=0.0305
+ $Y=0.1355 $X2=0.027 $Y2=0.13
r4 4 5 6.92294 $w=1.37692e-08 $l=3.25e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.13 $X2=0.027 $Y2=0.0975
r5 21 20 0.517402 $w=3.18182e-09 $l=1.11018e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.0305 $Y=0.1355 $X2=0.038 $Y2=0.134
r6 7 18 5.82974 $w=1.3e-08 $l=2.5e-08 $layer=M1 $thickness=3.6e-08 $X=0.056
+ $Y=0.134 $X2=0.081 $Y2=0.134
r7 7 20 4.19742 $w=1.3e-08 $l=1.8e-08 $layer=M1 $thickness=3.6e-08 $X=0.056
+ $Y=0.134 $X2=0.038 $Y2=0.134
r8 1 11 6.54019 $w=2.09524e-08 $layer=Gate_1 $thickness=5.21905e-08 $X=0.081
+ $Y=0.135 $X2=0.081 $Y2=0.135
r9 1 18 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.134
r10 14 13 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.2025 $X2=0.081 $Y2=0.1595
r11 12 13 2.7211 $w=2.1e-08 $l=8e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.1515 $X2=0.081 $Y2=0.1595
r12 11 12 5.79936 $w=2.03333e-08 $l=1.65e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.1515
r13 10 11 5.79936 $w=2.03333e-08 $l=1.65e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.1185 $X2=0.081 $Y2=0.135
r14 9 10 2.7211 $w=2.1e-08 $l=8e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.1105 $X2=0.081 $Y2=0.1185
r15 8 9 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0675 $X2=0.081 $Y2=0.1105
r16 8 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0675 $X2=0.081 $Y2=0.0245
.ends

.subckt PM_AOI21xp5_ASAP7_75t_R%Y vss 31 16 19 38 1 7 9 10 14
c1 1 vss 0.00729223f $X=0.162 $Y=0.054
c2 2 vss 0.00862403f $X=0.216 $Y=0.2025
c3 7 vss 0.00278844f $X=0.1475 $Y=0.0725
c4 8 vss 0.000137556f $X=0.1475 $Y=0.0455
c5 9 vss 0.000299902f $X=0.1475 $Y=0.0945
c6 10 vss 0.0367431f $X=0.2015 $Y=0.2025
c7 11 vss 0.0059912f $X=0.243 $Y=0.234
c8 12 vss 0.00290119f $X=0.243 $Y=0.036
c9 13 vss 0.0089258f $X=0.1485 $Y=0.036
c10 14 vss 0.00579348f $X=0.243 $Y=0.0575
r1 10 2 0.154321 $w=8.1e-08 $l=1.25e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.2015 $Y=0.2025 $X2=0.216 $Y2=0.2025
r2 38 10 0.0308642 $w=8.1e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.1986 $Y=0.2025 $X2=0.2015 $Y2=0.2025
r3 2 35 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.216 $Y=0.2025 $X2=0.216 $Y2=0.234
r4 35 36 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.216
+ $Y=0.234 $X2=0.2295 $Y2=0.234
r5 11 33 3.83327 $w=1.5093e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.243 $Y=0.234 $X2=0.243 $Y2=0.2125
r6 11 36 1.96775 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.243 $Y=0.234 $X2=0.2295 $Y2=0.234
r7 32 33 13.2335 $w=1.3e-08 $l=5.68e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.1557 $X2=0.243 $Y2=0.2125
r8 31 32 10.4352 $w=1.3e-08 $l=4.47e-08 $layer=M1 $thickness=3.6e-08 $X=0.245
+ $Y=0.111 $X2=0.243 $Y2=0.1557
r9 31 30 4.83869 $w=1.3e-08 $l=2.08e-08 $layer=M1 $thickness=3.6e-08 $X=0.245
+ $Y=0.111 $X2=0.243 $Y2=0.0902
r10 14 30 7.63696 $w=1.3e-08 $l=3.27e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.0575 $X2=0.243 $Y2=0.0902
r11 12 29 4.18063 $w=1.48e-08 $l=2.5e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.036 $X2=0.218 $Y2=0.036
r12 12 14 3.36447 $w=1.71023e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.243 $Y=0.036 $X2=0.243 $Y2=0.0575
r13 28 29 4.19742 $w=1.3e-08 $l=1.8e-08 $layer=M1 $thickness=3.6e-08 $X=0.2
+ $Y=0.036 $X2=0.218 $Y2=0.036
r14 27 28 2.56509 $w=1.3e-08 $l=1.1e-08 $layer=M1 $thickness=3.6e-08 $X=0.189
+ $Y=0.036 $X2=0.2 $Y2=0.036
r15 26 27 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1755
+ $Y=0.036 $X2=0.189 $Y2=0.036
r16 24 26 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.162
+ $Y=0.036 $X2=0.1755 $Y2=0.036
r17 13 24 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1485
+ $Y=0.036 $X2=0.162 $Y2=0.036
r18 1 24 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.162 $Y=0.054 $X2=0.162 $Y2=0.036
r19 9 1 0.462963 $w=2.7e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1475 $Y=0.0945 $X2=0.162 $Y2=0.054
r20 8 1 0.245304 $w=3.7e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1475 $Y=0.0455 $X2=0.162 $Y2=0.054
r21 19 18 0.147059 $w=1.7e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.054 $X2=0.1765 $Y2=0.0725
r22 17 18 0.264706 $w=1.7e-08 $l=4.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.172 $Y=0.0725 $X2=0.1765 $Y2=0.0725
r23 1 17 0.705882 $w=1.7e-08 $l=1.2e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.162 $Y=0.054 $X2=0.172 $Y2=0.0725
r24 7 1 0.735294 $w=1.7e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1475 $Y=0.0725 $X2=0.162 $Y2=0.054
r25 16 7 0.147059 $w=1.7e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.0675 $X2=0.1475 $Y2=0.0725
.ends

.subckt PM_AOI21xp5_ASAP7_75t_R%A1 vss 4 5 9 3
c1 1 vss 0.0128094f $X=0.135 $Y=0.135
c2 3 vss 0.0787319f $X=0.135 $Y=0.0245
c3 4 vss 0.0110098f $X=0.1355 $Y=0.1355
r1 1 7 6.54019 $w=2.09524e-08 $layer=Gate_1 $thickness=5.21905e-08 $X=0.135
+ $Y=0.135 $X2=0.135 $Y2=0.135
r2 4 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.1355 $Y=0.1355 $X2=0.135 $Y2=0.135
r3 9 8 16.4967 $w=2.1e-08 $l=4.85e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.2025 $X2=0.135 $Y2=0.154
r4 7 8 6.6497 $w=2.04211e-08 $l=1.9e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.135 $X2=0.135 $Y2=0.154
r5 6 7 6.6497 $w=2.04211e-08 $l=1.9e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.116 $X2=0.135 $Y2=0.135
r6 5 6 16.4967 $w=2.1e-08 $l=4.85e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.0675 $X2=0.135 $Y2=0.116
r7 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.0675 $X2=0.135 $Y2=0.0245
.ends

.subckt PM_AOI21xp5_ASAP7_75t_R%B vss 15 5 11 3 4 1
c1 1 vss 0.00350672f $X=0.189 $Y=0.135
c2 3 vss 0.082885f $X=0.189 $Y=0.0245
c3 4 vss 0.00966703f $X=0.189 $Y=0.0975
r1 15 4 8.51143 $w=1.3e-08 $l=3.65e-08 $layer=M1 $thickness=3.6e-08 $X=0.1865
+ $Y=0.1355 $X2=0.189 $Y2=0.0975
r2 1 8 6.54019 $w=2.09524e-08 $layer=Gate_1 $thickness=5.21905e-08 $X=0.189
+ $Y=0.135 $X2=0.189 $Y2=0.135
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.1865 $Y=0.1355 $X2=0.189 $Y2=0.135
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.2025 $X2=0.189 $Y2=0.1595
r5 9 10 2.7211 $w=2.1e-08 $l=8e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.1515 $X2=0.189 $Y2=0.1595
r6 8 9 5.79936 $w=2.03333e-08 $l=1.65e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.135 $X2=0.189 $Y2=0.1515
r7 7 8 5.79936 $w=2.03333e-08 $l=1.65e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.1185 $X2=0.189 $Y2=0.135
r8 6 7 7.31297 $w=2.1e-08 $l=2.15e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.097 $X2=0.189 $Y2=0.1185
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.054 $X2=0.189 $Y2=0.097
r10 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.054 $X2=0.189 $Y2=0.0245
.ends


* End of included file AOI21xp5_ASAP7_75t_R.pex.sp.pex



*
.subckt AOI21xp5_ASAP7_75t_CUSTOM_4_25_0_7 VSS VDD A2 A1 B Y
*
* VSS VSS
* VDD VDD
* A2 A2
* A1 A1
* B B
* Y Y
*
*

M0 noxref_8 N_M0_g VSS VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.071 $Y=0.027
M1 N_M1_d N_M1_g noxref_8 VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.125
+ $Y=0.027
M2 VSS N_M2_g N_M2_s VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.179 $Y=0.027
M3 VDD N_M3_g noxref_6 VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.071 $Y=0.162
M4 noxref_6 N_M4_g VDD VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.125 $Y=0.162
M5 N_M5_d N_M5_g noxref_6 VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.179
+ $Y=0.162


* .include "AOI21xp5_ASAP7_75t_R.pex.sp.pxi"

* Start of included file AOI21xp5_ASAP7_75t_R.pex.sp.pxi
x_PM_AOI21xp5_ASAP7_75t_R%A2 vss A2 N_M0_g N_M3_g N_A2_3
+ PM_AOI21xp5_ASAP7_75t_R%A2
x_PM_AOI21xp5_ASAP7_75t_R%Y vss Y N_M1_d N_M2_s N_M5_d N_Y_1 N_Y_7 N_Y_9 N_Y_10
+ N_Y_14 PM_AOI21xp5_ASAP7_75t_R%Y
cc_1 N_Y_1 A1 0.00146781f $X=0.162 $Y=0.054
cc_2 N_Y_7 N_A1_3 0.00953801f $X=0.1475 $Y=0.0725
cc_3 N_Y_9 N_A1_3 0.0325944f $X=0.1475 $Y=0.0945
cc_4 N_Y_1 N_B_3 0.00124591f $X=0.162 $Y=0.054
cc_5 N_Y_10 N_B_1 0.00170997f $X=0.2015 $Y=0.2025
cc_6 N_Y_9 N_B_3 0.00494927f $X=0.1475 $Y=0.0945
cc_7 N_Y_14 N_B_4 0.00732431f $X=0.243 $Y=0.0575
cc_8 N_Y_7 N_B_3 0.00983134f $X=0.1475 $Y=0.0725
cc_9 N_Y_10 N_B_3 0.0570856f $X=0.2015 $Y=0.2025
x_PM_AOI21xp5_ASAP7_75t_R%A1 vss A1 N_M1_g N_M4_g N_A1_3
+ PM_AOI21xp5_ASAP7_75t_R%A1
cc_10 A1 N_A2_3 0.00271048f $X=0.1355 $Y=0.1355
cc_11 N_A1_3 N_A2_3 0.0120878f $X=0.135 $Y=0.0245
x_PM_AOI21xp5_ASAP7_75t_R%B vss B N_M2_g N_M5_g N_B_3 N_B_4 N_B_1
+ PM_AOI21xp5_ASAP7_75t_R%B
cc_12 N_B_3 N_A1_3 0.00312559f $X=0.189 $Y=0.0245
cc_13 N_B_4 A1 0.00627518f $X=0.189 $Y=0.0975


* End of included file AOI21xp5_ASAP7_75t_R.pex.sp.pxi
.ends
