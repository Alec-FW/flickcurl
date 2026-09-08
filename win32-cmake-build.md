# Windows CMake Build Guide for Flickcurl

Build flickcurl and its utilities on Windows with CMake + Visual Studio.

**vcpkg is the recommended way to build this library on Windows** - it supplies
libcurl, libxml2 and their dependencies in a linkage-consistent form
(architecture, static/dynamic, CRT). An alternative is to supply your own
dependency builds and point CMake at them with `CURL_ROOT` / `LIBXML2_ROOT` /
`ZLIB_ROOT`.

---

## Prerequisites

1. **Visual Studio 2022** (or later) with the **Desktop development with C++** workload.
2. **CMake 3.25+** (bundled with VS).
3. **Git**.
4. **vcpkg** (supported way to get dependencies).

## Setup

```bat
git clone --recurse-submodules <repository-url> flickcurl
```

CMake locates vcpkg automatically in this order: `VCPKG_ROOT` environment
variable, `./vcpkg`, `../vcpkg`. If none is found, the build proceeds without
vcpkg and uses `find_package` to locate your dependencies instead.

## Options

| Option | Default | Effect |
|---|---|---|
| `BUILD_SHARED_LIBS` | `OFF` | `ON` = flickcurl is a DLL, deps are DLLs (triplet `<arch>-windows`). `OFF` = static flickcurl, static deps (triplet `<arch>-windows-static[-md]`). |
| `FLICKCURL_STATIC_CRT` | `OFF` | `ON` = static CRT (`/MT`). Only valid with `BUILD_SHARED_LIBS=OFF`. |
| `FLICKCURL_OFFLINE` / `FLICKCURL_CAPTURE` | `OFF` | Mutually exclusive web-response handling modes. |
| `-A <arch>` | VS default | `x64`, `Win32` (x86), `ARM64`. |
| `CURL_ROOT`, `LIBXML2_ROOT`, `ZLIB_ROOT` | — | Without vcpkg: where to find libcurl, libxml2, zlib. |

**Supported combinations:**
- `BUILD_SHARED_LIBS=ON` (+ `-DFLICKCURL_STATIC_CRT=OFF`) — dynamic build.
- `BUILD_SHARED_LIBS=OFF, FLICKCURL_STATIC_CRT=ON` — fully static build.
- `BUILD_SHARED_LIBS=OFF, FLICKCURL_STATIC_CRT=OFF` — static lib, dynamic CRT ("static-md").

`BUILD_SHARED_LIBS=ON` with `FLICKCURL_STATIC_CRT=ON` is rejected.

## Examples

All commands assume the VS CMake generator is available; use the one matching
your Visual Studio version.

### 1. Dynamic x64 (vcpkg)

```bat
cmake -B build-x64-windows -G "Visual Studio 18 2026" -A x64 -DBUILD_SHARED_LIBS=ON
cmake --build build-x64-windows --config Release
```

### 2. Static x86 with dynamic CRT (vcpkg)

```bat
cmake -B build-x86-windows-static-md -G "Visual Studio 18 2026" -A Win32 ^
    -DBUILD_SHARED_LIBS=OFF -DFLICKCURL_STATIC_CRT=OFF
cmake --build build-x86-windows-static-md --config Release
```

### 3. Without vcpkg (your own dependency builds)

```bat
cmake -B build-x64-custom -G "Visual Studio 18 2026" -A x64 ^
    -DBUILD_SHARED_LIBS=ON ^
    -DCURL_ROOT=C:\deps\curl ^
    -DLIBXML2_ROOT=C:\deps\libxml2 ^
    -DZLIB_ROOT=C:\deps\zlib
cmake --build build-x64-custom --config Release
```

On success the configure summary prints
`Dependencies resolved without vcpkg (find_package)`.

## Where the Artifacts Go

