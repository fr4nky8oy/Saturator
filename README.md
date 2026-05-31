# Saturate

A from-scratch audio **saturator / drive** plugin, built in C++ with the
[JUCE](https://juce.com) framework and compiled to **VST3**, **Audio Unit (AU)**,
and **Standalone** formats.

Development is structured as a series of reviewable increments. Every change
flows through an issue, a feature branch, a pull request, and a green CI build,
and is reviewed before it reaches a protected `main`.

---

## Status

| Phase | What | State |
|-------|------|-------|
| 0 | Green plugin skeleton — passthrough audio, empty window, builds in all 3 formats | ✅ done |
| 1 | Git workflow + CI build gate + protected `main` | ✅ done |
| 2 | Saturator DSP — one PR per increment | 🔨 in progress |
| 2.5 | `new-plugin.sh` scaffold generator | ⬜ |
| 4 | Unsigned `.dmg` / `.pkg` installer | ⬜ |
| 5a | Serial-number licensing (JUCE `juce_product_unlocking`) | ⬜ |

The plugin currently provides a **Drive** control (the saturation described
below) with a rotary knob in the editor. Further parameters and DSP land per the
roadmap above.

---

## Signal processing

The Drive stage is a **waveshaping saturator**. Each sample is shaped by the
hyperbolic tangent function:

```
output = tanh(drive × input)
```

`tanh` is a smooth, S-shaped soft-clipping curve: near-linear for low-level
signals (clean), and progressively flattening toward ±1 as level rises, so peaks
are rounded rather than hard-clipped. **Drive** scales the signal into the curve —
higher Drive pushes it onto the flatter shoulders, generating more harmonics and
more saturation. Drive is parameter-smoothed (50 ms, linear) to avoid zipper
noise; an output makeup-gain stage is planned.

**Analog correspondence.** A symmetric `tanh` is the transfer function of a
bipolar-transistor differential pair, so this models solid-state soft clipping
directly. Its symmetry produces odd-order harmonics — similar in character to
push-pull and transistor saturation, and to the static transfer curve of tape.
It does **not** model single-ended valve warmth (even-order harmonics, which
require an asymmetric curve), tape hysteresis or frequency-dependent behaviour,
and it does not yet oversample to suppress aliasing. These are deliberate future
refinements.

---

## Build

**Requirements**

- macOS (developed/tested on 14.7.8, Apple Silicon / arm64)
- [CMake](https://cmake.org) ≥ 3.22
- [Ninja](https://ninja-build.org) (build driver)
- [JUCE](https://github.com/juce-framework/JUCE) 8.x checked out locally
  (defaults to `~/JUCE`; override with `-DJUCE_PATH=/path/to/JUCE`)

**Configure and build**

```bash
cd Saturate
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

On a successful build the plugin is copied into your user plugin folders
(`~/Library/Audio/Plug-Ins/...`) so it appears in your DAW immediately.

**Build outputs** (`Saturate/build/`, git-ignored):

- `Saturate.vst3` — cross-DAW VST3
- `Saturate.component` — Audio Unit (Logic, GarageBand)
- `Saturate.app` — Standalone app (easiest for quick testing)

---

## Layout

```
.
├── Saturate/
│   ├── CMakeLists.txt         # build recipe (juce_add_plugin + module links)
│   └── Source/
│       ├── PluginProcessor.{h,cpp}   # the audio processor (DSP lives here)
│       └── PluginEditor.{h,cpp}      # the UI window
├── .github/workflows/ci.yml   # CI build gate every PR must pass
├── .gitignore
└── README.md
```

The source is thoroughly commented, documenting the rationale behind each
component inline.

---

## Engineering workflow

`main` is protected. Nothing is committed straight to it. Each unit of work is:

1. an **issue** (tracked on the board; milestones map to the phases above),
2. a short-lived **feature branch**,
3. a **pull request**,
4. a **green CI build** (configures + compiles the plugin on a clean macOS runner),
5. a **review**, then **merge**.

Branch protection, required status checks, and mandatory PR review are enforced
on `main`.

---

## Tech stack

- **Language:** C++17
- **Framework:** JUCE 8
- **Build:** CMake + Ninja
- **Audio formats:** VST3, AU, Standalone
- **CI:** GitHub Actions (macOS runner)
