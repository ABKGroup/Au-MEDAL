.subckt PM_NAND5xp2_ASAP7_75t_R%9 vss 2 3 1
c1 1 vss 0.000708668f $X=0.108 $Y=0.0675
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.0675 $X2=0.108 $Y2=0.0675
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1253 $Y=0.0675 $X2=0.108 $Y2=0.0675
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%11 vss 2 3 1
c1 1 vss 0.000691127f $X=0.216 $Y=0.0675
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1986 $Y=0.0675 $X2=0.216 $Y2=0.0675
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.2333 $Y=0.0675 $X2=0.216 $Y2=0.0675
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%12 vss 2 3 1
c1 1 vss 0.000787045f $X=0.27 $Y=0.0675
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.2526 $Y=0.0675 $X2=0.27 $Y2=0.0675
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.2873 $Y=0.0675 $X2=0.27 $Y2=0.0675
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%A vss 17 5 11 3 4 1
c1 1 vss 0.00259296f $X=0.0715 $Y=0.134
c2 3 vss 0.0428883f $X=0.081 $Y=0.0245
c3 4 vss 0.00973639f $X=0.081 $Y=0.0975
r1 17 4 8.51143 $w=1.3e-08 $l=3.65e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.135 $X2=0.081 $Y2=0.0975
r2 13 14 3.67195 $w=1.8e-08 $l=9.5e-09 $layer=LIG $thickness=4.8e-08 $X=0.081
+ $Y=0.134 $X2=0.0905 $Y2=0.134
r3 17 13 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.134
r4 1 13 3.67195 $w=1.8e-08 $l=9.5e-09 $layer=LIG $thickness=4.8e-08 $X=0.0715
+ $Y=0.134 $X2=0.081 $Y2=0.134
r5 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.216 $X2=0.081 $Y2=0.173
r6 9 10 7.82318 $w=2.1e-08 $l=2.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.15 $X2=0.081 $Y2=0.173
r7 8 9 5.59527 $w=2.04375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.134 $X2=0.081 $Y2=0.15
r8 8 14 3.40757 $w=1.98947e-08 $l=9.5e-09 $layer=Gate_1 $thickness=5.55789e-08
+ $X=0.081 $Y=0.134 $X2=0.0905 $Y2=0.134
r9 8 13 6.69299 $w=1.9e-08 $layer=LIG $thickness=5.2e-08 $X=0.081 $Y=0.134
+ $X2=0.081 $Y2=0.134
r10 7 8 5.25513 $w=2.04e-08 $l=1.5e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.119 $X2=0.081 $Y2=0.134
r11 6 7 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.1105 $X2=0.081 $Y2=0.119
r12 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0675 $X2=0.081 $Y2=0.1105
r13 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.0675 $X2=0.081 $Y2=0.0245
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%C vss 13 5 9 1 4 3
c1 1 vss 0.00154676f $X=0.189 $Y=0.1345
c2 3 vss 0.0382138f $X=0.189 $Y=0.0245
c3 4 vss 0.0127152f $X=0.189 $Y=0.0795
r1 13 4 12.7088 $w=1.3e-08 $l=5.45e-08 $layer=M1 $thickness=3.6e-08 $X=0.189
+ $Y=0.135 $X2=0.189 $Y2=0.0795
r2 1 7 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.189
+ $Y=0.1345 $X2=0.189 $Y2=0.1345
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.189 $Y=0.135 $X2=0.189 $Y2=0.1345
r4 9 8 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.216 $X2=0.189 $Y2=0.1665
r5 7 8 11.046 $w=2.07031e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.1345 $X2=0.189 $Y2=0.1665
r6 6 7 6.28405 $w=2.04722e-08 $l=1.8e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.1165 $X2=0.189 $Y2=0.1345
r7 5 6 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.0675 $X2=0.189 $Y2=0.1165
r8 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.0675 $X2=0.189 $Y2=0.0245
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%D vss 13 5 9 1 4 3
c1 1 vss 0.00178536f $X=0.243 $Y=0.1345
c2 3 vss 0.038707f $X=0.243 $Y=0.0245
c3 4 vss 0.0124537f $X=0.243 $Y=0.0795
r1 13 4 12.7088 $w=1.3e-08 $l=5.45e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.135 $X2=0.243 $Y2=0.0795
r2 1 7 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.243
+ $Y=0.1345 $X2=0.243 $Y2=0.1345
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.243 $Y=0.135 $X2=0.243 $Y2=0.1345
r4 9 8 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.216 $X2=0.243 $Y2=0.1665
r5 7 8 11.046 $w=2.07031e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.1345 $X2=0.243 $Y2=0.1665
r6 6 7 6.28405 $w=2.04722e-08 $l=1.8e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.1165 $X2=0.243 $Y2=0.1345
r7 5 6 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.0675 $X2=0.243 $Y2=0.1165
r8 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.0675 $X2=0.243 $Y2=0.0245
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%E vss 15 5 11 3 4
c1 1 vss 0.00492509f $X=0.297 $Y=0.1345
c2 3 vss 0.0798428f $X=0.297 $Y=0.0245
c3 4 vss 0.0162619f $X=0.297 $Y=0.0795
r1 15 4 12.7088 $w=1.3e-08 $l=5.45e-08 $layer=M1 $thickness=3.6e-08 $X=0.297
+ $Y=0.135 $X2=0.297 $Y2=0.0795
r2 1 8 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.297
+ $Y=0.1345 $X2=0.297 $Y2=0.1345
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.297 $Y=0.135 $X2=0.297 $Y2=0.1345
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.216 $X2=0.297 $Y2=0.173
r5 9 10 7.65311 $w=2.1e-08 $l=2.25e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.1505 $X2=0.297 $Y2=0.173
r6 8 9 5.60378 $w=2.04063e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.1345 $X2=0.297 $Y2=0.1505
r7 7 8 5.43371 $w=2.03871e-08 $l=1.55e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.119 $X2=0.297 $Y2=0.1345
r8 6 7 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.297
+ $Y=0.1105 $X2=0.297 $Y2=0.119
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.297
+ $Y=0.0675 $X2=0.297 $Y2=0.1105
r10 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.297
+ $Y=0.0675 $X2=0.297 $Y2=0.0245
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%Y vss 27 23 48 51 52 55 56 1 2 21 13 14 19 15 3
+ 4 16
c1 1 vss 0.00623559f $X=0.054 $Y=0.0675
c2 2 vss 0.00704951f $X=0.054 $Y=0.216
c3 3 vss 0.00730465f $X=0.162 $Y=0.216
c4 4 vss 0.00729642f $X=0.27 $Y=0.216
c5 13 vss 0.0369681f $X=0.056 $Y=0.0675
c6 14 vss 0.0279977f $X=0.056 $Y=0.216
c7 15 vss 0.00358238f $X=0.1475 $Y=0.216
c8 16 vss 0.00360124f $X=0.2555 $Y=0.216
c9 17 vss 0.00257828f $X=0.018 $Y=0.036
c10 18 vss 0.00255497f $X=0.018 $Y=0.234
c11 19 vss 0.00421484f $X=0.018 $Y=0.0575
c12 20 vss 0.00397693f $X=0.036 $Y=0.036
c13 21 vss 0.0250299f $X=0.036 $Y=0.234
r1 56 54 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.2873 $Y=0.216 $X2=0.2845 $Y2=0.216
r2 4 54 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.27 $Y=0.216 $X2=0.2845 $Y2=0.216
r3 16 4 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.2555 $Y=0.216 $X2=0.27 $Y2=0.216
r4 55 16 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.2526 $Y=0.216 $X2=0.2555 $Y2=0.216
r5 52 50 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.216 $X2=0.1765 $Y2=0.216
r6 3 50 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.162 $Y=0.216 $X2=0.1765 $Y2=0.216
r7 15 3 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1475 $Y=0.216 $X2=0.162 $Y2=0.216
r8 51 15 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.216 $X2=0.1475 $Y2=0.216
r9 48 47 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.216 $X2=0.0685 $Y2=0.216
r10 14 47 0.231482 $w=5.4e-08 $l=1.25e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.216 $X2=0.0685 $Y2=0.216
r11 4 43 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.27 $Y=0.216 $X2=0.27 $Y2=0.234
r12 3 37 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.162 $Y=0.216 $X2=0.162 $Y2=0.234
r13 2 30 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.054 $Y=0.216 $X2=0.054 $Y2=0.234
r14 42 43 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.2565
+ $Y=0.234 $X2=0.27 $Y2=0.234
r15 41 42 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.234 $X2=0.2565 $Y2=0.234
r16 40 41 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.216
+ $Y=0.234 $X2=0.243 $Y2=0.234
r17 39 40 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.189
+ $Y=0.234 $X2=0.216 $Y2=0.234
r18 38 39 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1755
+ $Y=0.234 $X2=0.189 $Y2=0.234
r19 37 38 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.162
+ $Y=0.234 $X2=0.1755 $Y2=0.234
r20 36 37 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1485
+ $Y=0.234 $X2=0.162 $Y2=0.234
r21 35 36 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.234 $X2=0.1485 $Y2=0.234
r22 34 35 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.108
+ $Y=0.234 $X2=0.135 $Y2=0.234
r23 33 34 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.234 $X2=0.108 $Y2=0.234
r24 32 33 2.3319 $w=1.3e-08 $l=1e-08 $layer=M1 $thickness=3.6e-08 $X=0.071
+ $Y=0.234 $X2=0.081 $Y2=0.234
r25 31 32 1.04935 $w=1.3e-08 $l=4.5e-09 $layer=M1 $thickness=3.6e-08 $X=0.0665
+ $Y=0.234 $X2=0.071 $Y2=0.234
r26 30 31 2.91487 $w=1.3e-08 $l=1.25e-08 $layer=M1 $thickness=3.6e-08 $X=0.054
+ $Y=0.234 $X2=0.0665 $Y2=0.234
r27 21 30 4.19742 $w=1.3e-08 $l=1.8e-08 $layer=M1 $thickness=3.6e-08 $X=0.036
+ $Y=0.234 $X2=0.054 $Y2=0.234
r28 18 29 3.36447 $w=1.71023e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.018 $Y=0.234 $X2=0.018 $Y2=0.2125
r29 18 21 2.5483 $w=1.55e-08 $l=1.8e-08 $layer=M1 $thickness=3.6e-08 $X=0.018
+ $Y=0.234 $X2=0.036 $Y2=0.234
r30 28 29 11.1348 $w=1.3e-08 $l=4.78e-08 $layer=M1 $thickness=3.6e-08 $X=0.018
+ $Y=0.1647 $X2=0.018 $Y2=0.2125
r31 27 28 8.33653 $w=1.3e-08 $l=3.57e-08 $layer=M1 $thickness=3.6e-08 $X=0.025
+ $Y=0.129 $X2=0.018 $Y2=0.1647
r32 27 26 6.9374 $w=1.3e-08 $l=2.98e-08 $layer=M1 $thickness=3.6e-08 $X=0.025
+ $Y=0.129 $X2=0.018 $Y2=0.0992
r33 19 26 9.73567 $w=1.3e-08 $l=4.17e-08 $layer=M1 $thickness=3.6e-08 $X=0.018
+ $Y=0.0575 $X2=0.018 $Y2=0.0992
r34 17 20 2.5483 $w=1.55e-08 $l=1.8e-08 $layer=M1 $thickness=3.6e-08 $X=0.018
+ $Y=0.036 $X2=0.036 $Y2=0.036
r35 17 19 3.36447 $w=1.71023e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.018 $Y=0.036 $X2=0.018 $Y2=0.0575
r36 20 24 4.19742 $w=1.3e-08 $l=1.8e-08 $layer=M1 $thickness=3.6e-08 $X=0.036
+ $Y=0.036 $X2=0.054 $Y2=0.036
r37 1 24 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.054 $Y=0.0675 $X2=0.054
+ $Y2=0.036
r38 23 22 0.0308642 $w=8.1e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.0675 $X2=0.0685 $Y2=0.0675
r39 13 22 0.154321 $w=8.1e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.0675 $X2=0.0685 $Y2=0.0675
r40 2 14 1e-05 $l=2e-09 $X=0.054 $Y=0.216 $X2=0.056 $Y2=0.216
r41 1 13 1e-05 $l=2e-09 $X=0.054 $Y=0.0675 $X2=0.056 $Y2=0.0675
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%10 vss 2 3 1
c1 1 vss 0.000685825f $X=0.162 $Y=0.0675
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.0675 $X2=0.162 $Y2=0.0675
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.0675 $X2=0.162 $Y2=0.0675
.ends

