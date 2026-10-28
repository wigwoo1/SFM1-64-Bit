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

Files omitted by policy may be required by historical research-oracle tests.
Do not claim the public snapshot includes those proprietary fixtures or that
all tests can run from a clean clone. Do not grant a blanket project license.

## Unaccepted research snapshot (2026-10-28)

`work/vertexlit_generic_post_blend_candidate.h` and the associated
`probe_vertexlit_generic_preparation.cpp`, `build_vertexlit_generic_preparation.cmd`
and `verify_vertexlit_generic_preparation.py` are unfinished research drafts.
Their isolated builds compiled without warnings, but full trace comparisons
failed. Neither native/x64 agreement nor independent prediction agreement has
been established for this extension. It is not integrated into the editor.
The existing editor download is unchanged. Do not describe these source files
as a verified renderer improvement; retain this notice until a reviewed passing
verification replaces the failed milestone.
