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


## Prepared public swept-hull trace checked (2026-11-27)

Fresh warning-free x86/x64 builds, each repeated twice, match 19,264 prepared
benchmark queries and 36 independent coordinate-finishing predictions. All 84
public trace bytes plus the displacement marker match, including the retained
brush surface index, asymmetric hull offsets, signed zero, disabled position
output and unavailable world. The previous 16,512-query benchmark regression
was freshly rebuilt and repeated in both architectures with unchanged records.

Original full wrapper/reset/copy/finish and geometry routines execute in the
private reference fixture; work/cache services and material property numbers
are explicitly supplied. No original instruction or lookup-table data is bundled.
This is an isolated prepared swept non-ray boundary, not a demonstrated editor
improvement. Original whole-world loading, stationary dispatch, production cache/
resource lifecycle, effective material properties, entities/static props and
full application compatibility remain unverified. All 543 editor dependencies
are unchanged; the current editor package is retained.


## Stationary hull brush primitive checked (2026-11-27)

Fresh warning-free x86/x64 builds, each repeated twice, match 790,368 cases
against the intact original stationary brush primitive, with no substituted
callees. All 1,866 benchmark brushes are exercised through classified and general
plane routes. Nonzero and asymmetric relative hulls, touching/adjacent outside
positions, bevels, empty brushes and prior metadata are covered. Complete trace
bytes and allowed changes to the native work buffer match; 255,072 box/empty
cases have additional independent interval predictions.

This verifies the brush calculation only. Stationary displacement overlap and
fallback tracing, leaf/world composition, actual spawn behavior and editor
integration remain unverified. Existing point/swept helpers and all 543 editor
dependencies are unchanged. No original instruction/table data, assets, reference
records or disassembly is included in the public source.


## Prepared stationary displacement overlap checked (2026-11-27)

Fresh warning-free x86/x64 builds, each repeated twice, match 233,161 direct
triangle and complete mesh overlap queries. Both benchmark face modes (43 meshes
each) and 16 controlled meshes are covered. Original complete tree/triangle/edge/
plane routines execute without substituted callees. Exact triangle order and
arguments match. Independent flat fixtures validate 1,225 predictions, including
the native plane-side boundary behavior. The owned x64 overlap test needs no
original engine or permutation fixture at runtime.

An earlier private handoff stopped at an early return; it has been corrected to
include the reachable triangle-testing tail through the actual function end.
All previous brush/public-wrapper helpers and all 543 editor dependencies remain
unchanged. This is isolated prepared collision work: normal/ray fallback, full
stationary world composition, actual spawn behavior and editor integration are
still unverified. Original code/table data, assets and reference evidence remain
excluded from publication.


## Stationary hull normal and prepared fallback checked (2026-11-27)

Fresh warning-free x86/x64 test builds, each repeated twice, match 11,200 full
normal-selection cases and 16,512 complete prepared fallback queries. The latter
executes original reset, temporary-ray arithmetic, nested visit setup, mixed hull
world geometry, depth finish and prior-end restoration. Only declared cache
storage/acquire/release services are supplied. All 560 working-buffer bytes match
after private-pointer/surface-name normalization. Independent normal/delta/inverse
predictions, generation rollover, preserved parent visits and LDR/HDR equality
are checked.

The shared hull helper now accepts an explicit displacement visit flag; existing
swept callers default to their previous behavior. Fresh synthetic mixed, benchmark
geometry and public swept-wrapper regressions repeat both architectures and retain
their accepted record hashes. All 543 editor dependencies are unchanged.

This is prepared collision work, not full stationary dispatch or an editor/spawn
improvement. Direct-overlap/fallback policy composition, stationary leaf/world
dispatch, effective material binding, global cache/resource lifecycle and engine/
server filtering remain unverified. Private permutation input is still required
by the isolated hull sweep fixture; none is bundled in public source or editor.


## Prepared stationary displacement dispatcher checked (2026-11-28)

Fresh warning-free x86/x64 test builds, each repeated twice, match 131,868 queries:
125,388 benchmark and 6,480 controlled. Complete working buffers, parent visits,
and exact cull/overlap call order match the intact original dispatcher. Controlled
two-leaf geometry produces 810 positive-solid fallbacks using real brush and
terrain calculations. Independent visit ordering, direct metadata preservation,
controlled normal/cull order and temporary-ray arithmetic checks also pass.

