#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <memory>
#include <vector>

// Isolated owned-x64 representation of one per-instance command list.
// This does not manage snapshot/pass ownership or execute GPU commands.
// Callers must serialize access. References from contents() are invalidated
// by a successful store(), clear(), or destruction.
//
// Allocator requirements: copying/equality comparison and deallocation must
// not throw; copies compare equal and can deallocate each other's storage.
template<class Allocator = std::allocator<std::uint32_t>>
class ShaderInstanceCommandCacheCandidateT final {
public:
    using value_type = std::uint32_t;
    using storage_type = std::vector<value_type, Allocator>;

    explicit ShaderInstanceCommandCacheCandidateT(
        const Allocator& allocator = Allocator{})
        : storage_(allocator) {}

    ShaderInstanceCommandCacheCandidateT(
        const ShaderInstanceCommandCacheCandidateT&) = delete;
    ShaderInstanceCommandCacheCandidateT& operator=(
        const ShaderInstanceCommandCacheCandidateT&) = delete;

    // Accepts an opaque word list ending in zero. Earlier zero words are legal.
    // The caller must supply a readable span for the stated count.
    bool store(const value_type* words, std::size_t count) {
        if (words == nullptr || count == 0 || count > 128 || words[count - 1] != 0) {
            return false;
        }

        if (count > storage_.size()) {
            try {
                storage_type replacement(storage_.get_allocator());
                replacement.assign(words, words + count);
                // get_allocator() copies are required to compare equal, so this
                // swap preserves allocator ownership and commits atomically.
                storage_.swap(replacement);
            } catch (const std::bad_alloc&) {
                return false;
            }
        } else {
            // memmove permits a caller to store a span that overlaps our own
            // existing storage. Only the prefix is overwritten; the tail stays.
            std::memmove(storage_.data(), words, count * sizeof(value_type));
        }

        usedWordCount_ = count;
        return true;
    }

    // Releases the high-water allocation and resets used and logical capacity.
    // Equal, nonthrowing allocator copies make the temporary-vector swap safe.
    void clear() {
        storage_type empty(storage_.get_allocator());
        storage_.swap(empty);
        usedWordCount_ = 0;
    }

    std::size_t usedWordCount() const noexcept { return usedWordCount_; }
    std::size_t logicalCapacityWords() const noexcept { return storage_.size(); }
    const storage_type& contents() const noexcept { return storage_; }
    const value_type* data() const noexcept { return storage_.data(); }

private:
    storage_type storage_;
    std::size_t usedWordCount_ = 0;
};
