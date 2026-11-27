# Daily GitHub publication

Destination: https://github.com/wigwoo1/SFM1-64-Bit (public, main branch).

The application is incomplete. Keep publication separate from feature development.
Original SFM files on F: are read-only. Do not upload the entire workspace.

1. Read state.json, inspect remote main, and compare current editor dependency
   hashes to docs/build-status.json. Rebuild work/build_sfm64_editor_x64.cmd if an
   editor dependency changed. Preserve the last published binary if build fails.
2. Run prepare_public_snapshot.py with the workspace Python. It stages explicitly
   allowlisted source and licensed C Vulkan headers/LZMA source, checks the editor
   include graph, builds a deterministic ZIP, verifies ZIP contents, and creates
   payloads/tree-N.json plus prepared.local.json. Review its exclusions and diff.
3. Review added/changed files for credentials and source provenance. Reject extra
   original assets, copied DLLs, sessions, memory/disassembly dumps and native
   instruction arrays. Do not relax the allowlist automatically.
4. Upload changed text batches through GitHub create_tree(content entries), using
   the preceding temporary tree as base. Upload large files via create_blob with
   base64 and add their returned Git blob SHAs. These temporary trees are not
   visible on main until committed. Rebase the staged changes on current main's
   tree to preserve remote-only files. Do not force-push or delete remote files.
5. Create a commit with the inspected current main as parent. Update main with
   expected_sha matching that parent and force=false. A mismatch requires a fresh
   read/review; never bypass it. Never print credentials.
6. Fetch the recursive remote tree and compare every manifest path's mode/type
   and Git blob SHA to the bytes in stage (Git hash is SHA1 of 'blob N\0'+bytes).
   Verify the actual branch head, experimental build blob and public visibility.
   Only then write state.json with commit, tree, files SHA256s and verified time.
   Keep a local receipt. Do not report uploaded objects as a successful publish
   until main points to the verified commit.
7. The daily automation checks at 9 AM America/Indianapolis. It updates the real
   sync-check timestamp even without source changes, and preserves identical
   binary ZIPs. It requires the local computer/Codex/workspace/connection online.

Use the connected GitHub tool APIs if local Git authentication is absent. Large
local JSON batches can be read with node_repl fs and passed directly through
functions.exec to tools without printing the full source/binary data to chat.
GitHub releases are not exposed by the current connector; the current build ZIP
is versioned at downloads/sfm64-experimental-windows-x64.zip instead.

For a required publication rebuild, use a fresh output directory with
`work/build_sfm64_editor_x64.cmd outputs/publication-editor-x64-<timestamp>`.
Then pass that directory to `prepare_public_snapshot.py --build-directory`.
This packages the actual isolated build without replacing the canonical editor
or changing feature-test checkpoints. The option accepts workspace outputs only.
Record its exit status, architecture checks and compiler warnings in the local
publication receipt. A successful compile alone does not establish visual parity.

The runtime ZIP must include the owned `shaders/` directory beside the editor.
`work/editor_shader_manifest.txt` explicitly lists the project-authored HLSL
sources and compiler profiles. The complete editor build compiles them with
DXC. `editor_runtime_package.py` validates those SPIR-V artifacts and packages
only their listed names and the fixed editor/documentation files. It never
copies Valve shader caches or the shader compiler. The snapshot dependency
hashes include these HLSL sources, local includes, and build manifest/scripts,
so a shader source change requires a rebuild. A missing shader or mismatch
between the renderer references and manifest must stop publication.

Files omitted by policy may be required by historical research-oracle tests.
Do not claim the public snapshot includes those proprietary fixtures or that
all tests can run from a clean clone. Do not grant a blanket project license.

## Bounded research correction verified (2026-10-28)

`work/vertexlit_generic_post_blend_candidate.h` and the associated
`probe_vertexlit_generic_preparation.cpp`, `build_vertexlit_generic_preparation.cmd`
and `verify_vertexlit_generic_preparation.py` now have a passing bounded offline
comparison. The incorrect expected-result variable and missing flashlight gate
were corrected. Four fresh warning-free builds passed; 6,204 cases match the
original x86 prefix, owned x64 helper, and independent prediction, with both
executables repeated. The preceding 5,088-case regression and observer self-test
also pass. Evidence is recorded locally in
`outputs/vertexlit-generic-preparation-correction-20261028/verification-63dui0b0/results.json`.
Historical failed receipts are retained separately.

This verifies 15 selected material locals and three route arguments at the
boundary before static/dynamic rendering, for the documented installed schema.
It does not verify GPU submission, rendered images, full generic-renderer
cleanup, cached contexts, or other shader routes. The helper is not integrated
into the editor; all 313 editor dependencies and its executable are unchanged.
Do not describe this as a verified visual improvement or a completed renderer.
Private reference inputs and traces remain excluded from public publication.

## Prepared runtime package checked (2026-10-30)

The complete editor was rebuilt with all 15 project-authored Vulkan shaders.
The prepared 20-file ZIP was extracted into a separate directory and executed
from an empty working directory. Two noninteractive control-test sequences
passed ten GPU frames: normal startup required no diagnostic environment or
output directory; explicit capture matched the prior verified candidate's
frames 0, 119, and 120 byte for byte. The same 15 compiler warnings remain.
The packager itself does not execute runtime tests; this check follows it.

