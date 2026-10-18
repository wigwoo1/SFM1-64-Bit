from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include <charconv>','#include <charconv>\n#include "single_model_sequence_manifest_candidate.h"')
old='auto directory=std::filesystem::path(sequencePath)/("frame-"+std::to_string(frame));auto pose='
assert old in s
s=s.replace(old,'auto directory=std::filesystem::path(sequencePath)/("frame-"+std::to_string(frame));checkSingleModelSequenceManifestCandidate(directory,mount.parent_path(),base);auto pose=')
p.write_text(s)
