v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_mux2_4} -240 -80 0 0 0.4 0.4 {}
N -240 220 -200 220 {}
N -240 260 -200 260 {}
N -240 300 -60 300 {}
N -60 180 -60 300 {}
N -60 180 -20 180 {}
N -60 300 -60 420 {}
N -60 420 -20 420 {}
N 20 0 20 120 {}
N 20 0 280 0 {}
N 20 120 20 150 {}
N 20 180 30 180 {}
N 20 210 20 240 {}
N 20 240 20 300 {}
N 20 300 20 360 {}
N 20 300 100 300 {}
N 20 360 20 390 {}
N 20 420 30 420 {}
N 20 450 20 480 {}
N 20 480 20 600 {}
N 20 600 280 600 {}
N 30 140 30 180 {}
N 30 140 100 140 {}
N 30 380 30 420 {}
N 30 380 100 380 {}
N 160 60 240 60 {}
N 160 180 240 180 {}
N 160 420 240 420 {}
N 160 540 240 540 {}
N 280 0 280 30 {}
N 280 0 540 0 {}
N 280 60 290 60 {}
N 280 90 280 120 {}
N 280 120 280 150 {}
N 280 120 360 120 {}
N 280 180 290 180 {}
N 280 210 280 240 {}
N 280 240 280 300 {}
N 280 300 280 360 {}
N 280 300 540 300 {}
N 280 360 280 390 {}
N 280 420 290 420 {}
N 280 450 280 480 {}
N 280 480 280 510 {}
N 280 480 360 480 {}
N 280 540 290 540 {}
N 280 570 280 600 {}
N 280 600 540 600 {}
N 290 20 290 60 {}
N 290 20 360 20 {}
N 290 140 290 180 {}
N 290 140 360 140 {}
N 290 380 290 420 {}
N 290 380 360 380 {}
N 290 500 290 540 {}
N 290 500 360 500 {}
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
N 540 300 620 300 {}
N 540 360 540 390 {}
N 540 420 550 420 {}
N 540 450 540 480 {}
N 540 480 540 510 {}
N 540 480 620 480 {}
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
N 620 300 720 300 {}
N 720 180 720 300 {}
N 720 180 760 180 {}
N 720 300 720 420 {}
N 720 420 760 420 {}
N 800 0 800 120 {}
N 800 0 1040 0 {}
N 800 120 800 150 {}
N 800 180 810 180 {}
N 800 210 800 240 {}
N 800 240 800 300 {}
N 800 300 800 360 {}
N 800 300 1040 300 {}
N 800 360 800 390 {}
N 800 420 810 420 {}
N 800 450 800 480 {}
N 800 480 800 600 {}
N 800 600 1040 600 {}
N 810 140 810 180 {}
N 810 140 880 140 {}
N 810 380 810 420 {}
N 810 380 880 380 {}
C {lab_wire.sym} 100 300 0 0 {name=l0 sig_type=std_logic lab=Sb}
C {lab_wire.sym} 620 300 0 0 {name=l1 sig_type=std_logic lab=net6}
C {lab_wire.sym} 360 120 0 0 {name=l2 sig_type=std_logic lab=net4}
C {lab_wire.sym} 620 120 0 0 {name=l3 sig_type=std_logic lab=net5}
C {lab_wire.sym} 360 480 0 0 {name=l4 sig_type=std_logic lab=net1}
C {lab_wire.sym} 620 480 0 0 {name=l5 sig_type=std_logic lab=net3}
C {lab_wire.sym} 360 20 0 0 {name=l6 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 60 0 0 {name=l7 sig_type=std_logic lab=S}
C {au_medal_sg13_lv_pmos.sym} 260 60 0 0 {name=MP1 w=1.000u l=130.00n ng=1 model=sg13_lv_pmos spiceprefix=X embed=true}
[
v {xschem version=3.4.8RC file_version=1.3
* Modified 2026-09-15 by Au-MEDAL contributors: removed multiplier
* parameter from netlisting formats, default properties and visible text.
* Original source: IHP-Open-PDK commit 22f2a25f1734796de3debbbf29cf697cbbc54081
*
* Copyright 2023  IHP PDK Authors
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     https://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

}
G {}
K {type=pmos
lvs_format="M@name @pinlist @model w=@w l=@l ng=@ng"
format="@spiceprefix@name @pinlist @model w=@w l=@l ng=@ng"
template="name=M1
l=0.13u
w=0.15u
ng=1
model=sg13_lv_pmos
spiceprefix=X
"
drc="fet_drc @name @symname @model @w @l @ng"
}
V {}
S {}
F {}
E {}
L 4 7.5 -22.5 7.5 22.5 {}
L 4 20 -30 20 -17.5 {}
L 4 20 17.5 20 30 {}
L 4 2.5 -15 2.5 15 {}
L 4 7.5 17.5 20 17.5 {}
L 4 7.5 -17.5 20 -17.5 {}
L 4 -20 -0 2 -0 {}
B 5 17.5 27.5 22.5 32.5 {name=D dir=inout}
B 5 -22.5 -2.5 -17.5 2.5 {name=G dir=in}
B 5 17.5 -32.5 22.5 -27.5 {name=S dir=inout}
B 5 19.921875 -0.078125 20.078125 0.078125 {name=B dir=in}
P 4 4 12.5 -20 7.5 -17.5 12.5 -15 12.5 -20 {fill=true}
P 5 4 15 -2.5 20 0 15 2.5 15 -2.5 {fill=true}
T {@name} 5 -30 0 1 0.2 0.2 {}
T {G} -10 -10 0 1 0.15 0.15 {layer=7}
T {@model} 30 31.25 2 1 0.2 0.2 {}
T {ng=@ng} 31.25 -2.5 0 0 0.2 0.2 { layer=13}
T {l=@l} 31.25 -15 0 0 0.2 0.2 {layer=13}
T {w=@w} 31.25 -26.25 0 0 0.2 0.2 { layer=13}
T {D} 22.5 17.5 0 0 0.15 0.15 {layer=7}
T {S} 22.5 -17.5 2 1 0.15 0.15 {layer=7}
T {B} 20 -10 0 0 0.15 0.15 {layer=7}
]
C {lab_wire.sym} 880 140 0 0 {name=l9 sig_type=std_logic lab=VDD}
C {au_medal_sg13_lv_pmos.sym} 780 180 0 0 {name=MP5 w=4.48u l=130.00n ng=4 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 140 0 0 {name=l11 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 180 0 0 {name=l12 sig_type=std_logic lab=A1}
C {au_medal_sg13_lv_pmos.sym} 520 180 0 0 {name=MP4 w=1.000u l=130.00n ng=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 140 0 0 {name=l14 sig_type=std_logic lab=VDD}
C {au_medal_sg13_lv_pmos.sym} 0 180 0 0 {name=MP0 w=840.00n l=130.00n ng=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 20 0 0 {name=l16 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 420 60 0 0 {name=l17 sig_type=std_logic lab=Sb}
C {au_medal_sg13_lv_pmos.sym} 520 60 0 0 {name=MP3 w=1.000u l=130.00n ng=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 140 0 0 {name=l19 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 180 0 0 {name=l20 sig_type=std_logic lab=A0}
C {au_medal_sg13_lv_pmos.sym} 260 180 0 0 {name=MP2 w=1.000u l=130.00n ng=1 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 620 500 0 0 {name=l22 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 420 540 0 0 {name=l23 sig_type=std_logic lab=S}
C {au_medal_sg13_lv_nmos.sym} 520 540 0 0 {name=MN4 w=740.00n l=130.00n ng=1 model=sg13_lv_nmos spiceprefix=X embed=true}
[
v {xschem version=3.4.8RC file_version=1.3
* Modified 2026-09-15 by Au-MEDAL contributors: removed multiplier
* parameter from netlisting formats, default properties and visible text.
* Original source: IHP-Open-PDK commit 22f2a25f1734796de3debbbf29cf697cbbc54081
*
* Copyright 2024  IHP PDK Authors
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     https://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

}
G {}
K {type=nmos
lvs_format="M@name @pinlist @model w=@w l=@l ng=@ng"
format="@spiceprefix@name @pinlist @model w=@w l=@l ng=@ng"
template="name=M1
l=0.13u
w=0.15u
ng=1
model=sg13_lv_nmos
spiceprefix=X
"
drc="fet_drc @name @symname @model @w @l @ng"
}
V {}
S {}
F {}
E {}
L 4 7.5 -22.5 7.5 22.5 {}
L 4 20 -30 20 -17.5 {}
L 4 20 17.5 20 30 {}
L 4 2.5 -15 2.5 15 {}
L 4 7.5 17.5 20 17.5 {}
L 4 7.5 -17.5 20 -17.5 {}
L 4 -20 -0 2 -0 {}
B 5 17.5 -32.5 22.5 -27.5 {name=D dir=inout}
B 5 -22.5 -2.5 -17.5 2.5 {name=G dir=in}
B 5 17.5 27.5 22.5 32.5 {name=S dir=inout}
B 5 19.921875 -0.078125 20.078125 0.078125 {name=B dir=in}
P 4 4 15 20 20 17.5 15 15 15 20 {fill=true}
P 5 4 20 2.5 15 0 20 -2.5 20 2.5 {fill=true}
T {@name} 5 -30 0 1 0.2 0.2 {}
T {G} -10 -10 0 1 0.15 0.15 {layer=7}
T {@model} 30 31.25 2 1 0.2 0.2 {}
T {ng=@ng} 31.25 -2.5 0 0 0.2 0.2 { layer=13}
T {l=@l} 31.25 -15 0 0 0.2 0.2 {layer=13}
T {w=@w} 31.25 -26.25 0 0 0.2 0.2 { layer=13}
T {S} 22.5 17.5 0 0 0.15 0.15 {layer=7}
T {D} 22.5 -17.5 2 1 0.15 0.15 {layer=7}
T {B} 20 -10 0 0 0.15 0.15 {layer=7}
]
C {lab_wire.sym} 360 500 0 0 {name=l25 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 540 0 0 {name=l26 sig_type=std_logic lab=Sb}
C {au_medal_sg13_lv_nmos.sym} 260 540 0 0 {name=MN2 w=740.00n l=130.00n ng=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 880 380 0 0 {name=l28 sig_type=std_logic lab=VSS}
C {au_medal_sg13_lv_nmos.sym} 780 420 0 0 {name=MN5 w=2.96u l=130.00n ng=4 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 380 0 0 {name=l30 sig_type=std_logic lab=VSS}
C {au_medal_sg13_lv_nmos.sym} 0 420 0 0 {name=MN0 w=550.00n l=130.00n ng=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 620 380 0 0 {name=l32 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 420 420 0 0 {name=l33 sig_type=std_logic lab=A1}
C {au_medal_sg13_lv_nmos.sym} 520 420 0 0 {name=MN3 w=740.00n l=130.00n ng=1 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 360 380 0 0 {name=l35 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 420 0 0 {name=l36 sig_type=std_logic lab=A0}
C {au_medal_sg13_lv_nmos.sym} 260 420 0 0 {name=MN1 w=740.00n l=130.00n ng=1 model=sg13_lv_nmos spiceprefix=X}
C {devices/opin.sym} 1040 300 0 0 {name=p1 lab=X}
C {devices/ipin.sym} -240 220 0 0 {name=p2 lab=A0}
C {devices/ipin.sym} -240 260 0 0 {name=p3 lab=A1}
C {devices/ipin.sym} -240 300 0 0 {name=p4 lab=S}
C {devices/iopin.sym} 1040 0 0 0 {name=p5 lab=VDD}
C {devices/iopin.sym} 1040 600 0 0 {name=p6 lab=VSS}
