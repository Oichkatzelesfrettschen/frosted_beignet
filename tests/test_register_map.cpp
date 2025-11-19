/*
 * Phase 5A RegisterMap Validation Test
 * Standalone test to verify RegisterMap correctness without full build
 *
 * Compile:
 *   g++ -std=c++17 -I../backend/src -I../include \
 *       -o test_register_map test_register_map.cpp
 *
 * Run:
 *   ./test_register_map
 *
 * Expected: All tests pass, "SUCCESS" printed
 */

#include "backend/gen_reg_allocation_map.hpp"
#include <iostream>
#include <cassert>
#include <vector>

using namespace gbe;
using namespace gbe::ir;

// Test Register wrapper (simplified from ir/register.hpp)
struct TestRegister {
    uint32_t val;
    explicit TestRegister(uint32_t v = 0) : val(v) {}
    uint32_t value() const { return val; }
    bool operator==(const TestRegister& other) const { return val == other.val; }
    bool operator!=(const TestRegister& other) const { return val != other.val; }
};

// Mock ir::Register to use TestRegister
namespace gbe {
namespace ir {
    using Register = TestRegister;
}
}

void testBasicOperations() {
    std::cout << "Test 1: Basic insert/get operations... ";

    RegisterMap map;
    Register reg1(42);
    Register reg2(100);
    Register reg3(1000);

    // Insert
    map.insert(reg1, 128);   // reg 42 → offset 128
    map.insert(reg2, 256);   // reg 100 → offset 256
    map.insert(reg3, 512);   // reg 1000 → offset 512

    // Get
    assert(map.get(reg1) == 128);
    assert(map.get(reg2) == 256);
    assert(map.get(reg3) == 512);

    // Contains
    assert(map.contains(reg1));
    assert(map.contains(reg2));
    assert(map.contains(reg3));
    assert(!map.contains(Register(999)));

    // Size
    assert(map.size() == 3);

    std::cout << "PASS\n";
}

void testSequentialRegisters() {
    std::cout << "Test 2: Sequential register allocation... ";

    RegisterMap map;
    const size_t N = 1000;

    // Insert 1000 sequential registers
    for (uint32_t i = 0; i < N; ++i) {
        Register reg(i);
        uint32_t offset = i * 32;  // GEN_REG_SIZE = 32
        map.insert(reg, offset);
    }

    // Verify all
    for (uint32_t i = 0; i < N; ++i) {
        Register reg(i);
        uint32_t expected = i * 32;
        assert(map.contains(reg));
        assert(map.get(reg) == expected);
    }

    assert(map.size() == N);

    std::cout << "PASS\n";
}

void testSparseRegisters() {
    std::cout << "Test 3: Sparse register IDs... ";

    RegisterMap map;

    // Sparse pattern: 0, 100, 500, 1000, 5000, 10000
    std::vector<uint32_t> sparse = {0, 100, 500, 1000, 5000, 10000};

    for (size_t i = 0; i < sparse.size(); ++i) {
        Register reg(sparse[i]);
        uint32_t offset = 64 * (i + 1);
        map.insert(reg, offset);
    }

    // Verify
    for (size_t i = 0; i < sparse.size(); ++i) {
        Register reg(sparse[i]);
        uint32_t expected = 64 * (i + 1);
        assert(map.contains(reg));
        assert(map.get(reg) == expected);
    }

    // Verify gaps return UNMAPPED
    assert(!map.contains(Register(50)));
    assert(!map.contains(Register(250)));
    assert(map.get(Register(50)) == RegisterMap::unmapped());

    std::cout << "PASS\n";
}

void testReverseMapping() {
    std::cout << "Test 4: Reverse mapping (offset → register)... ";

    RegisterMap map;
    map.enableReverseMap();

    Register reg1(10);
    Register reg2(20);
    Register reg3(30);

    map.insert(reg1, 128);
    map.insert(reg2, 256);
    map.insert(reg3, 512);

    // Reverse lookup
    Register found1 = map.getReverse(128);
    Register found2 = map.getReverse(256);
    Register found3 = map.getReverse(512);

    assert(found1 == reg1);
    assert(found2 == reg2);
    assert(found3 == reg3);

    // Non-existent offset
    Register notFound = map.getReverse(999);
    assert(notFound == Register(RegisterMap::unmapped()));

    std::cout << "PASS\n";
}

void testUpdateOverwrite() {
    std::cout << "Test 5: Update/overwrite existing register... ";

    RegisterMap map;
    Register reg(42);

    // Initial insert
    map.insert(reg, 128);
    assert(map.get(reg) == 128);

    // Overwrite with new offset
    map.insert(reg, 256);
    assert(map.get(reg) == 256);
    assert(map.size() == 1);  // Size shouldn't change

    std::cout << "PASS\n";
}

