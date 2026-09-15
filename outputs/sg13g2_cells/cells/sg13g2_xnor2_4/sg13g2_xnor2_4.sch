v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_xnor2_4} -240 -80 0 0 0.4 0.4 {}
N -240 280 -200 280 {}
N -240 320 -200 320 {}
N -100 180 -20 180 {}
N -100 420 -20 420 {}
N -100 540 -20 540 {}
N 20 0 20 120 {}
N 20 0 280 0 {}
N 20 120 20 150 {}
N 20 180 30 180 {}
N 20 210 20 240 {}
N 20 240 20 300 {}
N 20 300 20 360 {}
N 20 300 280 300 {}
N 20 360 20 390 {}
N 20 420 30 420 {}
N 20 450 20 480 {}
N 20 480 20 510 {}
N 20 480 100 480 {}
N 20 540 30 540 {}
N 20 570 20 600 {}
N 20 600 540 600 {}
N 30 140 30 180 {}
N 30 140 100 140 {}
N 30 380 30 420 {}
N 30 380 100 380 {}
N 30 500 30 540 {}
N 30 500 100 500 {}
N 160 180 240 180 {}
N 280 0 280 120 {}
N 280 0 540 0 {}
N 280 120 280 150 {}
N 280 180 290 180 {}
N 280 210 280 240 {}
N 280 240 280 300 {}
N 280 300 360 300 {}
N 290 140 290 180 {}
N 290 140 360 140 {}
N 420 60 500 60 {}
N 420 180 500 180 {}
N 420 420 500 420 {}
N 420 540 500 540 {}
N 540 0 540 30 {}
N 540 0 800 0 {}
N 540 60 550 60 {}
N 540 90 540 120 {}
N 540 120 540 150 {}
N 540 120 620 120 {}
N 540 180 550 180 {}
N 540 210 540 240 {}
N 540 240 540 300 {}
N 540 300 540 360 {}
N 540 300 800 300 {}
N 540 360 540 390 {}
N 540 420 550 420 {}
N 540 450 540 480 {}
N 540 480 540 510 {}
N 540 480 800 480 {}
N 540 540 550 540 {}
N 540 570 540 600 {}
N 540 600 800 600 {}
N 550 20 550 60 {}
N 550 20 620 20 {}
N 550 140 550 180 {}
N 550 140 620 140 {}
N 550 380 550 420 {}
N 550 380 620 380 {}
N 550 500 550 540 {}
N 550 500 620 500 {}
N 680 60 760 60 {}
N 680 540 760 540 {}
N 800 0 800 30 {}
N 800 0 1040 0 {}
N 800 60 810 60 {}
N 800 90 800 120 {}
N 800 120 800 240 {}
N 800 240 800 300 {}
N 800 300 1040 300 {}
N 800 480 800 510 {}
N 800 480 880 480 {}
N 800 540 810 540 {}
N 800 570 800 600 {}
N 800 600 1040 600 {}
N 810 20 810 60 {}
N 810 20 880 20 {}
N 810 500 810 540 {}
N 810 500 880 500 {}
C {lab_wire.sym} 360 300 0 0 {name=l0 sig_type=std_logic lab=net1}
C {lab_wire.sym} 100 480 0 0 {name=l1 sig_type=std_logic lab=net2}
C {lab_wire.sym} 620 120 0 0 {name=l2 sig_type=std_logic lab=net4}
C {lab_wire.sym} 880 480 0 0 {name=l3 sig_type=std_logic lab=net3}
C {lab_wire.sym} 880 20 0 0 {name=l4 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 680 60 0 0 {name=l5 sig_type=std_logic lab=net1}
C {sg13_lv_pmos.sym} 780 60 0 0 {name=MP4 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 140 0 0 {name=l7 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 180 0 0 {name=l8 sig_type=std_logic lab=B}
C {sg13_lv_pmos.sym} 520 180 0 0 {name=MP3 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 20 0 0 {name=l10 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 60 0 0 {name=l11 sig_type=std_logic lab=A}
C {sg13_lv_pmos.sym} 520 60 0 0 {name=MP2 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 140 0 0 {name=l13 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 180 0 0 {name=l14 sig_type=std_logic lab=B}
C {sg13_lv_pmos.sym} 260 180 0 0 {name=MP1 w=840.00n l=130.00n ng=1 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 140 0 0 {name=l16 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 180 0 0 {name=l17 sig_type=std_logic lab=A}
C {sg13_lv_pmos.sym} 0 180 0 0 {name=MP0 w=840.00n l=130.00n ng=1 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 380 0 0 {name=l19 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 420 420 0 0 {name=l20 sig_type=std_logic lab=net1}
C {sg13_lv_nmos.sym} 520 420 0 0 {name=MN3 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 500 0 0 {name=l22 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 540 0 0 {name=l23 sig_type=std_logic lab=A}
C {sg13_lv_nmos.sym} 0 540 0 0 {name=MN1 w=640.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 380 0 0 {name=l25 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l26 sig_type=std_logic lab=B}
C {sg13_lv_nmos.sym} 0 420 0 0 {name=MN0 w=640.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 880 500 0 0 {name=l28 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 680 540 0 0 {name=l29 sig_type=std_logic lab=B}
C {sg13_lv_nmos.sym} 780 540 0 0 {name=MN4 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 620 500 0 0 {name=l31 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 420 540 0 0 {name=l32 sig_type=std_logic lab=A}
C {sg13_lv_nmos.sym} 520 540 0 0 {name=MN2 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos spiceprefix=X}
C {devices/opin.sym} 1040 300 0 0 {name=p1 lab=Y}
C {devices/ipin.sym} -240 280 0 0 {name=p2 lab=A}
C {devices/ipin.sym} -240 320 0 0 {name=p3 lab=B}
C {devices/iopin.sym} 1040 0 0 0 {name=p4 lab=VDD}
C {devices/iopin.sym} 1040 600 0 0 {name=p5 lab=VSS}
