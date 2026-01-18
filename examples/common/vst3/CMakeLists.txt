cmake_minimum_required(VERSION 3.28.0)

project(nois-common-vst3 VERSION 1.0.0)

set(NOIS_COMMON_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_COMMON_INC_DIR "${NOIS_COMMON_ROOT_DIR}/include")


#--------------------------------------------------------------------------------------------------
#	Sources
#--------------------------------------------------------------------------------------------------
set(NOIS_COMMON_HSPSRC
	"${NOIS_COMMON_INC_DIR}/NoisVst3Parameter.hpp"
	"${NOIS_COMMON_INC_DIR}/NoisVst3Processor.hpp"
	"${NOIS_COMMON_INC_DIR}/NoisVst3Processor.inl"
)

source_group(
	TREE ${NOIS_COMMON_INC_DIR}
	PREFIX "Include Files"
	FILES ${NOIS_COMMON_HSPSRC}
)


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_library(
	nois-common-vst3
	INTERFACE
	"${NOIS_COMMON_HSPSRC}"
)

target_include_directories(
	nois-common-vst3
	INTERFACE
	"${NOIS_COMMON_INC_DIR}"
)

target_link_libraries(
	nois-common-vst3
	INTERFACE
	nois
	sdk
)

if(MSVC)
target_compile_options(
	nois-common-vst3
	INTERFACE
	$<$<CONFIG:Release>:/Ox>
	$<$<CONFIG:Release>:/GL>
	$<$<CONFIG:Distribution>:/Ox>
	$<$<CONFIG:Distribution>:/GL>
)
target_link_options(
	nois-common-vst3
	INTERFACE
	$<$<CONFIG:Release>:/LTCG>
	$<$<CONFIG:Distribution>:/LTCG>
)
else()
target_compile_options(
	nois-common-vst3
	INTERFACE
	-Wall
	-Wextra
	-Wpedantic
	-Wno-unused-parameter
	-Wno-sign-compare
	$<$<CONFIG:Debug>:-g>
	$<$<CONFIG:Release>:-g>
	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Release>:-funroll-loops>
	$<$<CONFIG:Distribution>:-O3>
	$<$<CONFIG:Distribution>:-flto>
	$<$<CONFIG:Distribution>:-funroll-loops>
)
target_link_options(
	nois-common-vst3
	INTERFACE
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Distribution>:-flto>
)
endif()

set_target_properties(
	nois-common-vst3
	PROPERTIES
	FOLDER "Examples"
	POSITION_INDEPENDENT_CODE ON
)
