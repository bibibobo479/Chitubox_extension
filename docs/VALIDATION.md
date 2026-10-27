# Validation — v1.0.0

Validated on 2026-10-27 on Linux x86_64 (Arch / Hyprland / XWayland) with the CHITUBOX Basic 2.3.1 library hashes listed in `compatibility.json`.

- Geometry regression suite: passed; includes 40 mm protrusion and recess, gaps, normal direction, spacing/endpoints and limits.
- Installer/launcher: 9 tests passed for install, upgrade, removal, preservation of user files, unsupported host rejection, archive/path/link validation, checksum mismatch, and isolated Qt environment.
- Touchpad routing: passed Ctrl-only routing, both axes, native event forwarding, held-button safety, momentum, balanced drags and disable switch.
- Native GUI: two successive rows created on the supplied 30 mm boss fixture without reselecting the tool. Native support component counts grew from 0 to 66 to 136 (these counts are components, not individual supports).
- Panel verified directly below Auto Supports. Collapsing it kept the tool active. Escape reset a pending first point; Escape without one exited the tool.
- Undo removed only the second row (136 → 66 components); redo restored it (66 → 136).
- Clicking native Add and selecting a native keyboard tool both exited the line tool.

Tests ran in a dedicated application profile with an authored fixture, not on a user's project. The complete build/install payload was also tested with the native GUI before release.

Other distributions, display scaling factors, input devices, printers, and CHITUBOX versions have not been comprehensively tested. Native support generation can reject individual contacts; complex models require inspection.
