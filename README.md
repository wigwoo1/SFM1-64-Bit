# SFM1 64-Bit — experimental reconstruction

An independent native Windows x64 application working toward compatibility with
Source Filmmaker sessions, assets and editing workflows. This is unfinished
research and prototype code. It is not an official Valve release, a recompiled
Valve SFM, or a fully compatible replacement.

## Current state

The prototype has a Win32 editor, Vulkan rendering experiments, session parsing
and editing components, and many isolated compatibility tests. Complete animation,
material, lighting, particle and workflow fidelity is still unverified. Isolated
passing tests do not establish full application compatibility.

Save and Save As intentionally save **new copies only**. Existing files must not
be replaced. Keep originals and backups; experimental output requires validation.

## Download and run

The current Windows x64 development build is in
[downloads/sfm64-experimental-windows-x64.zip](downloads/sfm64-experimental-windows-x64.zip).
Open that file on GitHub, use **Download raw file**, then extract the ZIP.
It includes usage instructions, its build manifest and SHA-256 checksums.
A fresh compile and AMD64/PE32+ header check are recorded. This publication run
did not repeat interactive rendering or establish visual equivalence.

You need your own installed SFM content and a working Vulkan driver. No Valve
assets, sessions or runtime DLLs are included. See `RUNNING.txt` in the ZIP.

## Build the complete editor

On Windows, install Visual Studio C++ Build Tools and Windows SDK, then run:

```bat
work\build_sfm64_editor_x64.cmd
```

The script names a particular local Visual Studio installation; adjust that path
for your installation. Licensed Vulkan headers and the LZMA decoder source subset
are included under `work/dependencies/`. The output is
`outputs/sfm64-editor-x64/sfm64_session_ui.exe`.
`build-session-core.cmd` builds the session core, not the complete editor.

Many historical tests require local SFM or fixture/evidence files and still
contain development-machine paths. They are research utilities, not a portable
CI suite. Original machine-code oracle arrays and proprietary fixtures are
excluded; tests that need them cannot run from this public snapshot alone.
The application does not depend on the x86 reference oracles.

## Publication contents and updates

Project-authored source/scripts and licensed source dependencies are included.
Dumps, logs, copied original binaries, game assets, sessions, `.blend` files,
object files and private workspace metadata stay local.
`publication/prepare_public_snapshot.py` defines the allowlist;
`docs/publication-manifest.json` records published file identities.

Daily synchronization is requested through the local Codex automation. It requires
the computer and Codex to be running with the workspace and GitHub connection
available. GitHub alone cannot upload unpublished files from this PC.

## Rights and third-party notices

Source Filmmaker and Source are Valve trademarks. This project is unaffiliated.
No blanket open-source license is granted in this snapshot. Third-party notices
remain with included Vulkan headers and LZMA decoder source. Public visibility
does not grant rights to redistribute Valve software or assets.
