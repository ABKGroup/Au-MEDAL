.subckt PM_NOR4xp25_ASAP7_75t_R%8 vss 2 3 1
c1 1 vss 0.000786002f $X=0.108 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.2025 $X2=0.108 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1253 $Y=0.2025 $X2=0.108 $Y2=0.2025
.ends

.subckt PM_NOR4xp25_ASAP7_75t_R%10 vss 2 3 1
c1 1 vss 0.000727067f $X=0.216 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1986 $Y=0.2025 $X2=0.216 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.2333 $Y=0.2025 $X2=0.216 $Y2=0.2025
.ends

.subckt PM_NOR4xp25_ASAP7_75t_R%D vss 17 5 11 3 4
c1 1 vss 0.00451133f $X=0.0715 $Y=0.135
c2 3 vss 0.0792136f $X=0.081 $Y=0.0245
c3 4 vss 0.0156723f $X=0.081 $Y=0.098
r1 17 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.132 $X2=0.081 $Y2=0.098
r2 13 14 3.67195 $w=1.8e-08 $l=9.5e-09 $layer=LIG $thickness=4.8e-08 $X=0.081
+ $Y=0.135 $X2=0.0905 $Y2=0.135
r3 17 13 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.081 $Y=0.132 $X2=0.081 $Y2=0.135
r4 1 13 3.67195 $w=1.8e-08 $l=9.5e-09 $layer=LIG $thickness=4.8e-08 $X=0.0715
+ $Y=0.135 $X2=0.081 $Y2=0.135
r5 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.2025 $X2=0.081 $Y2=0.1595
r6 9 10 3.06124 $w=2.1e-08 $l=9e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.1505 $X2=0.081 $Y2=0.1595
r7 8 9 5.4252 $w=2.04194e-08 $l=1.55e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.1505
r8 8 14 3.40757 $w=1.98947e-08 $l=9.5e-09 $layer=Gate_1 $thickness=5.55789e-08
+ $X=0.081 $Y=0.135 $X2=0.0905 $Y2=0.135
r9 8 13 6.69299 $w=1.9e-08 $layer=LIG $thickness=5.2e-08 $X=0.081 $Y=0.135
+ $X2=0.081 $Y2=0.135
r10 7 8 5.4252 $w=2.04194e-08 $l=1.55e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.1195 $X2=0.081 $Y2=0.135
r11 6 7 7.65311 $w=2.1e-08 $l=2.25e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.097 $X2=0.081 $Y2=0.1195
r12 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.054 $X2=0.081 $Y2=0.097
r13 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.054 $X2=0.081 $Y2=0.0245
.ends

.subckt PM_NOR4xp25_ASAP7_75t_R%B vss 13 5 9 1 4 3
c1 1 vss 0.0013783f $X=0.189 $Y=0.135
c2 3 vss 0.0378198f $X=0.189 $Y=0.0245
c3 4 vss 0.0129233f $X=0.189 $Y=0.098
r1 13 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.189
+ $Y=0.132 $X2=0.189 $Y2=0.098
r2 1 7 6.74717 $w=2e-08 $layer=Gate_1 $thickness=5.2e-08 $X=0.189 $Y=0.135
+ $X2=0.189 $Y2=0.135
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.189 $Y=0.132 $X2=0.189 $Y2=0.135
r4 9 8 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.2025 $X2=0.189 $Y2=0.1535
r5 7 8 6.46262 $w=2.04595e-08 $l=1.85e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.135 $X2=0.189 $Y2=0.1535
r6 6 7 11.0545 $w=2.06875e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.103 $X2=0.189 $Y2=0.135
r7 5 6 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.054 $X2=0.189 $Y2=0.103
r8 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.054 $X2=0.189 $Y2=0.0245
.ends

