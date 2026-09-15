v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_o22ai_2} -240 -80 0 0 0.4 0.4 {}
N -240 240 -200 240 {}
N -240 280 -200 280 {}
N -240 320 -200 320 {}
N -240 360 -200 360 {}
N -100 60 -20 60 {}
N -100 180 -20 180 {}
N -100 420 -20 420 {}
N -100 540 -20 540 {}
N 20 0 20 30 {}
N 20 0 280 0 {}
N 20 60 30 60 {}
N 20 90 20 120 {}
N 20 120 20 150 {}
N 20 120 100 120 {}
N 20 180 30 180 {}
N 20 210 20 240 {}
N 20 240 20 300 {}
N 20 300 20 360 {}
N 20 300 280 300 {}
N 20 360 20 390 {}
N 20 420 30 420 {}
N 20 450 20 480 {}
N 20 480 20 510 {}
N 20 480 280 480 {}
N 20 540 30 540 {}
N 20 570 20 600 {}
N 20 600 280 600 {}
N 30 20 30 60 {}
N 30 20 100 20 {}
N 30 140 30 180 {}
N 30 140 100 140 {}
N 30 380 30 420 {}
N 30 380 100 380 {}
N 30 500 30 540 {}
N 30 500 100 500 {}
N 160 60 240 60 {}
N 160 180 240 180 {}
N 160 420 240 420 {}
N 160 540 240 540 {}
N 280 0 280 30 {}
N 280 0 520 0 {}
N 280 60 290 60 {}
N 280 90 280 120 {}
N 280 120 280 150 {}
N 280 120 360 120 {}
N 280 180 290 180 {}
N 280 210 280 240 {}
N 280 240 280 300 {}
N 280 300 280 360 {}
N 280 300 520 300 {}
N 280 360 280 390 {}
N 280 420 290 420 {}
N 280 450 280 480 {}
N 280 480 280 510 {}
N 280 480 360 480 {}
N 280 540 290 540 {}
N 280 570 280 600 {}
N 280 600 520 600 {}
N 290 20 290 60 {}
N 290 20 360 20 {}
N 290 140 290 180 {}
N 290 140 360 140 {}
N 290 380 290 420 {}
N 290 380 360 380 {}
N 290 500 290 540 {}
N 290 500 360 500 {}
C {lab_wire.sym} 100 120 0 0 {name=l0 sig_type=std_logic lab=net2}
C {lab_wire.sym} 360 120 0 0 {name=l1 sig_type=std_logic lab=net3}
C {lab_wire.sym} 360 480 0 0 {name=l2 sig_type=std_logic lab=net1}
C {lab_wire.sym} 100 380 0 0 {name=l3 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l4 sig_type=std_logic lab=A1}
C {sg13_lv_nmos.sym} 0 420 0 0 {name=MN0 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 360 380 0 0 {name=l6 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 420 0 0 {name=l7 sig_type=std_logic lab=A2}
C {sg13_lv_nmos.sym} 260 420 0 0 {name=MN1 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 500 0 0 {name=l9 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 540 0 0 {name=l10 sig_type=std_logic lab=B1}
C {sg13_lv_nmos.sym} 0 540 0 0 {name=MN2 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 360 500 0 0 {name=l12 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 540 0 0 {name=l13 sig_type=std_logic lab=B2}
C {sg13_lv_nmos.sym} 260 540 0 0 {name=MN3 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 140 0 0 {name=l15 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 180 0 0 {name=l16 sig_type=std_logic lab=A1}
C {sg13_lv_pmos.sym} 0 180 0 0 {name=MP0 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 20 0 0 {name=l18 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l19 sig_type=std_logic lab=A2}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP1 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 140 0 0 {name=l21 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 180 0 0 {name=l22 sig_type=std_logic lab=B1}
C {sg13_lv_pmos.sym} 260 180 0 0 {name=MP2 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 20 0 0 {name=l24 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 60 0 0 {name=l25 sig_type=std_logic lab=B2}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP3 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos spiceprefix=X}
C {devices/opin.sym} 520 300 0 0 {name=p1 lab=Y}
C {devices/ipin.sym} -240 240 0 0 {name=p2 lab=A1}
C {devices/ipin.sym} -240 280 0 0 {name=p3 lab=A2}
C {devices/ipin.sym} -240 320 0 0 {name=p4 lab=B1}
C {devices/ipin.sym} -240 360 0 0 {name=p5 lab=B2}
C {devices/iopin.sym} 520 0 0 0 {name=p6 lab=VDD}
C {devices/iopin.sym} 520 600 0 0 {name=p7 lab=VSS}
