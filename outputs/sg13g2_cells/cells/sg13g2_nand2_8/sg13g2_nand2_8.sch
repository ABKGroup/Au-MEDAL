v {xschem version=3.4.7 file_version=1.2
}
G {}
K {}
V {}
S {}
E {}
N 20 0 20 30 {}
N 20 90 20 120 {}
N 280 0 280 30 {}
N 280 90 280 120 {}
N 20 240 20 270 {}
N 20 330 20 360 {}
N 20 360 20 390 {}
N 20 450 20 480 {}
N 20 120 20 180 {}
N 280 120 280 180 {}
N 20 180 20 240 {}
N 20 60 100 60 {}
N -100 60 -20 60 {}
N 280 60 360 60 {}
N 160 60 240 60 {}
N 20 300 100 300 {}
N -100 300 -20 300 {}
N 20 420 100 420 {}
N -100 420 -20 420 {}
N 20 180 280 180 {}
N 280 180 520 180 {}
N -240 160 -200 160 {}
N -240 200 -200 200 {}
N -380 0 20 0 {}
N 20 0 280 0 {}
N -380 480 20 480 {}
C {lab_wire.sym} 100 60 0 0 {name=l1 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l2 sig_type=std_logic lab=A}
C {lab_wire.sym} 360 60 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 60 0 0 {name=l4 sig_type=std_logic lab=B}
C {lab_wire.sym} 100 300 0 0 {name=l5 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 300 0 0 {name=l6 sig_type=std_logic lab=A}
C {lab_wire.sym} 100 420 0 0 {name=l7 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l8 sig_type=std_logic lab=B}
C {devices/ipin.sym} -240 160 0 0 {name=p2 lab=A}
C {devices/ipin.sym} -240 200 0 0 {name=p3 lab=B}
C {devices/opin.sym} 520 180 0 0 {name=p1 lab=Y}
C {devices/lab_pin.sym} -380 0 0 0 {name=p4 sig_type=std_logic lab=VDD}
C {devices/lab_pin.sym} -380 480 0 0 {name=p5 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 0 300 0 0 {name=MN0 w=5.92u l=130.00n ng=8 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 0 420 0 0 {name=MN1 w=5.92u l=130.00n ng=8 m=1 model=sg13_lv_nmos}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=8.96u l=130.00n ng=8 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=8.96u l=130.00n ng=8 m=1 model=sg13_lv_pmos}
C {devices/title-3.sym} -780 980 0 0 {name=l0 author="Au-MEDAL"}
