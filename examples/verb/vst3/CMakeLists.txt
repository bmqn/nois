cmake_minimum_required(VERSION 3.28.0)

project(verb-vst3 VERSION 1.0.0)

set(NOIS_VERB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_VERB_INC_DIR "${NOIS_VERB_ROOT_DIR}/include")
set(NOIS_VERB_SRC_DIR "${NOIS_VERB_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
smtg_add_vst3plugin(
	verb-vst3
	"${NOIS_VERB_SRC_DIR}/NoisVst3Plugin.cpp"
	"${NOIS_VERB_SRC_DIR}/NoisVst3Plugin.hpp"
	"${NOIS_VERB_SRC_DIR}/NoisVst3Factory.cpp"
)

smtg_target_configure_version_file(
	verb-vst3
)

target_link_libraries(
	verb-vst3
	PRIVATE sdk nois nois-common-vst3
)

if(MSVC)
target_compile_options(
	verb-vst3
	PRIVATE
	$<$<CONFIG:Release>:/Ox>
	$<$<CONFIG:Release>:/GL>
	$<$<CONFIG:Distribution>:/Ox>
	$<$<CONFIG:Distribution>:/GL>
)
target_link_options(
	verb-vst3
	PRIVATE
	$<$<CONFIG:Release>:/LTCG>
	$<$<CONFIG:Distribution>:/LTCG>
)
else()
target_compile_options(
	verb-vst3
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
	verb-vst3
	PRIVATE
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Distribution>:-flto>
)
endif()

set_target_properties(
	verb-vst3
	PROPERTIES
	FOLDER "Examples"
	POSITION_INDEPENDENT_CODE ON
)