The domain is prepared non-ray work with a nonempty displacement list and finite
centered relative hull bounds. Complete stationary leaf/world/public dispatch,
server spawn, entities/static props/filters, effective material binding and global
cache/resource lifecycle remain unverified. Original instructions/table data and
private fixtures stay excluded. All 543 editor dependencies and its package remain
unchanged; this is not a demonstrated editor improvement or full SFM compatibility.


## Prepared stationary hull leaf checked (2026-11-28)

Fresh warning-free x86/x64 test builds, each repeated twice, match 145,440 complete
leaf queries: 123,840 benchmark and 21,600 controlled. The full original leaf calls
real brush and terrain routines. Working buffers, complete parent visits and exact
brush/cull/overlap call order match. Independent policy replay and controlled box
predictions also pass. Tests cover the stationary rule differences: visit marking
before contents filtering, omission of swept surface exclusion, zero-fraction
termination only after a brush call, and the prior startSolid terrain gate.

This verifies prepared single-leaf processing with nonzero centered hulls. World
leaf selection/solid-leaf fallback, public stationary setup/finish, server spawn,
entities/static props/filters, material/physics binding and global cache/resource
lifecycle remain unverified. No original instruction/table data or private inputs
are bundled. All 543 editor dependencies and its package remain unchanged. This
is not a demonstrated editor improvement or complete SFM compatibility.


## Prepared stationary hull world checked (2026-11-28)

Fresh warning-free x86/x64 test builds, each repeated twice, match 147,240 complete
world queries. The original selector, stationary brush/terrain processing and real
nested fallback execute over prepared benchmark and controlled geometry. Full
working buffers, parent visits, selected/visited leaf order, top node and nested
fallback generations match. Independent selection, solidity-finish, zero-mask
trace and metadata checks pass. A 1,030-leaf fixture checks the 1,024-result cap;
1,080 cases distinguish selected non-solid leaves from those actually visited.

The existing selector is reused; no original instruction/table data is bundled.
Queue overflow and malformed topology remain outside this comparison. Public
stationary trace setup/finish, empty-world return, point-ray routing, engine/server
entities/static props/filters, effective material binding and global cache/resource
lifecycle remain separate gates. All 543 editor dependencies and its package are
unchanged. This is not a demonstrated editor improvement or full SFM compatibility.


## Public stationary hull traces checked (2026-11-28)

Fresh warning-free x86/x64 builds, each repeated twice, match 133,192 prepared
public stationary non-ray hull traces. Original reset/default setup, actual
stationary world collision, optional coordinate finishing and full result copy
execute in the private reference. All 84 public bytes and the separately observed
internal displacement marker match. Independent classification, coordinates,
zero-mask world finish, empty defaults and wrapped/fresh comparisons pass.

Tests retain 33,952 nonzero deltas classified nonswept, cover 11,152 overflowing
initial inverse components, and preserve signed-zero/cancellation results. Empty
worlds return before flag validation. Work/cache storage and material properties
are explicit fixtures; no original code or table data is bundled.

Available-world isRay=true point routing, effective material/physics binding,
loader/global cache lifecycle, server entities/static props/filters and actual
spawn/editor integration remain separate gates. Existing swept/world helpers and
all 543 editor dependencies are unchanged. The existing editor package is retained;
this is not a demonstrated editor improvement or complete SFM compatibility.


## Prepared point-world geometry checked (2026-11-28)

Fresh warning-free x86/x64 builds, each repeated twice, match 109,248 prepared
point-world queries: 66,048 benchmark and 43,200 controlled. Complete working
buffers, parent visits, leaf intervals and actual brush/terrain order match the
intact original routines. Hardware observations execute through original code;
no geometry or storage callee is replaced. Independent brush-policy, preserved
nonswept visits, material/plane metadata, zero-fraction and guard checks pass.

The point walker uses tiny hull extents at axial BSP planes and zero expansion
at oblique planes. Optimized brushes expand relative slabs even for point rays;
the adapter preserves their point solid-exit behavior and original plane/material
data. Actual brush-to-terrain and terrain-to-brush winner transfers are covered.
A fixture node-count binding and missing brush extent expansion were corrected
before final verification. Original instructions/table data and assets are not
bundled; the owned point kernel requires no private permutation table.

