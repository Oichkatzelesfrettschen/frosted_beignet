// Phase 5A Integration Patch for gen_reg_allocation.cpp
// This file documents the changes needed to integrate RegisterMap and IntervalStore

/*
 * INTEGRATION STRATEGY:
 *
 * 1. Add new headers after existing includes (line ~35)
 * 2. Add new data structures to Opaque class (line ~141-160)
 * 3. Modify insertNewReg to populate both old and new (around line 173)
 * 4. Add validation in debug mode
 * 5. Switch lookups to use RegisterMap
 * 6. Test thoroughly before removing old code
 */

// ============================================================================
// STEP 1: Add Headers (after line 35)
// ============================================================================

// Add these includes after line 35:
#include "backend/gen_reg_allocation_map.hpp"      // Phase 5A: O(1) register map
#include "backend/gen_reg_allocation_intervals.hpp" // Phase 5A: Index-based intervals

// Enable Phase 5A optimizations (can be disabled via CMake)
#ifndef USE_PHASE5A_OPTIMIZATIONS
#define USE_PHASE5A_OPTIMIZATIONS 1
#endif

// ============================================================================
// STEP 2: Add New Data Structures to Opaque Class (around line 141-160)
// ============================================================================

/*
 * In GenRegAllocator::Opaque class, add these members:
 *
 * BEFORE (line 141-160):
 *   map<ir::Register, uint32_t> RA;
 *   map<uint32_t, ir::Register> offsetReg;
 *   vector<GenRegInterval> intervals;
 *   vector<GenRegInterval*> starting;
 *   vector<GenRegInterval*> ending;
 *
 * AFTER (add alongside existing):
 */

#if USE_PHASE5A_OPTIMIZATIONS
    // Phase 5A: High-performance data structures
    RegisterMap registerMap_;      // O(1) lookups (replaces RA)
    IntervalStore intervalStore_;  // Index-based (replaces starting/ending)
    bool phase5aValidationMode_ = true; // Enable parallel validation
#endif

// ============================================================================
// STEP 3: Modify Constructor (line ~211)
// ============================================================================

/*
 * In GenRegAllocator::Opaque::Opaque(GenContext &ctx) constructor:
 *
 * BEFORE:
 *   GenRegAllocator::Opaque::Opaque(GenContext &ctx) : ctx(ctx) {}
 *
 * AFTER:
 */

GenRegAllocator::Opaque::Opaque(GenContext &ctx) : ctx(ctx) {
#if USE_PHASE5A_OPTIMIZATIONS
  // Phase 5A: Initialize new data structures
  registerMap_.reserve(1024);  // Hint: typical kernel has ~1000 registers
  intervalStore_.reserve(1024);

  // Enable reverse mapping for offsetReg compatibility
  registerMap_.enableReverseMap();

  std::cout << "[Phase 5A] Optimizations enabled\n";
#endif
}

// ============================================================================
// STEP 4: Modify allocatePayloadReg (line ~214-225)
// ============================================================================

/*
 * BEFORE:
 *   void GenRegAllocator::Opaque::allocatePayloadReg(ir::Register reg,
 *                                                     uint32_t offset,
 *                                                     uint32_t subOffset) {
 *     offset += subOffset;
 *     RA.insert(std::make_pair(reg, offset));
 *   }
 *
 * AFTER:
 */

void GenRegAllocator::Opaque::allocatePayloadReg(ir::Register reg,
                                                  uint32_t offset,
                                                  uint32_t subOffset) {
  using namespace ir;
  assert(offset >= GEN_REG_SIZE);
  offset += subOffset;

  // OLD: Keep for parallel validation
  RA.insert(std::make_pair(reg, offset));

#if USE_PHASE5A_OPTIMIZATIONS
  // NEW: Phase 5A RegisterMap
  registerMap_.insert(reg, offset);

  // VALIDATION: Ensure both methods agree
  if (phase5aValidationMode_) {
    assert(registerMap_.get(reg) == offset);
    assert(registerMap_.contains(reg) == RA.contains(reg));
  }
#endif
}

// ============================================================================
// STEP 5: Modify insertNewReg (around line 173, inline function)
// ============================================================================

/*
 * This function is called frequently to insert new register allocations.
 * Find its definition (likely around line 300-400 based on usage patterns).
 *
 * PATTERN TO FIND:
 *   INLINE void GenRegAllocator::Opaque::insertNewReg(...) {
 *     ...
 *     RA[reg] = grfOffset;
 *     offsetReg[grfOffset] = reg;
 *     ...
 *   }
 *
 * MODIFICATION:
 */

