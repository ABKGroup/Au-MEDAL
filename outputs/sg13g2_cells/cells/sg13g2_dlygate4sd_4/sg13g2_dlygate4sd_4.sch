v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_dlygate4sd_4} -240 -80 0 0 0.4 0.4 {}
N -240 180 -60 180 {}
N -60 60 -60 180 {}
N -60 60 -20 60 {}
N -60 180 -60 300 {}
N -60 300 -20 300 {}
N 20 0 20 30 {}
N 20 0 280 0 {}
N 20 60 30 60 {}
N 20 90 20 120 {}
N 20 120 20 180 {}
N 20 180 20 240 {}
N 20 180 100 180 {}
N 20 240 20 270 {}
N 20 300 30 300 {}
N 20 330 20 360 {}
N 20 360 280 360 {}
N 30 20 30 60 {}
N 30 20 100 20 {}
N 30 260 30 300 {}
N 30 260 100 260 {}
N 100 180 200 180 {}
N 200 60 200 180 {}
N 200 60 240 60 {}
N 200 180 200 300 {}
N 200 300 240 300 {}
N 280 0 280 30 {}
N 280 0 540 0 {}
N 280 60 290 60 {}
N 280 90 280 120 {}
N 280 120 280 180 {}
N 280 180 280 240 {}
N 280 180 360 180 {}
N 280 240 280 270 {}
N 280 300 290 300 {}
N 280 330 280 360 {}
N 280 360 540 360 {}
N 290 20 290 60 {}
N 290 20 360 20 {}
N 290 260 290 300 {}
N 290 260 360 260 {}
N 360 180 460 180 {}
N 460 60 460 180 {}
N 460 60 500 60 {}
N 460 180 460 300 {}
N 460 300 500 300 {}
N 540 0 540 30 {}
N 540 0 800 0 {}
N 540 60 550 60 {}
N 540 90 540 120 {}
N 540 120 540 180 {}
N 540 180 540 240 {}
N 540 180 620 180 {}
N 540 240 540 270 {}
N 540 300 550 300 {}
N 540 330 540 360 {}
N 540 360 800 360 {}
N 550 20 550 60 {}
N 550 20 620 20 {}
N 550 260 550 300 {}
N 550 260 620 260 {}
N 620 180 720 180 {}
N 720 60 720 180 {}
N 720 60 760 60 {}
N 720 180 720 300 {}
N 720 300 760 300 {}
N 800 0 800 30 {}
N 800 0 1040 0 {}
N 800 60 810 60 {}
N 800 90 800 120 {}
N 800 120 800 180 {}
N 800 180 800 240 {}
N 800 180 1040 180 {}
N 800 240 800 270 {}
N 800 300 810 300 {}
N 800 330 800 360 {}
N 800 360 1040 360 {}
N 810 20 810 60 {}
N 810 20 880 20 {}
N 810 260 810 300 {}
N 810 260 880 260 {}
C {lab_wire.sym} 100 180 0 0 {name=l0 sig_type=std_logic lab=net1}
C {lab_wire.sym} 360 180 0 0 {name=l1 sig_type=std_logic lab=net2}
C {lab_wire.sym} 620 180 0 0 {name=l2 sig_type=std_logic lab=net3}
C {lab_wire.sym} 880 20 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {sg13_lv_pmos.sym} 780 60 0 0 {name=MP3 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 20 0 0 {name=l5 sig_type=std_logic lab=VDD}
C {sg13_lv_pmos.sym} 520 60 0 0 {name=MP2 w=1.000u l=130.00n ng=1 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 20 0 0 {name=l7 sig_type=std_logic lab=VDD}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=1.000u l=130.00n ng=1 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 20 0 0 {name=l9 sig_type=std_logic lab=VDD}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 880 260 0 0 {name=l11 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 780 300 0 0 {name=MN3 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 620 260 0 0 {name=l13 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 520 300 0 0 {name=MN2 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 360 260 0 0 {name=l15 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 260 300 0 0 {name=MN1 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 260 0 0 {name=l17 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 0 300 0 0 {name=MN0 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos spiceprefix=X}
C {devices/opin.sym} 1040 180 0 0 {name=p1 lab=X}
C {devices/ipin.sym} -240 180 0 0 {name=p2 lab=A}
C {devices/iopin.sym} 1040 0 0 0 {name=p3 lab=VDD}
C {devices/iopin.sym} 1040 360 0 0 {name=p4 lab=VSS}