.subckt PM_NAND5xp2_ASAP7_75t_R%B vss 15 5 9 1 4 3
c1 1 vss 0.00180179f $X=0.135 $Y=0.1345
c2 3 vss 0.0387042f $X=0.135 $Y=0.0245
c3 4 vss 0.0116644f $X=0.135 $Y=0.0575
r1 15 14 8.51143 $w=1.3e-08 $l=3.65e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.135 $X2=0.135 $Y2=0.0975
r2 4 14 9.32759 $w=1.3e-08 $l=4e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.0575 $X2=0.135 $Y2=0.0975
r3 1 7 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.135
+ $Y=0.1345 $X2=0.135 $Y2=0.1345
r4 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.135 $Y=0.135 $X2=0.135 $Y2=0.1345
r5 9 8 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.216 $X2=0.135 $Y2=0.1665
r6 7 8 11.046 $w=2.07031e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.1345 $X2=0.135 $Y2=0.1665
r7 6 7 6.28405 $w=2.04722e-08 $l=1.8e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.1165 $X2=0.135 $Y2=0.1345
r8 5 6 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.0675 $X2=0.135 $Y2=0.1165
r9 5 3 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.0675 $X2=0.135 $Y2=0.0245
.ends


* End of included file NAND5xp2_ASAP7_75t_R.pex.sp.pex