This verifies prepared point geometry, not public point setup/finish or stationary
isRay=true dispatch. Effective material/physics IDs, original whole-world loading,
global lifecycle/concurrency, entity/static-prop/filtering, real spawn and editor
integration remain separate gates. All 543 editor dependencies and the existing
experimental package remain unchanged. Full SFM compatibility is incomplete.


## Prepared stationary point dispatcher checked (2026-11-28)

Fresh warning-free x86/x64 builds, each repeated twice, match 60,354 prepared
stationary point-displacement queries. Original direct overlap, normal selection,
fallback and actual point world geometry run without substituted callees or
storage services. Complete work buffers, parent/child visits, normal metadata
and cull/overlap order match. Every 57,514 fallback per native run is observed
entering the original point world route and restoring its prior end/depth.

The shared dispatcher accepts an explicit point flag while preserving its
non-ray default. Tiny nonzero point bounds still permit direct overlap. Tests
cover 2,840 direct contacts, 1,008 real solid fallbacks, generation rollover and
the different overlap/ray effects of runtime flags 4 and 8. Independent visit,
normal, overlap, arithmetic, metadata-preservation and guard predictions pass.
Fresh non-ray dispatcher and public stationary trace regressions repeat both
architectures and retain their accepted complete-record hashes.

This remains prepared single-list collision work. Complete stationary point
leaf/world/public orchestration, effective material/physics binding, original
whole-world loading, global lifecycle/concurrency, entity/static-prop/filtering,
real spawn and editor integration remain separate gates. The primary point
fixture needs no private permutation input; older non-ray regressions retain
their declared private fixtures and cache adapters. No original instructions,
lookup tables, assets or private evidence are bundled. All 543 editor dependencies
and the existing experimental package remain unchanged; full SFM compatibility
is incomplete.


## Prepared stationary point leaves and world checked (2026-11-28)

Fresh warning-free x86/x64 builds, each repeated twice, match 162,288 prepared
queries: 63,720 standalone leaf and 98,568 world calls over benchmark, controlled
and capacity fixtures. Full work buffers, parent/child visits, selected/visited
leaf order, stationary brush order, terrain gates and per-leaf state match the
intact original routines. Lossless child visit records retain generation history.
No original callee, storage service or geometry result is substituted.

The leaf/world bindings forward point mode with their non-ray defaults retained.
Tests include 108,144 tiny nonzero point-classified hulls, multiple nested
fallbacks, the 1,024-leaf selection cap and selected-but-unvisited non-solid
leaves. Independent selection, solidity finish, controlled brush/terrain gates,
zero-mask traces, metadata, visit lifecycle and ray/guard checks pass. Fresh
non-ray leaf, world and public stationary regressions repeat both architectures
and retain their accepted complete-record hashes.

Public stationary point setup/finishing, retained-underflow-delta handling at
that boundary, swept point routing, effective material/physics binding, original
whole-world loading, global lifecycle/concurrency, engine/server entities/static
props/filters, real spawn and editor integration remain separate gates. Primary
point tests need no private permutation/cache resource; older non-ray regressions
retain their declared fixtures/adapters. No original instructions/tables, assets
or private evidence are bundled. All 543 editor dependencies and the existing
experimental package remain unchanged. Full SFM compatibility is incomplete.


## Prepared public stationary point traces checked (2026-11-28)

The recorded component receipt verifies 133,192 prepared public stationary point
traces. Fresh warning-free x86/x64 builds each repeated twice match all 84 public
trace bytes and the separately observed internal displacement marker. Original
reset/setup, stationary geometry, optional coordinate finishing and copying run
with explicit work acquire/release fixtures. Independent coordinate, empty-world,
classification, signed-zero, tiny-hull and retained-delta checks pass. Fresh
stationary and swept non-ray regressions retain their accepted record hashes.

Point execution needs no private hull cache/permutation resource. Effective
material/physics binding, original whole-world loading, production pool lifetime
and concurrency, public swept point routing, entity/static-prop/filtering, real
spawn and editor integration remain separate gates. No original instructions,
lookup tables, assets or private evidence are bundled. All 543 editor dependencies
and the existing experimental x64 package remain unchanged. Full application
compatibility and visual equivalence remain incomplete.