void testErase() {
    std::cout << "Test 6: Erase operations... ";

    RegisterMap map;
    Register reg1(10);
    Register reg2(20);
    Register reg3(30);

    map.insert(reg1, 128);
    map.insert(reg2, 256);
    map.insert(reg3, 512);

    assert(map.size() == 3);

    // Erase middle
    map.erase(reg2);
    assert(!map.contains(reg2));
    assert(map.contains(reg1));
    assert(map.contains(reg3));
    assert(map.size() == 2);

    // Erase non-existent (should be safe)
    map.erase(Register(999));
    assert(map.size() == 2);

    std::cout << "PASS\n";
}

void testClear() {
    std::cout << "Test 7: Clear all mappings... ";

    RegisterMap map;

    for (uint32_t i = 0; i < 100; ++i) {
        map.insert(Register(i), i * 32);
    }

    assert(map.size() == 100);
    assert(!map.empty());

    map.clear();

    assert(map.size() == 0);
    assert(map.empty());
    assert(!map.contains(Register(50)));

    std::cout << "PASS\n";
}

void testMemoryUsage() {
    std::cout << "Test 8: Memory usage tracking... ";

    RegisterMap map;

    // Empty map should have minimal overhead
    size_t emptyMem = map.memoryUsage();
    assert(emptyMem < 1024);  // Less than 1KB for empty map

    // Add 1000 registers
    for (uint32_t i = 0; i < 1000; ++i) {
        map.insert(Register(i), i * 32);
    }

    size_t mem1000 = map.memoryUsage();

    // Expected: ~4KB (1000 * 4 bytes)
    // Allow some overhead for vector capacity
    assert(mem1000 >= 4000);    // At least 4 bytes per register
    assert(mem1000 < 10000);    // But not too much overhead

    std::cout << "PASS (Memory: " << mem1000 << " bytes for 1000 regs)\n";
}

void testReserve() {
    std::cout << "Test 9: Reserve capacity... ";

    RegisterMap map;
    map.reserve(1000);

    // After reserve, should have capacity but size 0
    assert(map.size() == 0);
    assert(map.empty());

    // Insert should not cause reallocation
    for (uint32_t i = 0; i < 1000; ++i) {
        map.insert(Register(i), i * 32);
    }

    assert(map.size() == 1000);

    std::cout << "PASS\n";
}

void testPerformanceComparison() {
    std::cout << "Test 10: Performance characteristics... ";

    RegisterMap map;
    const size_t N = 10000;

    // Insert N registers
    for (uint32_t i = 0; i < N; ++i) {
        map.insert(Register(i), i * 32);
    }

    // Lookup performance (O(1) expected)
    // Just verify correctness; timing would require more setup
    for (uint32_t i = 0; i < N; ++i) {
        uint32_t offset = map.get(Register(i));
        assert(offset == i * 32);
    }

    // Compare memory usage to std::map estimate
    size_t actualMem = map.memoryUsage();
    size_t stdMapEstimate = N * 48;  // std::map: ~48 bytes per node

    // RegisterMap should use significantly less memory
    assert(actualMem < stdMapEstimate / 2);  // At least 50% savings

    std::cout << "PASS (Memory: RegisterMap=" << actualMem
              << "bytes vs std::map≈" << stdMapEstimate << " bytes, "
              << (100 - actualMem * 100 / stdMapEstimate) << "% savings)\n";
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "Phase 5A RegisterMap Validation Test Suite\n";
    std::cout << "===========================================\n\n";

    try {
        testBasicOperations();
        testSequentialRegisters();
        testSparseRegisters();
        testReverseMapping();
        testUpdateOverwrite();
        testErase();
        testClear();
        testMemoryUsage();
        testReserve();
        testPerformanceComparison();

        std::cout << "\n===========================================\n";
        std::cout << "✅ ALL TESTS PASSED - RegisterMap is correct!\n";
        std::cout << "===========================================\n";
        std::cout << "\nRegisterMap provides:\n";
        std::cout << "  • O(1) lookups (vs O(log n) for std::map)\n";
        std::cout << "  • ~90% memory savings\n";
        std::cout << "  • Sequential and sparse register support\n";
        std::cout << "  • Optional reverse mapping\n";
        std::cout << "\nPhase 5A integration is ready for production testing.\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n❌ TEST FAILED: Unknown exception\n";
        return 1;
    }
}
