# P2STMap

A Nuke plugin that converts position passes (P) to STMap coordinates using camera matrices — more efficient than chaining three C44Matrix nodes.

Inspired by [Ivan Busquets's C44Matrix](https://www.nukepedia.com/tools/plugins/colour/c44matrix/).

## What It Does

P2STMap takes a world position pass and a camera, then applies the full projection pipeline in a single node:

1. **Inverse Transform** — World space → Camera space
2. **Projection** (with W-divide) — Camera space → NDC
3. **Format** (with W-divide) — NDC → Pixel coordinates
4. **Normalize** — Pixel coordinates → ST coordinates (0-1)

The output is a proper STMap that can be used with Nuke's STMap node for reprojection workflows.

## Inputs

| Input | Description |
|-------|-------------|
| **P** | Position pass (world coordinates in RGB, typically with A=1) |
| **cam** | Camera node for projection matrices |

## Output

| Channel | Value |
|---------|-------|
| R | U coordinate (normalized X) |
| G | V coordinate (normalized Y) |
| B | Z depth |
| A | W component |

## Compatibility

| Platform | Architecture |
|----------|--------------|
| Windows  | x64          |
| Linux    | x64          |
| macOS    | ARM64/x64    |

Pre-built binaries available in `COMPILED/`

## Installation

1. Copy the plugin from `COMPILED/` for your platform to your `.nuke` directory or plugin path
2. Add the path to Nuke via `init.py`:
   ```python
   nuke.pluginAddPath('/path/to/P2STMap')
   ```
3. Restart Nuke — the node appears under **Transform/P2STMap**

## Usage

1. Connect your position pass to the **P** input
2. Connect your camera to the **cam** input
3. The output is an STMap ready for use

**Typical workflow:**
```
[Position Pass] → [P2STMap] → [STMap] → [Reproject/Retime]
                      ↑
                  [Camera]
```

## Why Use This?

Previously, converting a position pass to STMap required chaining multiple C44Matrix nodes with different settings:
- C44Matrix (inverse transform, no w_divide)
- C44Matrix (projection, with w_divide)  
- C44Matrix (format, with w_divide)
- Expression (normalize by format dimensions)

P2STMap combines all of this into a single, optimized node.

## Building from Source

Platform-specific CMake files are provided:
- `CMakeLists_WIN.txt`
- `CMakeLists_LINUX.txt`
- `CMakeLists_MAC.txt`

See `building_step_by_step.txt` for detailed instructions.

### Requirements
- CMake
- Nuke NDK (included with Nuke installation)
- C++ compiler (MSVC on Windows, GCC on Linux, Clang on macOS)

## Related

- [C44Matrix](https://github.com/petermercell/C44Matrix) — General 4x4 matrix transformation plugin (with added Axis support)

## Author

[Peter Mercell](https://github.com/petermercell)
