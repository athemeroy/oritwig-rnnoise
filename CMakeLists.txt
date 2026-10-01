cmake_minimum_required(VERSION 3.16)
project(oritwig_rnnoise_proof VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(CTest)
find_package(Threads REQUIRED)
option(ORITWIG_BUILD_JNI "Build minimal Java/Android JNI binding" ${ANDROID})
option(ORITWIG_SANITIZE "Address and undefined-behavior sanitizers (host only)" OFF)

set(RNNOISE_SOURCES
  vendor/rnnoise/src/celt_lpc.c
  vendor/rnnoise/src/denoise.c
  vendor/rnnoise/src/kiss_fft.c
  vendor/rnnoise/src/pitch.c
  vendor/rnnoise/src/rnn_data.c
  vendor/rnnoise/src/rnn_reader.c
  vendor/rnnoise/src/rnn.c)
# The complete vendored subtree is byte-for-byte upstream. All namespacing is
# expressed in the build, following Telegram's original RNNoise target.
add_library(rnnoise_core STATIC ${RNNOISE_SOURCES})
add_library(RNNoise::Core ALIAS rnnoise_core)
set_target_properties(rnnoise_core PROPERTIES C_VISIBILITY_PRESET hidden)
target_include_directories(rnnoise_core PUBLIC vendor/rnnoise/include)
target_compile_definitions(rnnoise_core PRIVATE RNNOISE_BUILD
  _celt_autocorr=rnnoise__celt_autocorr celt_fir=rnnoise_celt_fir
  celt_iir=rnnoise_celt_iir _celt_lpc=rnnoise__celt_lpc
  celt_pitch_xcorr=rnnoise_celt_pitch_xcorr
  compute_band_corr=rnnoise_compute_band_corr
  compute_band_energy=rnnoise_compute_band_energy
  compute_dense=rnnoise_compute_dense compute_gru=rnnoise_compute_gru
  compute_rnn=rnnoise_compute_rnn interp_band_gain=rnnoise_interp_band_gain
  opus_fft_alloc=rnnoise_opus_fft_alloc
  opus_fft_alloc_arch_c=rnnoise_opus_fft_alloc_arch_c
  opus_fft_alloc_twiddles=rnnoise_opus_fft_alloc_twiddles
  opus_fft_c=rnnoise_opus_fft_c opus_fft_free=rnnoise_opus_fft_free
  opus_fft_free_arch_c=rnnoise_opus_fft_free_arch_c
  opus_fft_impl=rnnoise_opus_fft_impl opus_ifft_c=rnnoise_opus_ifft_c
  pitch_downsample=rnnoise_pitch_downsample pitch_filter=rnnoise_pitch_filter
  pitch_search=rnnoise_pitch_search remove_doubling=rnnoise_remove_doubling
  common=rnnoise_common)
target_link_libraries(rnnoise_core PUBLIC m)

add_library(oritwig_denoise STATIC src/denoise_adapter.c)
add_library(Oritwig::Denoise ALIAS oritwig_denoise)
target_include_directories(oritwig_denoise PUBLIC include)
target_link_libraries(oritwig_denoise PUBLIC rnnoise_core Threads::Threads)
target_compile_options(oritwig_denoise PRIVATE -Wall -Wextra -Werror)
add_executable(oritwig-denoise-pcm examples/denoise_pcm.c)
target_link_libraries(oritwig-denoise-pcm PRIVATE oritwig_denoise)
target_compile_options(oritwig-denoise-pcm PRIVATE -Wall -Wextra -Werror)

if(ORITWIG_BUILD_JNI)
  add_library(oritwig_rnnoise_jni SHARED src/denoise_jni.c)
  if(NOT ANDROID)
    find_package(JNI REQUIRED)
    target_include_directories(oritwig_rnnoise_jni PRIVATE ${JNI_INCLUDE_DIRS})
  endif()
  target_link_libraries(oritwig_rnnoise_jni PRIVATE oritwig_denoise)
  target_compile_options(oritwig_rnnoise_jni PRIVATE -Wall -Wextra -Werror)
  if(UNIX AND NOT APPLE)
    target_link_options(oritwig_rnnoise_jni PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/jni-exports.map")
    set_property(TARGET oritwig_rnnoise_jni APPEND PROPERTY LINK_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/src/jni-exports.map")
  endif()
endif()

if(BUILD_TESTING AND NOT ANDROID)
  add_executable(denoise_tests tests/denoise_tests.c)
  target_link_libraries(denoise_tests PRIVATE oritwig_denoise)
  target_compile_options(denoise_tests PRIVATE -Wall -Wextra -Werror)
  add_test(NAME native_contract COMMAND denoise_tests)
  # Independent unwrapped build of the exact original sources: no adapter and no
  # Telegram symbol-renaming definitions. Compare serialized IEEE float samples
  # and VAD values from separate processes, on the same compiler/architecture.
  add_library(rnnoise_original_reference STATIC ${RNNOISE_SOURCES})
  target_include_directories(rnnoise_original_reference PUBLIC vendor/rnnoise/include)
  target_link_libraries(rnnoise_original_reference PUBLIC m)
  add_executable(reference_fixture tests/reference_fixture.c)
  target_link_libraries(reference_fixture PRIVATE rnnoise_original_reference)
  add_executable(adapter_fixture tests/reference_fixture.c)
  target_compile_definitions(adapter_fixture PRIVATE TEST_ADAPTER=1)
  target_link_libraries(adapter_fixture PRIVATE oritwig_denoise)
  add_test(NAME original_reference_equivalence
    COMMAND ${CMAKE_COMMAND}
      -DREFERENCE=$<TARGET_FILE:reference_fixture>
      -DADAPTER=$<TARGET_FILE:adapter_fixture>
      -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_reference.cmake)
endif()
if(ORITWIG_SANITIZE)
  if(ANDROID)
    message(FATAL_ERROR "Sanitizer proof is host-only")
  endif()
  foreach(t rnnoise_core oritwig_denoise oritwig-denoise-pcm denoise_tests
      rnnoise_original_reference reference_fixture adapter_fixture)
    if(TARGET ${t})
      target_compile_options(${t} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${t} PRIVATE -fsanitize=address,undefined)
    endif()
  endforeach()
endif()
