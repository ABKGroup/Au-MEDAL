v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_buf_32} -240 -80 0 0 0.4 0.4 {}
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
N 280 0 520 0 {}
N 280 60 290 60 {}
N 280 90 280 120 {}
N 280 120 280 180 {}
N 280 180 280 240 {}
N 280 180 520 180 {}
N 280 240 280 270 {}
N 280 300 290 300 {}
N 280 330 280 360 {}
N 280 360 520 360 {}
N 290 20 290 60 {}
N 290 20 360 20 {}
N 290 260 290 300 {}
N 290 260 360 260 {}
C {lab_wire.sym} 100 180 0 0 {name=l0 sig_type=std_logic lab=net1}
C {lab_wire.sym} 100 260 0 0 {name=l1 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 0 300 0 0 {name=MN0 w=550.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 360 260 0 0 {name=l3 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 260 300 0 0 {name=MN1 w=23.68u l=130.00n ng=32 m=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 360 20 0 0 {name=l5 sig_type=std_logic lab=VDD}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=35.84u l=130.00n ng=32 m=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 20 0 0 {name=l7 sig_type=std_logic lab=VDD}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=840.00n l=130.00n ng=1 m=1 model=sg13_lv_pmos spiceprefix=X}
C {devices/opin.sym} 520 180 0 0 {name=p1 lab=X}
C {devices/ipin.sym} -240 180 0 0 {name=p2 lab=A}
C {devices/iopin.sym} 520 0 0 0 {name=p3 lab=VDD}
C {devices/iopin.sym} 520 360 0 0 {name=p4 lab=VSS}
