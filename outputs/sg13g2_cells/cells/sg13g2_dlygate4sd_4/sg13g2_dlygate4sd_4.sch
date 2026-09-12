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
N 540 0 540 30 {}
N 540 90 540 120 {}
N 540 240 540 270 {}
N 540 330 540 360 {}
N 540 120 540 180 {}
N 540 180 540 240 {}
N 800 0 800 30 {}
N 800 90 800 120 {}
N 800 240 800 270 {}
N 800 330 800 360 {}
N 800 120 800 180 {}
N 800 180 800 240 {}
N 20 60 100 60 {}
N 20 300 100 300 {}
N 280 60 360 60 {}
N 280 300 360 300 {}
N 540 60 620 60 {}
N 540 300 620 300 {}
N 800 60 880 60 {}
N 800 300 880 300 {}
N -60 60 -20 60 {}
N -60 300 -20 300 {}
N -60 60 -60 180 {}
N -60 180 -60 300 {}
N -240 180 -60 180 {}
N 200 60 240 60 {}
N 200 300 240 300 {}
N 200 60 200 180 {}
N 200 180 200 300 {}
N 460 60 500 60 {}
N 460 300 500 300 {}
N 460 60 460 180 {}
N 460 180 460 300 {}
N 720 60 760 60 {}
N 720 300 760 300 {}
N 720 60 720 180 {}
N 720 180 720 300 {}
N 20 180 200 180 {}
N 280 180 460 180 {}
N 540 180 720 180 {}
N 800 180 1040 180 {}
N -380 0 20 0 {}
N 20 0 280 0 {}
N 280 0 540 0 {}
N 540 0 800 0 {}
N -380 360 20 360 {}
N 20 360 280 360 {}
N 280 360 540 360 {}
N 540 360 800 360 {}
C {lab_wire.sym} 100 60 0 0 {name=l1 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 100 300 0 0 {name=l2 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 360 60 0 0 {name=l3 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 360 300 0 0 {name=l4 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 620 60 0 0 {name=l5 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 620 300 0 0 {name=l6 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 880 60 0 0 {name=l7 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 880 300 0 0 {name=l8 sig_type=std_logic lab=VSS}
C {devices/ipin.sym} -240 180 0 0 {name=p2 lab=A}
C {devices/opin.sym} 1040 180 0 0 {name=p1 lab=X}
C {devices/lab_pin.sym} -380 0 0 0 {name=p3 sig_type=std_logic lab=VDD}
C {devices/lab_pin.sym} -380 360 0 0 {name=p4 sig_type=std_logic lab=VSS}
C {sg13_lv_nmos.sym} 0 300 0 0 {name=MN0 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 260 300 0 0 {name=MN1 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 520 300 0 0 {name=MN2 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_nmos}
C {sg13_lv_nmos.sym} 780 300 0 0 {name=MN3 w=2.96u l=130.00n ng=4 m=1 model=sg13_lv_nmos}
C {sg13_lv_pmos.sym} 0 60 0 0 {name=MP0 w=420.00n l=130.00n ng=1 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=1.00u l=130.00n ng=1 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 520 60 0 0 {name=MP2 w=1.00u l=130.00n ng=1 m=1 model=sg13_lv_pmos}
C {sg13_lv_pmos.sym} 780 60 0 0 {name=MP3 w=4.48u l=130.00n ng=4 m=1 model=sg13_lv_pmos}
C {devices/title-3.sym} -780 860 0 0 {name=l0 author="Au-MEDAL"}
