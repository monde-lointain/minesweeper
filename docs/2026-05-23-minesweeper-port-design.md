# Cross-Platform Minesweeper Port — Design / Spec

## Context

Port the classic Windows Minesweeper (`~/development/projects/c/winmine`, Win32 C) to a
cross-platform C++20 application. The Win32 source is fully understood: board logic in
`rtns.c`, drawing in `grafix.c`, prefs in registry (`pref.c`), assets as palettized BMP
sprite sheets + WAVs in `winmine/bmp/`.

Goal: a faithful, **full-parity** reimplementation (minus the XYZZY cheat) that runs on
Linux/macOS/Windows, reusing the **original graphics**, built on (pinned GitHub tags via
FetchContent):
- **SDL3** `release-3.4.8` — window, renderer, input
- **SDL_mixer** `release-3.2.2` — WAV audio playback (tick / win / explode)
- **Dear ImGui** `v1.92.8` (`imgui_impl_sdl3` + `imgui_impl_sdlrenderer3`) — menu bar + dialogs only
- **mINI** `0.9.18` (`src/mini/ini.h`, header-only) — settings persistence replacing the registry
- **CMake ≥ 3.28**, **C++20**, based on `~/development/projects/cpp/cpp-template`
- **Orthodox C++** style, enforced by the **Orthodoxy** Clang plugin, built with **`clang++-22`**
  (the version Orthodoxy's `.so` is compiled against)

Decisions locked with the user:
- Scope: **full parity, no cheat** (3 difficulties + custom, best times, sound, ?-marks,
  color/B&W toggle, chord/both-button reveal, first-click safety, window-position memory)
- Rendering: **SDL3 board + ImGui chrome**
- Assets: **copy original BMP/WAV into repo, load at runtime**
- Dependencies: **FetchContent from GitHub** (pinned tags)

## Architecture

Separation of concerns, with the pure logic isolated so it is unit-testable without SDL:

```
src/
  game/        Pure game logic — NO SDL/ImGui. Unit-tested with GoogleTest.
  assets/      Load BMP sprite sheets -> SDL_Texture; load WAVs. Sub-rect tables.
  render/      Draw board, LED counters, smiley, 3D bevels via SDL_Renderer.
  audio/       SDL3 audio: tick / win / explode WAV playback.
  config/      Load/save settings via mINI (replaces registry).
  ui/          ImGui main menu bar + dialogs (Custom, Best Times, Enter Name, About).
  app/         SDL init, window/renderer lifecycle, event loop, glue. main.cc.
```

Header layout mirrors template: `include/minesweeper/<module>.h`, impl in `src/<module>/`.
Static lib `minesweeper_lib` (everything except `main.cc`) + `minesweeper` executable, so
tests link the lib. (Mirrors template's `myproject_lib` + `myproject` pattern.)

**Code structure (Orthodox):** no C++ namespace (drop the template's `namespace myproject`);
module-prefixed free functions (`game_*`, `render_*`, `config_*`, `audio_*`, `assets_*`,
`ui_*`, `app_*`), POD structs `CamelCase`, plain enums with `UPPER_SNAKE` members.

### Module: `game` (pure, testable)

Mirrors `rtns.c` logic in Orthodox C style. POD board + free functions taking `Board*`.

- Board: fixed max buffer (original uses 32×27 = 864). Reuse the bit-packed cell idea or a
  plain `struct Cell { u8 adjacent; bool mine, revealed; enum FlagState flag; }` array — plain
  struct preferred for clarity/testability (allocation irrelevant at this size).
- State enum: `enum CellFlag { FLAG_NONE, FLAG_MINE, FLAG_QUESTION }`.
- Functions (errno-style returns where fallible):
  - `game_reset(Board*, w, h, mines, RngFn, rng_ctx)` — clear, defer mine placement.
  - `game_place_mines(Board*, avoid_x, avoid_y)` — placed on first reveal so first click is
    always safe (subsumes original's "move bomb on first click").
  - `game_reveal(Board*, x, y)` — flood-fill (iterative queue, like `StepBox`), returns hit-mine /
    win / continue status.
  - `game_chord(Board*, x, y)` — both-button reveal of neighbours when flag count matches.
  - `game_cycle_flag(Board*, x, y, bool marks_enabled)` — none -> flag -> (?) -> none.
  - `game_status(const Board*)`, mine/flag counters, elapsed-time accessor.
- **Chord**: on a revealed number, reveal all non-flagged neighbours iff neighbour flag count
  == number; otherwise reveal them and detonate if any flag is wrong. `?`-marks never count as
  flags. Mine counter = mines - flags, may go negative (rendered with the negative LED glyph),
  clamped to 3 digits.
- **End states**: loss reveals all mines, marks wrong flags (`iBlkWrong`) and the detonated mine
  (`iBlkExplode`); win auto-flags the remaining mines. Best times: only Beginner/Intermediate/
  Expert qualify (not Custom); record when time < stored; defaults 999 / "Anonymous".
- RNG injected via function pointer for deterministic tests; production uses a tiny C-style
  xorshift PRNG seeded from time (no `<random>`).
- Difficulty table: Beginner 9×9/10, Intermediate 16×16/40, Expert 16×30/99 (W=30, H=16).
  Custom clamps H 9-24, W 9-30, mines 10..(W·H − 1). Board uses a fixed buffer sized for the
  30×24 max plus a 1-cell border sentinel ring (no dynamic allocation).

### Module: `assets`

- `SDL_LoadBMP` each sheet -> `SDL_CreateTextureFromSurface`. Color and B&W sheets both
  loaded; active one chosen by `config.color`.
- Sub-rect tables: blocks 16 tiles @16×16; led 12 digits @13×23; button 5 faces @24×24.
- Resolve asset dir via `SDL_GetBasePath()` + `assets/` (CMake copies assets next to exe).
- Window icon set from `winmine.ico` if it loads cleanly, else skipped.

### Module: `render`

- Reproduce original layout/offsets from `grafix.h` (12px borders, LED panel, smiley center,
  board origin). Win95 chrome palette: face `192,192,192`, highlight `255,255,255`, shadow
  `128,128,128`, dark `0,0,0`. Only the **outer frame + LED panel bevels** are hand-drawn; cell
  tiles already bake in their own bevels, so the board interior is pure sprite blits. B&W mode
  swaps only the sprite set; chrome stays gray.
- Compute window size from board dims; below the ImGui main-menu-bar height. Call
  `SDL_SetWindowSize` on new game / difficulty change.
- **Integer scaling**: all sprite/layout dimensions multiplied by `settings.scale` (default 2,
  adjustable via menu, persisted). Use `SDL_SetTextureScaleMode(..., SDL_SCALEMODE_NEAREST)` for
  crisp pixels; convert mouse coords back by dividing by scale. Window size scales accordingly.
- Draw order each frame: clear -> board+chrome via SDL_Renderer -> ImGui menu/dialogs -> present.

### Module: `audio`

- **SDL_mixer**: `Mix_OpenAudio` at startup; `Mix_LoadWAV` the WAVs; `Mix_PlayChannel` on game
  end only — **win.wav / explode.wav**. **No per-second tick** (matches the released game;
  `tick.wav` left unused). Respect `config.sound`.

### Module: `config` (mINI, thin C-style wrapper)

- **All mINI/STL/exception use confined to one `config.cc`.** Public interface is pure Orthodox:
  a POD `struct Settings` (ints, fixed `char name[3][32]` buffers) + `config_load(Settings*)` /
  `config_save(const Settings*)` returning errno-style codes. mINI's `std::string`/`INIStructure`
  never crosses the module boundary; the rest of the codebase sees only `Settings`.
- INI at platform pref dir (`SDL_GetPrefPath("", "winmine")`), sections `[game]` / `[window]` /
  `[scores]`. Keys mirror registry fields: difficulty, custom W/H/mines, sound, marks, color,
  **scale factor**, menu-visible, window x/y, best times[3] + names[3].
- Load at startup (defaults if missing), save on change + on exit. Restored window position is
  clamped on-screen.

### Module: `ui` (ImGui)

- **Theme**: `ImGui::StyleColorsLight` tuned toward Win95 gray so chrome blends with the board frame.
- `BeginMainMenuBar`: **Game** (New F2, Beginner, Intermediate, Expert, Custom, Marks ✓, Color ✓,
  Sound ✓, Scale 1x–4x, Best Times…, Exit) and **Help** (About).
- **Hide-menu toggle** (F5/F6, like original): hides the ImGui main menu bar; board reclaims that
  vertical space and the window shrinks accordingly. Persisted.
- Modal dialogs: Custom field (H/W/mines inputs), Best Times (3 rows + Reset), Enter Name
  (on new record), About.

### Module: `app`

- **SDL3 callback entry** (`#define SDL_MAIN_USE_CALLBACKS`): implement `SDL_AppInit`,
  `SDL_AppIterate`, `SDL_AppEvent`, `SDL_AppQuit` (no hand-written event loop). `main.cc` is just
  the SDL_main glue + these callbacks wiring the modules.
- `SDL_AppInit`: init SDL video+audio, create **fixed-size (non-resizable) window** + renderer,
  init ImGui SDL3 + SDLRenderer3 backends, load config/assets. Window resizes only
  programmatically (difficulty / scale / hide-menu).
- `SDL_AppEvent`: feed events to `ImGui_ImplSDL3_ProcessEvent`; when ImGui doesn't capture the
  mouse, translate clicks (divided by scale) to board coords and drive `game`. Left=reveal,
  right=cycle flag, both/middle=chord. Press feedback: left-down presses the covered cell (blank
  tile) and switches the smiley to the caution face until release; dragging moves the pressed
  cell; release off-board cancels. Track smiley states (happy/caution/lose/win/pressed).
  Handle F2 (new game), F5/F6 (hide/show menu), and `SDL_EVENT_WINDOW_*` for auto-pause.
- `SDL_AppIterate`: **continuous vsync redraw** each frame (new-frame ImGui → board+chrome →
  menu/dialogs → present). **Timer** starts on first reveal, increments per real second
  (`SDL_GetTicks`), capped at 999; **auto-pauses** while minimized/unfocused.

## Build System

Base on `cpp-template`; key changes:
1. `cmake_minimum_required(VERSION 3.28)`; project renamed `minesweeper`.
2. **Dependencies.cmake**: add FetchContent for SDL3, SDL_mixer, Dear ImGui, mINI, declared
   `SYSTEM` (3.25+) to silence dep warnings under `-Werror`. Pin GIT_TAG: SDL `release-3.4.8`,
   SDL_mixer `release-3.2.2`, imgui `v1.92.8`, mINI `0.9.18`. Make **SDL3 available before
   SDL_mixer** (SDL_mixer reuses the in-tree `SDL3::SDL3` target only if it already exists).
   - SDL_mixer: disable all decoders except WAVE (`SDLMIXER_WAVE=ON`, `SDLMIXER_FLAC/MP3/MOD/
     MIDI/OPUS/VORBIS_*/GME/AIFF/VOC/AU/WAVPACK=OFF`) so no codec submodules/system libs are
     needed; `SDLMIXER_TESTS/EXAMPLES/INSTALL=OFF`. Link `SDL3_mixer::SDL3_mixer`.
   - ImGui ships no CMake target: define an `imgui` static lib compiling imgui core + the two
     backends, with SDL3 include dirs.
   - mINI: INTERFACE target exposing `src/mini`.
3. **Orthodoxy**: `find_package(orthodoxy CONFIG OPTIONAL_COMPONENTS plugin)`; if found, apply
   `orthodoxy::plugin` (`-fplugin`) **only to `minesweeper_lib`/`minesweeper` targets** (not to
   SDL/SDL_mixer/ImGui — so dep *libraries* never see the plugin). **Confirmed installed**:
   `/usr/local/share/cmake/orthodoxy/orthodoxyConfig.cmake` + Clang-22 plugin at
   `/usr/local/lib/orthodoxy/22/orthodoxy.so`; `clang++-22` at `/usr/bin/clang++-22`. So configure
   with `-DCMAKE_CXX_COMPILER=clang++-22` (no `CMAKE_PREFIX_PATH` needed). Plugin is OPTIONAL so
   other-compiler builds still work (without enforcement).
   - **Header-scoping caveat (verified in Orthodoxy source):** the plugin does NOT skip system
     headers; it diagnoses every declaration in our TUs and selects rules by the *file location*
     of each declaration via the nearest `.orthodoxy.yml` (parent-dir search). Dep headers
     (`build/_deps/...`) we `#include` would otherwise inherit a root config. Therefore place the
     strict `.orthodoxy.yml` **inside `src/` and `include/minesweeper/` only**, with
     `InheritParent: false`, and keep no strict config at any level the deps' files inherit from.
     **Empirically verify** (early build spike) that ImGui/mINI headers produce no diagnostics and
     our code does. If deps still trip, drop a permissive (all-rules-allow) `.orthodoxy.yml` into
     the FetchContent base dir.
4. **.clang-tidy**: loosen for Orthodox C++ — disable `modernize-*`, the cast/ownership
   `cppcoreguidelines-pro-*` and `cppcoreguidelines-owning-memory`, and retune
   `readability-identifier-naming` to Orthodox conventions (lower_case funcs/vars, struct
   CamelCase, `UPPER_SNAKE` enum members, no namespace, C-style casts, `malloc`/`free`, no
   `auto`). Keep `bugprone-*`, `performance-*`, `portability-*`, `clang-analyzer-*`.
   The **tests target gets neither the Orthodoxy plugin nor strict tidy** (GoogleTest `TEST()`
   macros expand to classes/inheritance).
5. Copy `winmine/bmp/*.bmp` + `*.wav` into `assets/`; CMake copies `assets/` to
   `$<TARGET_FILE_DIR:minesweeper>/assets` (post-build) so `make run` finds them beside the exe.
6. **Trim template tooling**: keep build + tests + `format` + `tidy` + Orthodoxy. Drop Doxygen
   docs, CPack packaging, coverage, and the `cppcheck-rules` submodule (and its `.gitmodules`
   entry / `external/` dir).
7. **.clang-format**: plain Google style (`BasedOnStyle: Google`), no overrides.

## Orthodox C++ / Interop Notes

- Our code: POD structs, free functions, pointers, C-style casts, C headers, `printf`,
  errno-style returns, index loops, plain enums, no exceptions/RTTI/auto/lambdas.
- ImGui's C++ API (references, default args, overloads) is unavoidable at the boundary: keep
  those calls confined to `ui`/`app`, suppress per-line with `/* HERESY(rule-name) */` per the
  project's pragmatism clause. SDL is a C API — no friction.
- **Spike findings (verified):** Orthodoxy resolves rules per declaration's *file location*, so
  including `imgui.h`/`mini/ini.h` into strict-config TUs does NOT flag the deps' own classes/
  templates/refs (they live under build/_deps → permissive root config). The plugin only flags
  constructs WE write. Practical rules for boundary code: (1) **calling** overloaded/default-arg
  ImGui functions is fine (not flagged — only *defining* them is); (2) avoid declaring C++
  references — capture ImGui reference-returns as pointers, e.g.
  `ImGuiIO *io = &ImGui::GetIO();` (taking the address declares no reference); (3) for genuinely
  unavoidable references/casts, append `/* HERESY(lvalue-reference) */` etc. Orthodoxy config
  polarity: **`false` = disallow** the feature, `true`/unset = allow.

## Testing & Verification

- **Unit tests (GoogleTest, already in template)** on `game`: mine placement count + first-click
  safety, flood-fill reveal of zero regions, adjacency counts, flag cycling with/without marks,
  chord behaviour, win detection (all non-mines revealed) and loss (mine hit), counter math.
  Deterministic via injected RNG. Plus a **headless `config` round-trip** test (save `Settings`
  → load → assert equal), SDL-free. Both test targets are plugin-free.
- **Manual end-to-end**: `make run`; play each difficulty, custom dialog, win/loss, best-time
  entry + persistence across restart (inspect INI), sound on/off, color/B&W toggle, marks toggle,
  window-position memory, board resize on difficulty change.
- **Style gate**: build with `clang++-22` and confirm Orthodoxy emits no diagnostics on our
  modules (only suppressed boundary lines). `make tidy` clean under loosened config.
- Build on Linux first; macOS/Windows are best-effort cross-platform via SDL3 (no platform code).
- **CI**: GitHub Actions workflow — configure with `clang++-22`, build, run `ctest` and `tidy` on
  Linux. (Installs SDL build deps; Orthodoxy plugin enforcement runs where available.)

## Execution

- **One-shot build**: implement the full project end-to-end, then hand back a complete, building,
  tested repo (not milestone-gated).
- **Process**: this plan is the spec; on approval, first commit it into the repo (`docs/`), then
  implement. (`git init` the new project; the template's `.git` is the template's own.)

---

# Parallel Execution Topology

**Mandatory prerequisites (do not bypass):** `superpowers:dispatching-parallel-agents`,
`superpowers:using-git-worktrees`. Each fan-out stream runs in its own git worktree branched off
the merged foundation commit. Merge order enforces barriers below.

**Ownership invariant (R8):** Stream A owns ALL build files and ALL headers and freezes them
before fan-out. Fan-out streams own ONLY their `src/<module>/*.cc` and their `tests/*_test.cc`.
No fan-out stream edits any `CMakeLists.txt`, any `.h`, or any config file. Header/contract change
required → re-surface (interface drift), do not edit in a fan-out stream.

**Reference paths (include verbatim in EVERY dispatched agent prompt — A, B.*, C.*):** these are
read-only references for exact APIs and original game behavior; agents must consult them rather
than guess signatures or semantics.
- Original Minesweeper (game behavior, layout consts, asset specs): `~/development/projects/c/winmine/`
  (logic `rtns.c`, drawing `grafix.c`/`grafix.h`, prefs `pref.c`/`pref.h`, win proc `winmine.c`,
  constants `rtns.h`/`main.h`, assets `bmp/`).
- SDL3 (window/renderer/input/audio API + headers): `~/development/repos/SDL/` (public headers
  under `include/SDL3/`).
- SDL_mixer (WAV playback API): `~/development/repos/SDL_mixer/` (headers under `include/SDL3_mixer/`).
- Dear ImGui (immediate-mode API + SDL3/SDLRenderer3 backends): `~/development/repos/imgui/`
  (`imgui.h`, `backends/imgui_impl_sdl3.*`, `backends/imgui_impl_sdlrenderer3.*`).
- mINI (INI read/write API): `~/development/repos/mINI/` (header `src/mini/ini.h`).
Per-stream relevance: game→winmine; assets/render→winmine + SDL; audio→SDL_mixer (+ SDL); ui→imgui
(+ winmine dialogs `pref.dlg`); config→mINI + winmine `pref.c`; app→SDL + imgui backends + winmine.

```
A  (foundation, serialized barrier)
└── merge to main ──► fan-out off this commit
    ├─ B.1 game        ┐
    ├─ B.2 config      │ all Fully Parallel
    ├─ B.3 assets      │ (disjoint files,
    ├─ B.4 audio       │  compile against A's
    ├─ B.5 render      │  frozen headers)
    └─ B.6 ui          ┘
        └── merge all (fan-in barrier) ──►
            ├─ C.0 app/main integration (Fan-In, serialized)
            └─ C.1 CI workflow (Fully Parallel by file; verification-gated on green build)
```

## Stream A — Foundation
**Goal:** Standing build skeleton + frozen contracts + verified Orthodoxy scoping, on `main`.
**Scope:** git init; rename template→minesweeper (drop `namespace myproject`); root + `src/` +
`tests/` `CMakeLists.txt` (CMake 3.28, C++20); `cmake/Dependencies.cmake` (FetchContent SDL3
`release-3.4.8`, SDL_mixer `release-3.2.2` WAV-only, imgui `v1.92.8` static-lib target + backends,
mINI `0.9.18` interface); trim template tooling (drop docs/CPack/coverage/cppcheck submodule);
loosen `.clang-tidy`; `.clang-format` pointer-right; `.orthodoxy.yml` scoped to `src/`+`include/`
with `InheritParent:false`; `find_package(orthodoxy)` + plugin on lib/exe targets only; copy
`assets/`; **all `include/minesweeper/*.h` contracts** (POD `Board`/`Cell`/`Settings`,
sprite-rect tables, `GameStatus`/`CellFlag` enums, every module's function prototypes);
pre-declare `minesweeper_lib` full source manifest + `game_test`/`config_test` targets referencing
not-yet-created `.cc` files; commit this spec to `docs/`; **Orthodoxy header-scoping spike**.
**Files Owned:** `.git/`, `CMakeLists.txt`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`,
`cmake/**`, `.clang-tidy`, `.clang-format`, `.orthodoxy.yml`, `.gitmodules`, `include/minesweeper/**`,
`assets/**`, `docs/**`, `.gitignore`.
**Files Forbidden:** none (runs first, exclusive).
**Dependencies:** None — first.
**Parallelism Classification:** Serialized / Barrier-Gated (gates all of B).
**Verification:** `cmake -DCMAKE_CXX_COMPILER=clang++-22 -B build` configures; deps fetch+build;
empty `minesweeper_lib` + stub `minesweeper` link; **spike asserts**: a deliberately-non-Orthodox
probe TU in `src/` ERRORS under the plugin, and a TU that only `#include`s `<imgui.h>`/`<mini/ini.h>`
produces ZERO orthodoxy diagnostics. `make tidy` runs.
**Deliverables:** `main` commit: building empty skeleton, frozen headers, committed spec,
spike result note.
**Failure Isolation:** Total gate — nothing dispatches until A merges. Spike failure blocks all.
**Re-Surface Triggers:** spike shows deps trip the plugin (→ permissive `_deps` config decision);
SDL_mixer pulls codec submodules despite WAV-only flags; imgui has no usable backend build;
FetchContent tag/network failure; `find_package(orthodoxy)` not found under clang-22.

## Stream B.1 — game (pure logic)
**Goal:** Implement the SDL-free game engine + unit tests.
**Scope:** board/cell state, deferred mine placement w/ first-click safety, iterative flood-fill,
adjacency, flag cycle (marks-aware), chord, win/loss, counters, xorshift RNG behind injected fn ptr.
**Files Owned:** `src/game/*.cc`, `tests/game_test.cc`.
**Files Forbidden:** all `*.h`, all `CMakeLists.txt`, other `src/*` dirs.
**Dependencies:** A merged.
**Parallelism Classification:** Fully Parallel.
**Verification:** `ctest -R game_test` green (placement count, first-click safety, zero-region
cascade, adjacency, flag cycle ±marks, chord match/mismatch, win/loss, counter incl. negative);
plugin-clean compile.
**Deliverables:** `src/game/` impl + passing `game_test`.
**Failure Isolation:** Pure module; failure blocks only C.0 link, not sibling B streams.
**Re-Surface Triggers:** contract in `game.h` insufficient; chord/first-click semantics ambiguous.

## Stream B.2 — config (mINI wrapper)
**Goal:** Thin C-style `Settings` load/save over mINI + headless round-trip test.
**Scope:** POD `Settings` ↔ INI (`[game]/[window]/[scores]`); STL/exceptions confined to `config.cc`;
errno-style returns; on-screen window-pos clamp.
**Files Owned:** `src/config/*.cc`, `tests/config_test.cc`.
**Files Forbidden:** all `*.h`, all `CMakeLists.txt`, other `src/*` dirs.
**Dependencies:** A merged.
**Parallelism Classification:** Fully Parallel.
**Verification:** `ctest -R config_test` (save→load→assert-equal incl. names, scores, scale);
plugin-clean (no STL/`auto`/lambda leak past `config.cc`).
**Deliverables:** `src/config/` impl + passing `config_test`.
**Failure Isolation:** Blocks only C.0.
**Re-Surface Triggers:** `Settings` contract drift; mINI exception path can't be contained errno-style.

## Stream B.3 — assets
**Goal:** Load BMP sprite sheets → `SDL_Texture` + sub-rect tables; load WAVs; window icon.
**Scope:** `SDL_LoadBMP`+`SDL_CreateTextureFromSurface` (color & B&W), sprite-rect tables
(blocks/led/button), `Mix_LoadWAV`, `SDL_GetBasePath` asset resolution, optional `.ico` icon.
**Files Owned:** `src/assets/*.cc`.
**Files Forbidden:** all `*.h`, all `CMakeLists.txt`, other `src/*` dirs.
**Dependencies:** A merged.
**Parallelism Classification:** Fully Parallel.
**Verification:** plugin-clean compile + links into lib; runtime load asserted in C.0.
**Deliverables:** `src/assets/` impl.
**Failure Isolation:** Blocks only C.0.
**Re-Surface Triggers:** `SDL_LoadBMP` rejects 4-bit/1-bit palettized BMP; `.ico` unloadable.

## Stream B.4 — audio
**Goal:** SDL_mixer playback of win/explode (no tick).
**Scope:** `Mix_OpenAudio`, channel playback gated on `settings.sound`.
**Files Owned:** `src/audio/*.cc`.
**Files Forbidden:** all `*.h`, all `CMakeLists.txt`, other `src/*` dirs.
**Dependencies:** A merged.
**Parallelism Classification:** Fully Parallel.
**Verification:** plugin-clean compile + link; audible check in C.0.
**Deliverables:** `src/audio/` impl.
**Failure Isolation:** Blocks only C.0; degrades gracefully if device absent.
**Re-Surface Triggers:** SDL_mixer API mismatch vs SDL 3.4.8.

## Stream B.5 — render
**Goal:** Draw board/LEDs/smiley/chrome via SDL_Renderer at integer scale.
**Scope:** layout offsets, Win95 bevel palette, sprite blits from frozen sprite-rect contract,
nearest-neighbor integer scaling, negative-counter glyph, end-state rendering.
**Files Owned:** `src/render/*.cc`.
**Files Forbidden:** all `*.h`, all `CMakeLists.txt`, other `src/*` dirs.
**Dependencies:** A merged (consumes `assets.h` + `game` types via frozen headers only).
**Parallelism Classification:** Fully Parallel.
**Verification:** plugin-clean compile + link; visual check in C.0.
**Deliverables:** `src/render/` impl.
**Failure Isolation:** Blocks only C.0.
**Re-Surface Triggers:** sprite-rect/scale contract insufficient; chrome needs sizes not in headers.

## Stream B.6 — ui
**Goal:** ImGui menu bar + dialogs, light/Win95 theme.
**Scope:** main menu (Game/Help), Custom/Best-Times/Enter-Name/About modals, Scale & Marks/Color/
Sound toggles, hide-menu (F5/F6) state, `ImGui::StyleColorsLight` tuning.
**Files Owned:** `src/ui/*.cc`.
**Files Forbidden:** all `*.h`, all `CMakeLists.txt`, other `src/*` dirs.
**Dependencies:** A merged (consumes game/config/render contracts via frozen headers).
**Parallelism Classification:** Fully Parallel.
**Verification:** plugin-clean compile (ImGui-boundary `HERESY(...)` suppressions only) + link;
interaction check in C.0.
**Deliverables:** `src/ui/` impl.
**Failure Isolation:** Blocks only C.0.
**Re-Surface Triggers:** ImGui callback-model integration friction; needs state not exposed by headers.

## Stream C.0 — app/main integration
**Goal:** Wire all modules via SDL3 callbacks into a running, tested game.
**Scope:** `SDL_MAIN_USE_CALLBACKS` (`SDL_AppInit/Iterate/Event/Quit`), fixed-size window+renderer,
ImGui backend init, input→board mapping (scale-divided), press/smiley state machine, timer +
auto-pause, continuous vsync draw order, config load/save lifecycle, post-build asset copy.
**Files Owned:** `src/app/*.cc`, `src/main.cc`.
**Files Forbidden:** all module `src/<module>/*.cc`, all `*.h`, all `CMakeLists.txt`.
**Dependencies:** ALL of B.1–B.6 merged.
**Parallelism Classification:** Fan-In / Serialized.
**Verification:** full `make` links; `make run` launches; **E2E checklist**: each difficulty +
custom; win + loss end-states; best-time entry + persistence across restart (inspect INI); sound
on/off; color/B&W; marks; scale 1×–4×; hide-menu; auto-pause on minimize; window-pos memory.
`make tidy` clean.
**Deliverables:** complete building/tested executable; E2E confirmation.
**Failure Isolation:** Terminal stream; failures localized to glue or surface upstream contract gaps.
**Re-Surface Triggers:** module contracts don't compose; ImGui+SDL3-callbacks incompatibility;
HiDPI/scale mouse-mapping mismatch.

## Stream C.1 — CI workflow
**Goal:** GitHub Actions building+testing with clang-22 on Linux.
**Scope:** workflow: install SDL build deps, configure `-DCMAKE_CXX_COMPILER=clang++-22`, build,
`ctest`, `tidy`.
**Files Owned:** `.github/workflows/*.yml`.
**Files Forbidden:** everything else.
**Dependencies:** A merged (file-independent); **green build (C.0) required to pass**.
**Parallelism Classification:** Fully Parallel (by file); verification-gated on C.0 green.
**Verification:** workflow run green on push.
**Deliverables:** committed workflow + first green run.
**Failure Isolation:** Independent; CI red does not block local deliverable.
**Re-Surface Triggers:** runner lacks clang-22 / Orthodoxy (enforcement-optional fallback);
SDL build-dep provisioning fails.

## Risks & Unknowns

- **[HIGH] Orthodoxy header scoping** — plugin diagnoses dep headers we include unless config is
  scoped to our dirs only. Mitigation + early spike in Build System §3. Also unknown: default
  rule state when no config resolves (allow-all vs deny-all) — determines whether a permissive
  config must be dropped into `_deps`. Resolve empirically first.
- **[MED] mINI ⇄ Orthodox tension** — RESOLVED: thin C-style wrapper confines STL/exceptions to
  `config.cc`; public interface is a POD `Settings` struct (see config module).
- **[MED] SDL_mixer build surface** — must disable non-WAV decoders (see §2) or risk
  submodule/system-lib failures.
- **[LOW] HiDPI** — RESOLVED: integer scale factor (default 2x, adjustable 1x–4x, persisted),
  nearest-neighbor (see render module).
- **[LOW] Asset copyright** — winmine bitmaps/WAVs are Microsoft's; fine locally, murky for a
  public repo. Acknowledged; not blocking.
- **[LOW] Setup** — needs `git init`; Orthodoxy installed + clang-22; LTO via clang toolchain.
- **[LOW] First-click safety** — deferred placement (avoid clicked cell) matches intent, not bits.

## Resolved Decisions

- **Dependency tags**: SDL `release-3.4.8`, SDL_mixer `release-3.2.2`, imgui `v1.92.8`,
  mINI `0.9.18`.
- **Audio**: via **SDL_mixer**, not raw SDL3 audio.
- **Compiler**: **`clang++-22`** (Orthodoxy plugin's build target).
- **Asset directory**: install assets next to the executable; resolve via `SDL_GetBasePath`.
- **B&W mode**: default to **color**; expose the toggle only (no display auto-detect).
- **Window height**: growth by the ImGui menu-bar height vs. the pixel-exact original is
  acceptable.
- **Config module**: thin C-style wrapper around mINI; POD `Settings` interface.
- **Scaling**: integer scale factor, default 2x, adjustable 1x–4x, persisted, nearest-neighbor.
- **Tick sound**: none — only win/explode (matches released game).
- **Edge features**: keep both hide-menu toggle (F5/F6) and auto-pause on minimize/defocus.
- **Code structure**: no namespace; module-prefixed free functions; POD structs; plain enums.
- **Window**: fixed-size / non-resizable; resizes only programmatically.
- **Entry model**: SDL3 callback API (`SDL_MAIN_USE_CALLBACKS`), no hand-written loop.
- **ImGui theme**: light/classic gray tuned to Win95.
- **Redraw**: continuous vsync.
- **Tooling**: trimmed to build/tests/format/tidy/Orthodoxy (drop docs/packaging/coverage/cppcheck).
- **Tests**: game-logic unit tests + headless config round-trip.
- **CI**: GitHub Actions, clang-22 build + ctest/tidy on Linux.
- **Execution**: one-shot full build; commit this spec into the repo before implementing.
- **Orthodoxy**: confirmed installed (Clang-22 plugin); configure with `-DCMAKE_CXX_COMPILER=clang++-22`.

No open questions remain.
