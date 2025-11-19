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
 * Phase 5A: Extracted for IntervalStore compatibility
 */

#ifndef __GBE_GEN_REG_INTERVAL_HPP__
#define __GBE_GEN_REG_INTERVAL_HPP__

#include "ir/register.hpp"
#include "sys/platform.hpp"
#include <climits>
#include <cstdint>

namespace gbe {

  /**
   * @brief Liveness interval for a register
   *
   * Tracks the lifetime of a virtual register through the program,
   * including its first and last use points (minID, maxID).
   */
  struct GenRegInterval {
    INLINE GenRegInterval(ir::Register reg) :
      reg(reg), minID(INT_MAX), maxID(-INT_MAX), accessCount(0),
      blockID(-1), conflictReg(0), b3OpAlign(0), usedHole(false), isHole(false){}

    ir::Register reg;         //!< (virtual) register of the interval
    int32_t minID, maxID;     //!< Starting and ending points
    int32_t accessCount;      //!< Number of times this register is accessed
    int32_t blockID;          //!< blockID for in-block regs that can reuse hole
    ir::Register conflictReg; //!< Has bank conflict with this register
    bool b3OpAlign;           //!< Requires 3-op alignment (16-byte)
    bool usedHole;            //!< This interval reuses a hole
    bool isHole;              //!< This interval is a hole (can be reused)
  };

} // namespace gbe

#endif /* __GBE_GEN_REG_INTERVAL_HPP__ */
