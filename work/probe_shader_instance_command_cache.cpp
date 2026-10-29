#include "shader_instance_command_cache_candidate.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <vector>

namespace {
struct Tracker {
    std::size_t attempts = 0;
    std::size_t successes = 0;
    std::size_t frees = 0;
    std::size_t outstanding = 0;
    std::size_t failOnAttempt = (std::numeric_limits<std::size_t>::max)();
};

template<class T>
class TrackingAllocator {
public:
    using value_type = T;
    Tracker* tracker;

    explicit TrackingAllocator(Tracker* state = nullptr) noexcept : tracker(state) {}
    template<class U>
    TrackingAllocator(const TrackingAllocator<U>& other) noexcept : tracker(other.tracker) {}

    T* allocate(std::size_t count) {
        const std::size_t index = tracker->attempts++;
        if (index == tracker->failOnAttempt) {
            throw std::bad_alloc{};
        }
        T* result = static_cast<T*>(::operator new(count * sizeof(T)));
        ++tracker->successes;
        ++tracker->outstanding;
        return result;
    }

    void deallocate(T* pointer, std::size_t) noexcept {
        if (pointer != nullptr) {
            --tracker->outstanding;
            ++tracker->frees;
            ::operator delete(pointer);
        }
    }

    template<class U> struct rebind { using other = TrackingAllocator<U>; };
    template<class U> friend class TrackingAllocator;
};

template<class T, class U>
bool operator==(const TrackingAllocator<T>& a, const TrackingAllocator<U>& b) noexcept {
    return a.tracker == b.tracker;
}
template<class T, class U>
bool operator!=(const TrackingAllocator<T>& a, const TrackingAllocator<U>& b) noexcept {
    return !(a == b);
}

using Cache = ShaderInstanceCommandCacheCandidateT<TrackingAllocator<std::uint32_t>>;

std::vector<std::uint32_t> copyContents(const Cache& cache) {
    return std::vector<std::uint32_t>(cache.contents().begin(), cache.contents().end());
}

bool equalsContents(const Cache& cache, const std::vector<std::uint32_t>& expected) {
    return cache.contents().size() == expected.size() &&
           std::equal(cache.contents().begin(), cache.contents().end(), expected.begin());
}

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(2);
    }
}

template<class T>
void readExact(std::ifstream& input, T& value) {
    input.read(reinterpret_cast<char*>(&value), sizeof(value));
    require(static_cast<bool>(input), "truncated reference corpus");
}

template<class T>
void readWords(std::ifstream& input, std::vector<T>& words, std::size_t count) {
    words.resize(count);
    if (count != 0) {
        input.read(reinterpret_cast<char*>(words.data()),
                   static_cast<std::streamsize>(count * sizeof(T)));
    }
    require(static_cast<bool>(input), "truncated reference words");
}

void compare(const Cache& cache, const std::vector<std::uint32_t>& expected,
             std::size_t used, std::size_t capacity, const char* label) {
    require(cache.usedWordCount() == used, label);
    require(cache.logicalCapacityWords() == capacity, label);
    require(cache.contents().size() == capacity, label);
    require(std::equal(cache.contents().begin(), cache.contents().end(), expected.begin()), label);
}

void testReferenceCorpus(const char* path) {
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "open reference corpus");
    std::uint32_t magic = 0, cases = 0;
    readExact(input, magic); readExact(input, cases);
    require(magic == 0x43414348u && cases == 400, "reference corpus header");
    Tracker tracker;
    for (std::uint32_t i = 0; i < cases; ++i) {
        std::uint32_t serial = 0, used0 = 0, cap0 = 0, used1 = 0, cap1 = 0;
        readExact(input, serial); readExact(input, used0); readExact(input, cap0);
        require(used0 > 0 && used0 <= cap0 && cap0 <= 128, "first reference dimensions");
        std::vector<std::uint32_t> words0, expected0;
        readWords(input, words0, used0); readWords(input, expected0, cap0);
        readExact(input, used1); readExact(input, cap1);
        require(used1 > 0 && used1 <= cap1 && cap1 <= 128, "second reference dimensions");
        std::vector<std::uint32_t> words1, expected1;
        readWords(input, words1, used1); readWords(input, expected1, cap1);

        Cache cache{TrackingAllocator<std::uint32_t>(&tracker)};
        require(cache.store(words0.data(), words0.size()), "first reference store");
        compare(cache, expected0, used0, cap0, "first full reference storage mismatch");
        require(cache.store(words1.data(), words1.size()), "second reference store");
        compare(cache, expected1, used1, cap1, "second full reference storage/tail mismatch");
        require(cache.contents().back() == 0, "reference terminator");
        cache.clear();
        require(cache.usedWordCount() == 0 && cache.logicalCapacityWords() == 0,
                "reference clear");
    }
    require(input.peek() == std::char_traits<char>::eof(), "reference corpus trailing bytes");
    require(tracker.outstanding == 0 && tracker.successes == tracker.frees,
            "reference corpus allocations balanced");
}

