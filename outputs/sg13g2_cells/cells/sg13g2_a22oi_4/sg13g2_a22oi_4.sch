v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
T {sg13g2_a22oi_4} -240 -80 0 0 0.4 0.4 {}
N -240 240 -200 240 {}
N -240 280 -200 280 {}
N -240 320 -200 320 {}
N -240 360 -200 360 {}
N -100 60 -20 60 {}
N -100 180 -20 180 {}
N -100 420 -20 420 {}
N -100 540 -20 540 {}
N 20 0 20 30 {}
N 20 0 280 0 {}
N 20 60 30 60 {}
N 20 90 20 120 {}
N 20 120 20 150 {}
N 20 120 280 120 {}
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
N 20 600 280 600 {}
N 30 20 30 60 {}
N 30 20 100 20 {}
N 30 140 30 180 {}
N 30 140 100 140 {}
N 30 380 30 420 {}
N 30 380 100 380 {}
N 30 500 30 540 {}
N 30 500 100 500 {}
N 160 60 240 60 {}
N 160 180 240 180 {}
N 160 420 240 420 {}
N 160 540 240 540 {}
N 280 0 280 30 {}
N 280 0 520 0 {}
N 280 60 290 60 {}
N 280 90 280 120 {}
N 280 120 280 150 {}
N 280 120 360 120 {}
N 280 180 290 180 {}
N 280 210 280 240 {}
N 280 240 280 300 {}
N 280 300 280 360 {}
N 280 300 520 300 {}
N 280 360 280 390 {}
N 280 420 290 420 {}
N 280 450 280 480 {}
N 280 480 280 510 {}
N 280 480 360 480 {}
N 280 540 290 540 {}
N 280 570 280 600 {}
N 280 600 520 600 {}
N 290 20 290 60 {}
N 290 20 360 20 {}
N 290 140 290 180 {}
N 290 140 360 140 {}
N 290 380 290 420 {}
N 290 380 360 380 {}
N 290 500 290 540 {}
N 290 500 360 500 {}
C {lab_wire.sym} 360 120 0 0 {name=l0 sig_type=std_logic lab=net3}
C {lab_wire.sym} 100 480 0 0 {name=l1 sig_type=std_logic lab=net2}
C {lab_wire.sym} 360 480 0 0 {name=l2 sig_type=std_logic lab=net1}
C {lab_wire.sym} 360 500 0 0 {name=l3 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 540 0 0 {name=l4 sig_type=std_logic lab=B2}
C {au_medal_sg13_lv_nmos.sym} 260 540 0 0 {name=MN0 w=2.96u l=130.00n ng=4 model=sg13_lv_nmos spiceprefix=X embed=true}
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
C {lab_wire.sym} 360 380 0 0 {name=l6 sig_type=std_logic lab=VSS}
C {lab_wire.sym} 160 420 0 0 {name=l7 sig_type=std_logic lab=B1}
C {au_medal_sg13_lv_nmos.sym} 260 420 0 0 {name=MN1 w=2.96u l=130.00n ng=4 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 500 0 0 {name=l9 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 540 0 0 {name=l10 sig_type=std_logic lab=A2}
C {au_medal_sg13_lv_nmos.sym} 0 540 0 0 {name=MN2 w=2.96u l=130.00n ng=4 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 380 0 0 {name=l12 sig_type=std_logic lab=VSS}
C {lab_wire.sym} -100 420 0 0 {name=l13 sig_type=std_logic lab=A1}
C {au_medal_sg13_lv_nmos.sym} 0 420 0 0 {name=MN3 w=2.96u l=130.00n ng=4 model=sg13_lv_nmos spiceprefix=X}
C {lab_wire.sym} 100 140 0 0 {name=l15 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 180 0 0 {name=l16 sig_type=std_logic lab=B1}
C {au_medal_sg13_lv_pmos.sym} 0 180 0 0 {name=MP0 w=4.48u l=130.00n ng=4 model=sg13_lv_pmos spiceprefix=X embed=true}
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
C {lab_wire.sym} 360 140 0 0 {name=l18 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 180 0 0 {name=l19 sig_type=std_logic lab=B2}
C {au_medal_sg13_lv_pmos.sym} 260 180 0 0 {name=MP1 w=4.48u l=130.00n ng=4 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 360 20 0 0 {name=l21 sig_type=std_logic lab=VDD}
C {lab_wire.sym} 160 60 0 0 {name=l22 sig_type=std_logic lab=A2}
C {au_medal_sg13_lv_pmos.sym} 260 60 0 0 {name=MP2 w=4.48u l=130.00n ng=4 model=sg13_lv_pmos spiceprefix=X}
C {lab_wire.sym} 100 20 0 0 {name=l24 sig_type=std_logic lab=VDD}
C {lab_wire.sym} -100 60 0 0 {name=l25 sig_type=std_logic lab=A1}
C {au_medal_sg13_lv_pmos.sym} 0 60 0 0 {name=MP3 w=4.48u l=130.00n ng=4 model=sg13_lv_pmos spiceprefix=X}
C {devices/opin.sym} 520 300 0 0 {name=p1 lab=Y}
C {devices/ipin.sym} -240 240 0 0 {name=p2 lab=A1}
C {devices/ipin.sym} -240 280 0 0 {name=p3 lab=A2}
C {devices/ipin.sym} -240 320 0 0 {name=p4 lab=B1}
C {devices/ipin.sym} -240 360 0 0 {name=p5 lab=B2}
C {devices/iopin.sym} 520 0 0 0 {name=p6 lab=VDD}
C {devices/iopin.sym} 520 600 0 0 {name=p7 lab=VSS}