Tested editor SHA-256:
`d01549900fe244c4fc8c039f75b9b63725e3b0117194881ff96aec606f644db1`.
Tested ZIP SHA-256:
`e02bdbda30c6e75d805f4baf26048bebd2fc524e8bcb3270f1229e21648e8994`.
Original session/gameinfo hashes remained unchanged. Runtime logs and benchmark
captures stay local and are excluded from publication. This establishes this
package's startup and measured pixel regression, not full Source shading,
interactive workflow acceptance, or complete SFM compatibility.

## Current collision research status (2026-11-27)

The current editor package is unchanged by the newly published collision research.
Isolated comparisons cover swept hull BSP traversal, brush contacts/dispatch,
brush-only world composition, displacement tree selection, triangle contacts for
supplied caches, and edge-plane geometry. These do not establish complete world
collision or editor integration.

`work/source_displacement_plane_pool_candidate.h` now requires explicit
permutation data; its incorrect random-generator default has been removed.
Hash, exact/sign lookup, cache IDs and bucket records pass isolated native/x64
comparisons with an identified private table fixture. All 256 published Source SDK
table entries were independently checked against the installed reference.
No Valve permutation table is embedded in project source or the editor package.
Production resource distribution, full cache lifecycle and editor integration
remain unverified. Existing editor runtime is unaffected. Private table fixtures,
failure receipts and native reference dumps remain excluded from publication.


## Prepared displacement sweep composition checked (2026-11-27)

An isolated composed helper now connects generated hull cache data, tree selection
and actual triangle contacts. Fresh warning-free x86/x64 builds each repeated
twice match 41,472 queries over 48 prepared meshes, including complete trace bytes
and closer-hit results. The native oracle executes original geometry/contact
routines with explicit fixed-storage/cache-service adapters; no original code
arrays or permutation constants are added to the public source or editor.

This remains prepared-input collision research. Global cache lifecycle, original
BSP import through this sweep, outer world metadata, full world composition,
stationary dispatch and editor integration are separate gates. All 543 editor
dependencies are unchanged; this is not a new editor build or a demonstrated
benchmark collision improvement. Private reference fixtures and receipts remain
excluded from publication.


## Outer world hull metadata checked (2026-11-27)

The prepared displacement sweep now has an independently verified world metadata
adapter. Fresh warning-free x86/x64 builds, each repeated twice, match all 544
working-buffer bytes for 41,472 cases. Real contacts exercise both material
property choices, retained prior surface information and zero-fraction metadata.
Native leaf-interval argument variations produce identical results. Owned code
uses semantic surface identity; native addresses occur only in the local probe.

World leaf culling/dedup/finish, complete world composition, native global cache
lifecycle, stationary dispatch and editor integration remain unverified. This
research does not change the current editor build. Private reference inputs and
receipts remain excluded from public publication.


## Hull leaf displacement processing checked (2026-11-27)

An isolated hull leaf policy now composes contents filtering, visit suppression,
hull-expanded bounds culling, real displacement contacts and final solid-state
updates. Fresh warning-free x86/x64 builds, each repeated twice, match 30,720
queries including complete working buffers, visit tables and candidate order.
Native bounds/outer-entry observations execute through unchanged routines.

Brush input is empty or a zero-contents record; mixed brush-contact/world traversal
is still unverified. Prepared mesh/cache/cull/material inputs, native global cache
lifecycle, production permutation resource, stationary dispatch and editor
integration remain separate gates. This work does not change the current editor
package or demonstrate a benchmark spawn improvement. Private oracle inputs,
records and native dumps remain excluded from publication.


## Mixed prepared world hull geometry checked (2026-11-27)

The owned swept-box world helper now combines BSP traversal, brush contacts and
real displacement contacts. Fresh warning-free x86/x64 builds, each repeated
twice, match 64,512 controlled queries and final trace/material fields. Native
observations establish both brush-over-displacement and displacement-over-brush
winner changes. A missing default material update on brush solid-exit reset was
identified from native instructions and corrected before final verification.

This is prepared synthetic world geometry equality. Full original BSP loading,
material binding, cache lifecycle/resource distribution, public trace setup and
finish, stationary dispatch, entities/static props and editor integration remain
separate gates. The current editor build is unchanged. Private reference inputs,
failed receipts, native dumps and test records remain excluded from publication.


## Benchmark-backed prepared hull geometry checked (2026-11-27)

The mixed hull query now has an isolated comparison using actual mtt_soldier BSP
brush/node/leaf tables and all 43 imported displacement meshes. Fresh warning-free
x86/x64 builds, each repeated twice, match 16,512 queries across LDR/HDR face modes;
the two modes produce identical trace records. Actual 2,478 texture-info bindings,
87 material identities and merged BSP surface flags are used. Reusable derived
cache/cull/leaf tables were added with explicit permutation input.

Material property numbers are distinguishing test fixtures, not verified original
IMaterial/physics IDs. Native query routines receive prepared tables; this does
not execute the whole original BSP loader or prove public trace setup/finish,
stationary dispatch, cache lifecycle, entity/static-prop filtering, measured spawn
or editor integration. The editor package is unchanged. Original BSP/assets,
permutation data, native dumps and test records remain excluded from publication.


## Public swept-hull wrapper under verification (2026-11-27)

The newest public-trace wrapper and associated probe are work in progress.
An initial comparison passed 19,264 prepared benchmark queries and 36 coordinate
finishing cases. Final repeated x86/x64 verification and the signed-zero fixture
revision remain pending. Do not treat this wrapper as a completed compatibility
gate or an editor improvement. It is isolated from the current editor package.
Original world loading, stationary dispatch, production cache/resource lifecycle,
entity/static-prop handling and full application compatibility remain unverified.
