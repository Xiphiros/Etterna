# Fork notes

This repository is a fork of Etterna. `master` is an Etterna release plus the changes listed
below. The changes are not offered to the upstream project.

## Base

- `master` is based on the upstream tag `v0.75.1`.
- Upstream is `https://github.com/etternagame/etterna`; its `master` is the latest release and its
  `develop` is unreleased work.

## Changes on top of the release

| Change | File | Effect |
| :--- | :--- | :--- |
| Copy an actor's definition node only on the legacy XML path | `src/Etterna/Actor/Base/ActorUtil.cpp` | Screen load time no longer grows with nesting depth. Measured on one Lua theme screen of 18,818 actors: 0.65 s instead of 1.69 s. |
| Never upload a score to the online service's score API | `src/Etterna/Singletons/DownloadManager.cpp` | While `SCORE_UPLOAD_ENABLED` is false, `ShouldUploadScores` returns false, `UploadScore` and `UploadBulkScores` log the skip and return without sending a request, and `DLMAN:IsScoreUploadEnabled()` returns false. An upload carries the SHA-256 of the running binary, which differs from the release binary's. Scores are still saved in the local profile. |
| Disable EtternaOnline HTTP login | `src/Etterna/Singletons/DownloadManager.cpp` | While `ONLINE_LOGIN_ENABLED` is false, `LoggedIn()` returns false, `OnLogin()` clears auth state, `LoginWithToken()` logs the skip and invokes `done(false)` silently, `LoginRequest()` logs the skip, broadcasts `LoginFailed`, and invokes `callback(false)` with no network request, and `DLMAN:IsOnlineLoginEnabled()` returns false. Unauthenticated version checks (`/settings/version`), pack tags (`/tags`), pack search, and pack downloads remain enabled. |
| Disable multiplayer connections and login | `src/Etterna/Singletons/NetworkSyncManager.cpp` | While `MULTIPLAYER_ENABLED` is false, constructor splash text and `StartUp()` skip connecting (`AutoConnectMultiplayer` does not open a socket), `PostStartUp()` and `Login()` log the skip and return without allocating `ETTProtocol` or sending a pack list or login, and `NSMAN:IsMultiplayerEnabled()` returns false. |
| Cache thread IDs in `thread_local` storage and log TLS status at startup | `src/RageUtil/Misc/RageThreads.cpp`, `src/archutils/Common/PthreadHelpers.cpp`, `src/Etterna/Globals/StepMania.cpp` | `s_bSystemSupportsTLS` is enabled and the dead `HAVE_TLS` guards are removed so `GetCurrentThreadId()` caches `gettid()` per thread instead of issuing a syscall on every `RageMutex` lock, and startup logging always reports TLS availability. |
| Verify TLS certificates and omit empty bearer headers in HTTP requests | `src/Etterna/Singletons/DownloadManager.cpp` | `CURLOPT_SSL_VERIFYPEER` and `CURLOPT_SSL_VERIFYHOST` are enabled on libcurl handles, and `Authorization` headers are omitted when `authToken` is empty. |
| Harden `.smzip` extraction paths against traversal and symlinks | `src/Etterna/Singletons/SongManager.cpp` | Package extraction rejects relative traversal (`..`), drive prefixes, invalid roots, and symbolic links along destination paths inside `Songs/`, and creates directory entries without opening file streams. |
| Precompute song sort keys and cache sorted song groups for wheel builds | `src/Etterna/Models/Songs/Song.h`, `src/Etterna/Models/Songs/Song.cpp`, `src/Etterna/Models/Songs/SongUtil.cpp`, `src/Etterna/Singletons/SongManager.h`, `src/Etterna/Singletons/SongManager.cpp`, `src/Etterna/Actor/Menus/MusicWheel.cpp` | Normalized sort keys for main title, subtitle, artist, and simfile author are cached on `Song` during load, tidy, and save; `SongManager` sorts group vectors lazily via `EnsureGroupsSorted()` and invalidates them on song or group reloads and saves; and `MusicWheel` checks filtered song membership with `std::unordered_set`. |
| Keep the visited Lua table set on the stack in `XNodeFromTable` | `src/Etterna/FileTypes/XmlFileUtil.cpp` | `XNodeFromTableRecursive` tracks visited tables at a fixed Lua stack index with `lua_rawget` and `lua_rawset` instead of registry `LuaReference` round-trips, and formats array indices with `std::to_string`. |
| Restore `Actor:hibernate(fSecs)` | `src/Etterna/Actor/Base/Actor.h`, `src/Etterna/Actor/Base/Actor.cpp`, `src/Etterna/Actor/Base/Sprite.cpp` | `Hibernate()` and the `Actor:hibernate` Lua method suppress drawing and updates until `m_fHibernateSecsLeft` elapses without contributing to `GetTweenTimeLeft()`, allowing themes to freeze `ActorFrameTexture` subtrees with `hibernate(math.huge)` without blocking screen transitions. |
| Skip updates on hidden dormant `ActorFrame` subtrees in O(1) | `src/Etterna/Actor/Base/Actor.h`, `src/Etterna/Actor/Base/Actor.cpp`, `src/Etterna/Actor/Base/ActorFrame.h`, `src/Etterna/Actor/Base/ActorFrame.cpp`, `src/Etterna/Actor/Base/BitmapText.h`, `src/Etterna/Actor/Base/Sprite.h`, `src/Etterna/Actor/Base/Sprite.cpp` | `ActorFrame::UpdateInternal` returns early when hidden and `IsSubtreeDormant()` is true. Dormancy requires `CLOCK_TIMER`, is cached per subtree, invalidated up `m_pDormancyParent` on state transitions, and restricted via exact `typeid` checks to concrete `Actor`, `ActorFrame`, `ActorFrameAutoDeleteChildren`, `BitmapText`, `Sprite`, and `Quad` instances. |
| Add optional MSAA support to `ActorFrameTexture` | `src/Etterna/Actor/Base/ActorFrameTexture.h`, `src/Etterna/Actor/Base/ActorFrameTexture.cpp`, `src/RageUtil/Graphics/RageDisplay.h`, `src/RageUtil/Graphics/RageDisplay_OGL.cpp` | `ActorFrameTexture:EnableMSAA(bool)` sets `RenderTargetParam::bWithMSAA`. When OpenGL framebuffer multisample and blit extensions are available, `RenderTarget_FramebufferObject` renders into a multisampled FBO (up to 4x) and resolves to the single-sample texture in `FinishRenderingTo()`, falling back cleanly to a single-sample FBO otherwise. |
| Fix compressed `Etterna.xml.gz` profile I/O and buffer XML serialization | `src/Etterna/Models/Misc/Profile.h`, `src/Etterna/Models/Misc/Profile.cpp`, `src/Etterna/Models/Misc/XMLProfile.cpp`, `src/Etterna/FileTypes/XmlFileUtil.cpp` | `XMLProfile` avoids mutating path strings in place, loads `Etterna.xml.gz` via `GunzipFile` with CRC32 verification when `Etterna.xml` is absent, writes to `Etterna.xml.gz` when `ProfileDataCompress` is enabled and closes the file before removing the stale counterpart, and `XmlFileUtil` writes escaped XML through a 64 KiB chunk buffer. |
| Add `--bench` telemetry mode and honor X11 swap interval in windowed mode | `src/arch/LowLevelWindow/LowLevelWindow_X11.cpp`, `src/Etterna/Globals/StepMania.cpp`, `src/Etterna/Globals/GameLoop.h`, `src/Etterna/Globals/GameLoop.cpp`, `src/RageUtil/Graphics/RageDisplay.h`, `src/RageUtil/Graphics/RageDisplay.cpp`, `src/RageUtil/Graphics/RageDisplay_OGL.h`, `src/RageUtil/Graphics/RageDisplay_OGL.cpp`, `src/RageUtil/Graphics/RageBitmapTexture.cpp` | `LowLevelWindow_X11::TryVideoMode` applies GLX swap-interval control in windowed as well as fullscreen mode; `--bench=<path>` (with optional `--bench-dt=<seconds>` and `--bench-seed=<uint32>`) seeds engine and Lua PRNGs deterministically, sets a fixed update step, and exposes `DISPLAY:IsBenchEnabled()`, `BenchBeginPhase(name, isWarmup)`, `BenchEndPhase()`, and `BenchRequestScreenshot(vfsPath)` to record per-frame CPU/GPU timings, OpenGL draw/state/query/texture counters, and pre-swap backbuffer PNG captures to JSON Lines. |
| Shadow-cache OpenGL state, batch 2D quads, upload NPOT textures, and remove hot-actor allocations | `src/RageUtil/Graphics/RageDisplay.h`, `src/RageUtil/Graphics/RageDisplay.cpp`, `src/RageUtil/Graphics/RageDisplay_OGL.h`, `src/RageUtil/Graphics/RageDisplay_OGL.cpp`, `src/RageUtil/Graphics/RageBitmapTexture.cpp`, `src/arch/MovieTexture/MovieTexture_Generic.cpp`, `src/Etterna/Actor/Base/ActorMultiVertex.cpp`, `src/Etterna/Actor/Base/BitmapText.cpp`, `src/Etterna/Actor/Gameplay/NoteDisplay.h`, `src/Etterna/Actor/Gameplay/NoteDisplay.cpp`, `src/Etterna/Actor/Menus/MusicWheelItem.cpp` | `RageDisplay` tracks per-stack matrix revisions; `RageDisplay_Legacy` shadow-caches OpenGL render state, per-texture filter/wrap/mipmap metadata, and GLSL effect uniform locations, gates `DebugFlushGLErrors` and `DebugAssertNoGLError` on `VideoRendererDebugMode`, binds interleaved `RageSpriteVertex` arrays directly (`GL_BGRA`), coalesces 2D quads into indexed triangle batches (`FlushQuadBatch`), and uploads GPU-native non-power-of-two bitmap and movie textures when `GL_ARB_texture_non_power_of_two` is present; and `ActorMultiVertex`, `BitmapText`, `NoteDisplay`, and `MusicWheelItem` avoid per-draw container copies and heap allocations. |