.subckt PM_NOR4xp25_ASAP7_75t_R%Y vss 43 22 40 41 47 52 19 1 13 2 14 4 16 15 20
c1 1 vss 0.00769203f $X=0.054 $Y=0.054
c2 2 vss 0.00716797f $X=0.162 $Y=0.054
c3 3 vss 0.00714264f $X=0.27 $Y=0.054
c4 4 vss 0.00645259f $X=0.27 $Y=0.2025
c5 13 vss 0.0282577f $X=0.056 $Y=0.054
c6 14 vss 0.00366055f $X=0.1475 $Y=0.054
c7 15 vss 0.0282034f $X=0.2555 $Y=0.054
c8 16 vss 0.0371283f $X=0.2555 $Y=0.2025
c9 17 vss 0.0056223f $X=0.297 $Y=0.234
c10 18 vss 0.00268975f $X=0.297 $Y=0.036
c11 19 vss 0.0211954f $X=0.0425 $Y=0.036
c12 20 vss 0.00431429f $X=0.297 $Y=0.0575
r1 16 4 0.154321 $w=8.1e-08 $l=1.25e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.2555 $Y=0.2025 $X2=0.27 $Y2=0.2025
r2 52 16 0.0308642 $w=8.1e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.2526 $Y=0.2025 $X2=0.2555 $Y2=0.2025
r3 4 49 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.27 $Y=0.2025 $X2=0.27 $Y2=0.234
r4 49 50 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.27
+ $Y=0.234 $X2=0.2835 $Y2=0.234
r5 17 45 3.83327 $w=1.5093e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.297 $Y=0.234 $X2=0.297 $Y2=0.2125
r6 17 50 1.96775 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.297 $Y=0.234 $X2=0.2835 $Y2=0.234
r7 15 3 0.231482 $w=5.4e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.2555 $Y=0.054 $X2=0.27 $Y2=0.054
r8 47 15 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.2526 $Y=0.054 $X2=0.2555 $Y2=0.054
r9 44 45 11.7178 $w=1.3e-08 $l=5.03e-08 $layer=M1 $thickness=3.6e-08 $X=0.297
+ $Y=0.1622 $X2=0.297 $Y2=0.2125
r10 43 44 8.91951 $w=1.3e-08 $l=3.82e-08 $layer=M1 $thickness=3.6e-08 $X=0.297
+ $Y=0.124 $X2=0.297 $Y2=0.1622
r11 43 42 6.35442 $w=1.3e-08 $l=2.73e-08 $layer=M1 $thickness=3.6e-08 $X=0.297
+ $Y=0.124 $X2=0.297 $Y2=0.0967
r12 20 42 9.1527 $w=1.3e-08 $l=3.92e-08 $layer=M1 $thickness=3.6e-08 $X=0.297
+ $Y=0.0575 $X2=0.297 $Y2=0.0967
r13 41 39 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.054 $X2=0.1765 $Y2=0.054
r14 2 39 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.162 $Y=0.054 $X2=0.1765 $Y2=0.054
r15 14 2 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1475 $Y=0.054 $X2=0.162 $Y2=0.054
r16 40 14 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.054 $X2=0.1475 $Y2=0.054
r17 3 36 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.27 $Y=0.054 $X2=0.27 $Y2=0.036
r18 18 37 1.49895 $w=1.63333e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.297 $Y=0.036 $X2=0.2835 $Y2=0.036
r19 18 20 3.36447 $w=1.71023e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.297 $Y=0.036 $X2=0.297 $Y2=0.0575
r20 2 29 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.162 $Y=0.054 $X2=0.162 $Y2=0.036
r21 36 37 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.27
+ $Y=0.036 $X2=0.2835 $Y2=0.036
r22 35 36 2.68168 $w=1.3e-08 $l=1.15e-08 $layer=M1 $thickness=3.6e-08 $X=0.2585
+ $Y=0.036 $X2=0.27 $Y2=0.036
r23 34 35 1.04935 $w=1.3e-08 $l=4.5e-09 $layer=M1 $thickness=3.6e-08 $X=0.254
+ $Y=0.036 $X2=0.2585 $Y2=0.036
r24 33 34 2.56509 $w=1.3e-08 $l=1.1e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.036 $X2=0.254 $Y2=0.036
r25 32 33 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.216
+ $Y=0.036 $X2=0.243 $Y2=0.036
r26 31 32 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.189
+ $Y=0.036 $X2=0.216 $Y2=0.036
r27 30 31 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1755
+ $Y=0.036 $X2=0.189 $Y2=0.036
r28 29 30 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.162
+ $Y=0.036 $X2=0.1755 $Y2=0.036
r29 28 29 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1485
+ $Y=0.036 $X2=0.162 $Y2=0.036
r30 27 28 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.036 $X2=0.1485 $Y2=0.036
r31 26 27 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.108
+ $Y=0.036 $X2=0.135 $Y2=0.036
r32 25 26 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.036 $X2=0.108 $Y2=0.036
r33 24 25 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.0675
+ $Y=0.036 $X2=0.081 $Y2=0.036
r34 23 24 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.054
+ $Y=0.036 $X2=0.0675 $Y2=0.036
r35 19 23 2.68168 $w=1.3e-08 $l=1.15e-08 $layer=M1 $thickness=3.6e-08 $X=0.0425
+ $Y=0.036 $X2=0.054 $Y2=0.036
r36 1 23 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.054 $Y=0.054 $X2=0.054 $Y2=0.036
r37 22 21 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.054 $X2=0.0685 $Y2=0.054
r38 13 21 0.231482 $w=5.4e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.054 $X2=0.0685 $Y2=0.054
r39 1 13 1e-05 $l=2e-09 $X=0.054 $Y=0.054 $X2=0.056 $Y2=0.054
.ends

