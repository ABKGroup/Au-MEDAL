v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_nand3_4} -240 -80 0 0 0.4 0.4 {}
N -240 140 -200 140 {}
N -240 180 -200 180 {}
N -240 220 -200 220 {}
N -100 60 -20 60 {}
N -100 300 -20 300 {}
N -100 420 -20 420 {}
N -100 540 -20 540 {}
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
N 20 600 780 600 {}
N 30 20 30 60 {}
N 30 20 100 20 {}
N 30 260 30 300 {}
N 30 260 100 260 {}
N 30 380 30 420 {}
N 30 380 100 380 {}
N 30 500 30 540 {}
N 30 500 100 500 {}
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
N 540 0 780 0 {}
N 540 60 550 60 {}
N 540 90 540 120 {}
N 540 120 540 180 {}
N 540 180 780 180 {}
N 550 20 550 60 {}
N 550 20 620 20 {}
C {lab_wire.sym} 100 360 0 0 {name=l0 sig_type=std_logic lab=net2}
C {lab_wire.sym} 100 480 0 0 {name=l1 sig_type=std_logic lab=net3}
C {lab_wire.sym} 100 20 0 0 {name=l2 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l3 sig_type=std_logic lab=A}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 20 0 0 {name=l5 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 60 0 0 {name=l6 sig_type=std_logic lab=B}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 20 0 0 {name=l8 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 60 0 0 {name=l9 sig_type=std_logic lab=C}
C {sg13_lv_pmos.sym} 520 60 0 0 {name=MP2 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 380 0 0 {name=l11 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l12 sig_type=std_logic lab=B}
C {sg13_lv_nmos.sym} 0 420 0 0 {name=MN1 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 500 0 0 {name=l14 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 540 0 0 {name=l15 sig_type=std_logic lab=C}
C {sg13_lv_nmos.sym} 0 540 0 0 {name=MN2 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 260 0 0 {name=l17 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 300 0 0 {name=l18 sig_type=std_logic lab=A}
C {sg13_lv_nmos.sym} 0 300 0 0 {name=MN0 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos spiceprefix=X}
C {devices/opin.sym} 780 180 0 0 {name=p1 lab=Y}
C {devices/ipin.sym} -240 140 0 0 {name=p2 lab=A}
C {devices/ipin.sym} -240 180 0 0 {name=p3 lab=B}
C {devices/ipin.sym} -240 220 0 0 {name=p4 lab=C}
C {devices/iopin.sym} 780 0 0 0 {name=p5 lab=VDD}
C {devices/iopin.sym} 780 600 0 0 {name=p6 lab=VSS}
