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
)

smtg_target_configure_version_file(
	nois-stretch-vst3
)

target_link_libraries(
	nois-stretch-vst3
	PRIVATE
		nois-common-vst3
)

set_target_properties(
	nois-stretch-vst3
	PROPERTIES
		FOLDER "Examples"
		POSITION_INDEPENDENT_CODE ON
)
