# 3SX synthetic `:maincpu` telemetry v1

The libretro_EXT `program` space is a read-only, flat byte address space. Multi-byte values are
encoded and returned as unsigned little-endian integers. Reads are assembled byte-by-byte, so
unaligned reads are supported. The valid range is `0x0000..0x004f`; other CPU tags, spaces, or
out-of-range reads return zero. Writes are ignored.

| Offset | Size | Field | Authoritative 3SX source |
|---:|---:|---|---|
| `0x00` | 4 | `version` | telemetry layout version (`1`) |
| `0x04` | 4 | `size` | telemetry byte size (`0x50`) |
| `0x08` | 4 | `frame_number` | low 32 bits of `get_frame_number()` |
| `0x0c` | 4 | `game_state` | normalized from `mpp_w.inGame` |
| `0x10` | 4 | `match_state` | normalized from `mpp_w.inGame`, `G_No`, `Conclusion_Flag`, `PL_Wins`, and configured battle length |
| `0x14` | 4 | `p1_score` | displayed integer `Score[0][Play_Type] + Continue_Coin[0]` |
| `0x18` | 4 | `p2_score` | displayed integer `Score[1][Play_Type] + Continue_Coin[1]` |
| `0x1c` | 1 | `p1_rounds` | `PL_Wins[0]` |
| `0x1d` | 1 | `p2_rounds` | `PL_Wins[1]` |
| `0x1e` | 1 | `p1_character` | native character ID `My_char[0]` |
| `0x1f` | 1 | `p2_character` | native character ID `My_char[1]` |
| `0x20` | 2 | `p1_health` | nonnegative `plw[0].wu.vital_new` |
| `0x22` | 2 | `p2_health` | nonnegative `plw[1].wu.vital_new` |
| `0x24` | 2 | `timer` | unsigned value of gameplay `round_timer` |
| `0x26` | 2 | `reserved0` | zero |
| `0x28` | 4 | `raw_game_state` | bytes `G_No[0..3]`, packed most-significant first |
| `0x2c` | 4 | `raw_match_state` | bytes `C_No[0..3]`, packed most-significant first |
| `0x30` | 32 | `reserved` | zero |

Normalized match states are `0 NONE`, `1 INTRO`, `2 ACTIVE`, `3 ROUND_END`, `4 MATCH_END`,
and `5 RESULTS`. The snapshot is rebuilt after each completed libretro frame and immediately
after successful state restoration. It is derived state and is not serialized separately.
