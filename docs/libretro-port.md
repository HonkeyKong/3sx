# 3SX libretro port

## Status and architecture

`3sx_libretro` is a loadable libretro shared library; the existing `3sx` SDL3
application remains separate. Both call `Main_Init()`, `Main_StepFrame()`, and
`Main_FinishFrame()`. The core excludes the SDL app/event loop, SDL controller
discovery, window/GPU/OpenGL backends, ImGui, GekkoNet, SDL_net, and matchmaking.

The existing `core/input.c` and `core/renderer.c` dispatch points select two
libretro joypads and a windowless 384x224 XRGB8888 software framebuffer. Path
and direct-content setters provide frontend directories and the selected AFS.
Effects audio calls the existing 48 kHz `SPU_Tick()` mixer; a fractional
samples-per-frame accumulator supplies batches at 59.59949 Hz.

## Audit findings

The original CMake globbed all C sources into one executable. Debug builds add
GekkoNet/SDL_net; rendering selects SDL_GPU then OpenGL. The SDL callback entry
point is `src/platform/app/sdl/sdl_app.c`. Its initialized frame order is:

1. `AFS_RunServer()`
2. `Main_StepFrame()` (input, tasks, drawing, `flFlip()`)
3. `ADX_ProcessTracks()`, overlays, presentation, and SDL pacing
4. `Main_FinishFrame()` (interrupt timer, screen bookkeeping, BGM server)

`Main_Init()` owns initialization. `flInitialize()` allocates a 24 MiB FMS
arena and 10 MiB sub-arena; subsequent game heaps come from it. Tasks and most
gameplay state are global/static. Standalone historically relied on process
exit. `Main_Shutdown()` now closes audio/input and releases the top-level FMS
allocation, but all static reload invariants still need valid-content testing.

Rendering uses `Renderer_*`. SDL_GPU/OpenGL upload PS2-style textures and
palettes, collect quads, and render a 384x224 canvas. `flFlip()` flushes temp
memory and runs the sound server; it does not swap the window. The libretro
backend snapshots uploads and rasterizes queued quads into a persistent buffer,
without a graphics context, presentation call, or per-frame framebuffer alloc.

Input flows `Input_ButtonState` -> `scePad2Read()` -> `tarPADRead()` ->
`flPADGetALL()` -> `keyConvert()`. Libretro polls once per frame and fills both
slots.

| SDL-style physical input | Game function |
|---|---|
| D-pad | Direction |
| A / B / X | X / Y / R |
| Y / LB / RB | A / B / RT |
| Start | Start |
| Back | PS2 Select (service/menu where used) |

In libretro's RetroPad naming, those six physical inputs are B / A / Y / X / L / R.

Effects audio is 48 kHz signed 16-bit stereo. Standalone drives it with an SDL
audio callback under a mutex; libretro drives it in `retro_run()`. ADX music is
a separate SDL audio stream in standalone. Device output is disabled in the
core, so music is currently a known limitation.

Game, SDL-compatibility, and fatal-error logging is routed through the
frontend-provided libretro log callback when available. Without that interface,
the core falls back to standard output/error streams.

Standalone resources use SDL's preference path and can extract `SF33RD.AFS`
from ISO using libiso9660. The core advertises only `afs`, requires a full path,
validates the archive, and never invokes native dialogs or ISO extraction.
Frontend paths resolve to `<system>/3sx/` and `<save>/3sx/`; configuration goes
to the save path. Game data is not copied.

SDL remains linked for widespread utility APIs: IO streams, allocation/string/
endian helpers, a mixer mutex, filesystem checks, and logging. The core never
initializes SDL video/audio/gamepad/events and opens no SDL window or device.
SDL timing/delay code is standalone-only. The core creates no SDL thread.

GekkoNet is excluded and `NETPLAY_ENABLED` is unset. The reserved option
`3SX_LIBRETRO_INTERNAL_NETPLAY` defaults off. Checked-in generated inputs are
decompiled C tables and `port/sound/interp_table.inc`; no proprietary asset was
added.

Legacy fatal helpers and decompilation-era infinite assertions remain in shared
code. Expected missing/invalid-content paths are recoverable, but malformed
data reaching deeper legacy assertions may still abort the frontend.

## Content, persistence, and state

Only a legally obtained `SF33RD.AFS` is supported. ISO/no-content modes are not
advertised. No `RETRO_MEMORY_SAVE_RAM` or `RETRO_MEMORY_SYSTEM_RAM` is exposed:
no stable safe contiguous region is identified. Configuration is the only
confirmed routed persistent data.

Experimental rollback states are exposed through the standard libretro
serialize/unserialize API. Outside active gameplay they contain only the input
history and interrupt timer. The versioned format contains same-process pointers and is intended
for short-lived rollback buffers, not persistent or cross-version save files.

Peer checksums must use the optional export
`uint32_t retro_get_state_hash(const void *data, size_t size)` instead of hashing
serialized bytes directly. It canonicalizes known pointers so ASLR does not
create false mismatches.

The `3sx_start_mode` core option defaults to `Normal`. Selecting `Online Only`
skips the warning, opening, title, and attract/menu flow after core initialization and enters the
arcade-style two-human-player character-select flow. The frontend remains
responsible for exchanging inputs and rollback states; no internal networking
backend is started.

## Building

```bash
cmake -S . -B build-libretro -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -D3SX_BUILD_LIBRETRO=ON -D3SX_BUILD_STANDALONE=OFF
cmake --build build-libretro -j"$(nproc)"

cmake -S . -B build-all -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -D3SX_BUILD_LIBRETRO=ON -D3SX_BUILD_STANDALONE=ON
cmake --build build-all -j"$(nproc)"
```

Run `build-deps.sh` first in a normal checkout. The core can use system SDL3 if
the bundled dependency build is absent.

## Testing

```bash
nm -D --defined-only build-libretro/3sx_libretro.so | grep ' retro_'
ldd build-libretro/3sx_libretro.so
retroarch -L build-libretro/3sx_libretro.so /path/to/SF33RD.AFS
```

Verify metadata, graceful missing-content failure, one video and audio batch per
run, both ports, then unload/reload. Use user-supplied legal data. ASan/UBSan and
repeated reload tests are recommended.

## Known limitations and next steps

- ADX music is not mixed into libretro output; effects audio is.
- The GLES renderer needs visual comparison for skewed primitives and PS2
  alpha/color edge cases.
- ISO, no-content, rumble, save memory, and persistent save states
  are absent.
- Only Linux x86_64 was built. Windows, macOS, and AArch64 are unvalidated.
- Valid-content video/audio and reload testing was not possible without game data.

Rollback state currently mirrors the existing GekkoNet gameplay/effect state and
adds the interrupt timer and input history. Texture-cache state is deliberately
not embedded: the libretro renderer drops stale deferred draw requests whose MTS
slot was released across a rollback boundary, allowing the normal resource
lifecycle to recover without inflating every state. SPU state and in-flight
streaming transitions still need coverage and deterministic replay tests.
