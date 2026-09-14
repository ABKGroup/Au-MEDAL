v {xschem version=3.4.7 file_version=1.2
}
G {}
K {}
V {}
S {}
E {}
N 20 0 20 30 {}
N 20 90 20 120 {}
N 20 120 20 150 {}
N 20 210 20 240 {}
N 20 240 20 270 {}
N 20 330 20 360 {}
N 20 360 20 390 {}
N 20 450 20 480 {}
N 20 600 20 630 {}
N 20 690 20 720 {}
N 280 600 280 630 {}
N 280 690 280 720 {}
N 540 600 540 630 {}
N 540 690 540 720 {}
N 800 600 800 630 {}
N 800 690 800 720 {}
N 20 480 20 540 {}
N 20 540 20 600 {}
N 280 540 280 600 {}
N 540 540 540 600 {}
N 800 540 800 600 {}
N 20 60 100 60 {}
N -100 60 -20 60 {}
N 20 180 100 180 {}
N -100 180 -20 180 {}
N 20 300 100 300 {}
N -100 300 -20 300 {}
N 20 420 100 420 {}
N -100 420 -20 420 {}
N 20 660 100 660 {}
N -100 660 -20 660 {}
N 280 660 360 660 {}
N 160 660 240 660 {}
N 540 660 620 660 {}
N 420 660 500 660 {}
N 800 660 880 660 {}
N 680 660 760 660 {}
N 20 540 280 540 {}
N 280 540 540 540 {}
N 540 540 800 540 {}
N 800 540 1040 540 {}
N -240 480 -200 480 {}
N -240 520 -200 520 {}
N -240 560 -200 560 {}
N -240 600 -200 600 {}
N -380 0 20 0 {}
N -380 720 20 720 {}
N 20 720 280 720 {}
N 280 720 540 720 {}
N 540 720 800 720 {}
C {lab_wire.sym} 100 60 0 0 {name=l1 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l2 sig_type=std_logic lab=A}
C {lab_wire.sym} 100 180 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 180 0 0 {name=l4 sig_type=std_logic lab=B}
C {lab_wire.sym} 100 300 0 0 {name=l5 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 300 0 0 {name=l6 sig_type=std_logic lab=C}
C {lab_wire.sym} 100 420 0 0 {name=l7 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 420 0 0 {name=l8 sig_type=std_logic lab=D}
C {lab_wire.sym} 100 660 0 0 {name=l9 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 660 0 0 {name=l10 sig_type=std_logic lab=A}
C {lab_wire.sym} 360 660 0 0 {name=l11 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 660 0 0 {name=l12 sig_type=std_logic lab=B}
C {lab_wire.sym} 620 660 0 0 {name=l13 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 420 660 0 0 {name=l14 sig_type=std_logic lab=C}
C {lab_wire.sym} 880 660 0 0 {name=l15 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 680 660 0 0 {name=l16 sig_type=std_logic lab=D}
C {devices/ipin.sym} -240 480 0 0 {name=p2 lab=A}
C {devices/ipin.sym} -240 520 0 0 {name=p3 lab=B}
C {devices/ipin.sym} -240 560 0 0 {name=p4 lab=C}
C {devices/ipin.sym} -240 600 0 0 {name=p5 lab=D}
C {devices/opin.sym} 1040 540 0 0 {name=p1 lab=Y}
C {devices/lab_pin.sym} -380 0 0 0 {name=p6 sig_type=std_logic lab=VDD}
C {devices/lab_pin.sym} -380 720 0 0 {name=p7 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 0 660 0 0 {name=MN0 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 780 660 0 0 {name=MN3 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 260 660 0 0 {name=MN1 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 520 660 0 0 {name=MN2 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 0 180 0 0 {name=MP1 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 0 300 0 0 {name=MP2 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 0 420 0 0 {name=MP3 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {devices/title-3.sym} -780 1220 0 0 {name=l0 author="Au-MEDAL"}