std::vector<std::uint32_t> makeWords(std::size_t count, std::uint32_t salt) {
    std::vector<std::uint32_t> result;
    for (std::size_t i = 0; i + 1 < count; ++i) {
        result.push_back(salt + static_cast<std::uint32_t>(i * 17u));
    }
    result.push_back(0);
    return result;
}

void testLengthsAndReuse() {
    Tracker tracker;
    {
        Cache cache{TrackingAllocator<std::uint32_t>(&tracker)};
        for (std::size_t count = 1; count <= 128; ++count) {
            const auto words = makeWords(count, 0x12340000u);
            require(cache.store(words.data(), words.size()), "length store");
            require(cache.usedWordCount() == count && cache.logicalCapacityWords() == count,
                    "length size/capacity");
            require(std::equal(cache.contents().begin(), cache.contents().end(), words.begin()),
                    "length full content");
        }
        const std::array<std::uint32_t, 5> earlyZeros{{0u, 17u, 0u, 29u, 0u}};
        const auto beforeEarlyZeros = copyContents(cache);
        const auto attemptsBeforeEarlyZeros = tracker.attempts;
        require(cache.store(earlyZeros.data(), earlyZeros.size()), "earlier zero operands are legal");
        require(tracker.attempts == attemptsBeforeEarlyZeros && cache.usedWordCount() == 5 &&
                cache.logicalCapacityWords() == 128, "earlier-zero reuse has no allocation");
        for (std::size_t i = 0; i < earlyZeros.size(); ++i) {
            require(cache.contents()[i] == earlyZeros[i], "earlier-zero payload");
        }
        for (std::size_t i = earlyZeros.size(); i < beforeEarlyZeros.size(); ++i) {
            require(cache.contents()[i] == beforeEarlyZeros[i], "earlier-zero tail preserved");
        }

        auto smaller = makeWords(9, 0xabcdef00u);
        const auto prior = copyContents(cache);
        const auto attemptsBeforeShrink = tracker.attempts;
        require(cache.store(smaller.data(), smaller.size()), "shrink reuse");
        require(tracker.attempts == attemptsBeforeShrink, "shrink reuse has no allocation");
        require(cache.usedWordCount() == smaller.size() && cache.logicalCapacityWords() == 128,
                "shrink preserves high-water capacity");
        for (std::size_t i = 0; i < smaller.size(); ++i) {
            require(cache.contents()[i] == smaller[i], "shrink prefix");
        }
        for (std::size_t i = smaller.size(); i < prior.size(); ++i) {
            require(cache.contents()[i] == prior[i], "shrink tail preserved");
        }

        auto overlapping = makeWords(10, 100u);
        const auto attemptsBeforeSeed = tracker.attempts;
        require(cache.store(overlapping.data(), overlapping.size()), "overlap seed");
        require(tracker.attempts == attemptsBeforeSeed, "overlap seed reuses storage");
        const auto beforeOverlap = copyContents(cache);
        const std::array<std::uint32_t, 4> overlapExpected{{202u, 219u, 236u, 0u}};
        const auto attemptsBeforeOverlap = tracker.attempts;
        require(cache.store(cache.data() + 6, overlapExpected.size()), "overlapping self-store");
        require(tracker.attempts == attemptsBeforeOverlap, "overlap reuse has no allocation");
        require(cache.usedWordCount() == 4 && cache.logicalCapacityWords() == 128,
                "overlap dimensions");
        for (std::size_t i = 0; i < overlapExpected.size(); ++i) {
            require(cache.contents()[i] == overlapExpected[i], "overlap memmove prefix");
        }
        for (std::size_t i = overlapExpected.size(); i < 128; ++i) {
            require(cache.contents()[i] == beforeOverlap[i], "overlap tail preserved");
        }

        const auto before = copyContents(cache);
        const auto beforeUsed = cache.usedWordCount();
        const auto beforeCapacity = cache.logicalCapacityWords();
        const auto allocationAttempts = tracker.attempts;
        const std::array<std::uint32_t, 3> badTerminator{{1u, 0u, 2u}};
        const std::array<std::uint32_t, 1> validOne{{0u}};
        const auto tooMany = makeWords(129, 99u);
        require(!cache.store(nullptr, 1), "reject null");
        require(!cache.store(validOne.data(), 0), "reject empty");
        require(!cache.store(tooMany.data(), tooMany.size()), "reject over limit");
        require(!cache.store(badTerminator.data(), badTerminator.size()), "reject missing end");
        require(tracker.attempts == allocationAttempts, "invalid input no allocations");
        require(equalsContents(cache, before) && cache.usedWordCount() == beforeUsed &&
                cache.logicalCapacityWords() == beforeCapacity, "invalid input no mutation");

        cache.clear(); cache.clear();
        require(cache.usedWordCount() == 0 && cache.logicalCapacityWords() == 0 &&
                cache.contents().empty(), "repeated clear empties cache");
        require(tracker.outstanding == 0, "clear releases storage");
    }
    require(tracker.outstanding == 0 && tracker.successes == tracker.frees,
            "destruction balances allocations");
}

