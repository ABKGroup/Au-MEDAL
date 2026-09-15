# IHP transistor symbols without a multiplier

`generate_ihp_views.py` embeds these two symbols once per model in each generated
schematic. They use distinct `au_medal_` names, so neither the installed IHP
symbols nor a user's Xschem configuration needs modification.

Source: [IHP-Open-PDK](https://github.com/IHP-GmbH/IHP-Open-PDK/tree/22f2a25f1734796de3debbbf29cf697cbbc54081/ihp-sg13g2/libs.tech/xschem/sg13g2_pr),
commit `22f2a25f1734796de3debbbf29cf697cbbc54081`.

The adaptations remove only the multiplier from the SPICE/LVS formats, default
properties and visible text. Pin coordinates, order, device drawing, W/L/ng and
the original copyright notice are retained. See [LICENSE](LICENSE) for the
upstream Apache-2.0 terms.

Original symbol SHA-256:

- `sg13_lv_nmos.sym`: `655fe72d9eb0a760976288e5b80f8710eeae6749ba80c15433f734091b3bea03`
- `sg13_lv_pmos.sym`: `bb552eb4973115c3541bd70457164ee2b49b79bf8bc1c0b85d557812fc1be63d`
