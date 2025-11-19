/*
 * Copyright © 2012-2025 Intel Corporation
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
 * Author: Frosted Beignet Development Team
 */

/**
 * Gen6 (Sandy Bridge) Device Configuration
 *
 * Sandy Bridge is Intel's 2nd generation Core processor (2011)
 * - Codename: Sandy Bridge
 * - Generation: Gen6
 * - Process: 32nm
 * - Execution Units: 6 (GT1) or 12 (GT2)
 * - OpenCL Support: 1.1 (software implementation, no native hardware support)
 * - DirectX: 10.1
 * - Architecture: First "modern" Intel integrated GPU
 *
 * Key Limitations vs Gen7:
 * - No native OpenCL hardware
 * - ~50% lower IPC per EU
 * - Smaller L3 cache (512KB vs 768KB)
 * - Limited atomic operations
 * - No scatter/gather optimization
 * - Lower memory bandwidth
 */

/* Common fields for Gen6 (Sandy Bridge) devices */

/* Memory Configuration */
.max_parameter_size = 1024,                    // Max parameter size for kernels
.global_mem_cache_line_size = 64,             // Cache line size in bytes
.global_mem_cache_size = 512 << 10,           // 512 KB L3 cache (shared)
.local_mem_type = CL_LOCAL,                    // Hardware local memory
.local_mem_size = 64 << 10,                    // 64 KB local memory per work-group
.scratch_mem_size = 12 << 10,                  // 12 KB scratch memory per thread
.max_mem_alloc_size = 1 * 1024 * 1024 * 1024ul, // 1 GB max single allocation (conservative)
.global_mem_size = 1 * 1024 * 1024 * 1024ul,   // 1 GB addressable memory (system RAM shared)

/* Compute Capabilities */
.max_compute_unit = 12,                        // Max 12 EUs (GT2 configuration)
.max_thread_per_unit = 7,                      // 7 threads per EU (Sandy Bridge spec)
.sub_slice_count = 1,                          // Single slice architecture

/* Work Group Configuration */
.max_work_item_dimensions = 3,                 // Standard 3D work items
.max_work_item_sizes = {512, 512, 512},        // Conservative max per dimension
.max_work_group_size = 512,                    // Max work-group size (SIMD8 optimized)

/* Global Work Sizes for Builtin Kernels */
.max_1d_global_work_sizes = {1024 * 1024, 0, 0},
.max_2d_global_work_sizes = {8192, 8192, 0},
.max_3d_global_work_sizes = {2048, 2048, 2048},

/* Image Support (Limited on Gen6) */
.image_support = CL_TRUE,
.max_read_image_args = 128,
.max_write_image_args = 8,                     // Limited write support
.max_read_write_image_args = 0,                // No read-write images on Gen6
.image2d_max_width = 8192,
.image2d_max_height = 8192,
.image3d_max_width = 2048,
.image3d_max_height = 2048,
.image3d_max_depth = 2048,
.image_max_array_size = 2048,
.image_mem_size = 64 * 1024 * 1024,            // 64 MB max image memory
.max_samplers = 16,

/* Vector Widths */
.preferred_vector_width_char = 16,
.preferred_vector_width_short = 8,
.preferred_vector_width_int = 4,
.preferred_vector_width_long = 1,              // 64-bit not preferred
.preferred_vector_width_float = 4,
.preferred_vector_width_double = 0,            // No double precision
.preferred_vector_width_half = 0,              // No half precision (f16)

.native_vector_width_char = 16,
.native_vector_width_short = 8,
.native_vector_width_int = 4,
.native_vector_width_long = 1,
.native_vector_width_float = 4,
.native_vector_width_double = 0,               // No native double support
.native_vector_width_half = 0,                 // No native half support

/* Floating Point Configuration */
.single_fp_config = CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST,
.double_fp_config = 0,                         // No double precision on Gen6
.half_fp_config = 0,                           // No half precision on Gen6

/* Memory Properties */
.global_mem_cache_type = CL_READ_WRITE_CACHE,
.mem_base_addr_align = 1024,                   // 1024-bit (128-byte) alignment
.min_data_type_align_size = 128,               // 128 bytes

/* Clock Frequency */
.max_clock_frequency = 1350,                   // Up to 1.35 GHz (varies by SKU)

/* OpenCL 2.0 Features (Not Supported on Gen6) */
.max_pipe_args = 0,
.pipe_max_active_reservations = 0,
.pipe_max_packet_siz = 0,
.max_global_variable_size = 0,
.global_variable_preferred_total_size = 0,
.svm_capabilities = 0,                         // No SVM on Gen6
.preferred_platform_atomic_alignment = 0,
.preferred_global_atomic_alignment = 0,
.preferred_local_atomic_alignment = 0,
.max_read_write_image_args = 0,

/* Device Queue (OpenCL 2.0 - Not Supported) */
.max_on_device_queues = 0,
.max_on_device_events = 0,
.queue_on_device_preferred_size = 0,
.queue_on_device_max_size = 0,

/* Subgroups (Not Supported on Gen6) */
.sub_group_sizes = {0, 0},
.sub_group_sizes_sz = 0,

/* Profiling */
.profiling_timer_resolution = 80,              // 80 ns resolution

/* Device Properties */
.address_bits = 32,                            // 32-bit addressing (Gen6 is 32-bit)
.error_correction_support = CL_FALSE,
.host_unified_memory = CL_TRUE,                // Shared system memory
.endian_little = CL_TRUE,
.available = CL_TRUE,
.compiler_available = CL_TRUE,
.linker_available = CL_TRUE,
.execution_capabilities = CL_EXEC_KERNEL,
.queue_properties = CL_QUEUE_PROFILING_ENABLE,
.queue_on_host_properties = CL_QUEUE_PROFILING_ENABLE,
.queue_on_device_properties = 0,               // No device-side queues
.platform = nullptr,                           // Set at runtime
.printf_buffer_size = 1024 * 1024,             // 1 MB printf buffer
.interop_user_sync = CL_TRUE,

/* Device Identification */
.name = "Intel(R) HD Graphics Sandy Bridge (Gen6)",
.vendor = "Intel",
.version = "OpenCL 1.1 beignet",               // Gen6 supports OpenCL 1.1
.profile = "FULL_PROFILE",
.opencl_c_version = "OpenCL C 1.1",
.driver_version = "1.4.0",                     // Beignet version
.spir_versions = "",                           // No SPIR support on Gen6
.built_in_kernels = "",                        // No built-in kernels on Gen6

/* Extension Support (Limited on Gen6) */
.extensions =
    "cl_khr_global_int32_base_atomics "
    "cl_khr_global_int32_extended_atomics "
    "cl_khr_local_int32_base_atomics "
    "cl_khr_local_int32_extended_atomics "
    "cl_khr_byte_addressable_store "
    "cl_khr_icd "
    // Note: No cl_khr_fp16, cl_khr_fp64, cl_khr_3d_image_writes on Gen6
    ,

.parent_device = nullptr,
.partition_max_sub_device = 1,
.partition_property = {0},
.affinity_domain = 0,
.partition_type = {0},

.image_pitch_alignment = 1,
.image_base_address_alignment = 4096,

#define GEN6_DEVICE
#include "cl_gt_device.h"
#undef GEN6_DEVICE