void testAllocationFailures() {
    std::size_t injected = 0;
    for (std::size_t count = 1; count <= 128; ++count) {
        Tracker tracker;
        tracker.failOnAttempt = 0;
        {
            Cache cache{TrackingAllocator<std::uint32_t>(&tracker)};
            const auto words = makeWords(count, static_cast<std::uint32_t>(count));
            require(!cache.store(words.data(), words.size()), "fresh allocation failure returns false");
            require(tracker.attempts == 1 && tracker.successes == 0 &&
                    cache.contents().empty() && cache.usedWordCount() == 0 &&
                    cache.logicalCapacityWords() == 0 && tracker.outstanding == 0,
                    "fresh allocation failure preserves empty state");
            ++injected;
        }
        require(tracker.outstanding == 0 && tracker.successes == tracker.frees,
                "fresh allocation failure destruction balances allocations");
    }
    for (std::size_t oldCapacity = 1; oldCapacity < 128; ++oldCapacity) {
        Tracker growthTracker;
        {
            Cache cache{TrackingAllocator<std::uint32_t>(&growthTracker)};
            const auto oldWords = makeWords(oldCapacity, 0x50000000u +
                                            static_cast<std::uint32_t>(oldCapacity));
            require(cache.store(oldWords.data(), oldWords.size()), "growth seed allocation");
            const auto oldContents = copyContents(cache);
            const auto oldUsed = cache.usedWordCount();
            const auto oldLogicalCapacity = cache.logicalCapacityWords();
            const auto newWords = makeWords(oldCapacity + 1,
                                            0x60000000u + static_cast<std::uint32_t>(oldCapacity));
            const std::size_t failAt = growthTracker.attempts;
            growthTracker.failOnAttempt = failAt;
            require(!cache.store(newWords.data(), newWords.size()),
                    "growth allocation failure returns false");
            require(growthTracker.attempts == failAt + 1 &&
                    equalsContents(cache, oldContents) && cache.usedWordCount() == oldUsed &&
                    cache.logicalCapacityWords() == oldLogicalCapacity &&
                    growthTracker.outstanding == 1, "growth failure preserves prior state");
            growthTracker.failOnAttempt = (std::numeric_limits<std::size_t>::max)();
            cache.clear();
            require(growthTracker.outstanding == 0, "failed growth clear balances storage");
            ++injected;
        }
        require(growthTracker.outstanding == 0 &&
                growthTracker.successes == growthTracker.frees,
                "failed growth destruction balances allocations");
    }
    std::cout << "allocation_failure_injections=" << injected << '\n';
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: probe_shader_instance_command_cache <reference-cases.bin>\n";
        return 2;
    }
    testReferenceCorpus(argv[1]);
    testLengthsAndReuse();
    testAllocationFailures();
    std::cout << "PASS reference_fresh_entry_cases=400 lengths=1..128 overlap=PASS "
                 "invalid_inputs=PASS clear_destructor=PASS x64_owned_cache=PASS\n";
    return 0;
}
