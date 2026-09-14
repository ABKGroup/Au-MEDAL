v {xschem version=3.4.7 file_version=1.2
}
G {}
K {}
V {}
S {}
E {}
N 20 0 20 30 {}
N 20 90 20 120 {}
N 20 240 20 270 {}
N 20 330 20 360 {}
N 20 120 20 180 {}
N 20 180 20 240 {}
N 280 0 280 30 {}
N 280 90 280 120 {}
N 280 240 280 270 {}
N 280 330 280 360 {}
N 280 120 280 180 {}
N 280 180 280 240 {}
N 20 60 100 60 {}
N 20 300 100 300 {}
N 280 60 360 60 {}
N 280 300 360 300 {}
N -60 60 -20 60 {}
N -60 300 -20 300 {}
N -60 60 -60 180 {}
N -60 180 -60 300 {}
N -240 180 -60 180 {}
N 200 60 240 60 {}
N 200 300 240 300 {}
N 200 60 200 180 {}
N 200 180 200 300 {}
N 20 180 200 180 {}
N 280 180 520 180 {}
N -380 0 20 0 {}
N 20 0 280 0 {}
N -380 360 20 360 {}
N 20 360 280 360 {}
C {lab_wire.sym} 100 60 0 0 {name=l1 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 100 300 0 0 {name=l2 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 360 60 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 360 300 0 0 {name=l4 sig_type=std_logic lab=VSS}
C {devices/ipin.sym} -240 180 0 0 {name=p2 lab=A}
C {devices/opin.sym} 520 180 0 0 {name=p1 lab=X}
C {devices/lab_pin.sym} -380 0 0 0 {name=p3 sig_type=std_logic lab=VDD}
C {devices/lab_pin.sym} -380 360 0 0 {name=p4 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 260 300 0 0 {name=MN1 w=23.68u l=130.00n ng=32 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 0 300 0 0 {name=MN0 w=550.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=840.00n l=130.00n ng=1 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=35.84u l=130.00n ng=32 m=1 model=sg13_lv_pmos}
C {devices/title-3.sym} -780 860 0 0 {name=l0 author="Au-MEDAL"}
