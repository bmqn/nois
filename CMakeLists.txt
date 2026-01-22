cmake_minimum_required(VERSION 3.28.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(SetupConfigs)


#-------------------------------------------------------------------------------------------------
#	Project
#--------------------------------------------------------------------------------------------------
set(NOIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_INC_DIR "${NOIS_ROOT_DIR}/include")
set(NOIS_SRC_DIR "${NOIS_ROOT_DIR}/src")
set(NOIS_LIB_DIR "${NOIS_ROOT_DIR}/libs")

project(nois VERSION 1.0.0)


#--------------------------------------------------------------------------------------------------
#	Configuration
#--------------------------------------------------------------------------------------------------
set(NOIS_EXAMPLES_PLATFORM "VST3" CACHE STRING "Platform to target for examples")
option(NOIS_ENABLE_PROFILING "Enable profiling" OFF)

if(NOIS_EXAMPLES_PLATFORM STREQUAL "Versio")
add_compile_definitions(STM32H750xx=1 USE_HAL_DRIVER)
list(APPEND CMAKE_C_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
list(APPEND CMAKE_CXX_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
list(APPEND CMAKE_ASM_FLAGS "-mcpu=cortex-m7 -mthumb")
list(APPEND CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs --specs=nano.specs")
endif()


#--------------------------------------------------------------------------------------------------
#	Sources
#--------------------------------------------------------------------------------------------------
set(NOIS_HSPSRC
	"${NOIS_INC_DIR}/nois/Nois.hpp"
	"${NOIS_INC_DIR}/nois/NoisConfig.hpp"
	"${NOIS_INC_DIR}/nois/NoisMacros.hpp"
	"${NOIS_INC_DIR}/nois/NoisTypes.hpp"
	"${NOIS_INC_DIR}/nois/NoisUtil.hpp"

	"${NOIS_INC_DIR}/nois/analysis/NoisFilterBank.hpp"

	"${NOIS_INC_DIR}/nois/core/NoisBuffer.hpp"
	"${NOIS_INC_DIR}/nois/core/NoisRingBuffer.hpp"
	"${NOIS_INC_DIR}/nois/core/NoisStream.hpp"

	"${NOIS_INC_DIR}/nois/dynamic/NoisExpander.hpp"
	"${NOIS_INC_DIR}/nois/dynamic/NoisTransientShaper.hpp"

	"${NOIS_INC_DIR}/nois/effect/NoisDistorter.hpp"
	"${NOIS_INC_DIR}/nois/effect/NoisFilter.hpp"
	"${NOIS_INC_DIR}/nois/effect/NoisGainer.hpp"
	"${NOIS_INC_DIR}/nois/effect/NoisReverb.hpp"
	"${NOIS_INC_DIR}/nois/effect/NoisSignalDelayer.hpp"
	"${NOIS_INC_DIR}/nois/effect/NoisTimeStretcher.hpp"

	"${NOIS_INC_DIR}/nois/math/NoisMatrix.hpp"

	"${NOIS_INC_DIR}/nois/midi/NoisMidiBuffer.hpp"
	"${NOIS_INC_DIR}/nois/midi/NoisMidiStream.hpp"

	"${NOIS_INC_DIR}/nois/parameter/NoisParameter.hpp"
	
	"${NOIS_INC_DIR}/nois/route/NoisCombiner.hpp"
	"${NOIS_INC_DIR}/nois/route/NoisSplitter.hpp"

	"${NOIS_INC_DIR}/nois/util/NoisDelay.hpp"
	"${NOIS_INC_DIR}/nois/util/NoisSmallVector.hpp"
)

set(NOIS_SRC
	"${NOIS_SRC_DIR}/NoisPrefix.pch"

	"${NOIS_SRC_DIR}/NoisLog.h"
	"${NOIS_SRC_DIR}/NoisMacros.hpp"

	#"${NOIS_SRC_DIR}/analysis/NoisFilterBank.cpp"

	"${NOIS_SRC_DIR}/dynamic/NoisExpander.cpp"
	"${NOIS_SRC_DIR}/dynamic/NoisTransientShaper.cpp"

	"${NOIS_SRC_DIR}/effect/NoisDistorter.cpp"
	"${NOIS_SRC_DIR}/effect/NoisFilter.cpp"
	"${NOIS_SRC_DIR}/effect/NoisGainer.cpp"
	"${NOIS_SRC_DIR}/effect/NoisReverb.cpp"
	"${NOIS_SRC_DIR}/effect/NoisSignalDelayer.cpp"
	"${NOIS_SRC_DIR}/effect/NoisTimeStretcher.cpp"

	"${NOIS_SRC_DIR}/parameter/NoisParameter.cpp"

	"${NOIS_SRC_DIR}/route/NoisCombiner.cpp"
	"${NOIS_SRC_DIR}/route/NoisSplitter.cpp"
)

source_group(
	TREE ${NOIS_INC_DIR}
	PREFIX "Include Files"
	FILES ${NOIS_HSPSRC}
)

source_group(
	TREE ${NOIS_SRC_DIR}
	PREFIX "Source Files"
	FILES ${NOIS_SRC}
)


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_library(
	nois
	STATIC
	"${NOIS_HSPSRC}"
	"${NOIS_SRC}"
)

target_compile_features(
	nois
	PUBLIC
		cxx_std_20
)

target_compile_definitions(
	nois
	PUBLIC
	$<$<CONFIG:Debug>:NOIS_DEBUG=1>
	$<$<CONFIG:Release>:NOIS_RELEASE=1>
	$<$<CONFIG:Distribution>:NOIS_DISTRIBUTION=1>
)

if(NOIS_ENABLE_PROFILING)
target_compile_definitions(
	nois
	PUBLIC NOIS_ENABLE_PROFILING=1
)
endif()

target_include_directories(
	nois
	PUBLIC "${NOIS_INC_DIR}"
	PRIVATE "${NOIS_INC_DIR}" "${NOIS_SRC_DIR}" "${NOIS_LIB_DIR}/debugbreak"
)

target_precompile_headers(
	nois
	PRIVATE "${NOIS_SRC_DIR}/NoisPrefix.pch"
)

if(MSVC)
target_compile_options(
	nois
	PRIVATE
	$<$<CONFIG:Release>:/Ox>
	$<$<CONFIG:Release>:/GL>
	/Zc:__cplusplus
)
else()
target_compile_options(
	nois
	PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Wno-unused-parameter
	-Wno-sign-compare
	$<$<CONFIG:Debug>:-g>
	$<$<CONFIG:Release>:-g>
	$<$<CONFIG:Release>:-O3>
	# $<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Release>:-funroll-loops>
	$<$<CONFIG:Distribution>:-O3>
	$<$<CONFIG:Distribution>:-flto>
	$<$<CONFIG:Distribution>:-funroll-loops>
	PUBLIC
	-mavx
)
endif()

if(MSVC)
target_link_options(
	nois
	PRIVATE
	$<$<CONFIG:Release>:/LTCG>
)
else()
target_link_options(
	nois
	PRIVATE
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Distribution>:-flto>
)
endif()

if(NOIS_ENABLE_PROFILING)
target_link_libraries(
	nois
	PUBLIC
	Tracy::TracyClient
)
endif()

set_target_properties(
	nois
	PROPERTIES
	POSITION_INDEPENDENT_CODE ON
)


#-------------------------------------------------------------------------------------------------
#	Sub-directories
#--------------------------------------------------------------------------------------------------
if(NOIS_ENABLE_PROFILING)
# tracy
add_subdirectory("${NOIS_LIB_DIR}/tracy")
endif()

# examples
add_subdirectory("${NOIS_ROOT_DIR}/examples")


#-------------------------------------------------------------------------------------------------
#	Sub-directories build
#--------------------------------------------------------------------------------------------------
if(NOIS_ENABLE_PROFILING)
# tracy
set_target_properties(
	TracyClient
	PROPERTIES
	POSITION_INDEPENDENT_CODE ON
)
endif()