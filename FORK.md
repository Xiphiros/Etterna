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
| Never upload a score to the online service's score API | `src/Etterna/Singletons/DownloadManager.cpp` | While `SCORE_UPLOAD_ENABLED` is false, `ShouldUploadScores` returns false, and `UploadScore` and `UploadBulkScores` log the skip and return without sending a request. An upload carries the SHA-256 of the running binary, which differs from the release binary's. Scores are still saved in the local profile. Logging in, leaderboards, goals, favorites, playlists and pack downloads work as in the release, except that login no longer starts a score sync. A player logged in to a multiplayer server still reports each score to it. |

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
