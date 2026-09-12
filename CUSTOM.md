# RawTherapee custom build

Patched RawTherapee 5.13 for macOS (Apple Silicon). Changes on top of the 5.13 tag:

## Film Simulation as a folder tree
The film simulation picker is a permanently visible tree (like Lightroom's preset panel)
instead of cascading menus. Folders are bold and toggle on click, films apply on click,
Up/Down step through films once the tree has focus, Left/Right collapse/expand.
A label above the tree shows the film currently applied.

Right-click a film to **Star** / **Unstar** it. Starred films get a ★ marker and are
collected in a "★ Favorites" folder at the top of the tree. The list is stored in the
RawTherapee options file (`[Color Management] FilmSimulationFavorites`, paths relative
to the HaldCLUT folder) so it survives restarts and folder moves.
Files: `rtgui/tools/filmsimulation.{h,cc}`, `rtgui/options.{h,cc}`.

## Global Film Grain tool
A new **Film Grain** tool in the Color tab, right under Film Simulation, with
ISO (grain size distribution), Strength, Scale and Gamma sliders. It applies the
Local Adjustments grain generator to the whole image and is saved in the `.pp3`
under `[FilmGrain]`. Judge it at 100% zoom: the fit-to-screen preview is smoother
than the exported file.
Files: `rtgui/tools/filmgrain.{h,cc}`, `rtengine/ipgrain.cc` (`filmGrainGlobal`),
`rtengine/procparams.*`, pipeline calls in `improccoordinator.cc`, `dcrop.cc`, `simpleprocess.cc`.

## Keyboard shortcuts
| Key | Action | Displaced binding |
| --- | --- | --- |
| `d` (file browser) | open selection in the editor | clear filters is now `Shift+D` |
| `f` | toggle fullscreen | quick inspector / focus mask are now `Shift+F` |
| `Cmd+Z` / `Cmd+Shift+Z` | undo / redo in the editor | (Ctrl+Z still works) |

## Building on macOS (Homebrew, arm64)
```sh
brew install libtiff gtk+3 gtkmm3 gtk-mac-integration adwaita-icon-theme libsigc++@2 \
  little-cms2 libiptcdata fftw lensfun expat pkgconf llvm shared-mime-info exiv2 \
  jpeg-xl libomp automake libtool simde imagemagick libffi
```
Configure with the flags from the arm64 job in `.github/workflows/macos.yml`, plus
`PKG_CONFIG_PATH` starting with `/opt/homebrew/opt/libffi/lib/pkgconfig` (Homebrew's
libffi shim points at a Command Line Tools SDK that may not exist). Keep MacPorts
(`/opt/local`) out of `PATH`. Then:
```sh
make -C build -j"$(sysctl -n hw.ncpu)" install
tools/osx/install-custom.sh     # bundle, re-sign, install to /Applications
```
`install-custom.sh` re-signs the bundle as `com.rawtherapee.rawtherapee5` (the id the
official release uses, so existing settings carry over) and without the hardened
runtime (ad-hoc dylibs fail library validation otherwise). `tools/osx/macosx_bundle.sh`
has its `sudo` calls removed so the bundle step runs unattended.
