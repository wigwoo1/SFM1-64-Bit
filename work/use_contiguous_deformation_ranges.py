from pathlib import Path
p=Path('work/geometry_deformation_worker_pool.h');s=p.read_text()
a='for(size_t n=index;n<r->size();n+=count)'
b='const size_t batch=r->size()/count,remainder=r->size()%count;const size_t first=index*batch+std::min(index,remainder),last=first+batch+(index<remainder?1:0);for(size_t n=first;n<last;++n)'
assert s.count(a)==1;s=s.replace(a,b)
s=s.replace('// One batch at a time; immutable inputs remain alive until deform() returns.','// One batch at a time; immutable inputs remain alive until deform() returns.\n// Contiguous balanced ranges avoid interleaved output writes across workers.')
p.write_text(s)