Use one build directory per configuration (as the examples above do). All final
artifacts — the library, utilities, samples, and (for dynamic builds) the
dependency DLLs — are written to a single `Release/` (or `Debug/`)
directory at the root of the build directory:

- dynamic build → `flickcurl.dll` + import `flickcurl.lib` + executables
- static build → `libflickcurl.lib` + executables

## Building Without vcpkg

Without vcpkg, CMake resolves dependencies with the standard find modules
(`FindCURL`, `FindLibXml2`, `FindZLIB`). Point the `*_ROOT` variables at any
prefix with the usual layout:

```
<prefix>/include/...    e.g. include/curl/curl.h, include/libxml/xpath.h
<prefix>/lib/...        e.g. lib/libcurl.lib, lib/libxml2.lib, lib/zlib.lib
```

Notes:

- Be mindful of architecture and linkage. The binaries you supply must match
  the build you are attempting in architecture, static/dynamic linkage, and CRT
  (`/MD` vs `/MT`). Mismatches surface as configure or link errors (or, worse,
  runtime crashes).
- Static libcurl needs zlib. vcpkg provides this automatically; without
  vcpkg set `ZLIB_ROOT`. If zlib is found it is linked; 
  if not found the build proceeds and may fail at link time.
- **pkg-config interference.** If a `pkg-config` binary is on `PATH`, some find
  modules may consult it and pick up wrong `.pc` entries from unrelated
  toolchains (e.g. an MSYS2 libz). If you see unexpected include paths or link
  errors, remove `pkg-config` from `PATH` while configuring.

## Using the Installed Library

Installing is **opt-in**: pass `-DCMAKE_INSTALL_PREFIX=<dir>` at configure
time. Without it the install target is disabled (a message is printed at
configure time), because CMake's default (`C:/Program Files/flickcurl`) is
rarely what you want.

If you build several configurations it is recommended to install them in prefix like
`flickcurl-dev/<triplet>` Then you can put [build selector .cmake](#collection-directory-multiple-builds-automatic-selection) in it
and enjoy your consumer picking up the build that matches your consumer automatically. 
The triplet directory name is a convention only — nothing in the CMake files
automatically generates or requires this format.

```bat
cmake -B build-x64-windows -G "Visual Studio 18 2026" -A x64 -DBUILD_SHARED_LIBS=ON ^
    -DCMAKE_INSTALL_PREFIX=C:\flickcurl-dev\x64-windows
cmake --build build-x64-windows --config Release --target install
cmake --build build-x64-windows --config Debug --target install
```

(one `--target install` per configuration you want to ship). The layout mirrors
what vcpkg produces for its own packages, so Debug and Release artifacts never
overwrite each other:

```
<prefix>/lib/flickcurl.lib          <prefix>/bin/flickcurl.dll        (non-Debug configs)
<prefix>/debug/lib/flickcurl.lib    <prefix>/debug/bin/flickcurl.dll  (Debug)
```

CMake package files live in `<prefix>/share/flickcurl/`, pkg-config files in
`<prefix>/lib/pkgconfig/` (Release) and `<prefix>/debug/lib/pkgconfig/` (Debug) —
the same conventions vcpkg uses for its own packages.

The generated export files record both configurations (`IMPORTED_CONFIGURATIONS`),
so a consumer linking `flickcurl::libflickcurl` automatically picks the matching
configuration at build time — a Debug app build links the Debug library and gets
the Debug DLL, a Release build gets the Release ones. Install at least Release;
add Debug if consumers will build Debug configurations.

Each installation carries a CMake package (`share/flickcurl/`):

- `flickcurl-config.cmake` — imports `flickcurl::libflickcurl` and verifies the
  consuming project's architecture, CRT, and libcurl/libxml2 static-vs-shared
  flavors against the recorded build parameters (FATAL_ERROR with a fix
  hint on mismatch).
- `flickcurl-package-info.cmake` — the recorded metadata (version, arch,
  shared/static, CRT, dependency flavors).

```cmake
# CMake consumers: point FLICKCURL_ROOT at the installation (or collection)
if(FLICKCURL_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${FLICKCURL_ROOT}")
endif()
find_package(flickcurl REQUIRED)
target_link_libraries(my_app PRIVATE flickcurl::libflickcurl)
```

Linking `flickcurl::libflickcurl` is all that is required: a static package
exports `FLICKCURL_STATIC=1` on the target (no manual `#define` or CMake
definition needed), and the libxml2 include directories are propagated because
`flickcurl.h` exposes libxml2 types. For a shared build, set
`-DFLICKCURL_USE_SHARED=ON` (the consumer-side knob — not CMake's
`BUILD_SHARED_LIBS`, which governs your own libraries) and copy
`flickcurl.dll` next to your executable in a post-build step:

```cmake
get_target_property(_flickcurl_type flickcurl::libflickcurl TYPE)
if(NOT _flickcurl_type MATCHES "^STATIC")
    # Shared libflickcurl: vcpkg's post-build (applocal) step only knows
    # about vcpkg-managed DLLs, so copy flickcurl.dll next to the exe.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:flickcurl::libflickcurl>
                $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()
```

### Collection directory (multiple builds, automatic selection)

Keep several builds side by side:

```
flickcurl-dev/
├── flickcurl-config.cmake   # selector (see below)
├── x86-windows-static-md/   # one install per build variant, named after the
└── x64-windows/             #   vcpkg triplet used to build it
```

Copy the selector `flickcurl-collection-selector.cmake` from flickcurl repository 
to the collection root as `flickcurl-config.cmake`. It scans the subdirectories'
`share/flickcurl/flickcurl-package-info.cmake`,
derives the consumer's requirements (arch from the generator platform, CRT from
`CMAKE_MSVC_RUNTIME_LIBRARY`, shared/static from `FLICKCURL_USE_SHARED`), and
hands over to the matching build's config. No match → FATAL_ERROR listing what
each subdirectory offers.

