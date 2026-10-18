from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text().replace('generic_shader_combo_candidate.h','pending_draw_flush_candidate.h').replace('stdshader_dx9.dll','shaderapidx9.dll')
s='#include <array>\n'+s
s=s.replace('int main(','''#ifndef _WIN64
static uint8_t* activeState;static uint32_t events[2];static unsigned eventCount;
static void nativeEvent(unsigned id){if(eventCount>=2)throw std::runtime_error("unexpected reentry");events[eventCount++]=id|(uint32_t(activeState[0x30])<<8)|(uint32_t(activeState[0x32])<<16)|(uint32_t(activeState[0x33])<<24);}
static void reenter(){reinterpret_cast<void(__thiscall*)(void*)>(0x10007f50)(activeState);}
static void __fastcall ordinary(void*,void*,int32_t first,int32_t second){if(first!=-1||second)throw std::runtime_error("ordinary args");nativeEvent(1);reenter();}
static void __fastcall alternate(void*,void*,void* data,int32_t first,int32_t second){if(data!=activeState+0x1c||first!=-1||second)throw std::runtime_error("alternate args");nativeEvent(2);reenter();}
static void __fastcall finish(void*,void*,int32_t first,int32_t second){if(first||second)throw std::runtime_error("finish args");nativeEvent(3);}
#endif
int main(''')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,10> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  PendingDrawFlushCandidate state{record[0]!=0,uint8_t(record[1]),uint8_t(record[2]),uint8_t(record[3])};std::array<uint32_t,2> events{};unsigned eventCount=0;
  auto event=[&](unsigned id){if(eventCount>=2)throw std::runtime_error("owned reentry");events[eventCount++]=id|(uint32_t(state.guard)<<8)|(uint32_t(state.pending)<<16)|(uint32_t(state.alternate)<<24);};
  state.flush([&](bool alternate){event(alternate?2:1);unsigned nested=0;state.flush([&](bool){++nested;},[&]{++nested;});if(nested)throw std::runtime_error("reentry callback");},[&]{event(3);});
  if(state.guard!=record[4]||state.pending!=record[5]||state.alternate!=record[6]||eventCount!=record[7]||events[0]!=record[8]||events[1]!=record[9])throw std::runtime_error("flush mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_pending_flushes_exact="<<cases<<'\\n';
'''+s[b:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' using Flush=void(__thiscall*)(void*);auto flush=reinterpret_cast<Flush>(0x10007f50);
 std::array<void*,26> table{};table[12]=reinterpret_cast<void*>(&ordinary);table[25]=reinterpret_cast<void*>(&alternate);table[22]=reinterpret_cast<void*>(&finish);auto tp=table.data();void* endpoint=&tp;
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,10> record{};record[0]=i&1;record[1]=(i>>1)&1;record[2]=(i>>2)&255;record[3]=(i>>10)&63;
  std::array<uint8_t,0x40> object{};auto ep=record[0]?endpoint:nullptr;std::memcpy(object.data()+0x18,&ep,4);object[0x30]=uint8_t(record[1]);object[0x32]=uint8_t(record[2]);object[0x33]=uint8_t(record[3]);activeState=object.data();eventCount=0;events[0]=events[1]=0;
  flush(object.data());record[4]=object[0x30];record[5]=object[0x32];record[6]=object[0x33];record[7]=eventCount;record[8]=events[0];record[9]=events[1];
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_pending_flushes=65536\\n";
'''+s[b:]
Path('work/probe_pending_draw_flush.cpp').write_text(s)
Path('work/build_pending_draw_flush.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_pending_draw_flush'))


