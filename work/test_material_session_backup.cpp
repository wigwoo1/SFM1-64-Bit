#include "material_session_backup_candidate.h"
#include <iostream>
#include <stdexcept>
struct BackupSafetyServices:MaterialSessionBackupServicesCandidate{
 unsigned finds=0,reads=0;bool present=true,found=true,failScalar=false;
 uint32_t value=0;std::array<uint32_t,16>words{};std::optional<std::string>text=std::string("new");std::vector<std::string>names;
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{++finds;names.emplace_back(name);return {present?uintptr_t(1):0,found};}
 uint32_t integer(uintptr_t)override{++reads;return value;}
 uint32_t scalar(uintptr_t)override{++reads;if(failScalar)throw std::runtime_error("Explicit scalar service failure");return value;}
 std::array<uint32_t,4>vector(uintptr_t,uint32_t)override{++reads;std::array<uint32_t,4>v{};std::copy_n(words.begin(),4,v.begin());return v;}
 std::array<uint32_t,16>matrix(uintptr_t)override{++reads;return words;}
 std::optional<std::string>stringValue(uintptr_t)override{++reads;return text;}
};
static bool same(const std::vector<MaterialSessionOverrideInputCandidate>&a,const std::vector<MaterialSessionOverrideInputCandidate>&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].name!=b[i].name||a[i].nameAvailable!=b[i].nameAvailable||a[i].typeFlags!=b[i].typeFlags||a[i].words!=b[i].words||a[i].textureName!=b[i].textureName)return false;return true;}
int main(){try{
 static_assert(sizeof(void*)==8,"Owned backup safety x64");unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Backup safety assertion "+std::to_string(checks));++checks;};
 MaterialSessionOverrideInputCandidate r;r.name="$a";r.typeFlags=2;r.words.fill(0);r.textureName="old";BackupSafetyServices s;s.value=42;
 std::vector<MaterialSessionOverrideInputCandidate>v{r};check(captureMaterialSessionBackupCandidate(v,s,0)&&v[0].words[0]==42&&v[0].typeFlags==(2|0x4080));
 s={};v.assign(4097,r);auto before=v;check(!captureMaterialSessionBackupCandidate(v,s,0)&&!s.finds&&same(v,before));
 s={};v={r,r};v[1].name.assign(4096,'x');before=v;check(!captureMaterialSessionBackupCandidate(v,s,0)&&!s.finds&&same(v,before));
 s={};v={r,r};v[1].textureName=std::string(4096,'x');before=v;check(!captureMaterialSessionBackupCandidate(v,s,0)&&!s.finds&&same(v,before));
 s={};s.value=42;s.text.reset();v={r,r};v[1].typeFlags=5;before=v;check(!captureMaterialSessionBackupCandidate(v,s,0)&&s.finds==2&&s.reads==2&&same(v,before));
 s={};s.value=42;s.text=std::string(4096,'x');v={r,r};v[1].typeFlags=5;before=v;check(!captureMaterialSessionBackupCandidate(v,s,0)&&same(v,before));
 s={};s.value=42;s.failScalar=true;v={r,r};v[1].typeFlags=3;before=v;bool threw=false;try{captureMaterialSessionBackupCandidate(v,s,0);}catch(const std::runtime_error&){threw=true;}check(threw&&s.finds==2&&same(v,before));
 s={};s.value=42;v={r};v[0].typeFlags|=0x20;before=v;check(captureMaterialSessionBackupCandidate(v,s,0)&&s.reads==1&&same(v,before));
 s={};s.value=42;v={r};before=v;check(captureMaterialSessionBackupCandidate(v,s,3)&&same(v,before));
 s={};s.value=42;v={r};v[0].typeFlags|=0x400;before=v;check(captureMaterialSessionBackupCandidate(v,s,4)&&same(v,before));
 s={};s.value=42;v={r};check(captureMaterialSessionBackupCandidate(v,s,4)&&v[0].words[0]==42);
 s={};s.present=false;s.value=42;v={r};before=v;check(captureMaterialSessionBackupCandidate(v,s,0)&&!s.reads&&same(v,before));
 s={};s.found=false;s.value=42;v={r};before=v;check(captureMaterialSessionBackupCandidate(v,s,0)&&!s.reads&&same(v,before));
 s={};v={r,r,r};v[0].name="$FLAGS";v[1].name="$flags2";v[2].name="plain";before=v;check(captureMaterialSessionBackupCandidate(v,s,0)&&!s.finds&&same(v,before));
 s={};v={r,r,r};v[0].name="$b";v[2].name="$b";check(captureMaterialSessionBackupCandidate(v,s,0)&&s.names==std::vector<std::string>({"$b","$a","$b"}));
 s={};s.value=0x80000000;v={r};v[0].typeFlags=3;before=v;check(captureMaterialSessionBackupCandidate(v,s,0)&&same(v,before));
 s={};s.words.fill(0x80000000);v={r};v[0].typeFlags=14;check(captureMaterialSessionBackupCandidate(v,s,0)&&v[0].words[0]==0x80000000&&v[0].typeFlags==(14|0x4080));
 s={};s.words.fill(0x7fa12345);v={r};v[0].typeFlags=14;v[0].words=s.words;before=v;check(captureMaterialSessionBackupCandidate(v,s,0)&&same(v,before));
 s={};s.words.fill(0x7fa12345);v={r};v[0].typeFlags=9;v[0].words=s.words;check(captureMaterialSessionBackupCandidate(v,s,0)&&v[0].words[0]==0x7fe12345&&v[0].words[1]==0x7fe12345&&v[0].words[2]==0x7fa12345);
 s={};s.words.fill(0x3f000000);v={r};v[0].typeFlags=8;check(captureMaterialSessionBackupCandidate(v,s,0)&&v[0].words[0]==0x007f7f7f);
 s={};s.text="[1 1 1]";v={r};v[0].typeFlags=5;check(captureMaterialSessionBackupCandidate(v,s,0)&&v[0].textureName==s.text&&v[0].words==r.words);
 s={};s.text=std::string("a\0other",7);v={r};v[0].typeFlags=5;v[0].textureName=std::string("a\0suffix",8);before=v;check(captureMaterialSessionBackupCandidate(v,s,0)&&same(v,before));
 std::cout<<"PASS backup_safety_checks="<<checks<<" failed_capture_preserves_input=1 native_owner_undo_notifications=NOT_RECONSTRUCTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
