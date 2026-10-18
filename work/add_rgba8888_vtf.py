from pathlib import Path
p=Path('work/vtf_texture_layout.h');s=p.read_text();s=s.replace('#include <stdexcept>','#include <stdexcept>\n#include <string>')
s=s.replace('throw std::runtime_error("unsupported VTF block format");','throw std::runtime_error("unsupported VTF block format "+std::to_string(format));')
s=s.replace('return uint64_t((w+3)/4)*((h+3)/4)*blockBytes(format);','if(format==0)return uint64_t(w)*h*4;return uint64_t((w+3)/4)*((h+3)/4)*blockBytes(format);')
s=s.replace('auto format=word(52);blockBytes(format);','auto format=word(52);if(format!=0)blockBytes(format);')
s=s.replace('single-frame 2D block-compressed texture candidate.','single-frame 2D RGBA8888 or block-compressed texture candidate.')
p.write_text(s)
p=Path('work/vtf_block_decode.h');s=p.read_text();a='auto mip=layout.mips[level];if(layout.format!=13';b='auto mip=layout.mips[level];if(layout.format==0){VtfRgbaImageCandidate image;image.width=mip.width;image.height=mip.height;image.rgba.assign(bytes.begin()+mip.offset,bytes.begin()+mip.offset+mip.size);return image;}if(layout.format!=13';assert s.count(a)==1;s=s.replace(a,b);p.write_text(s)