INLINE void GenRegAllocator::Opaque::insertNewReg(const Selection &selection,
                                                    ir::Register reg,
                                                    uint32_t grfOffset,
                                                    bool isVector) {
  // ... existing code ...

  // OLD: std::map inserts
  RA[reg] = grfOffset;
  offsetReg[grfOffset] = reg;

#if USE_PHASE5A_OPTIMIZATIONS
  // NEW: Phase 5A RegisterMap inserts
  registerMap_.insert(reg, grfOffset);

  // VALIDATION: Ensure equivalence
  if (phase5aValidationMode_) {
    assert(registerMap_.get(reg) == grfOffset);
    assert(registerMap_.contains(reg));

    // Check reverse mapping
    auto revReg = registerMap_.getReverse(grfOffset);
    assert(revReg.value() == reg.value());
  }
#endif

  // ... existing code ...
}

// ============================================================================
// STEP 6: Modify isAllocated (line ~82-84)
// ============================================================================

/*
 * BEFORE:
 *   INLINE bool isAllocated(const ir::Register &reg) {
 *     return RA.contains(reg);
 *   }
 *
 * AFTER (gradually switch to RegisterMap):
 */

INLINE bool isAllocated(const ir::Register &reg) {
#if USE_PHASE5A_OPTIMIZATIONS
  bool newResult = registerMap_.contains(reg);

  // VALIDATION: Ensure old and new agree
  if (phase5aValidationMode_) {
    bool oldResult = RA.contains(reg);
    if (newResult != oldResult) {
      std::cerr << "[Phase 5A] MISMATCH in isAllocated for reg "
                << reg.value() << ": old=" << oldResult
                << " new=" << newResult << "\n";
      assert(false);
    }
  }

  return newResult;
#else
  return RA.contains(reg);
#endif
}

// ============================================================================
// STEP 7: Modify genReg (around line 1590-1595)
// ============================================================================

/*
 * This is a critical lookup function called frequently.
 *
 * BEFORE:
 *   GenRegister genReg(const GenRegister &reg) {
 *     GBE_ASSERT(RA.contains(reg.reg()) != false);
 *     const uint32_t grfOffset = RA.find(reg.reg())->second;
 *     ...
 *   }
 *
 * AFTER:
 */

GenRegister GenRegAllocator::Opaque::genReg(const GenRegister &reg) {
#if USE_PHASE5A_OPTIMIZATIONS
  // NEW: O(1) lookup with RegisterMap
  GBE_ASSERT(registerMap_.contains(reg.reg()));
  const uint32_t grfOffset = registerMap_.get(reg.reg());

  // VALIDATION: Ensure matches old method
  if (phase5aValidationMode_) {
    auto it = RA.find(reg.reg());
    assert(it != RA.end());
    assert(it->second == grfOffset);
  }
#else
  // OLD: O(log n) lookup with std::map
  GBE_ASSERT(RA.contains(reg.reg()) != false);
  const uint32_t grfOffset = RA.find(reg.reg())->second;
#endif

  // ... rest of function unchanged ...
  GenRegister physical = reg;
  // ... existing code continues ...
}

// ============================================================================
// STEP 8: Initialize Intervals (find where intervals are built)
// ============================================================================

/*
 * Find where intervals vector is populated (likely in allocateGRFs function).
 * After all intervals are added to the vector, initialize IntervalStore:
 *
 * PATTERN TO FIND:
 *   intervals.push_back(GenRegInterval(reg));
 *   ... (many times)
 *
 * AFTER all intervals added, ADD:
 */

#if USE_PHASE5A_OPTIMIZATIONS
  // Phase 5A: Copy intervals to IntervalStore
  for (const auto& interval : intervals) {
    intervalStore_.add(interval);
  }

  // Create sorted index arrays
  intervalStore_.sortByStart();
  intervalStore_.sortByEnd();

  std::cout << "[Phase 5A] IntervalStore initialized with "
            << intervalStore_.size() << " intervals\n";
#endif

// ============================================================================
// STEP 9: Modify Interval Iteration (find loops over starting/ending)
// ============================================================================

/*
 * PATTERN TO FIND:
 *   for (auto* interval : starting) {
 *     // Process interval
 *   }
 *
 * AFTER (gradual migration):
 */

#if USE_PHASE5A_OPTIMIZATIONS
  // NEW: Index-based iteration
  for (size_t i = 0; i < intervalStore_.size(); ++i) {
    auto& interval = intervalStore_.byStart(i);
    // Process interval

    // VALIDATION: Ensure same order as old method
    if (phase5aValidationMode_ && i < starting.size()) {
      assert(&interval == starting[i] ||
             interval.reg.value() == starting[i]->reg.value());
    }
  }
#else
  // OLD: Pointer-based iteration
  for (auto* interval : starting) {
    // Process interval
  }
#endif

// ============================================================================
// STEP 10: Add Performance Measurement
// ============================================================================

/*
 * Add timing code around register allocation:
 */

