# commonlibf4rd-xmake

xmake build for [CommonLibF4RD](https://github.com/Zzyxz/CommonLibF4RD), which ships
CMake + vcpkg only, plus the compatibility layer the WattzIO mods need when moving off
`libxse/commonlibf4`.

Upstream is a nested submodule and is not modified.

## Layout

| | |
|---|---|
| `xmake.lua` | Static-library target for upstream. Mirrors its `CMakeLists.txt`. |
| `compat/WattzIO/` | Declarations and helpers libxse provides and CommonLibF4RD does not |
| `tools/` | Runtime Database lookups. Not needed to build. |
| `CommonLibF4RD/` | Upstream, nested submodule |

## Use

```
git submodule add <url> lib/commonlibf4rd
git submodule update --init --recursive
```

```lua
includes("lib/commonlibf4rd")

target("WIO-YourMod")
    set_kind("shared")
    add_deps("commonlibf4rd")
```

```cpp
// pch.h
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <WattzIO/Compat.h>
```

Record both submodule commits in the consuming project's `source/BUILD.md`; a source
mirror without a `.git` folder cannot carry them.

```
git submodule status --recursive
```

## compat/WattzIO

| Header | Provides |
|---|---|
| `Logging.h` | `REX::INFO` / `DEBUG` / `WARN` / `ERROR`, `UNRESTRICTED_CAST` |
| `InputMap.h` | `F4SE::InputMap` |
| `REMissing.h` | `RE::` types and members absent upstream |
| `Translations.h` | `WIO::Translations::LoadFile` / `LoadForMod` |
| `Plugin.h` | `F4SE_PLUGIN_LOAD`, `WIO_PLUGIN_VERSION`, `WIO::Init` |
| `Papyrus.h` | Variadic `DispatchStaticCall` / `DispatchMethodCall` |
| `Settings.h` | `REX::TIniSetting`, `REX::FIniSettingStore` |
| `Relocation.h` | `REL::write_fill`, `REL::replace_func`, `REL::write_call` |
| `Compat.h` | Umbrella |

## Deviations from upstream's CMakeLists

Exported PUBLIC, so consuming projects inherit them:

| | Reason |
|---|---|
| spdlog and fmt compiled, not header-only | Header-only spdlog includes `<windows.h>`; its `MEM_RELEASE` / `PAGE_EXECUTE_READWRITE` macros collide with `F4SE/Impl/WinAPI.h` |
| `/utf-8` | fmt 12.x `static_assert`s without it |
| `_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING` | `RE/msvc/functional.h` uses `std::aligned_storage_t`; upstream builds at C++20 |
| `Ole32`, `Shell32` | `F4SE::log::log_directory` needs `SHGetKnownFolderPath` / `CoTaskMemFree` |
| `/wd4100` | Upstream headers trip it under `allextra` |

## Tools

Set `F4RD_RUNTIME_BIN` to `<game>/Data/F4SE/Plugins/f4rd-runtime.bin`, or pass
`--db <path>`.

```
python tools/f4rd_probe.py 2268334 2234801      # ID -> RVA per runtime
python tools/f4rd_rlookup.py 0x1da36b0          # RVA -> ID
python tools/f4rd_vtable_audit.py CommonLibF4RD/CommonLibF4/include/RE
```

`f4rd_vtable_audit.py` finds upstream classes whose `VTABLE` member names a
differently-sized array. Currently two: `PlayerCamera` (names `VTABLE::TESCamera`, 1
entry vs 5) and `TESNPC` (names `VTABLE::TESActorBase`, 14 vs 19). Indexing past the
end reads out of bounds and resolves a garbage `REL::ID`. **Use `RE::VTABLE::X[n]`
rather than `RE::X::VTABLE[n]`.** Re-run after updating upstream.

## Runtime requirement

Plugins built against this need the
[Runtime Database](https://www.nexusmods.com/fallout4/mods/108394) installed at
`Data/F4SE/Plugins/f4rd-runtime.bin`. It replaces Address Library.

## License

MIT, matching upstream. `xmake.lua`, `compat/` and `tools/` are original work;
CommonLibF4RD is included unmodified under its own license.
