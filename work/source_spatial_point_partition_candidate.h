#pragma once
#include "source_displacement_bounds_gate_candidate.h"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
struct SourceSpatialPointEntryCandidate {
    uint64_t entityId=0;
    uint32_t mask=0;
    std::array<float,6>bounds{}; // Unexpanded physics minsXYZ/maxsXYZ.
};
// Point-ray partition with the original four levels, list order and DDA.
// Mutations require exclusive access; readers share a completed partition.
class SourceSpatialPointPartitionCandidate {
    struct CellRange {
        unsigned level = 0;
        std::array<int, 3> min{}, max{};

        bool contains(int x, int y, int z) const {
            return x >= min[0] && x <= max[0] && y >= min[1] && y <= max[1] && z >= min[2] && z <= max[2];
        }
    };
    struct Record {
        uint64_t id = 0;
        uint32_t mask = 0;
        std::array<float, 6> box{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(), std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};
        std::array<std::optional<CellRange>, 2> links{};
    };
    using Cells=std::unordered_map<uint32_t,std::vector<uint32_t>>;
    static constexpr std::array<uint32_t, 2> treeMasks{0x1ac, 0xfe53};
    std::array<std::array<Cells,4>,2>trees_;
    std::vector<Record>records_;
    std::unordered_set<uint64_t> ids_;
    std::vector<uint32_t> freeHandles_;
    static float clamp(float x){return x>16384.f?16384.f:x< -16384.f?-16384.f:x;}
    static uint32_t key(int x,int y,int z){return (uint32_t(x)&2047u)|((uint32_t(y)&2047u)<<11)|(uint32_t(z)<<22);}
    static uint32_t parent(uint32_t k){return (k>>2)&0x3fcff9ffu;}
    static int coordinate(float x,unsigned level){const float relative=x-(-16384.f);if(!std::isfinite(relative)||relative< -2147483648.f||relative>=2147483648.f)throw std::runtime_error("spatial coordinate exceeds checked CVTTSS2SI domain");return int(relative)>>int(8+level*2);}
    static void finite(float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite spatial partition arithmetic");}
    const Record& record(size_t handle) const {
        const auto& existing = records_.at(handle);
        if (!existing.id) throw std::runtime_error("inactive spatial handle");
        return existing;
    }

    static std::array<float, 6> paddedBounds(const std::array<float, 6>& bounds) {
        auto padded = bounds;
        for (size_t i = 0; i < padded.size(); ++i) {
            finite(padded[i]);
            padded[i] += i < 3 ? -.03125f : .03125f;
        }
        for (size_t i = 0; i < 3; ++i) {
            if (bounds[i] > bounds[i + 3]) throw std::runtime_error("spatial input box inverted");
        }
        return padded;
    }

    static CellRange cellRange(const std::array<float, 6>& box) {
        CellRange range;
        for (; range.level < 3; ++range.level) {
            const float size = float(256u << (range.level * 2));
            if (size > box[3] - box[0] && size > box[4] - box[1] && size > box[5] - box[2]) break;
        }
        for (size_t axis = 0; axis < 3; ++axis) {
            range.min[axis] = coordinate(box[axis], range.level);
            range.max[axis] = coordinate(box[axis + 3], range.level);
        }
        return range;
    }

    template<class Callback>
    static void eachCell(const CellRange& range, Callback&& fn) {
        for (int x = range.min[0]; x <= range.max[0]; ++x) {
            for (int y = range.min[1]; y <= range.max[1]; ++y) {
                for (int z = range.min[2]; z <= range.max[2]; ++z) fn(x, y, z);
            }
        }
    }

    void prepareLinks(unsigned tree, const CellRange& range) {
        auto& cells = trees_[tree][range.level];
        eachCell(range, [&](int x, int y, int z) {
            auto& members = cells[key(x, y, z)];
            if (members.size() == members.capacity()) members.reserve(members.size() * 2 + 1);
        });
    }

    void unlink(uint32_t handle, unsigned tree, const std::optional<CellRange>& keep = std::nullopt) {
        auto& links = records_[handle].links[tree];
        if (!links) return;
        auto& cells = trees_[tree][links->level];
        eachCell(*links, [&](int x, int y, int z) {
            const auto cell = cells.find(key(x, y, z));
            auto& members = cell->second;
            members.erase(std::find(members.begin(), members.end(), handle));
            if (members.empty() && (!keep || keep->level != links->level || !keep->contains(x, y, z))) {
                cells.erase(cell);
            }
        });
        links.reset();
    }

    void link(uint32_t handle, unsigned tree, const CellRange& range) {
        auto& cells = trees_[tree][range.level];
        eachCell(range, [&](int x, int y, int z) { cells.at(key(x, y, z)).push_back(handle); });
        records_[handle].links[tree] = range;
    }

    static bool worldClip(std::array<float,3>&start,std::array<float,3>&delta){
        std::array<float,3>end{};for(unsigned j=0;j<3;++j){end[j]=start[j]+delta[j];finite(end[j]);}
        auto inside=[](const auto&p){for(float x:p)if(x< -16384.f||x>16384.f)return false;return true;};const bool startIn=inside(start),endIn=inside(end);
        if(!startIn&&!endIn)return false;
        // Preserve original1BDDB0/1BDF90 operation grouping/asymmetry, including
        // 5-unit inward start clipping and both-endpoints-outside refusal.
        if(!startIn){
            for(unsigned j=0;j<3;++j){if(std::abs(double(delta[j]))<1e-10)continue;float t=0;bool clip=false;
                if(delta[j]>0.f&&start[j]< -16384.f){t=(-16379.f-start[j])/delta[j];clip=true;}
                else if(delta[j]<=0.f&&start[j]>16384.f){t=(start[j]-16379.f)*(-1.f/delta[j]);clip=true;}
                if(clip)for(unsigned k=0;k<3;++k){start[k]=delta[k]*t+start[k];finite(start[k]);}
            }for(unsigned j=0;j<3;++j)delta[j]=end[j]-start[j];
        }else if(!endIn){
            for(unsigned j=0;j<3;++j){if(std::abs(double(delta[j]))<1e-10)continue;float t=0;bool clip=false;
                if(delta[j]<0.f&&end[j]< -16384.f){t=(start[j]-(-16379.f))*(-1.f/delta[j]);clip=true;}
                else if(delta[j]>=0.f&&end[j]>16384.f){t=(start[j]-(-16379.f))/delta[j];clip=true;}
                if(clip)for(unsigned k=0;k<3;++k){end[k]=delta[k]*t+start[k];finite(end[k]);}
            }for(unsigned j=0;j<3;++j)delta[j]=end[j]-start[j];
        }
        for(float x:delta)finite(x);return true;
    }
public:
    explicit SourceSpatialPointPartitionCandidate(const std::vector<SourceSpatialPointEntryCandidate>&entries){
        if (entries.size() > 65535) throw std::runtime_error("spatial handle domain exhausted");
        records_.reserve(entries.size());
        ids_.reserve(entries.size());
        for (const auto& entry : entries) create(entry);
    }

    uint16_t create(const SourceSpatialPointEntryCandidate& entry) {
        if (!entry.entityId || ids_.count(entry.entityId) || entry.mask > 65535) {
            throw std::runtime_error("spatial identity/mask invalid");
        }
        const bool reuse = !freeHandles_.empty();
        if (!reuse && records_.size() >= 65535) throw std::runtime_error("spatial handle domain exhausted");
        const auto handle = reuse ? freeHandles_.back() : uint32_t(records_.size());
        auto box = paddedBounds(entry.bounds);
        for (float& value : box) value = clamp(value);
        const auto range = cellRange(box);
        for (unsigned tree = 0; tree < treeMasks.size(); ++tree) {
            if (entry.mask & treeMasks[tree]) prepareLinks(tree, range);
        }
        ids_.insert(entry.entityId);
        try {
            if (!reuse) records_.emplace_back();
        } catch (...) {
            ids_.erase(entry.entityId);
            throw;
        }
        if (reuse) freeHandles_.pop_back();
        auto& added = records_[handle];
        added.id = entry.entityId;
        added.mask = entry.mask;
        if (entry.mask) added.box = box;
        for (unsigned tree = 0; tree < treeMasks.size(); ++tree) {
            if (entry.mask & treeMasks[tree]) link(handle, tree, range);
        }
        return static_cast<uint16_t>(handle);
    }

    void destroy(uint16_t handle) {
        const auto id = record(handle).id;
        freeHandles_.push_back(handle);
        for (unsigned tree = 0; tree < treeMasks.size(); ++tree) unlink(handle, tree);
        ids_.erase(id);
        records_[handle] = {};
    }

    void replaceLists(uint16_t handle, uint32_t removeMask, uint32_t insertMask) {
        if (removeMask > 65535 || insertMask > 65535) throw std::runtime_error("spatial list mask invalid");
        const auto previous = record(handle).mask;
        records_[handle].mask = (previous & ~removeMask) | insertMask;
    }

    void move(uint16_t handle, const std::array<float, 6>& unpadded) {
        const auto& existing = record(handle);
        const auto padded = paddedBounds(unpadded);
        auto newBox = padded;
        for (float& value : newBox) value = clamp(value);
        const auto range = cellRange(newBox);
        auto sharedBox = existing.box;
        std::array<bool, 2> reinsert{};
        for (unsigned tree = 0; tree < treeMasks.size(); ++tree) {
            if (!(existing.mask & treeMasks[tree])) continue;
            // The second tree sees the first tree's updated shared bounds.
            if (existing.links[tree] && padded == sharedBox) continue;
            prepareLinks(tree, range);
            reinsert[tree] = true;
            sharedBox = newBox;
        }
        for (unsigned tree = 0; tree < treeMasks.size(); ++tree) {
            if (!reinsert[tree]) continue;
            unlink(handle, tree, range);
            link(handle, tree, range);
            records_[handle].box = newBox;
        }
    }

    size_t size()const{return ids_.size();}
    const auto&storedBounds(size_t i)const{return record(i).box;}
    uint32_t storedMask(size_t i)const{return record(i).mask;}
    uint32_t storedFlags(size_t i)const{
        const auto& links = record(i).links;
        return (links[0] ? 2u : 0u) | (links[1] ? 4u : 0u);
    }
    std::vector<uint64_t>queryPoint(const std::array<float,3>&inputStart,const std::array<float,3>&inputDelta,uint32_t mask,size_t stopAfter=0)const{
        if(mask>65535)throw std::runtime_error("spatial query mask outside supported domain");for(const auto&v:{inputStart,inputDelta})for(float x:v)finite(x);if(!mask)return {};
        const float squared=(inputDelta[0]*inputDelta[0]+inputDelta[1]*inputDelta[1])+inputDelta[2]*inputDelta[2];finite(squared);
        return queryPointRay(inputStart,inputDelta,squared!=0.f,mask,stopAfter);
    }
    // TraceRay copies m_IsSwept even when world clipping shortens delta to0.
    // Scene adapters must pass that flag, not derive it from the clipped delta.
    std::vector<uint64_t>queryPointRay(const std::array<float,3>&inputStart,const std::array<float,3>&inputDelta,bool swept,uint32_t mask,size_t stopAfter=0)const{
        if(mask>65535)throw std::runtime_error("spatial query mask outside supported domain");for(const auto&v:{inputStart,inputDelta})for(float x:v)finite(x);
        std::vector<uint64_t>answer;if(!mask)return answer;std::vector<bool>visited(records_.size(),false);const unsigned tree=mask&0x1ac?0:1;
        auto start=inputStart,delta=inputDelta;
        if(swept&&!worldClip(start,delta))return answer;
        std::array<float,3>inverse{};if(swept)for(unsigned j=0;j<3;++j){inverse[j]=delta[j]!=0.f?1.f/delta[j]:std::numeric_limits<float>::max();finite(inverse[j]);}
        auto visit=[&](unsigned level,uint32_t cell){
            const auto found=trees_[tree][level].find(cell);if(found==trees_[tree][level].end())return true;
            for(auto it=found->second.rbegin();it!=found->second.rend();++it){const auto i=*it;const auto&r=records_[i];if(!(r.mask&mask)||visited[i])continue;visited[i]=true;bool hit=true;
                if(swept){const std::array<float,3>lo{r.box[0],r.box[1],r.box[2]},hi{r.box[3],r.box[4],r.box[5]};hit=sourceDisplacementBoundsGateCandidate(lo,hi,start,delta,inverse,0.f);}
                // Original stationary ray delegates to EnumerateInBox, which
                // clamps the query point before both lookup AND overlap tests.
                else for(unsigned j=0;j<3;++j)if(clamp(inputStart[j])<r.box[j]||clamp(inputStart[j])>r.box[j+3])hit=false;
                if(hit){answer.push_back(r.id);if(stopAfter&&answer.size()>=stopAfter)return false;}
            }return true;
        };
        if(!swept){
            for(unsigned level=0;level<4;++level){const uint32_t cell=key(coordinate(clamp(start[0]),level),coordinate(clamp(start[1]),level),coordinate(clamp(start[2]),level));if(!visit(level,cell))break;}return answer;
        }
        std::array<float,3>end{};for(unsigned j=0;j<3;++j){end[j]=start[j]+delta[j];finite(end[j]);}
        uint32_t cell=key(coordinate(start[0],0),coordinate(start[1],0),coordinate(start[2],0));const uint32_t endCell=key(coordinate(end[0],0),coordinate(end[1],0),coordinate(end[2],0));
        std::array<int,3>step{};std::array<float,3>next{},increment{};
        for(unsigned j=0;j<3;++j){const float s=start[j]-(-16384.f),e=end[j]-(-16384.f);if(s==e){next[j]=increment[j]=65536.f;continue;}step[j]=delta[j]<0.f?-1:1;const int c=int(s)>>8;const float boundary=float(int32_t(uint32_t(c+(step[j]>0?1:0))*256u));const float a=step[j]<0?s-boundary:boundary-s,b=step[j]<0?e-boundary:boundary-e;
            if(a>0&&b>0)next[j]=increment[j]=65536.f;else{const float inv=step[j]<0?-inverse[j]:inverse[j];next[j]=a*inv;increment[j]=256.f*inv;}finite(next[j]);finite(increment[j]);
        }
        std::array<uint32_t,4>last{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
        for(size_t iteration=0;;++iteration){uint32_t at=cell;
            for(unsigned level=0;level<4;++level){if(level==0||last[level]!=at){last[level]=at;if(!visit(level,at))return answer;}at=parent(at);}
            if(cell==endCell&&iteration==0)return answer;
            if(next[0]>=1.f&&next[1]>=1.f&&next[2]>=1.f)return answer;
            const unsigned axis=next[1]>next[0]?(next[2]>next[0]?0:2):(next[2]>next[1]?1:2);const float previous=next[axis];next[axis]=increment[axis]+next[axis];if(!(next[axis]>previous))throw std::runtime_error("spatial DDA arithmetic made no progress");
            const unsigned shift=axis*11;if(axis<2){const uint32_t changed=((cell+(uint32_t(step[axis])<<shift))^cell)&(2047u<<shift);cell^=changed;}else cell+=uint32_t(step[axis])<<22;
        }
    }
};