#if USE_PHASE5A_OPTIMIZATIONS
#include <chrono>

bool GenRegAllocator::Opaque::allocate(Selection &selection) {
  auto start = std::chrono::high_resolution_clock::now();

  // ... existing allocation code ...

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "[Phase 5A] Register allocation took: "
            << duration.count() << " microseconds\n";
  std::cout << "[Phase 5A] RegisterMap memory: "
            << registerMap_.memoryUsage() / 1024 << " KB\n";
  std::cout << "[Phase 5A] IntervalStore memory: "
            << intervalStore_.memoryUsage() / 1024 << " KB\n";

  // VALIDATION: Report if old and new differ
  if (phase5aValidationMode_) {
    size_t oldMapSize = RA.size();
    size_t newMapSize = registerMap_.size();
    if (oldMapSize != newMapSize) {
      std::cerr << "[Phase 5A] SIZE MISMATCH: old=" << oldMapSize
                << " new=" << newMapSize << "\n";
    }
  }

  return true; // ... or existing return value
}
#endif

// ============================================================================
// STEP 11: CMake Integration
// ============================================================================

/*
 * Add to backend/src/CMakeLists.txt:
 *
 * option(ENABLE_PHASE5A "Enable Phase 5A data structure optimizations" ON)
 *
 * if(ENABLE_PHASE5A)
 *   add_definitions(-DUSE_PHASE5A_OPTIMIZATIONS=1)
 * else()
 *   add_definitions(-DUSE_PHASE5A_OPTIMIZATIONS=0)
 * endif()
 *
 * To disable optimizations:
 *   cmake .. -DENABLE_PHASE5A=OFF
 */

// ============================================================================
// TESTING PROCEDURE
// ============================================================================

/*
 * 1. Build with Phase 5A enabled:
 *    cd build
 *    cmake .. -DENABLE_PHASE5A=ON
 *    make -j$(nproc)
 *
 * 2. Run all tests:
 *    cd utests
 *    ./utest_run
 *
 * 3. Check for validation errors:
 *    grep "Phase 5A.*MISMATCH" test_output.log
 *
 * 4. If all tests pass and no mismatches:
 *    - Set phase5aValidationMode_ = false
 *    - Remove old RA and offsetReg maps
 *    - Remove old starting/ending vectors
 *    - Clean up validation code
 *
 * 5. Rebuild and retest:
 *    make clean && make -j$(nproc)
 *    cd utests && ./utest_run
 *
 * 6. Performance measurement:
 *    time ./utest_run > /dev/null
 *    Compare with old time
 */

// ============================================================================
// ROLLBACK PROCEDURE
// ============================================================================

/*
 * If any issues found:
 *
 * 1. Disable via CMake:
 *    cmake .. -DENABLE_PHASE5A=OFF
 *    make clean && make -j$(nproc)
 *
 * 2. Or set in code:
 *    #define USE_PHASE5A_OPTIMIZATIONS 0
 *
 * 3. Rebuild and verify:
 *    cd utests && ./utest_run
 */

// ============================================================================
// EXPECTED RESULTS
// ============================================================================

/*
 * Compile-time improvements:
 * - Small kernels (<1000 regs): 2-5% faster
 * - Medium kernels (1000-5000 regs): 5-8% faster
 * - Large kernels (5000+ regs): 8-10% faster
 *
 * Memory improvements:
 * - 1000 registers: ~60KB → ~8KB (87% reduction)
 * - 10000 registers: ~600KB → ~120KB (80% reduction)
 *
 * Performance characteristics:
 * - RegisterMap lookups: O(log n) → O(1)
 * - Cache misses: ~50% reduction
 * - Memory allocations: ~90% reduction
 */

// ============================================================================
// COMPLETION CHECKLIST
// ============================================================================

/*
 * Phase 5A Integration Checklist:
 *
 * [  ] 1. Add new header includes
 * [  ] 2. Add RegisterMap and IntervalStore to Opaque class
 * [  ] 3. Initialize in constructor
 * [  ] 4. Modify allocatePayloadReg
 * [  ] 5. Modify insertNewReg
 * [  ] 6. Modify isAllocated
 * [  ] 7. Modify genReg
 * [  ] 8. Initialize IntervalStore after intervals built
 * [  ] 9. Modify interval iteration patterns
 * [  ] 10. Add performance measurement
 * [  ] 11. Add CMake option
 * [  ] 12. Build with optimizations enabled
 * [  ] 13. Run all 615 tests
 * [  ] 14. Verify zero validation errors
 * [  ] 15. Measure performance improvement
 * [  ] 16. Disable validation mode
 * [  ] 17. Remove old data structures
 * [  ] 18. Rebuild and retest
 * [  ] 19. Document results
 * [  ] 20. Commit Phase 5A integration
 */
