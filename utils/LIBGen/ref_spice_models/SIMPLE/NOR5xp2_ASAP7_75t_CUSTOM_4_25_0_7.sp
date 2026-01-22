.subckt PM_NOR5xp2_ASAP7_75t_R%9 vss 2 3 1
c1 1 vss 0.000710975f $X=0.108 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.2025 $X2=0.108 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1253 $Y=0.2025 $X2=0.108 $Y2=0.2025
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%12 vss 2 3 1
c1 1 vss 0.000786598f $X=0.27 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.2526 $Y=0.2025 $X2=0.27 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.2873 $Y=0.2025 $X2=0.27 $Y2=0.2025
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%A vss 17 5 11 3 4 1
c1 1 vss 0.00268372f $X=0.0715 $Y=0.136
c2 3 vss 0.0424736f $X=0.081 $Y=0.0245
c3 4 vss 0.0101777f $X=0.081 $Y=0.0985
r1 17 4 8.74462 $w=1.3e-08 $l=3.75e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.135 $X2=0.081 $Y2=0.0985
r2 13 14 3.67195 $w=1.8e-08 $l=9.5e-09 $layer=LIG $thickness=4.8e-08 $X=0.081
+ $Y=0.136 $X2=0.0905 $Y2=0.136
r3 17 13 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.081 $Y=0.135 $X2=0.081 $Y2=0.136
r4 1 13 3.67195 $w=1.8e-08 $l=9.5e-09 $layer=LIG $thickness=4.8e-08 $X=0.0715
+ $Y=0.136 $X2=0.081 $Y2=0.136
r5 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.2025 $X2=0.081 $Y2=0.1595
r6 9 10 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.151 $X2=0.081 $Y2=0.1595
r7 8 9 5.25513 $w=2.04e-08 $l=1.5e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.136 $X2=0.081 $Y2=0.151
r8 8 14 3.40757 $w=1.98947e-08 $l=9.5e-09 $layer=Gate_1 $thickness=5.55789e-08
+ $X=0.081 $Y=0.136 $X2=0.0905 $Y2=0.136
r9 8 13 6.69299 $w=1.9e-08 $layer=LIG $thickness=5.2e-08 $X=0.081 $Y=0.136
+ $X2=0.081 $Y2=0.136
r10 7 8 5.59527 $w=2.04375e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.12 $X2=0.081 $Y2=0.136
r11 6 7 7.82318 $w=2.1e-08 $l=2.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.097 $X2=0.081 $Y2=0.12
r12 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.081
+ $Y=0.054 $X2=0.081 $Y2=0.097
r13 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.081 $Y=0.054 $X2=0.081 $Y2=0.0245
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%B vss 13 5 9 1 4 3
c1 1 vss 0.0017889f $X=0.135 $Y=0.1355
c2 3 vss 0.0387664f $X=0.135 $Y=0.0245
c3 4 vss 0.0117049f $X=0.135 $Y=0.0985
r1 13 4 8.74462 $w=1.3e-08 $l=3.75e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.136 $X2=0.135 $Y2=0.0985
r2 1 7 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.135
+ $Y=0.1355 $X2=0.135 $Y2=0.1355
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.135 $Y=0.136 $X2=0.135 $Y2=0.1355
r4 9 8 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.2025 $X2=0.135 $Y2=0.1535
r5 7 8 6.28405 $w=2.04722e-08 $l=1.8e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.1355 $X2=0.135 $Y2=0.1535
r6 6 7 11.046 $w=2.07031e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.135 $Y=0.1035 $X2=0.135 $Y2=0.1355
r7 5 6 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.054 $X2=0.135 $Y2=0.1035
r8 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.135
+ $Y=0.054 $X2=0.135 $Y2=0.0245
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%C vss 13 5 9 1 4 3
c1 1 vss 0.00155227f $X=0.189 $Y=0.1355
c2 3 vss 0.0382312f $X=0.189 $Y=0.0245
c3 4 vss 0.0127729f $X=0.189 $Y=0.0985
r1 13 4 8.74462 $w=1.3e-08 $l=3.75e-08 $layer=M1 $thickness=3.6e-08 $X=0.189
+ $Y=0.135 $X2=0.189 $Y2=0.0985
r2 1 7 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.189
+ $Y=0.1355 $X2=0.189 $Y2=0.1355
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.189 $Y=0.135 $X2=0.189 $Y2=0.1355
r4 9 8 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.2025 $X2=0.189 $Y2=0.1535
r5 7 8 6.28405 $w=2.04722e-08 $l=1.8e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.1355 $X2=0.189 $Y2=0.1535
r6 6 7 11.046 $w=2.07031e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.189 $Y=0.1035 $X2=0.189 $Y2=0.1355
r7 5 6 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.054 $X2=0.189 $Y2=0.1035
r8 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.189
+ $Y=0.054 $X2=0.189 $Y2=0.0245
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%D vss 13 5 9 1 4 3
c1 1 vss 0.00176754f $X=0.243 $Y=0.1355
c2 3 vss 0.0386336f $X=0.243 $Y=0.0245
c3 4 vss 0.0123201f $X=0.243 $Y=0.0985
r1 13 4 8.74462 $w=1.3e-08 $l=3.75e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.135 $X2=0.243 $Y2=0.0985
r2 1 7 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.243
+ $Y=0.1355 $X2=0.243 $Y2=0.1355
r3 13 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.243 $Y=0.135 $X2=0.243 $Y2=0.1355
r4 9 8 16.6668 $w=2.1e-08 $l=4.9e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.2025 $X2=0.243 $Y2=0.1535
r5 7 8 6.28405 $w=2.04722e-08 $l=1.8e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.1355 $X2=0.243 $Y2=0.1535
r6 6 7 11.046 $w=2.07031e-08 $l=3.2e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.243 $Y=0.1035 $X2=0.243 $Y2=0.1355
r7 5 6 16.8368 $w=2.1e-08 $l=4.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.054 $X2=0.243 $Y2=0.1035
r8 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.243
+ $Y=0.054 $X2=0.243 $Y2=0.0245
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%E vss 15 5 11 3 4
c1 1 vss 0.00450518f $X=0.297 $Y=0.1355
c2 3 vss 0.0798937f $X=0.297 $Y=0.0245
c3 4 vss 0.0155654f $X=0.297 $Y=0.0985
r1 15 4 8.74462 $w=1.3e-08 $l=3.75e-08 $layer=M1 $thickness=3.6e-08 $X=0.297
+ $Y=0.135 $X2=0.297 $Y2=0.0985
r2 1 8 6.8952 $w=1.94872e-08 $layer=LIG $thickness=5.18974e-08 $X=0.297
+ $Y=0.1355 $X2=0.297 $Y2=0.1355
r3 15 1 19.3796 $a=3.24e-16 $layer=V0LIG $X=0.297 $Y=0.135 $X2=0.297 $Y2=0.1355
r4 11 10 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.2025 $X2=0.297 $Y2=0.1595
r5 9 10 2.89117 $w=2.1e-08 $l=8.5e-09 $layer=Gate_1 $thickness=5.6e-08 $X=0.297
+ $Y=0.151 $X2=0.297 $Y2=0.1595
r6 8 9 5.43371 $w=2.03871e-08 $l=1.55e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.1355 $X2=0.297 $Y2=0.151
r7 7 8 5.60378 $w=2.04063e-08 $l=1.6e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.1195 $X2=0.297 $Y2=0.1355
r8 6 7 7.65311 $w=2.1e-08 $l=2.25e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.297
+ $Y=0.097 $X2=0.297 $Y2=0.1195
r9 5 6 14.6259 $w=2.1e-08 $l=4.3e-08 $layer=Gate_1 $thickness=5.6e-08 $X=0.297
+ $Y=0.054 $X2=0.297 $Y2=0.097
r10 5 3 10.0341 $w=2.1e-08 $l=2.95e-08 $layer=Gate_1 $thickness=5.6e-08
+ $X=0.297 $Y=0.054 $X2=0.297 $Y2=0.0245
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%Y vss 43 23 24 48 49 51 56 2 1 20 16 13 19 3 14
+ 4 15
c1 1 vss 0.0063486f $X=0.054 $Y=0.2025
c2 2 vss 0.00724009f $X=0.108 $Y=0.054
c3 3 vss 0.00715638f $X=0.216 $Y=0.054
c4 4 vss 0.00802393f $X=0.324 $Y=0.054
c5 13 vss 0.00358749f $X=0.0935 $Y=0.054
c6 14 vss 0.0036f $X=0.2015 $Y=0.054
c7 15 vss 0.0280301f $X=0.3095 $Y=0.054
c8 16 vss 0.0369372f $X=0.056 $Y=0.2025
c9 17 vss 0.00294709f $X=0.027 $Y=0.036
c10 18 vss 0.00578658f $X=0.027 $Y=0.234
c11 19 vss 0.00570438f $X=0.027 $Y=0.0575
c12 20 vss 0.0259766f $X=0.0945 $Y=0.036
r1 56 55 0.0308642 $w=8.1e-08 $l=2.5e-09 $layer=P_src_drn $thickness=1e-09
+ $X=0.0713 $Y=0.2025 $X2=0.0685 $Y2=0.2025
r2 16 55 0.154321 $w=8.1e-08 $l=1.25e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.056 $Y=0.2025 $X2=0.0685 $Y2=0.2025
r3 1 53 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.054 $Y=0.2025 $X2=0.054 $Y2=0.234
r4 52 53 3.03147 $w=1.3e-08 $l=1.3e-08 $layer=M1 $thickness=3.6e-08 $X=0.041
+ $Y=0.234 $X2=0.054 $Y2=0.234
r5 18 45 3.31868 $w=1.62766e-08 $l=2.25e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.234 $X2=0.027 $Y2=0.2115
r6 18 52 1.9532 $w=1.65714e-08 $l=1.4e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.234 $X2=0.041 $Y2=0.234
r7 15 4 0.231482 $w=5.4e-08 $l=1.25e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.3095 $Y=0.054 $X2=0.324 $Y2=0.054
r8 51 15 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.3066 $Y=0.054 $X2=0.3095 $Y2=0.054
r9 49 47 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.2333 $Y=0.054 $X2=0.2305 $Y2=0.054
r10 3 47 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.216 $Y=0.054 $X2=0.2305 $Y2=0.054
r11 14 3 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.2015 $Y=0.054 $X2=0.216 $Y2=0.054
r12 48 14 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1986 $Y=0.054 $X2=0.2015 $Y2=0.054
r13 44 45 7.10752 $w=1.5e-08 $l=4.18e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.1697 $X2=0.027 $Y2=0.2115
r14 43 44 4.8944 $w=1.5e-08 $l=2.87e-08 $layer=M1 $thickness=3.6e-08 $X=0.025
+ $Y=0.141 $X2=0.027 $Y2=0.1697
r15 43 42 6.08608 $w=1.5e-08 $l=3.58e-08 $layer=M1 $thickness=3.6e-08 $X=0.025
+ $Y=0.141 $X2=0.027 $Y2=0.1052
r16 19 42 8.12896 $w=1.5e-08 $l=4.77e-08 $layer=M1 $thickness=3.6e-08 $X=0.027
+ $Y=0.0575 $X2=0.027 $Y2=0.1052
r17 4 40 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.324 $Y=0.054 $X2=0.324 $Y2=0.036
r18 3 34 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.216 $Y=0.054 $X2=0.216 $Y2=0.036
r19 17 29 4.28601 $w=1.71509e-08 $l=2.65e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.036 $X2=0.0535 $Y2=0.036
r20 17 19 2.56637 $w=1.7093e-08 $l=2.15e-08 $layer=M1 $thickness=3.6e-08
+ $X=0.027 $Y=0.036 $X2=0.027 $Y2=0.0575
r21 39 40 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.3105
+ $Y=0.036 $X2=0.324 $Y2=0.036
r22 38 39 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.297
+ $Y=0.036 $X2=0.3105 $Y2=0.036
r23 37 38 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.27
+ $Y=0.036 $X2=0.297 $Y2=0.036
r24 36 37 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.243
+ $Y=0.036 $X2=0.27 $Y2=0.036
r25 35 36 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.2295
+ $Y=0.036 $X2=0.243 $Y2=0.036
r26 34 35 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.216
+ $Y=0.036 $X2=0.2295 $Y2=0.036
r27 33 34 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.2025
+ $Y=0.036 $X2=0.216 $Y2=0.036
r28 32 33 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.189
+ $Y=0.036 $X2=0.2025 $Y2=0.036
r29 31 32 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.162
+ $Y=0.036 $X2=0.189 $Y2=0.036
r30 30 31 6.29612 $w=1.3e-08 $l=2.7e-08 $layer=M1 $thickness=3.6e-08 $X=0.135
+ $Y=0.036 $X2=0.162 $Y2=0.036
r31 28 29 4.08082 $w=1.3e-08 $l=1.75e-08 $layer=M1 $thickness=3.6e-08 $X=0.071
+ $Y=0.036 $X2=0.0535 $Y2=0.036
r32 27 28 2.3319 $w=1.3e-08 $l=1e-08 $layer=M1 $thickness=3.6e-08 $X=0.081
+ $Y=0.036 $X2=0.071 $Y2=0.036
r33 26 30 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.1215
+ $Y=0.036 $X2=0.135 $Y2=0.036
r34 25 26 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.108
+ $Y=0.036 $X2=0.1215 $Y2=0.036
r35 20 25 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.0945
+ $Y=0.036 $X2=0.108 $Y2=0.036
r36 20 27 3.14806 $w=1.3e-08 $l=1.35e-08 $layer=M1 $thickness=3.6e-08 $X=0.0945
+ $Y=0.036 $X2=0.081 $Y2=0.036
r37 2 25 19.3796 $a=3.24e-16 $layer=V0LISD $X=0.108 $Y=0.054 $X2=0.108 $Y2=0.036
r38 24 22 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.1253 $Y=0.054 $X2=0.1225 $Y2=0.054
r39 2 22 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.108 $Y=0.054 $X2=0.1225 $Y2=0.054
r40 13 2 0.268519 $w=5.4e-08 $l=1.45e-08 $layer=N_src_drn $thickness=1e-09
+ $X=0.0935 $Y=0.054 $X2=0.108 $Y2=0.054
r41 23 13 0.0462963 $w=5.4e-08 $l=2.5e-09 $layer=N_src_drn $thickness=1e-09
+ $X=0.0906 $Y=0.054 $X2=0.0935 $Y2=0.054
r42 1 16 1e-05 $l=2e-09 $X=0.054 $Y=0.2025 $X2=0.056 $Y2=0.2025
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%10 vss 2 3 1
c1 1 vss 0.00066328f $X=0.162 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1446 $Y=0.2025 $X2=0.162 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1793 $Y=0.2025 $X2=0.162 $Y2=0.2025
.ends

