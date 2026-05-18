cmake_minimum_required(VERSION 3.28.0)

project(NzStretch VERSION 1.0.0)

set(NOIS_STRETCH_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_STRETCH_INC_DIR "${NOIS_STRETCH_ROOT_DIR}/include")
set(NOIS_STRETCH_SRC_DIR "${NOIS_STRETCH_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
smtg_add_vst3plugin(
	NzStretch
	"${NOIS_STRETCH_SRC_DIR}/Vst3Stretch.cpp"
)

smtg_target_configure_version_file(
	NzStretch
)

target_link_libraries(
	NzStretch
	PRIVATE
		NzCommon
)

set_target_properties(
	NzStretch
	PROPERTIES
		FOLDER "Examples"
		POSITION_INDEPENDENT_CODE ON
)
