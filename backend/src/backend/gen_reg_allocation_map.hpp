/*
 * Copyright © 2025 Frosted Beignet Contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Phase 5A: Data Structure Modernization
 * High-performance register mapping with O(1) lookups
 */

/**
 * \file gen_reg_allocation_map.hpp
 * \brief Array-based register map replacing std::map for O(1) lookups
 *
 * Replaces std::map<ir::Register, uint32_t> with direct array indexing,
 * exploiting the fact that Register is TYPE_SAFE(Register, uint32_t) with
 * sequential numbering.
 *
 * Performance improvements:
 * - Lookup complexity: O(log n) → O(1)
 * - Memory per entry: ~48 bytes → 4 bytes
 * - Cache locality: Poor (tree) → Excellent (array)
 *
 * Expected compile-time improvement: 5-10% for register-heavy kernels
 */

#ifndef __GBE_GEN_REG_ALLOCATION_MAP_HPP__
#define __GBE_GEN_REG_ALLOCATION_MAP_HPP__

#include "ir/register.hpp"
#include <vector>
#include <map>
#include <cstdint>

namespace gbe {

/**
 * @brief High-performance register mapping with O(1) lookups
 *
 * Direct array indexing for virtual→physical register mapping.
 * Uses the property that ir::Register is sequentially numbered.
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
   * @param offset Physical register offset in bytes
   */
  void insert(ir::Register reg, uint32_t offset) {
    const uint32_t index = reg.value();

    // Grow vector if needed (amortized O(1))
    if (index >= physicalOffsets_.size()) {
      physicalOffsets_.resize(index + 1, UNMAPPED);
    }

    physicalOffsets_[index] = offset;

    // Track reverse mapping if enabled
    if (needReverseMap_) {
      reverseMap_[offset] = reg;
    }
  }

  /**
   * @brief Get physical offset for a virtual register
   * @param reg Virtual register
   * @return Physical offset if mapped, UNMAPPED otherwise
   *
   * O(1) lookup time
   */
  uint32_t get(ir::Register reg) const {
    const uint32_t index = reg.value();

    if (index >= physicalOffsets_.size())
      return UNMAPPED;

    return physicalOffsets_[index];
  }

  /**
   * @brief Check if a register is mapped
   * @param reg Virtual register
   * @return true if mapped, false otherwise
   *
   * O(1) lookup time
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
   * Call before any insert() operations if reverse lookups needed.
   * Adds memory overhead but enables getReverse().
   */
  void enableReverseMap() {
    needReverseMap_ = true;
  }

  /**
   * @brief Get virtual register from physical offset
   * @param offset Physical offset
   * @return Virtual register if found, invalid register otherwise
   *
   * Requires enableReverseMap() called before inserts.
   * O(log n) lookup due to std::map (used rarely, so acceptable)
   */
  ir::Register getReverse(uint32_t offset) const {
    if (!needReverseMap_)
      return ir::Register(); // Invalid register

    auto it = reverseMap_.find(offset);
    if (it != reverseMap_.end())
      return it->second;

    return ir::Register(); // Invalid register
  }

  /**
   * @brief Check if reverse mapping is enabled
   */
  bool hasReverseMap() const {
    return needReverseMap_;
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
   * @brief Get maximum register index (capacity)
   */
  size_t capacity() const {
    return physicalOffsets_.size();
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
    // Approximate map overhead: each entry ~32 bytes (implementation dependent)
    bytes += reverseMap_.size() * (sizeof(uint32_t) + sizeof(ir::Register) + 32);
    return bytes;
  }

  /**
   * @brief Get unmapped sentinel value
   */
  static constexpr uint32_t unmapped() {
    return UNMAPPED;
  }

private:
  static constexpr uint32_t UNMAPPED = UINT32_MAX;

  // Primary mapping: virtual register index → physical offset
  // Direct array indexing for O(1) lookups
  // Sparse registers waste some space but still more efficient than std::map
  std::vector<uint32_t> physicalOffsets_;

  // Reverse mapping (optional, for offsetReg replacement)
  // Only populated if enableReverseMap() called
  // Uses map because physical offsets are not sequential
  std::map<uint32_t, ir::Register> reverseMap_;
  bool needReverseMap_ = false;
};

} // namespace gbe

#endif /* __GBE_GEN_REG_ALLOCATION_MAP_HPP__ */
