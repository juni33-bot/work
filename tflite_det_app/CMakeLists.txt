#
# Copyright 2020 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# Builds the minimal Tensorflow Lite example.

cmake_minimum_required(VERSION 3.16)
project(app_tflite_det C CXX)

set(3RD_PARTY_DIR "${CMAKE_CURRENT_LIST_DIR}/../common/third_party")
message("third_party directory: ${3RD_PARTY_DIR}")
set(TENSORFLOW_DIR "${3RD_PARTY_DIR}/tensorflow")
message("tensorflow directory: ${TENSORFLOW_DIR}")
set(TENSORFLOW_DEP_DIR "${3RD_PARTY_DIR}/tensorflow_others")
message("tensorflow directory: ${TENSORFLOW_DEP_DIR}")
set(OPENCV2_DIR "${3RD_PARTY_DIR}/opencv2")
set(OPENCV2_DEP_DIR "${3RD_PARTY_DIR}/opencv_others")
include_directories("${OPENCV2_DIR}")
include_directories("${OPENCV2_DEP_DIR}")
include_directories("${3RD_PARTY_DIR}")
include_directories("${TENSORFLOW_DIR}")
include_directories("${TENSORFLOW_DEP_DIR}/flatbuffers/include")

set(CMAKE_CXX_STANDARD 17)
add_executable(app_tflite_det
  tflite_det_app.cc
)

target_link_libraries(app_tflite_det
  "${TENSORFLOW_DIR}/libtensorflow-lite.so"
  "${OPENCV2_DEP_DIR}/libjpeg.so.8"
  "${OPENCV2_DEP_DIR}/libpng16.so.16"
  "${OPENCV2_DEP_DIR}/libjasper.so.4"
  "${OPENCV2_DEP_DIR}/libtiff.so.5"
  "${OPENCV2_DIR}/lib/libz.so.1"
  "${OPENCV2_DIR}/lib/libopencv_core.so"
  "${OPENCV2_DIR}/lib/libopencv_imgproc.so"
  "${OPENCV2_DIR}/lib/libopencv_imgcodecs.so"
  "${TENSORFLOW_DIR}/libabsl_graphcycles_internal.so"
  "${TENSORFLOW_DIR}/libabsl_kernel_timeout_internal.so"
  "${TENSORFLOW_DIR}/libabsl_synchronization.so"
  "${TENSORFLOW_DIR}/libabsl_int128.so"
  "${TENSORFLOW_DIR}/libabsl_time_zone.so"
  "${TENSORFLOW_DIR}/libabsl_civil_time.so"
  "${TENSORFLOW_DIR}/libabsl_time.so"
  "${TENSORFLOW_DIR}/libabsl_cordz_info.so"
  "${TENSORFLOW_DIR}/libabsl_cord.so"
  "${TENSORFLOW_DIR}/libabsl_cordz_handle.so"
  "${TENSORFLOW_DIR}/libabsl_cord_internal.so"
  "${TENSORFLOW_DIR}/libabsl_strings.so"
  "${TENSORFLOW_DIR}/libabsl_str_format_internal.so"
  "${TENSORFLOW_DIR}/libabsl_string_view.so"
  "${TENSORFLOW_DIR}/libabsl_cordz_functions.so"
  "${TENSORFLOW_DIR}/libabsl_strings_internal.so"
  "${TENSORFLOW_DIR}/libabsl_stacktrace.so"
  "${TENSORFLOW_DIR}/libabsl_demangle_internal.so"
  "${TENSORFLOW_DIR}/libabsl_debugging_internal.so"
  "${TENSORFLOW_DIR}/libabsl_symbolize.so"
  "${TENSORFLOW_DIR}/libabsl_flags_marshalling.so"
  "${TENSORFLOW_DIR}/libabsl_flags_config.so"
  "${TENSORFLOW_DIR}/libabsl_flags_commandlineflag.so"
  "${TENSORFLOW_DIR}/libabsl_flags_commandlineflag_internal.so"
  "${TENSORFLOW_DIR}/libabsl_flags_program_name.so"
  "${TENSORFLOW_DIR}/libabsl_flags_private_handle_accessor.so"
  "${TENSORFLOW_DIR}/libabsl_flags_reflection.so"
  "${TENSORFLOW_DIR}/libabsl_flags_internal.so"
  "${TENSORFLOW_DIR}/libabsl_flags.so"
  "${TENSORFLOW_DIR}/libabsl_log_severity.so"
  "${TENSORFLOW_DIR}/libabsl_strerror.so"
  "${TENSORFLOW_DIR}/libabsl_malloc_internal.so"
  "${TENSORFLOW_DIR}/libabsl_spinlock_wait.so"
  "${TENSORFLOW_DIR}/libabsl_throw_delegate.so"
  "${TENSORFLOW_DIR}/libabsl_base.so"
  "${TENSORFLOW_DIR}/libabsl_raw_logging_internal.so"
  "${TENSORFLOW_DIR}/libabsl_bad_optional_access.so"
  "${TENSORFLOW_DIR}/libabsl_bad_any_cast_impl.so"
  "${TENSORFLOW_DIR}/libabsl_bad_variant_access.so"
  "${TENSORFLOW_DIR}/libabsl_low_level_hash.so"
  "${TENSORFLOW_DIR}/libabsl_hash.so"
  "${TENSORFLOW_DIR}/libabsl_city.so"
  "${TENSORFLOW_DIR}/libabsl_status.so"
  "${TENSORFLOW_DIR}/libabsl_exponential_biased.so"
  "${TENSORFLOW_DIR}/libabsl_crc_cord_state.so"
  "${TENSORFLOW_DIR}/libabsl_crc_cpu_detect.so"
  "${TENSORFLOW_DIR}/libabsl_crc32c.so"
  "${TENSORFLOW_DIR}/libabsl_crc_internal.so"
  "${TENSORFLOW_DIR}/libabsl_hashtablez_sampler.so"
  "${TENSORFLOW_DIR}/libabsl_raw_hash_set.so"
  "${TENSORFLOW_DIR}/libcpuinfo.so"
  "${TENSORFLOW_DIR}/libfft2d_fftsg.so"
  "${TENSORFLOW_DIR}/libfft2d_fftsg2d.so"
  "${TENSORFLOW_DIR}/libfarmhash.so"
  "${TENSORFLOW_DIR}/libeight_bit_int_gemm.so"
)

target_compile_definitions(app_tflite_det PRIVATE USE_GPU=1)