## Build on Linux

Packages: `cmake`, `ninja`, `gcc`, `nasm`, the libraries listed in `Docs/Building.md`, and OpenSSL 3.
The example uses the paths of Arch Linux's `openssl-3` package; adjust the three OpenSSL paths for
another system.

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTS=FALSE -DBUILD_EXAMPLES=FALSE \
  -DCMAKE_CXX_FLAGS="-D_DEBUG" \
  -DWITH_CRASHPAD=OFF -DWITH_VULKAN=OFF \
  -DOPENSSL_INCLUDE_DIR=/usr/include/openssl-3 \
  -DOPENSSL_SSL_LIBRARY=/usr/lib/openssl-3/libssl.so \
  -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/openssl-3/libcrypto.so \
  -DCURL_DISABLE_LDAP=ON -DCURL_DISABLE_LDAPS=ON -DUSE_NGHTTP2=OFF -DUSE_LIBIDN2=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_JACK=TRUE \
  -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
ninja -C build
```

Why each group of options is there:

- `RelWithDebInfo` and `-D_DEBUG` are what the upstream release workflow uses
  (`.github/workflows/ci-workflow.yml`).
- Crashpad needs Google's depot tools and Vulkan needs the Vulkan SDK; both are left out. The
  OpenGL renderer is unaffected.
- The four curl options and the JACK switch keep the binary from linking system libraries the
  release binary does not use. LDAP in particular pulls a second OpenSSL version into the process.
- `CMAKE_BUILD_WITH_INSTALL_RPATH` gives the binary the install run path set in
  `CMake/Helpers/CPackSetup.cmake` (`$ORIGIN`, `$ORIGIN/../lib`, `$ORIGIN/ffmpeg`, plus the folder
  OpenSSL is linked from, here `/usr/lib/openssl-3`), so it finds the Discord and ffmpeg libraries
  of the game folder it is copied into and does not depend on this source tree.

The binary is written to the repository root as `Etterna-RelWithDebInfo`. Copy it into an installed
game folder under a name of its own, beside the release binary, and start it from there: the game
uses the folder of the executable as its data folder.

## Moving to a new release

1. Fetch the new release tag from upstream.
2. Rebase the changes listed above onto the new tag and resolve conflicts.
3. Build, then compare load times and a play session against the release binary before relying
   on the result.
