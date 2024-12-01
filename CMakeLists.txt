cmake_minimum_required(VERSION 3.25.0)

set(NOIS_ROOT_DIR	"${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_INC_DIR	"${NOIS_ROOT_DIR}/include")
set(NOIS_SRC_DIR	"${NOIS_ROOT_DIR}/src")
set(NOIS_LIB_DIR	"${NOIS_ROOT_DIR}/libs")

project(nois)


#--------------------------------------------------------------------------------------------------
#	Configuration
#--------------------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if(MSVC)
add_compile_options(
	$<$<CONFIG:RELEASE>:/Ox>
	$<$<CONFIG:RELEASE>:/GL>
)
add_link_options(
	$<$<CONFIG:RELEASE>:/LTCG>
)
else()
add_compile_options(
	-mavx
	$<$<CONFIG:RELEASE>:-Ofast>
)
add_link_options(
	$<$<CONFIG:RELEASE>:-flto>
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
	"${NOIS_HSP}/nois/NoisTypes.hpp"
	"${NOIS_HSP}/nois/NoisUtil.hpp"

	"${NOIS_HSP}/nois/core/NoisChannel.hpp"
	"${NOIS_HSP}/nois/core/NoisStream.hpp"

	"${NOIS_HSP}/nois/effect/NoisDistorter.hpp"
	"${NOIS_HSP}/nois/effect/NoisFilter.hpp"
	"${NOIS_HSP}/nois/effect/NoisGainer.hpp"
	"${NOIS_HSP}/nois/effect/NoisSignalDelayer.hpp"
	"${NOIS_HSP}/nois/effect/NoisTimeStretcher.hpp"
	
	"${NOIS_HSP}/nois/route/NoisCombiner.hpp"
	"${NOIS_HSP}/nois/route/NoisMixer.hpp"
	"${NOIS_HSP}/nois/route/NoisSplitter.hpp"
)

set(NOIS_SRC
	"${NOIS_SRC_DIR}/NoisPrefix.pch"
	"${NOIS_SRC_DIR}/NoisLog.h"

	"${NOIS_SRC_DIR}/core/NoisChannel.cpp"

	"${NOIS_SRC_DIR}/effect/NoisDistorter.cpp"
	"${NOIS_SRC_DIR}/effect/NoisFilter.cpp"
	"${NOIS_SRC_DIR}/effect/NoisGainer.cpp"
	"${NOIS_SRC_DIR}/effect/NoisSignalDelayer.cpp"
	"${NOIS_SRC_DIR}/effect/NoisTimeStretcher.cpp"

	"${NOIS_SRC_DIR}/route/NoisCombiner.cpp"
	"${NOIS_SRC_DIR}/route/NoisMixer.cpp"
	"${NOIS_SRC_DIR}/route/NoisSplitter.cpp"
)

source_group("Include Files" FILES "${NOIS_HSPSRC}")
source_group("Source Files" FILES "${NOIS_SRC}")


#--------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_library(
	nois STATIC
	"${NOIS_SRC}" "${NOIS_HSPSRC}"
)

target_include_directories(
	nois
	PUBLIC "${NOIS_INC_DIR}" "${NOIS_LIB_DIR}/debugbreak"
	PRIVATE "${NOIS_INC_DIR}" "${NOIS_SRC_DIR}"
)

target_precompile_headers(
	nois
	PRIVATE "${NOIS_SRC_DIR}/NoisPrefix.pch"
)