.subckt PM_NOR5xp2_ASAP7_75t_R%11 vss 2 3 1
c1 1 vss 0.000691354f $X=0.216 $Y=0.2025
r1 2 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.1986 $Y=0.2025 $X2=0.216 $Y2=0.2025
r2 3 1 0.209877 $w=8.1e-08 $l=1.7e-08 $layer=P_src_drn $thickness=1e-09
+ $X=0.2333 $Y=0.2025 $X2=0.216 $Y2=0.2025
.ends


* End of included file NOR5xp2_ASAP7_75t_R.pex.sp.pex



*
.subckt NOR5xp2_ASAP7_75t_CUSTOM_4_25_0_7 VSS VDD A B C D E Y
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

M0 N_M0_d N_M0_g VSS VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.071 $Y=0.027
M1 VSS N_M1_g N_M1_s VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.125 $Y=0.027
M2 N_M2_d N_M2_g VSS VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.179 $Y=0.027
M3 VSS N_M3_g N_M3_s VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.233 $Y=0.027
M4 N_M4_d N_M4_g VSS VSS nmos_rvt L=2e-08 W=5.4e-08 nfin=2 $X=0.287 $Y=0.027
M5 N_M5_d N_M5_g N_M5_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.071 $Y=0.162
M6 N_M6_d N_M6_g N_M6_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.125 $Y=0.162
M7 N_M7_d N_M7_g N_M7_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.179 $Y=0.162
M8 N_M8_d N_M8_g N_M8_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.233 $Y=0.162
M9 VDD N_M9_g N_M9_s VDD pmos_rvt L=2e-08 W=8.1e-08 nfin=3 $X=0.287 $Y=0.162