*
.subckt NAND5xp2_ASAP7_75t_CUSTOM_4_25_0_7 VSS VDD A B C D E Y
*
* VSS VSS
* VDD VDD
* A A
* B B
* C C
* D D
* E E
* Y Y
*
*

M0 N_M0_d N_M0_g N_M0_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.071 $Y=0.027
M1 N_M1_d N_M1_g N_M1_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.125 $Y=0.027
M2 N_M2_d N_M2_g N_M2_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.179 $Y=0.027
M3 N_M3_d N_M3_g N_M3_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.233 $Y=0.027
M4 VSS N_M4_g N_M4_s VSS nmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.287 $Y=0.027
M5 VDD N_M5_g N_M5_s VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.071 $Y=0.189
M6 N_M6_d N_M6_g VDD VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.125 $Y=0.189
M7 VDD N_M7_g N_M7_s VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.179 $Y=0.189
M8 N_M8_d N_M8_g VDD VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.233 $Y=0.189
M9 VDD N_M9_g N_M9_s VDD pmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.287 $Y=0.189


* .include "NAND5xp2_ASAP7_75t_R.pex.sp.pxi"

* Start of included file NAND5xp2_ASAP7_75t_R.pex.sp.pxi
x_PM_NAND5xp2_ASAP7_75t_R%9 vss N_M0_d N_M1_s N_9_1 PM_NAND5xp2_ASAP7_75t_R%9
cc_1 N_9_1 N_A_3 0.0168921f $X=0.108 $Y=0.0675
cc_2 N_9_1 N_B_3 0.0170215f $X=0.108 $Y=0.0675
x_PM_NAND5xp2_ASAP7_75t_R%11 vss N_M2_d N_M3_s N_11_1 PM_NAND5xp2_ASAP7_75t_R%11
cc_3 N_11_1 N_C_3 0.017008f $X=0.216 $Y=0.0675
cc_4 N_11_1 N_D_3 0.0169514f $X=0.216 $Y=0.0675
x_PM_NAND5xp2_ASAP7_75t_R%12 vss N_M3_d N_M4_s N_12_1 PM_NAND5xp2_ASAP7_75t_R%12
cc_5 N_12_1 N_D_3 0.0170222f $X=0.27 $Y=0.0675
cc_6 N_12_1 N_E_3 0.0168465f $X=0.27 $Y=0.0675
x_PM_NAND5xp2_ASAP7_75t_R%A vss A N_M0_g N_M5_g N_A_3 N_A_4 N_A_1
+ PM_NAND5xp2_ASAP7_75t_R%A
x_PM_NAND5xp2_ASAP7_75t_R%C vss C N_M2_g N_M7_g N_C_1 N_C_4 N_C_3
+ PM_NAND5xp2_ASAP7_75t_R%C
cc_7 N_C_1 N_B_1 0.00153262f $X=0.189 $Y=0.1345
cc_8 N_C_4 N_B_4 0.00641241f $X=0.189 $Y=0.0795
cc_9 N_C_3 N_B_3 0.00808159f $X=0.189 $Y=0.0245
x_PM_NAND5xp2_ASAP7_75t_R%D vss D N_M3_g N_M8_g N_D_1 N_D_4 N_D_3
+ PM_NAND5xp2_ASAP7_75t_R%D
cc_10 N_D_1 N_C_1 0.00155508f $X=0.243 $Y=0.1345
cc_11 N_D_4 N_C_4 0.00640534f $X=0.243 $Y=0.0795
cc_12 N_D_3 N_C_3 0.00807713f $X=0.243 $Y=0.0245
x_PM_NAND5xp2_ASAP7_75t_R%E vss E N_M4_g N_M9_g N_E_3 N_E_4
+ PM_NAND5xp2_ASAP7_75t_R%E
cc_13 N_E_3 N_D_1 0.00159183f $X=0.297 $Y=0.0245
cc_14 N_E_4 N_D_4 0.00629254f $X=0.297 $Y=0.0795
cc_15 N_E_3 N_D_3 0.00804035f $X=0.297 $Y=0.0245
x_PM_NAND5xp2_ASAP7_75t_R%Y vss Y N_M0_s N_M5_s N_M6_d N_M7_s N_M8_d N_M9_s
+ N_Y_1 N_Y_2 N_Y_21 N_Y_13 N_Y_14 N_Y_19 N_Y_15 N_Y_3 N_Y_4 N_Y_16
+ PM_NAND5xp2_ASAP7_75t_R%Y
cc_16 N_Y_1 N_A_3 0.00245331f $X=0.054 $Y=0.0675
cc_17 N_Y_2 N_A_3 0.000718618f $X=0.054 $Y=0.216
cc_18 N_Y_21 N_A_4 0.00112448f $X=0.036 $Y=0.234
cc_19 N_Y_13 N_A_1 0.00198646f $X=0.056 $Y=0.0675
cc_20 N_Y_14 N_A_3 0.0106342f $X=0.056 $Y=0.216
cc_21 N_Y_19 N_A_4 0.00690426f $X=0.018 $Y=0.0575
cc_22 N_Y_13 N_A_3 0.0499457f $X=0.056 $Y=0.0675
cc_23 N_Y_15 N_B_1 0.000334184f $X=0.1475 $Y=0.216
cc_24 N_Y_3 N_B_3 0.000669958f $X=0.162 $Y=0.216
cc_25 N_Y_21 N_B_4 0.00113891f $X=0.036 $Y=0.234
cc_26 N_Y_1 N_B_4 0.00213788f $X=0.054 $Y=0.0675
cc_27 N_Y_15 N_B_3 0.0262859f $X=0.1475 $Y=0.216
cc_28 N_Y_21 N_C_4 0.0011318f $X=0.036 $Y=0.234
cc_29 N_Y_3 N_C_4 0.0017156f $X=0.162 $Y=0.216
cc_30 N_Y_15 N_C_3 0.0265705f $X=0.1475 $Y=0.216
cc_31 N_Y_21 N_D_4 0.00113465f $X=0.036 $Y=0.234
cc_32 N_Y_4 N_D_4 0.00161941f $X=0.27 $Y=0.216
cc_33 N_Y_16 N_D_3 0.0259885f $X=0.2555 $Y=0.216
cc_34 N_Y_21 N_E_3 0.001072f $X=0.036 $Y=0.234
cc_35 N_Y_4 N_E_4 0.00147974f $X=0.27 $Y=0.216
cc_36 N_Y_16 N_E_3 0.0258888f $X=0.2555 $Y=0.216
x_PM_NAND5xp2_ASAP7_75t_R%10 vss N_M1_d N_M2_s N_10_1 PM_NAND5xp2_ASAP7_75t_R%10
cc_37 N_10_1 N_B_3 0.016866f $X=0.162 $Y=0.0675
cc_38 N_10_1 N_C_3 0.0169284f $X=0.162 $Y=0.0675
x_PM_NAND5xp2_ASAP7_75t_R%B vss B N_M1_g N_M6_g N_B_1 N_B_4 N_B_3
+ PM_NAND5xp2_ASAP7_75t_R%B
cc_39 N_B_1 N_A_3 0.00142764f $X=0.135 $Y=0.1345
cc_40 N_B_4 N_A_4 0.00536488f $X=0.135 $Y=0.0575
cc_41 N_B_3 N_A_3 0.00802962f $X=0.135 $Y=0.0245


* End of included file NAND5xp2_ASAP7_75t_R.pex.sp.pxi
.ends
