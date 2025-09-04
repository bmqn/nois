cmake_minimum_required(VERSION 3.25.0)

set(NOIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_INC_DIR "${NOIS_ROOT_DIR}/include")
set(NOIS_SRC_DIR "${NOIS_ROOT_DIR}/src")
set(NOIS_LIB_DIR "${NOIS_ROOT_DIR}/libs")

set(VST3_SDK_PATH "${NOIS_ROOT_DIR}/libs/vst3sdk")
set(VST3_SDK_ROOT "${NOIS_ROOT_DIR}/libs/vst3sdk")
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)

project(nois VERSION 1.0.0)


#--------------------------------------------------------------------------------------------------
#	Configuration
#--------------------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_compile_options(
	-w
)

if(MSVC)
add_compile_options(
	$<$<CONFIG:Release>:/Ox>
	$<$<CONFIG:Release>:/GL>
)
add_link_options(
	$<$<CONFIG:Release>:/LTCG>
)
else()
add_compile_options(
	$<$<CONFIG:Debug>:-g>
	$<$<CONFIG:Release>:-g>
	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Release>:-funroll-loops>
)
add_link_options(
	$<$<CONFIG:Release>:-flto>
)
endif()


#--------------------------------------------------------------------------------------------------
#	Sources
#--------------------------------------------------------------------------------------------------
set(NOIS_HSP
	"${NOIS_INC_DIR}"
)

set(NOIS_HSPSRC
	"${NOIS_HSP}/nois/Nois.hpp"
	"${NOIS_HSP}/nois/NoisConfig.hpp"
	"${NOIS_HSP}/nois/NoisMacros.hpp"
	"${NOIS_HSP}/nois/NoisTypes.hpp"
	"${NOIS_HSP}/nois/NoisUtil.hpp"

	"${NOIS_HSP}/nois/analysis/NoisFilterBank.hpp"
	"${NOIS_HSP}/nois/analysis/NoisPitchDetector.hpp"

	"${NOIS_HSP}/nois/core/NoisBuffer.hpp"
	"${NOIS_HSP}/nois/core/NoisRingBuffer.hpp"
	"${NOIS_HSP}/nois/core/NoisStream.hpp"

	"${NOIS_HSP}/nois/dynamic/NoisExpander.hpp"
	"${NOIS_HSP}/nois/dynamic/NoisTransientShaper.hpp"

	"${NOIS_HSP}/nois/effect/NoisDistorter.hpp"
	"${NOIS_HSP}/nois/effect/NoisFilter.hpp"
	"${NOIS_HSP}/nois/effect/NoisGainer.hpp"
	"${NOIS_HSP}/nois/effect/NoisSignalDelayer.hpp"
	"${NOIS_HSP}/nois/effect/NoisTimeStretcher.hpp"

	"${NOIS_HSP}/nois/midi/NoisMidiBuffer.hpp"
	"${NOIS_HSP}/nois/midi/NoisMidiStream.hpp"

	"${NOIS_HSP}/nois/parameter/NoisParameter.hpp"
	
	"${NOIS_HSP}/nois/route/NoisCombiner.hpp"
	"${NOIS_HSP}/nois/route/NoisSplitter.hpp"
)

source_group(
	TREE ${NOIS_HSP}
	PREFIX "Include Files"
	FILES ${NOIS_HSPSRC}
)

set(NOIS_SRC
	"${NOIS_SRC_DIR}/NoisPrefix.pch"

	"${NOIS_SRC_DIR}/NoisLog.h"
	"${NOIS_SRC_DIR}/NoisMacros.hpp"

	"${NOIS_SRC_DIR}/analysis/NoisFilterBank.cpp"
	"${NOIS_SRC_DIR}/analysis/NoisPitchDetector.cpp"

	"${NOIS_SRC_DIR}/dynamic/NoisExpander.cpp"
	"${NOIS_SRC_DIR}/dynamic/NoisTransientShaper.cpp"

	"${NOIS_SRC_DIR}/effect/NoisDistorter.cpp"
	"${NOIS_SRC_DIR}/effect/NoisFilter.cpp"
	"${NOIS_SRC_DIR}/effect/NoisGainer.cpp"
	"${NOIS_SRC_DIR}/effect/NoisSignalDelayer.cpp"
	"${NOIS_SRC_DIR}/effect/NoisTimeStretcher.cpp"

	"${NOIS_SRC_DIR}/parameter/NoisParameter.cpp"

	"${NOIS_SRC_DIR}/route/NoisCombiner.cpp"
	"${NOIS_SRC_DIR}/route/NoisSplitter.cpp"
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
	STATIC "${NOIS_SRC}" "${NOIS_HSPSRC}"
)

target_include_directories(
	nois
	PUBLIC "${NOIS_INC_DIR}"
	PRIVATE "${NOIS_INC_DIR}" "${NOIS_SRC_DIR}" "${NOIS_LIB_DIR}/debugbreak"
)

target_precompile_headers(
	nois
	PRIVATE "${NOIS_SRC_DIR}/NoisPrefix.pch"
)

target_compile_options(
	nois
	PRIVATE
	-Wall
	-Wextra
	-Wpedantic
)

set_property(
	TARGET nois
	PROPERTY POSITION_INDEPENDENT_CODE ON
)


#-------------------------------------------------------------------------------------------------
#	Subdirectories
#--------------------------------------------------------------------------------------------------
# vst3sdk
add_subdirectory(${VST3_SDK_PATH})

# nois-sretch-vst3
add_subdirectory("${NOIS_ROOT_DIR}/examples/nois-stretch/vst3")
