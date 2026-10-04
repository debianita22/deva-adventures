# SDL2 for the Windows and macOS packages

The PC program loads SDL2 at run time. On Linux it uses the system's; the Windows and macOS packages
carry the official build of the SDL project, pinned and checked here:

| | |
| --- | --- |
| version | 2.32.10 (tag `release-2.32.10`, 1 September 2025) |
| Windows | `SDL2-2.32.10-win32-x64.zip`, SHA-256 `6cf9706eefd0a4a06dc764007934d428afaf029fabdd408a9e646048c91e18fb` |
| macOS | `SDL2-2.32.10.dmg` (SDL2.framework, arm64 + x86_64, signed by the SDL project), SHA-256 `4a7ac31640d70214e848f994be8a12849c0f97918a7e6c2e27a40036166d1a7f` |
| source | <https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10> |
| license | zlib, `LICENSE.txt` here (from the same tag) |

`make sdl2-windows` and `tools/release/mkapp.sh` download them into `build/sdl2/` and refuse any other
bytes. To move to a newer SDL2: change the version and both hashes here, in the `Makefile` and in
`tools/release/mkapp.sh`, then run `make test-windows` and the CI.
