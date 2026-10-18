from pathlib import Path
s=Path('work/probe_generic_ambient_expansion.cpp').read_text().replace('generic_ambient_expansion_candidate.h','queued_vertex_metadata_candidate.h')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,72> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  const unsigned width=record[0]?4:1,index=record[1],count=record[2];
  VertexMetadataUploadCandidate command{record[0]?VertexMetadataKindCandidate::Integer:VertexMetadataKindCandidate::Boolean,index,{record.begin()+4,record.begin()+4+count*width},record[3]!=0};
  bool changed=command.force||!std::equal(command.words.begin(),command.words.end(),record.begin()+20+index*width);
  auto queue=changed?queuedVertexMetadataCandidate(command):std::vector<uint32_t>{};
  if(queue.size()!=record[36]||!std::equal(queue.begin(),queue.end(),record.begin()+37))throw std::runtime_error("native queue words mismatch");
  auto cache=std::vector<uint32_t>(record.begin()+20,record.begin()+36);
  if(changed)std::copy(command.words.begin(),command.words.end(),cache.begin()+index*width);
  if(!std::equal(cache.begin(),cache.end(),record.begin()+56))throw std::runtime_error("native queue cache mismatch");
  ++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_metadata_queues_exact="<<cases<<"\\n";
'''+s[b:]
a=s.index(' // Only original');b=s.index('#endif',a)
s=s[:a]+''' // Original metadata methods and private memcpy only. Queue space avoids refill.
 using Set=void(__thiscall*)(void*,uint32_t,const uint32_t*,uint32_t,uint32_t);
 auto booleanSet=reinterpret_cast<Set>(0x100100a0),integerSet=reinterpret_cast<Set>(0x100101e0);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x714abcd3;
 for(unsigned i=0;i<65536;++i){
  std::array<uint32_t,72> record{};record[0]=i&1;const unsigned width=record[0]?4:1;
  record[2]=(i/2)%5;record[1]=(i/10)%(16/width+1-record[2]);record[3]=(i/50)%2;
  for(unsigned word=0;word<16;++word){seed=seed*1664525u+1013904223u;record[4+word]=seed;seed=seed*1664525u+1013904223u;record[20+word]=seed;}
  if(i%3==0)std::copy_n(record.begin()+20+record[1]*width,record[2]*width,record.begin()+4);
  auto sourceBefore=record;
  std::array<uint32_t,16> cache{},mirror{};std::copy_n(record.begin()+20,16,cache.begin());mirror=cache;
  std::array<uint32_t,32> queue{};queue.fill(0xdab18743u);
  std::vector<uint32_t> state(0x2600/4);
  state[0x388/4]=1;state[0x390/4]=reinterpret_cast<uint32_t>(queue.data());state[0x394/4]=32;
  state[(record[0]?0x1718:0x1714)/4]=reinterpret_cast<uint32_t>(mirror.data());
  state[(record[0]?0x2590:0x258c)/4]=reinterpret_cast<uint32_t>(cache.data());
  (record[0]?integerSet:booleanSet)(state.data(),record[1],record.data()+4,record[2],record[3]);
  const auto used=(state[0x390/4]-reinterpret_cast<uint32_t>(queue.data()))/4;
  if(used>19||state[0x394/4]!=32-used||cache!=mirror||!std::equal(record.begin()+4,record.begin()+20,sourceBefore.begin()+4))throw std::runtime_error("native queue bounds or cache mutation");
  for(unsigned n=used;n<32;++n)if(queue[n]!=0xdab18743u)throw std::runtime_error("queue tail overwritten");
  record[36]=used;std::copy_n(queue.begin(),used,record.begin()+37);std::copy(cache.begin(),cache.end(),record.begin()+56);
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_metadata_queues=65536\\n";
'''+s[b:]
Path('work/probe_metadata_queue.cpp').write_text(s)
Path('work/build_metadata_queue.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_metadata_queue'))
v=Path('work/verify_generic_ambient_expansion.py').read_text().replace('ambient_expansion','metadata_queue').replace('ambient-expansion','metadata-queue').replace('ambient_expansions','metadata_queues').replace('metadata_queues'+'s','metadata_queues').replace('65536*43*4','65536*72*4')
v=v.replace('Original expansion only, masked x87 exceptions. Full upload dispatch and GPU rendering remain unverified. Count argument is varied but unused by recovered routine.','Original queued BOOL/INT4 methods, initialized caches, force and identical cases, bounded index/counts, two cache copies and queue tails. Refill, direct GPU device path and complete renderer remain unverified.')
Path('work/verify_metadata_queue.py').write_text(v.replace('probe_generic_metadata_queue','probe_metadata_queue'))


