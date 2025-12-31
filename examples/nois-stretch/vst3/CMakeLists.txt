cmake_minimum_required(VERSION 3.28.0)

project(nois-stretch-vst3 VERSION 1.0.0)

set(NOIS_STRETCH_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_STRETCH_INC_DIR "${NOIS_STRETCH_ROOT_DIR}/include")
set(NOIS_STRETCH_SRC_DIR "${NOIS_STRETCH_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
smtg_add_vst3plugin(
	nois-stretch-vst3
	"${NOIS_STRETCH_SRC_DIR}/NoisVst3Plugin.cpp"
	"${NOIS_STRETCH_SRC_DIR}/NoisVst3Plugin.hpp"
	"${NOIS_STRETCH_SRC_DIR}/NoisVst3Factory.cpp"
	"${NOIS_STRETCH_SRC_DIR}/NoisVst3Parameter.hpp"
)

smtg_target_configure_version_file(
	nois-stretch-vst3
)

target_link_libraries(
	nois-stretch-vst3
	PRIVATE sdk nois
)

if(MSVC)
target_compile_options(
	nois-stretch-vst3
	PRIVATE
	$<$<CONFIG:Release>:/Ox>
	$<$<CONFIG:Release>:/GL>
)
target_link_options(nois-stretch-vst3
	PRIVATE
	$<$<CONFIG:Release>:/LTCG>
)
else()
target_compile_options(
	nois-stretch-vst3
	PRIVATE
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
	nois-stretch-vst3
	PRIVATE
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Distribution>:-flto>
)
endif()

set_property(
	TARGET nois-stretch-vst3
	PROPERTY POSITION_INDEPENDENT_CODE ON
)
