# TF2003 QVM

Maintained QVM version of the **Team Fortress mod for QuakeWorld**, intended for MVDSV and other QuakeWorld servers with QVM support.

This fork focuses on bug fixes, compatibility improvements, and safe server-load optimizations while preserving the original gameplay.

## Prerequisites

- **Linux:** a C compiler, CMake, and Ninja.
- **Windows:** Visual Studio 2022 with **Desktop development with C++** and **C++ CMake tools for Windows** installed.

> [!NOTE]
> The required QVM compiler (`q3lcc`) and linker (`q3asm`) are included in the repository and built automatically.

## Building the QVM

### Linux

On Debian or Ubuntu, install the required tools and build the `qvm` target:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build

cmake -S . -B _cmake/qvm -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build _cmake/qvm --target qvm
```

Build artifacts are written to:

```text
_cmake/qvm/qwprogs.qvm
_cmake/qvm/qwprogs.map
```

### Windows — Visual Studio 2022

Open **Developer PowerShell for VS 2022** in the repository directory and run:

```powershell
cmake -S . -B _cmake/qvm_vs2022 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build _cmake/qvm_vs2022 --target qvm
```

Build artifacts are written to:

```text
_cmake/qvm_vs2022/qwprogs.qvm
_cmake/qvm_vs2022/qwprogs.map
```

## Server installation

Copy `qwprogs.qvm` to the server's `fortress` directory. For MVDSV, enable QVM execution by starting the server with `-progtype 3`.

Starting with `v2026.10.02`, also copy the release archive's **`progs` and `skins` directories** into `fortress`. The models are required for Spy disguise colors and immutable corpse appearances and are downloaded by clients during signon. Live disguise colors use embedded model textures and do not depend on downloading new PCX skins during play; the `skins` directory remains included for compatibility. See [installation and resource preparation](docs/spy-appearance.md) for details, including custom team colors.

## License

This project is distributed under the [GNU General Public License v2.0](LICENSE).
