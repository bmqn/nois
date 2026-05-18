cmake_minimum_required(VERSION 3.28.0)

project(NzCommon VERSION 1.0.0)

set(NOIS_COMMON_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_COMMON_INC_DIR "${NOIS_COMMON_ROOT_DIR}/include")
set(NOIS_COMMON_SRC_DIR "${NOIS_COMMON_ROOT_DIR}/src")


#--------------------------------------------------------------------------------------------------
#	Sources
#--------------------------------------------------------------------------------------------------
set(NOIS_COMMON_HSPSRC
	"${NOIS_COMMON_INC_DIR}/NoisVst3Parameter.hpp"
	"${NOIS_COMMON_INC_DIR}/NoisVst3Processor.hpp"
	"${NOIS_COMMON_INC_DIR}/NoisVst3Processor.inl"
)

set(NOIS_COMMON_SRC
	"${NOIS_COMMON_SRC_DIR}/NoisVst3.cpp"
)

source_group(
	TREE ${NOIS_COMMON_INC_DIR}
	PREFIX "Include Files"
	FILES ${NOIS_COMMON_HSPSRC}
)

source_group(
	TREE ${NOIS_COMMON_SRC_DIR}
	PREFIX "Source Files"
	FILES ${NOIS_COMMON_SRC}
)


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_library(
	NzCommon
	OBJECT
		"${NOIS_COMMON_SRC}"
		"${NOIS_COMMON_HSPSRC}"
)

target_include_directories(
	NzCommon
	PUBLIC
		"${NOIS_COMMON_INC_DIR}"
)

target_link_libraries(
	NzCommon
	PUBLIC
		nois
		sdk
)

set_target_properties(
	NzCommon
	PROPERTIES
		FOLDER "Examples"
		POSITION_INDEPENDENT_CODE ON
)