The consumer project's vcpkg environment (manifest + triplet) must provide
libcurl/libxml2/zlib in flavors matching the selected flickcurl build — the
same rule as when building flickcurl itself.

```bat
:: pkg-config consumers: use a mingw64/ucrt64 pkg-config (NOT the msys one), e.g.
:: (the Debug .pc lives in debug\lib\pkgconfig, the Release one in lib\pkgconfig)
set PKG_CONFIG_PATH=C:\flickcurl-dev\x64-windows\lib\pkgconfig
C:\msys64\mingw64\bin\pkg-config --cflags --libs flickcurl
```

Notes for pkg-config / autotools consumers:

- The installed .pc files are generated from the original autotools template.
  `libdir`/`includedir` are `${prefix}`-relative; `libdir` of the Debug .pc
  is `${prefix}/debug/lib`.
- Windows pkg-config builds (pkgconf) re-derive `prefix` from the .pc file's
  own location by default, which shifts paths for the Debug file. Consumers
  should pass `--dont-define-prefix` for debug version.
- Use mingw64 or ucrt64 version of pkg-config to avoid issues with paths
  (--define-prefix is known to do strange things on msys2 version)
- `Libs.private` is generated at install time from the CMake link targets:
  the full transitive set (libxml2, libcurl, zlib and the system libraries a
  static dependencies require), with Debug dependency names (`-llibcurl-d`,
  `-lzlibd`, ...) in the Debug .pc. Unlike autotools generated .pc this 
  does not include -Lpath options, as these paths will likely be ephemeral
  on Windows, and might lead to confusing link errors.
- `Requires: libcurl libxml2` needs those packages' .pc files on the
  `PKG_CONFIG_PATH` for `--static` to resolve the full graph.
- The `Cflags:` line is just the include directory — the template has no hook
  for compile definitions. A consumer linking the **static** libflickcurl
  must add `-DFLICKCURL_STATIC=1` to its own CFLAGS; with a shared build it is
  not needed.

