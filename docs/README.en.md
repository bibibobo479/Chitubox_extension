# CHITUBOX Extension

Unofficial line-support tool and **Ctrl + touchpad camera orbit** for native **CHITUBOX Basic 2.3.1 / Linux x86_64**. CHITUBOX must be installed separately.

## Install

Requires Python 3.9+, curl and glibc 2.38+. Run as your desktop user, **without sudo**:

```bash
curl -fsSL https://raw.githubusercontent.com/bibibobo479/Chitubox_extension/main/install.py | python3 -
```

This executes the repository's installer. To review it first, download [install.py](../install.py), read it, then run `python3 install.py`. Launch **CHITUBOX + Extensions** from your application menu. Running the installer again updates the extension. If detection fails, use `python3 install.py --app "/path/to/CHITUBOX"` (directory containing `bin/CHITUBOX_Basic` and `lib`).

For offline installation, download the `.tar.gz` asset and `SHA256SUMS` from [Releases](https://github.com/bibibobo479/Chitubox_extension/releases/latest), verify with `sha256sum -c SHA256SUMS`, extract into an empty directory and run its `install.py`.

## Use

1. Open the Supports workspace; select **Regular → Add** and the desired native support size and parameters.
2. Open **Линейные поддержки** (Line supports) under Auto Supports. Set spacing in millimetres and whether to include the last endpoint.
3. Select **Инструмент «Линия»** (Line tool), or press **Ctrl+Shift+L**.
4. Hover over the model until the native support preview appears, then click two surface points on the same model and side. A row is created using the selected native support parameters.
5. Keep selecting pairs: the tool remains active until you choose another mode. Native Add/Delete/Edit switches out of the tool. Clicking the Line button again also exits.

Esc clears a pending first point; with no pending point it exits. Ctrl+Z / Ctrl+Y undo/redo a whole row. The panel can be collapsed while the tool remains active. **The extension panel currently uses Russian text**; the host application's language is unchanged.

Intermediate points project onto the outermost facing model surface along interpolated endpoint normals, including high protrusions and recesses. Spacing is measured along the straight 3D chord, not along the curved surface. Holes and contacts rejected by CHITUBOX are skipped and counted. Limits: 0.2–100 mm spacing, 500 positions, 2 million triangles, 50 million triangle-position checks. Regular supports only; complex overhangs still require visual inspection.

Hold **Ctrl** and move two fingers to orbit the camera. Other input retains native behavior. XWayland uses `xcb`. On devices where Qt cannot identify touchpad input, the extension reads touch contact state from an accessible Touchpad input device. It does not grant input-device permissions; unavailable access means ambiguous gestures retain native behavior.

```bash
CHITUBOX_TOUCHPAD_SPEED=0.7 ~/.local/bin/chitubox-extended
CHITUBOX_TOUCHPAD_DISABLE=1 ~/.local/bin/chitubox-extended
CHITUBOX_LINE_SUPPORT_DISABLE=1 ~/.local/bin/chitubox-extended
~/.local/bin/chitubox-extended --vanilla
```

Speed range: 0.1–4. Use installer `--no-touchpad` for line supports only.

## Compatibility

Tested on Arch Linux / Hyprland / XWayland with Basic 2.3.1 matching [these hashes](../compatibility.json). Other modern glibc 2.38+ Linux systems with the same host build are expected to work but have not been tested. Windows, macOS, ARM, Flatpak and other CHITUBOX builds are unsupported. An unknown host update disables the extensions at launch.

This uses `LD_PRELOAD` and undocumented internal host APIs. Vendor files are not patched. The release contains only our extension libraries and supporting files, not CHITUBOX or Qt libraries.

Uninstall:

```bash
python3 "${XDG_DATA_HOME:-$HOME/.local/share}/chitubox-extension/install.py" --uninstall
```

CHITUBOX, projects and extension settings are kept. [Development and tests](DEVELOPMENT.md) · [Validation](VALIDATION.md).

## License

Our original code is **MIT**, copyright 2026 SSI, preserving the repository's original license. This project is independent and not endorsed by CHITUBOX / CBD-Tech. MIT grants no rights to CHITUBOX itself and does not supersede its license terms. Use of internal APIs may be subject to separate contractual and legal limits. Qt header/runtime notices and LGPLv3/GPLv3 texts accompany the distribution; see [NOTICE.md](../NOTICE.md). No universal legal clearance is claimed.
