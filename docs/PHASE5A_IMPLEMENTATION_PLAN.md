# Phase 5A: Data Structure Modernization - Implementation Plan

**Status:** 🚀 **IN PROGRESS** - Implementing high-performance data structures
**Start Date:** 2025-11-19
**Target:** O(1) register lookups, better cache locality, reduced memory overhead

## Executive Summary

Phase 5A focuses on replacing slow, allocation-heavy data structures with modern, cache-friendly alternatives. Primary targets are register allocation maps and interval storage which represent 30% of compile-time overhead.

**Key Optimizations:**
1. **RegisterMap:** std::map → Array-based O(1) lookups
2. **IntervalStore:** Pointer-based → Index-based contiguous storage
3. **Views:** Reference parameters → std::span non-owning views

**Expected Results:** 5-10% compile-time improvement, 10-15% memory reduction

---

## Table of Contents

1. [Implementation Strategy](#implementation-strategy)
2. [Optimization 1: Array-Based RegisterMap](#optimization-1-array-based-registermap)
3. [Optimization 2: IntervalStore](#optimization-2-intervalstore)
4. [Optimization 3: std::span Views](#optimization-3-stdspan-views)
5. [Testing Strategy](#testing-strategy)
6. [Performance Measurement](#performance-measurement)
7. [Rollback Plan](#rollback-plan)

---

## Implementation Strategy

### Principles

1. **Incremental:** One optimization at a time
2. **Compatible:** Maintain existing interfaces initially
3. **Validated:** Test after each change
4. **Measured:** Benchmark before and after
5. **Reversible:** Keep rollback capability

### Approach

```
Phase 5A-1: RegisterMap (O(log n) → O(1))      [2-3 hours]
    ↓
Phase 5A-2: IntervalStore (pointers → indices) [2-3 hours]
    ↓
Phase 5A-3: std::span views (copies → views)   [1-2 hours]
    ↓
Phase 5A-4: Testing & Validation               [2-3 hours]
    ↓
Phase 5A-5: Performance Measurement            [1 hour]
    ↓
Phase 5A-6: Documentation & Commit             [1 hour]
```

**Total Estimated Time:** 9-15 hours

---

## Optimization 1: Array-Based RegisterMap

### Current Implementation

```cpp
// backend/src/backend/gen_reg_allocation.cpp
class GenRegAllocator::Opaque {
private:
  map<ir::Register, uint32_t> RA;         // Virtual → Physical: O(log n)
  map<uint32_t, ir::Register> offsetReg;  // Physical → Virtual: O(log n)
};
```

**Analysis:**
- Register is TYPE_SAFE(Register, uint32_t)
- Registers numbered sequentially: 0, 1, 2, 3, ...
- Maximum registers: ~10,000 for large kernels
- Memory overhead: ~48 bytes per node (std::map red-black tree)

**Bottleneck:**
- Every register translation: O(log n) lookup
- Called thousands of times during code generation
- Poor cache locality (tree traversal)

### New Implementation: RegisterMap Class

```cpp
// backend/src/backend/gen_reg_allocation_map.hpp
#pragma once

#include "ir/register.hpp"
#include <vector>
#include <optional>
#include <cstdint>

namespace gbe {

/**
 * @brief High-performance register mapping with O(1) lookups
 *
 * Replaces std::map<ir::Register, uint32_t> with direct array indexing.
 * Exploits the fact that Register is TYPE_SAFE(Register, uint32_t) with
 * sequential numbering.
 *
 * Performance:
 * - Lookup: O(log n) → O(1)
 * - Memory: 48 bytes/entry → 4-8 bytes/entry
 * - Cache: Tree traversal → Linear array (excellent locality)
 */
class RegisterMap {
public:
  RegisterMap() = default;

  /**
   * @brief Reserve space for expected register count
   * @param count Expected number of registers (optimization hint)
   */
  void reserve(size_t count) {
    physicalOffsets_.reserve(count);
  }

  /**
   * @brief Map a virtual register to a physical offset
   * @param reg Virtual register
   * @param offset Physical register offset
   */
  void insert(ir::Register reg, uint32_t offset) {
    const uint32_t index = reg.value();

    // Grow vector if needed
    if (index >= physicalOffsets_.size()) {
      physicalOffsets_.resize(index + 1, UNMAPPED);
    }

    physicalOffsets_[index] = offset;

    // Track reverse mapping if needed
    if (needReverseMap_) {
      reverseMap_[offset] = reg;
    }
  }

  /**
   * @brief Get physical offset for a virtual register
   * @param reg Virtual register
   * @return Physical offset if mapped, std::nullopt otherwise
   */
  std::optional<uint32_t> get(ir::Register reg) const {
    const uint32_t index = reg.value();

    if (index >= physicalOffsets_.size())
      return std::nullopt;

    const uint32_t offset = physicalOffsets_[index];
    if (offset == UNMAPPED)
      return std::nullopt;

    return offset;
  }

  /**
   * @brief Check if a register is mapped
   * @param reg Virtual register
   * @return true if mapped, false otherwise
   */
  bool contains(ir::Register reg) const {
    const uint32_t index = reg.value();
    return index < physicalOffsets_.size() &&
           physicalOffsets_[index] != UNMAPPED;
  }

  /**
   * @brief Remove a register mapping
   * @param reg Virtual register to unmap
   */
  void erase(ir::Register reg) {
    const uint32_t index = reg.value();

    if (index < physicalOffsets_.size()) {
      const uint32_t offset = physicalOffsets_[index];
      physicalOffsets_[index] = UNMAPPED;

      if (needReverseMap_ && offset != UNMAPPED) {
        reverseMap_.erase(offset);
      }
    }
  }

  /**
   * @brief Enable reverse mapping (physical → virtual)
   *
   * Only enable if needed, as it adds memory overhead.
   */
  void enableReverseMap() {
    needReverseMap_ = true;
  }

  /**
   * @brief Get virtual register from physical offset
   * @param offset Physical offset
   * @return Virtual register if found, std::nullopt otherwise
   *
   * Requires enableReverseMap() called first.
   */
  std::optional<ir::Register> getReverse(uint32_t offset) const {
    if (!needReverseMap_)
      return std::nullopt;

    auto it = reverseMap_.find(offset);
    if (it != reverseMap_.end())
      return it->second;

    return std::nullopt;
  }

  /**
   * @brief Get number of mapped registers
   */
  size_t size() const {
    size_t count = 0;
    for (uint32_t offset : physicalOffsets_) {
      if (offset != UNMAPPED)
        ++count;
    }
    return count;
  }

  /**
   * @brief Clear all mappings
   */
  void clear() {
    physicalOffsets_.clear();
    reverseMap_.clear();
  }

  /**
   * @brief Get memory usage in bytes
   */
  size_t memoryUsage() const {
    size_t bytes = physicalOffsets_.capacity() * sizeof(uint32_t);
    bytes += reverseMap_.size() * (sizeof(uint32_t) + sizeof(ir::Register) + 24); // map overhead
    return bytes;
  }

private:
  static constexpr uint32_t UNMAPPED = UINT32_MAX;

  // Primary mapping: virtual register index → physical offset
  // Direct array indexing for O(1) lookups
  std::vector<uint32_t> physicalOffsets_;

  // Reverse mapping (optional, for offsetReg replacement)
  // Only populated if enableReverseMap() called
  std::map<uint32_t, ir::Register> reverseMap_;
  bool needReverseMap_ = false;
};

} // namespace gbe
```

### Migration Strategy

**Step 1: Add new class alongside old**
```cpp
// gen_reg_allocation.cpp
class GenRegAllocator::Opaque {
private:
  // OLD (keep temporarily)
  map<ir::Register, uint32_t> RA;

  // NEW
  RegisterMap registerMap_;
};
```

**Step 2: Parallel population**
```cpp
void insertNewReg(...) {
  // Populate both
  RA[reg] = offset;           // OLD
  registerMap_.insert(reg, offset);  // NEW
}
```

**Step 3: Switch lookups to new**
```cpp
GenRegister genReg(const GenRegister &reg) {
  // OLD: auto it = RA.find(reg.reg());
  // NEW:
  auto offset = registerMap_.get(reg.reg());
  if (!offset)
    return /* not allocated */;

  // Use *offset...
}
```

**Step 4: Validate equivalence**
```cpp
#ifdef DEBUG
  assert(RA.contains(reg) == registerMap_.contains(reg));
  if (auto offset = registerMap_.get(reg)) {
    assert(RA[reg] == *offset);
  }
#endif
```

**Step 5: Remove old implementation**
- Once all tests pass
- Remove map<ir::Register, uint32_t> RA
- Remove validation code

### Performance Impact

**Expected Improvements:**

| Metric | Before | After | Gain |
|--------|--------|-------|------|
| Lookup | O(log n) | O(1) | **~3-5x faster** |
| Memory/entry | 48 bytes | 4 bytes | **92% reduction** |
| Cache misses | High | Low | **~50% reduction** |

**For 1,000 register kernel:**
- Lookup: log₂(1000) ≈ 10 ops → 1 op
- Memory: 48KB → 4KB
- Compile time improvement: **~5%**

**For 10,000 register kernel:**
- Lookup: log₂(10000) ≈ 13 ops → 1 op
- Memory: 480KB → 40KB
- Compile time improvement: **~8-10%**

---

## Optimization 2: IntervalStore

### Current Implementation

```cpp
class GenRegAllocator::Opaque {
private:
  vector<GenRegInterval> intervals;      // Contiguous storage (good!)
  vector<GenRegInterval*> starting;      // Pointers (bad for cache)
  vector<GenRegInterval*> ending;        // Pointers (bad for cache)
};
```

**Issues:**
- Pointers are 64-bit (8 bytes each)
- Poor cache locality (pointer chasing)
- Extra indirection on every access

### New Implementation: IntervalStore

```cpp
// backend/src/backend/gen_reg_allocation_intervals.hpp
#pragma once

#include "backend/gen_reg_allocation.hpp"
#include <vector>
#include <span>
#include <algorithm>

namespace gbe {

/**
 * @brief Cache-friendly interval storage with index-based sorting
 *
 * Replaces vector<GenRegInterval*> with vector<uint32_t> indices.
 *
 * Benefits:
 * - Indices are 32-bit (4 bytes) vs 64-bit pointers (8 bytes)
 * - Better cache locality (smaller data)
 * - Same O(log n) binary search, but faster due to cache
 */
class IntervalStore {
public:
  /**
   * @brief Add an interval to the store
   * @param interval The interval to add
   * @return Index of the added interval
   */
  uint32_t add(const GenRegInterval& interval) {
    const uint32_t index = static_cast<uint32_t>(intervals_.size());
    intervals_.push_back(interval);
    return index;
  }

  /**
   * @brief Get interval by index
   */
  GenRegInterval& operator[](uint32_t index) {
    return intervals_[index];
  }

  const GenRegInterval& operator[](uint32_t index) const {
    return intervals_[index];
  }

  /**
   * @brief Get total number of intervals
   */
  size_t size() const {
    return intervals_.size();
  }

  /**
   * @brief Sort by starting point
   */
  void sortByStart() {
    startingSorted_.clear();
    startingSorted_.reserve(intervals_.size());

    for (uint32_t i = 0; i < intervals_.size(); ++i) {
      startingSorted_.push_back(i);
    }

    std::sort(startingSorted_.begin(), startingSorted_.end(),
              [this](uint32_t a, uint32_t b) {
                return intervals_[a].minID < intervals_[b].minID;
              });
  }

  /**
   * @brief Sort by ending point
   */
  void sortByEnd() {
    endingSorted_.clear();
    endingSorted_.reserve(intervals_.size());

    for (uint32_t i = 0; i < intervals_.size(); ++i) {
      endingSorted_.push_back(i);
    }

    std::sort(endingSorted_.begin(), endingSorted_.end(),
              [this](uint32_t a, uint32_t b) {
                return intervals_[a].maxID < intervals_[b].maxID;
              });
  }

  /**
   * @brief Get interval by start position
   */
  GenRegInterval& byStart(size_t pos) {
    return intervals_[startingSorted_[pos]];
  }

  const GenRegInterval& byStart(size_t pos) const {
    return intervals_[startingSorted_[pos]];
  }

  /**
   * @brief Get interval by end position
   */
  GenRegInterval& byEnd(size_t pos) {
    return intervals_[endingSorted_[pos]];
  }

  const GenRegInterval& byEnd(size_t pos) const {
    return intervals_[endingSorted_[pos]];
  }

  /**
   * @brief Get all intervals as a span
   */
  std::span<GenRegInterval> all() {
    return std::span(intervals_);
  }

  std::span<const GenRegInterval> all() const {
    return std::span(intervals_);
  }

  /**
   * @brief Get sorted interval indices
   */
  std::span<const uint32_t> startOrder() const {
    return std::span(startingSorted_);
  }

  std::span<const uint32_t> endOrder() const {
    return std::span(endingSorted_);
  }

  /**
   * @brief Clear all intervals
   */
  void clear() {
    intervals_.clear();
    startingSorted_.clear();
    endingSorted_.clear();
  }

  /**
   * @brief Reserve space
   */
  void reserve(size_t count) {
    intervals_.reserve(count);
    startingSorted_.reserve(count);
    endingSorted_.reserve(count);
  }

private:
  // Contiguous storage for all intervals
  std::vector<GenRegInterval> intervals_;

  // Sorted indices (32-bit, not 64-bit pointers!)
  std::vector<uint32_t> startingSorted_;
  std::vector<uint32_t> endingSorted_;
};

} // namespace gbe
```

### Migration Example

**Before:**
```cpp
// Iterate over sorted intervals
for (auto* interval : starting) {
  if (interval->minID > limit)
    break;
  // Process interval
}
```

**After:**
```cpp
// Iterate over sorted intervals
for (size_t i = 0; i < intervalStore.size(); ++i) {
  const auto& interval = intervalStore.byStart(i);
  if (interval.minID > limit)
    break;
  // Process interval
}
```

### Performance Impact

**Memory Savings:**

| Intervals | Pointers (old) | Indices (new) | Savings |
|-----------|----------------|---------------|---------|
| 1,000 | 16KB | 8KB | 50% |
| 10,000 | 160KB | 80KB | 50% |

**Cache Performance:**
- Smaller index arrays fit better in L1/L2 cache
- Fewer cache lines needed
- Better prefetching

**Expected:** 2-3% compile-time improvement

---

## Optimization 3: std::span Views

### Current Pattern

```cpp
void processIntervals(const vector<GenRegInterval*>& intervals) {
  // Function takes ownership semantic (copying vector)
  // OR takes reference but unclear if modified
}
```

**Issues:**
- Unclear ownership
- Potential copies
- No bounds checking

### New Pattern with std::span

```cpp
#include <span>

void processIntervals(std::span<const GenRegInterval> intervals) {
  // Clear: non-owning view, read-only
  // No copies
  // Bounds checking in debug mode

  for (const auto& interval : intervals) {
    // Process...
  }
}
```

### Migration Examples

**Before:**
```cpp
void allocateGRFs(Selection& selection,
                  vector<GenRegInterval*>& starting,
                  vector<GenRegInterval*>& ending);
```

**After:**
```cpp
void allocateGRFs(Selection& selection,
                  std::span<const uint32_t> startOrder,
                  std::span<const uint32_t> endOrder);
```

**Benefits:**
- Clear intent (view, not ownership)
- No accidental copies
- Works with any contiguous container
- Debug bounds checking

---

## Testing Strategy

### Unit Tests

```cpp
// tests/backend/test_register_map.cpp
#include "backend/gen_reg_allocation_map.hpp"
#include <gtest/gtest.h>

TEST(RegisterMap, BasicOperations) {
  RegisterMap map;

  ir::Register r0(0), r1(1), r10(10);

  // Insert
  map.insert(r0, 100);
  map.insert(r1, 200);
  map.insert(r10, 1000);

  // Lookup
  EXPECT_EQ(*map.get(r0), 100);
  EXPECT_EQ(*map.get(r1), 200);
  EXPECT_EQ(*map.get(r10), 1000);

  // Contains
  EXPECT_TRUE(map.contains(r0));
  EXPECT_TRUE(map.contains(r1));
  EXPECT_TRUE(map.contains(r10));
  EXPECT_FALSE(map.contains(ir::Register(5)));

  // Erase
  map.erase(r1);
  EXPECT_FALSE(map.contains(r1));
  EXPECT_FALSE(map.get(r1).has_value());
}

TEST(RegisterMap, ReverseMapping) {
  RegisterMap map;
  map.enableReverseMap();

  ir::Register r0(0), r1(1);
  map.insert(r0, 100);
  map.insert(r1, 200);

  EXPECT_EQ(map.getReverse(100)->value(), 0);
  EXPECT_EQ(map.getReverse(200)->value(), 1);
  EXPECT_FALSE(map.getReverse(300).has_value());
}

TEST(RegisterMap, SparseRegisters) {
  RegisterMap map;

  // Test with sparse register IDs
  map.insert(ir::Register(0), 0);
  map.insert(ir::Register(100), 100);
  map.insert(ir::Register(1000), 1000);

  EXPECT_EQ(*map.get(ir::Register(0)), 0);
  EXPECT_EQ(*map.get(ir::Register(100)), 100);
  EXPECT_EQ(*map.get(ir::Register(1000)), 1000);

  // Unmapped registers in between should return nullopt
  EXPECT_FALSE(map.get(ir::Register(50)).has_value());
}
```

```cpp
// tests/backend/test_interval_store.cpp
#include "backend/gen_reg_allocation_intervals.hpp"
#include <gtest/gtest.h>

TEST(IntervalStore, SortingByStart) {
  IntervalStore store;

  GenRegInterval i0(ir::Register(0));
  i0.minID = 10; i0.maxID = 20;

  GenRegInterval i1(ir::Register(1));
  i1.minID = 5; i1.maxID = 15;

  GenRegInterval i2(ir::Register(2));
  i2.minID = 15; i2.maxID = 25;

  store.add(i0);
  store.add(i1);
  store.add(i2);

  store.sortByStart();

  // Should be sorted: i1(5), i0(10), i2(15)
  EXPECT_EQ(store.byStart(0).minID, 5);
  EXPECT_EQ(store.byStart(1).minID, 10);
  EXPECT_EQ(store.byStart(2).minID, 15);
}

TEST(IntervalStore, IndexAccess) {
  IntervalStore store;

  GenRegInterval i0(ir::Register(0));
  uint32_t idx = store.add(i0);

  EXPECT_EQ(idx, 0);
  EXPECT_EQ(store[idx].reg.value(), 0);
}
```

### Integration Tests

```bash
# Run full test suite
cd build/utests
./utest_run

# All 615 tests must pass
# Verify no regressions
```

### Validation Tests

```cpp
// Temporary validation during migration
#ifdef PHASE5A_VALIDATION
void validateRegisters() {
  // Check old vs new map equivalence
  for (auto [reg, offset] : oldRA) {
    auto newOffset = registerMap_.get(reg);
    assert(newOffset.has_value());
    assert(*newOffset == offset);
  }

  // Check size matches
  assert(oldRA.size() == registerMap_.size());
}
#endif
```

---

## Performance Measurement

### Benchmark Framework

```cpp
// benchmarks/register_allocation_bench.cpp
#include <benchmark/benchmark.h>
#include "backend/gen_reg_allocation.hpp"

static void BM_RegisterMapLookup_Old(benchmark::State& state) {
  std::map<ir::Register, uint32_t> oldMap;

  // Populate with N registers
  for (int i = 0; i < state.range(0); ++i) {
    oldMap[ir::Register(i)] = i * 32;
  }

  for (auto _ : state) {
    // Random lookups
    for (int i = 0; i < 1000; ++i) {
      benchmark::DoNotOptimize(oldMap.find(ir::Register(i % state.range(0))));
    }
  }
}
BENCHMARK(BM_RegisterMapLookup_Old)->Range(100, 10000);

static void BM_RegisterMapLookup_New(benchmark::State& state) {
  RegisterMap newMap;

  // Populate with N registers
  for (int i = 0; i < state.range(0); ++i) {
    newMap.insert(ir::Register(i), i * 32);
  }

  for (auto _ : state) {
    // Random lookups
    for (int i = 0; i < 1000; ++i) {
      benchmark::DoNotOptimize(newMap.get(ir::Register(i % state.range(0))));
    }
  }
}
BENCHMARK(BM_RegisterMapLookup_New)->Range(100, 10000);
```

### Compile-Time Measurement

```bash
# Before optimization
time ./build/backend/src/gbe_bin_generater input.cl -o output.bin

# After optimization
time ./build/backend/src/gbe_bin_generater input.cl -o output.bin

# Compare times
```

### Memory Measurement

```cpp
// Add to GenRegAllocator
size_t memoryUsage() const {
  size_t bytes = 0;
  bytes += registerMap_.memoryUsage();
  bytes += intervalStore.memoryUsage();
  return bytes;
}

// Log during compilation
std::cout << "Register allocator memory: " << memoryUsage() / 1024 << " KB\n";
```

---

## Rollback Plan

### Compilation Flag

```cpp
// Use preprocessor flag for easy rollback
#ifdef USE_PHASE5A_OPTIMIZATIONS
  // New implementation
  registerMap_.insert(reg, offset);
#else
  // Old implementation
  RA[reg] = offset;
#endif
```

### CMake Option

```cmake
# CMakeLists.txt
option(ENABLE_PHASE5A "Enable Phase 5A data structure optimizations" ON)

if(ENABLE_PHASE5A)
  add_definitions(-DUSE_PHASE5A_OPTIMIZATIONS)
endif()
```

### Rollback Procedure

```bash
# If issues found, rollback:
cd build
cmake .. -DENABLE_PHASE5A=OFF
make clean && make -j$(nproc)
```

---

## Success Criteria

### Functional

- [  ] All 615 tests pass
- [  ] No crashes or errors
- [  ] Generated code identical to baseline

### Performance

- [  ] Compile time: 3-10% faster
- [  ] Memory usage: 10-15% lower
- [  ] RegisterMap lookups: 3-5x faster (microbenchmark)

### Code Quality

- [  ] Clear, documented code
- [  ] Type-safe interfaces
- [  ] No raw pointers where avoidable

---

## Timeline

| Phase | Task | Duration | Status |
|-------|------|----------|--------|
| 5A-1 | RegisterMap implementation | 2-3h | Pending |
| 5A-2 | IntervalStore implementation | 2-3h | Pending |
| 5A-3 | std::span migration | 1-2h | Pending |
| 5A-4 | Testing & validation | 2-3h | Pending |
| 5A-5 | Performance measurement | 1h | Pending |
| 5A-6 | Documentation & commit | 1h | Pending |

**Total:** 9-15 hours

---

## Next Steps

1. ✅ Implementation plan complete
2. ⏭️ Create RegisterMap header
3. ⏭️ Create IntervalStore header
4. ⏭️ Modify gen_reg_allocation.cpp
5. ⏭️ Test and validate
6. ⏭️ Measure performance
7. ⏭️ Document and commit

---

**Document Version:** 1.0
**Plan Date:** 2025-11-19
**Status:** 🚀 Ready to implement