.subckt PM_NOR4xp25_ASAP7_75t_R%A vss 15 5 11 3 4 1
c1 1 vss 0.00188191f $X=0.243 $Y=0.135
c2 3 vss 0.0415995f $X=0.243 $Y=0.0245
c3 4 vss 0.0103956f $X=0.243 $Y=0.098
r1 15 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.132 $X2=0.243 $Y2=0.098
r2 1 8 7.07951 $w=1.89474e-08 $layer=LIG $thickness=5.17895e-08 $X=0.243
+ $Y=0.135 $X2=0.243 $Y2=0.135
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.243 $Y=0.132 $X2=0.243 $Y2=0.135
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.2025 $X2=0.243 $Y2=0.1595
r5 9 10 3.06124 $w=2.1e-08 $l=9e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.1505 $X2=0.243 $Y2=0.1595
r6 8 9 5.4252 $w=2.04194e-08 $l=1.55e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.135 $X2=0.243 $Y2=0.1505
r7 7 8 5.4252 $w=2.04194e-08 $l=1.55e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.1195 $X2=0.243 $Y2=0.135
r8 6 7 7.65311 $w=2.1e-08 $l=2.25e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.097 $X2=0.243 $Y2=0.1195
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.054 $X2=0.243 $Y2=0.097
r10 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.054 $X2=0.243 $Y2=0.0245
.ends

.subckt PM_NOR4xp25_ASAP7_75t_R%9 vss 2 3 1
c1 1 vss 0.000680333f $X=0.162 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.2025 $X2=0.162 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.2025 $X2=0.162 $Y2=0.2025
.ends

.subckt PM_NOR4xp25_ASAP7_75t_R%C vss 13 5 9 1 4 3
c1 1 vss 0.00210186f $X=0.135 $Y=0.135
c2 3 vss 0.0374611f $X=0.135 $Y=0.0245
c3 4 vss 0.0121234f $X=0.135 $Y=0.098
r1 13 4 8.62802 $w=1.3e-08 $l=3.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.132 $X2=0.135 $Y2=0.098
r2 1 7 7.07951 $w=1.89474e-08 $layer=LIG $thickness=5.17895e-08 $X=0.135
+ $Y=0.135 $X2=0.135 $Y2=0.135
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.135 $Y=0.132 $X2=0.135 $Y2=0.135
r4 9 8 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.2025 $X2=0.135 $Y2=0.153
r5 7 8 6.27555 $w=2.05e-08 $l=1.8e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.135 $X2=0.135 $Y2=0.153
r6 6 7 10.8674 $w=2.07143e-08 $l=3.15e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.1035 $X2=0.135 $Y2=0.135
r7 5 6 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.054 $X2=0.135 $Y2=0.1035
r8 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.054 $X2=0.135 $Y2=0.0245
.ends


* End of included file NOR4xp25_ASAP7_75t_R.pex.sp.pex



*
.subckt NOR4xp25_ASAP7_75t_CUSTOM_4_25_0_7 VSS VDD D C B A Y
*
* VSS VSS
* VDD VDD
* D D
* C C
* B B
* A A
* Y Y
*
*

M0 VSS N_M0_g N_M0_s VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.071 $Y=0.027
M1 N_M1_d N_M1_g VSS VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.125 $Y=0.027
M2 VSS N_M2_g N_M2_s VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.179 $Y=0.027
M3 N_M3_d N_M3_g VSS VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.233 $Y=0.027
M4 N_M4_d N_M4_g VDD VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.071 $Y=0.162
M5 N_M5_d N_M5_g N_M5_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.125 $Y=0.162
M6 N_M6_d N_M6_g N_M6_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.179 $Y=0.162
M7 N_M7_d N_M7_g N_M7_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.233 $Y=0.162


* .include "NOR4xp25_ASAP7_75t_R.pex.sp.pxi"