* .include "NOR5xp2_ASAP7_75t_R.pex.sp.pxi"

* Start of included file NOR5xp2_ASAP7_75t_R.pex.sp.pxi
x_PM_NOR5xp2_ASAP7_75t_R%9 vss N_M5_d N_M6_s N_9_1 PM_NOR5xp2_ASAP7_75t_R%9
cc_1 N_9_1 N_A_3 0.0169268f $X=0.108 $Y=0.2025
cc_2 N_9_1 N_B_3 0.0170178f $X=0.108 $Y=0.2025
x_PM_NOR5xp2_ASAP7_75t_R%12 vss N_M8_d N_M9_s N_12_1 PM_NOR5xp2_ASAP7_75t_R%12
cc_3 N_12_1 N_D_3 0.0170112f $X=0.27 $Y=0.2025
cc_4 N_12_1 N_E_3 0.0168579f $X=0.27 $Y=0.2025
x_PM_NOR5xp2_ASAP7_75t_R%A vss A N_M0_g N_M5_g N_A_3 N_A_4 N_A_1
+ PM_NOR5xp2_ASAP7_75t_R%A
x_PM_NOR5xp2_ASAP7_75t_R%B vss B N_M1_g N_M6_g N_B_1 N_B_4 N_B_3
+ PM_NOR5xp2_ASAP7_75t_R%B
cc_5 N_B_1 N_A_3 0.00139668f $X=0.135 $Y=0.1355
cc_6 N_B_4 N_A_4 0.00526408f $X=0.135 $Y=0.0985
cc_7 N_B_3 N_A_3 0.00802853f $X=0.135 $Y=0.0245
x_PM_NOR5xp2_ASAP7_75t_R%C vss C N_M2_g N_M7_g N_C_1 N_C_4 N_C_3
+ PM_NOR5xp2_ASAP7_75t_R%C
cc_8 N_C_1 N_B_1 0.00153523f $X=0.189 $Y=0.1355
cc_9 N_C_4 N_B_4 0.00639887f $X=0.189 $Y=0.0985
cc_10 N_C_3 N_B_3 0.00808377f $X=0.189 $Y=0.0245
x_PM_NOR5xp2_ASAP7_75t_R%D vss D N_M3_g N_M8_g N_D_1 N_D_4 N_D_3
+ PM_NOR5xp2_ASAP7_75t_R%D
cc_11 N_D_1 N_C_1 0.00155217f $X=0.243 $Y=0.1355
cc_12 N_D_4 N_C_4 0.00638622f $X=0.243 $Y=0.0985
cc_13 N_D_3 N_C_3 0.00807584f $X=0.243 $Y=0.0245
x_PM_NOR5xp2_ASAP7_75t_R%E vss E N_M4_g N_M9_g N_E_3 N_E_4
+ PM_NOR5xp2_ASAP7_75t_R%E
cc_14 N_E_3 N_D_1 0.00159392f $X=0.297 $Y=0.0245
cc_15 N_E_4 N_D_4 0.00637011f $X=0.297 $Y=0.0985
cc_16 N_E_3 N_D_3 0.00804398f $X=0.297 $Y=0.0245
x_PM_NOR5xp2_ASAP7_75t_R%Y vss Y N_M0_d N_M1_s N_M2_d N_M3_s N_M4_d N_M5_s
+ N_Y_2 N_Y_1 N_Y_20 N_Y_16 N_Y_13 N_Y_19 N_Y_3 N_Y_14 N_Y_4 N_Y_15
+ PM_NOR5xp2_ASAP7_75t_R%Y
cc_17 N_Y_2 N_A_3 0.000650927f $X=0.108 $Y=0.054
cc_18 N_Y_1 N_A_1 0.000706587f $X=0.054 $Y=0.2025
cc_19 N_Y_20 N_A_4 0.0010319f $X=0.0945 $Y=0.036
cc_20 N_Y_1 N_A_3 0.00171764f $X=0.054 $Y=0.2025
cc_21 N_Y_16 N_A_1 0.00185901f $X=0.056 $Y=0.2025
cc_22 N_Y_13 N_A_3 0.010576f $X=0.0935 $Y=0.054
cc_23 N_Y_19 N_A_4 0.0078329f $X=0.027 $Y=0.0575
cc_24 N_Y_16 N_A_3 0.0499093f $X=0.056 $Y=0.2025
cc_25 N_Y_13 N_B_1 0.000344029f $X=0.0935 $Y=0.054
cc_26 N_Y_2 N_B_3 0.000707855f $X=0.108 $Y=0.054
cc_27 N_Y_20 N_B_4 0.00111271f $X=0.0945 $Y=0.036
cc_28 N_Y_1 N_B_4 0.00216653f $X=0.054 $Y=0.2025
cc_29 N_Y_13 N_B_3 0.0262095f $X=0.0935 $Y=0.054
cc_30 N_Y_20 N_C_4 0.00114195f $X=0.0945 $Y=0.036
cc_31 N_Y_3 N_C_4 0.00172814f $X=0.216 $Y=0.054
cc_32 N_Y_14 N_C_3 0.0264082f $X=0.2015 $Y=0.054
cc_33 N_Y_20 N_D_4 0.0011491f $X=0.0945 $Y=0.036
cc_34 N_Y_3 N_D_4 0.00164474f $X=0.216 $Y=0.054
cc_35 N_Y_14 N_D_3 0.0261579f $X=0.2015 $Y=0.054
cc_36 N_Y_20 N_E_4 0.00123984f $X=0.0945 $Y=0.036
cc_37 N_Y_4 N_E_4 0.00186156f $X=0.324 $Y=0.054
cc_38 N_Y_15 N_E_3 0.0265066f $X=0.3095 $Y=0.054
x_PM_NOR5xp2_ASAP7_75t_R%10 vss N_M6_d N_M7_s N_10_1 PM_NOR5xp2_ASAP7_75t_R%10
cc_39 N_10_1 N_B_3 0.016965f $X=0.162 $Y=0.2025
cc_40 N_10_1 N_C_3 0.0170271f $X=0.162 $Y=0.2025
x_PM_NOR5xp2_ASAP7_75t_R%11 vss N_M7_d N_M8_s N_11_1 PM_NOR5xp2_ASAP7_75t_R%11
cc_41 N_11_1 N_C_3 0.017008f $X=0.216 $Y=0.2025
cc_42 N_11_1 N_D_3 0.0169514f $X=0.216 $Y=0.2025


* End of included file NOR5xp2_ASAP7_75t_R.pex.sp.pxi
.ends
