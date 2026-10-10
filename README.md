# Alien Shooter 2 Engine

A Visual Studio 2022 Win32/x86 source project for **Alien Shooter 2 (Gold)** and **Alien Shooter: Vengeance**, intended for preservation, maintenance, study and source-level modification of the game engine.

This repository contains engine and game source code, Visual Studio project files, the Win32 resource payload required by the executable, and the bundled Ogg/Vorbis sources. The original game data required to run either edition is not included.

## Requirements

- Windows.
- Visual Studio 2022 with the **Desktop development with C++** workload.
- **MSVC v143**, Win32/x86 build support and a **Windows 10 SDK**. The project selects an installed `10.0` SDK and uses C++14.
- A legally obtained **Alien Shooter 2 (Gold)** or **Alien Shooter: Vengeance** installation for runtime game data.
- The **32-bit `d3dx9_31.dll`** runtime dependency, available through the DirectX runtime installation or beside the executable.

Ogg/Vorbis is compiled from the included sources. The project uses the Windows SDK's Direct3D 9 import library and loads D3DX at runtime; a separate DirectX SDK is not required to build it.

## Build

Open `AlienShooter.sln` in Visual Studio 2022, select the required configuration with the **Win32** platform, and build the `AlienShooter` project.

| Edition | Release configuration | Debug configuration |
| --- | --- | --- |
| Gold | `Release` | `Debug` |
| Vengeance | `Release Vengeance` | `Debug Vengeance` |

You can also build from a command prompt in the repository directory:

```bat
BUILD.bat Release Gold
BUILD.bat Release Vengeance
```

Replace `Release` with `Debug` for a debug build. Running `BUILD.bat` without arguments builds **Release Gold**. The direct configuration names, such as `BUILD.bat ReleaseVengeance`, are also accepted.

The generated files are:

- Executable and symbols: `build\vs2022\<Configuration>\AlienShooter.exe`, `.pdb` and `.map`.
- Intermediate files: `build\vs2022\obj\<Configuration>\`.
- Command-line build log: `build\vs2022\build-<Configuration>.log`.

The configurations use separate output directories. Setting `AS2_BUILD_ROOT` before running `BUILD.bat` replaces the `build\vs2022` directory with that path.

## Running

Place the built `AlienShooter.exe` in the matching game's installation directory and run it with that directory as its working directory. Keep a backup of the original executable.

The build configuration selects the effect file used by the renderer:

| Edition | Effect path | Fallback path |
| --- | --- | --- |
| Gold | `vid\pseudo3d.fx` | `pseudo3d.fx` |
| Vengeance | `vid\pseudo4d.fx` | `pseudo4d.fx` |

Use the shader and game data from the selected edition. The build selects these paths; it does not rename or copy files in the game installation.

## Repository layout

- `src\` — engine and game systems, including rendering, maps, sprites, input, sound, scripts and shared runtime services.
- `src\zm\` — inventory, RPG, quests, menus, campaign and supporting game systems.
- `third_party\` — bundled libogg and libvorbis sources and license notices.
- `resources\` — executable exports and required Win32 resource payload.
- `vs2022\` — Visual Studio 2022 project and filters.
- `tools\build\` — MSBuild support.
- `AlienShooter.sln` / `BUILD.bat` — solution and command-line build entry point.

## Third-party components

The project includes **libogg** and **libvorbis**. Their license texts are retained in:

- [third_party/libogg/COPYING](third_party/libogg/COPYING)
- [third_party/libvorbis/COPYING](third_party/libvorbis/COPYING)

Those components remain subject to their respective licenses.

## License

See [LICENSE.md](LICENSE.md) for the licensing and rights notice supplied with this source package.

Alien Shooter names, game data, artwork, audio and related game content remain the property of their respective rights holders. The included Win32 resource payload remains subject to the rights of the relevant copyright holder. Third-party components remain under their own license terms.
