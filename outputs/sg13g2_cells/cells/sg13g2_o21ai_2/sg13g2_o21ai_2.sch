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
N 280 0 280 30 {}
N 280 90 280 120 {}
N 280 120 280 240 {}
N 20 360 20 390 {}
N 20 450 20 480 {}
N 20 480 20 510 {}
N 20 570 20 600 {}
N 280 480 280 510 {}
N 280 570 280 600 {}
N 20 480 280 480 {}
N 20 240 20 300 {}
N 280 240 280 300 {}
N 20 300 20 360 {}
N 20 60 100 60 {}
N -100 60 -20 60 {}
N 20 180 100 180 {}
N -100 180 -20 180 {}
N 280 60 360 60 {}
N 160 60 240 60 {}
N 20 420 100 420 {}
N -100 420 -20 420 {}
N 20 540 100 540 {}
N -100 540 -20 540 {}
N 280 540 360 540 {}
N 160 540 240 540 {}
N 20 300 280 300 {}
N 280 300 520 300 {}
N -240 260 -200 260 {}
N -240 300 -200 300 {}
N -240 340 -200 340 {}
N -380 0 20 0 {}
N 20 0 280 0 {}
N -380 600 20 600 {}
N 20 600 280 600 {}
C {lab_wire.sym} 100 60 0 0 {name=l1 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l2 sig_type=std_logic lab=A1}
C {lab_wire.sym} 100 180 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 180 0 0 {name=l4 sig_type=std_logic lab=A2}
C {lab_wire.sym} 360 60 0 0 {name=l5 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 60 0 0 {name=l6 sig_type=std_logic lab=B1}
C {lab_wire.sym} 100 420 0 0 {name=l7 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l8 sig_type=std_logic lab=B1}
C {lab_wire.sym} 100 540 0 0 {name=l9 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 540 0 0 {name=l10 sig_type=std_logic lab=A1}
C {lab_wire.sym} 360 540 0 0 {name=l11 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 540 0 0 {name=l12 sig_type=std_logic lab=A2}
C {devices/ipin.sym} -240 260 0 0 {name=p2 lab=A1}
C {devices/ipin.sym} -240 300 0 0 {name=p3 lab=A2}
C {devices/ipin.sym} -240 340 0 0 {name=p4 lab=B1}
C {devices/opin.sym} 520 300 0 0 {name=p1 lab=Y}
C {devices/lab_pin.sym} -380 0 0 0 {name=p5 sig_type=std_logic lab=VDD}
C {devices/lab_pin.sym} -380 600 0 0 {name=p6 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 0 420 0 0 {name=MN1 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 260 540 0 0 {name=MN0 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 0 540 0 0 {name=MN2 w=1.48u l=130.00n ng=2 m=1 model=sg13_lv_nmos}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP2 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 0 180 0 0 {name=MP1 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=2.24u l=130.00n ng=2 m=1 model=sg13_lv_pmos}
C {devices/title-3.sym} -780 1100 0 0 {name=l0 author="Au-MEDAL"}
