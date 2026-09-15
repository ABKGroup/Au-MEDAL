v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_nand4_2} -240 -80 0 0 0.4 0.4 {}
N -240 120 -200 120 {}
N -240 160 -200 160 {}
N -240 200 -200 200 {}
N -240 240 -200 240 {}
N -100 60 -20 60 {}
N -100 300 -20 300 {}
N -100 420 -20 420 {}
N -100 540 -20 540 {}
N -100 660 -20 660 {}
N 20 0 20 30 {}
N 20 0 280 0 {}
N 20 60 30 60 {}
N 20 90 20 120 {}
N 20 120 20 180 {}
N 20 180 20 240 {}
N 20 180 280 180 {}
N 20 240 20 270 {}
N 20 300 30 300 {}
N 20 330 20 360 {}
N 20 360 20 390 {}
N 20 360 100 360 {}
N 20 420 30 420 {}
N 20 450 20 480 {}
N 20 480 20 510 {}
N 20 480 100 480 {}
N 20 540 30 540 {}
N 20 570 20 600 {}
N 20 600 20 630 {}
N 20 600 100 600 {}
N 20 660 30 660 {}
N 20 690 20 720 {}
N 20 720 1040 720 {}
N 30 20 30 60 {}
N 30 20 100 20 {}
N 30 260 30 300 {}
N 30 260 100 260 {}
N 30 380 30 420 {}
N 30 380 100 380 {}
N 30 500 30 540 {}
N 30 500 100 500 {}
N 30 620 30 660 {}
N 30 620 100 620 {}
N 160 60 240 60 {}
N 280 0 280 30 {}
N 280 0 540 0 {}
N 280 60 290 60 {}
N 280 90 280 120 {}
N 280 120 280 180 {}
N 280 180 540 180 {}
N 290 20 290 60 {}
N 290 20 360 20 {}
N 420 60 500 60 {}
N 540 0 540 30 {}
N 540 0 800 0 {}
N 540 60 550 60 {}
N 540 90 540 120 {}
N 540 120 540 180 {}
N 540 180 800 180 {}
N 550 20 550 60 {}
N 550 20 620 20 {}
N 680 60 760 60 {}
N 800 0 800 30 {}
N 800 0 1040 0 {}
N 800 60 810 60 {}
N 800 90 800 120 {}
N 800 120 800 180 {}
N 800 180 1040 180 {}
N 810 20 810 60 {}
N 810 20 880 20 {}
C {lab_wire.sym} 100 360 0 0 {name=l0 sig_type=std_logic lab=net2}
C {lab_wire.sym} 100 480 0 0 {name=l1 sig_type=std_logic lab=net3}
C {lab_wire.sym} 100 600 0 0 {name=l2 sig_type=std_logic lab=net5}
C {lab_wire.sym} 880 20 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 680 60 0 0 {name=l4 sig_type=std_logic lab=D}
C {sg13_lv_pmos.sym} 780 60 0 0 {name=MP3 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 20 0 0 {name=l6 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l7 sig_type=std_logic lab=A}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 20 0 0 {name=l9 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 60 0 0 {name=l10 sig_type=std_logic lab=B}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 20 0 0 {name=l12 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 60 0 0 {name=l13 sig_type=std_logic lab=C}
C {sg13_lv_pmos.sym} 520 60 0 0 {name=MP2 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 380 0 0 {name=l15 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l16 sig_type=std_logic lab=B}
C {sg13_lv_nmos.sym} 0 420 0 0 {name=MN1 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 500 0 0 {name=l18 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 540 0 0 {name=l19 sig_type=std_logic lab=C}
C {sg13_lv_nmos.sym} 0 540 0 0 {name=MN2 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 260 0 0 {name=l21 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 300 0 0 {name=l22 sig_type=std_logic lab=A}
C {sg13_lv_nmos.sym} 0 300 0 0 {name=MN0 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 620 0 0 {name=l24 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 660 0 0 {name=l25 sig_type=std_logic lab=D}
C {sg13_lv_nmos.sym} 0 660 0 0 {name=MN3 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {devices/opin.sym} 1040 180 0 0 {name=p1 lab=Y}
C {devices/ipin.sym} -240 120 0 0 {name=p2 lab=A}
C {devices/ipin.sym} -240 160 0 0 {name=p3 lab=B}
C {devices/ipin.sym} -240 200 0 0 {name=p4 lab=C}
C {devices/ipin.sym} -240 240 0 0 {name=p5 lab=D}
C {devices/iopin.sym} 1040 0 0 0 {name=p6 lab=VDD}
C {devices/iopin.sym} 1040 720 0 0 {name=p7 lab=VSS}
