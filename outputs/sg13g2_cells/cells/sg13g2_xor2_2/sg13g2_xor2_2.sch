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
N 20 360 20 390 {}
N 20 450 20 480 {}
N 280 360 280 390 {}
N 280 450 280 480 {}
N 20 480 20 600 {}
N 280 480 280 600 {}
N 20 240 20 300 {}
N 20 300 20 360 {}
N 280 300 280 360 {}
N 540 0 540 30 {}
N 540 90 540 120 {}
N 800 0 800 30 {}
N 800 90 800 120 {}
N 540 120 540 150 {}
N 540 210 540 240 {}
N 540 120 800 120 {}
N 540 360 540 390 {}
N 540 450 540 480 {}
N 540 480 540 510 {}
N 540 570 540 600 {}
N 800 360 800 390 {}
N 800 450 800 480 {}
N 800 480 800 600 {}
N 540 240 540 300 {}
N 540 300 540 360 {}
N 800 300 800 360 {}
N 20 60 100 60 {}
N -100 60 -20 60 {}
N 20 180 100 180 {}
N -100 180 -20 180 {}
N 20 420 100 420 {}
N -100 420 -20 420 {}
N 280 420 360 420 {}
N 160 420 240 420 {}
N 540 60 620 60 {}
N 420 60 500 60 {}
N 800 60 880 60 {}
N 680 60 760 60 {}
N 540 180 620 180 {}
N 420 180 500 180 {}
N 540 420 620 420 {}
N 420 420 500 420 {}
N 540 540 620 540 {}
N 420 540 500 540 {}
N 800 420 880 420 {}
N 680 420 760 420 {}
N 20 300 280 300 {}
N 280 300 360 300 {}
N 540 300 800 300 {}
N 800 300 1040 300 {}
N -240 280 -200 280 {}
N -240 320 -200 320 {}
N -380 0 20 0 {}
N 20 0 540 0 {}
N 540 0 800 0 {}
N -380 600 20 600 {}
N 20 600 280 600 {}
N 280 600 540 600 {}
N 540 600 800 600 {}
C {lab_wire.sym} 100 60 0 0 {name=l1 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l2 sig_type=std_logic lab=A}
C {lab_wire.sym} 100 180 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 180 0 0 {name=l4 sig_type=std_logic lab=B}
C {lab_wire.sym} 100 420 0 0 {name=l5 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l6 sig_type=std_logic lab=A}
C {lab_wire.sym} 360 420 0 0 {name=l7 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 420 0 0 {name=l8 sig_type=std_logic lab=B}
C {lab_wire.sym} 620 60 0 0 {name=l9 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 60 0 0 {name=l10 sig_type=std_logic lab=A}
C {lab_wire.sym} 880 60 0 0 {name=l11 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 680 60 0 0 {name=l12 sig_type=std_logic lab=B}
C {lab_wire.sym} 620 180 0 0 {name=l13 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 180 0 0 {name=l14 sig_type=std_logic lab=n1}
C {lab_wire.sym} 620 420 0 0 {name=l15 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 420 420 0 0 {name=l16 sig_type=std_logic lab=B}
C {lab_wire.sym} 620 540 0 0 {name=l17 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 420 540 0 0 {name=l18 sig_type=std_logic lab=A}
C {lab_wire.sym} 880 420 0 0 {name=l19 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 680 420 0 0 {name=l20 sig_type=std_logic lab=n1}
C {lab_wire.sym} 360 300 0 0 {name=l21 sig_type=std_logic lab=n1}
C {devices/ipin.sym} -240 280 0 0 {name=p2 lab=A}
C {devices/ipin.sym} -240 320 0 0 {name=p3 lab=B}
C {devices/opin.sym} 1040 300 0 0 {name=p1 lab=X}
C {devices/lab_pin.sym} -380 0 0 0 {name=p4 sig_type=std_logic lab=VDD}
C {devices/lab_pin.sym} -380 600 0 0 {name=p5 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 0 420 0 0 {name=MN0 w=550.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 520 420 0 0 {name=MN3 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 780 420 0 0 {name=MN2 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 520 540 0 0 {name=MN4 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 260 420 0 0 {name=MN1 w=550.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=1.00u l=130.00n ng=1 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 0 180 0 0 {name=MP1 w=1.00u l=130.00n ng=1 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 520 60 0 0 {name=MP2 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 780 60 0 0 {name=MP4 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 520 180 0 0 {name=MP3 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {devices/title-3.sym} -780 1100 0 0 {name=l0 author="Au-MEDAL"}