* Start of included file NOR4xp25_ASAP7_75t_R.pex.sp.pxi
x_PM_NOR4xp25_ASAP7_75t_R%8 vss N_M4_d N_M5_s N_8_1 PM_NOR4xp25_ASAP7_75t_R%8
cc_1 N_8_1 N_D_3 0.0168437f $X=0.108 $Y=0.2025
cc_2 N_8_1 N_C_3 0.0170258f $X=0.108 $Y=0.2025
x_PM_NOR4xp25_ASAP7_75t_R%10 vss N_M6_d N_M7_s N_10_1 PM_NOR4xp25_ASAP7_75t_R%10
cc_3 N_10_1 N_B_3 0.0170349f $X=0.216 $Y=0.2025
cc_4 N_10_1 N_A_3 0.0168187f $X=0.216 $Y=0.2025
x_PM_NOR4xp25_ASAP7_75t_R%D vss D N_M0_g N_M4_g N_D_3 N_D_4
+ PM_NOR4xp25_ASAP7_75t_R%D
x_PM_NOR4xp25_ASAP7_75t_R%B vss B N_M2_g N_M6_g N_B_1 N_B_4 N_B_3
+ PM_NOR4xp25_ASAP7_75t_R%B
cc_5 N_B_1 N_C_1 0.00150175f $X=0.189 $Y=0.135
cc_6 N_B_4 N_C_4 0.0064203f $X=0.189 $Y=0.098
cc_7 N_B_3 N_C_3 0.00799301f $X=0.189 $Y=0.0245
x_PM_NOR4xp25_ASAP7_75t_R%Y vss Y N_M0_s N_M1_d N_M2_s N_M3_d N_M7_d N_Y_19
+ N_Y_1 N_Y_13 N_Y_2 N_Y_14 N_Y_4 N_Y_16 N_Y_15 N_Y_20 PM_NOR4xp25_ASAP7_75t_R%Y
cc_8 N_Y_19 N_D_4 0.00126007f $X=0.0425 $Y=0.036
cc_9 N_Y_1 N_D_4 0.00192102f $X=0.054 $Y=0.054
cc_10 N_Y_13 N_D_3 0.0267323f $X=0.056 $Y=0.054
cc_11 N_Y_19 N_C_4 0.0011453f $X=0.0425 $Y=0.036
cc_12 N_Y_2 N_C_4 0.0017217f $X=0.162 $Y=0.054
cc_13 N_Y_14 N_C_3 0.026757f $X=0.1475 $Y=0.054
cc_14 N_Y_14 N_B_1 0.000381152f $X=0.1475 $Y=0.054
cc_15 N_Y_2 N_B_3 0.000676004f $X=0.162 $Y=0.054
cc_16 N_Y_19 N_B_4 0.00113704f $X=0.0425 $Y=0.036
cc_17 N_Y_4 N_B_4 0.00215407f $X=0.27 $Y=0.2025
cc_18 N_Y_14 N_B_3 0.0265246f $X=0.1475 $Y=0.054
cc_19 N_Y_4 N_A_1 0.000784828f $X=0.27 $Y=0.2025
cc_20 N_Y_19 N_A_4 0.00102766f $X=0.0425 $Y=0.036
cc_21 N_Y_16 N_A_1 0.00132929f $X=0.2555 $Y=0.2025
cc_22 N_Y_4 N_A_3 0.00158841f $X=0.27 $Y=0.2025
cc_23 N_Y_15 N_A_3 0.0107513f $X=0.2555 $Y=0.054
cc_24 N_Y_20 N_A_4 0.00777198f $X=0.297 $Y=0.0575
cc_25 N_Y_16 N_A_3 0.0513974f $X=0.2555 $Y=0.2025
x_PM_NOR4xp25_ASAP7_75t_R%A vss A N_M3_g N_M7_g N_A_3 N_A_4 N_A_1
+ PM_NOR4xp25_ASAP7_75t_R%A
cc_26 N_A_3 N_B_1 0.00155475f $X=0.243 $Y=0.0245
cc_27 N_A_4 N_B_4 0.00532401f $X=0.243 $Y=0.098
cc_28 N_A_3 N_B_3 0.00799961f $X=0.243 $Y=0.0245
x_PM_NOR4xp25_ASAP7_75t_R%9 vss N_M5_d N_M6_s N_9_1 PM_NOR4xp25_ASAP7_75t_R%9
cc_29 N_9_1 N_C_3 0.0169766f $X=0.162 $Y=0.2025
cc_30 N_9_1 N_B_3 0.0169985f $X=0.162 $Y=0.2025
x_PM_NOR4xp25_ASAP7_75t_R%C vss C N_M1_g N_M5_g N_C_1 N_C_4 N_C_3
+ PM_NOR4xp25_ASAP7_75t_R%C
cc_31 N_C_1 N_D_3 0.00147495f $X=0.135 $Y=0.135
cc_32 N_C_4 N_D_4 0.00637458f $X=0.135 $Y=0.098
cc_33 N_C_3 N_D_3 0.00796439f $X=0.135 $Y=0.0245


* End of included file NOR4xp25_ASAP7_75t_R.pex.sp.pxi
.ends
