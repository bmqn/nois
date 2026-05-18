cmake_minimum_required(VERSION 3.28.0)

project(NzDistort VERSION 1.0.0)

set(NOIS_DISTORT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_DISTORT_INC_DIR "${NOIS_DISTORT_ROOT_DIR}/include")
set(NOIS_DISTORT_SRC_DIR "${NOIS_DISTORT_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
smtg_add_vst3plugin(
	NzDistort
	"${NOIS_DISTORT_SRC_DIR}/Vst3Distort.cpp"
)

smtg_target_configure_version_file(
	NzDistort
)

target_link_libraries(
	NzDistort
	PRIVATE
		NzCommon
)

set_target_properties(
	NzDistort
	PROPERTIES
		FOLDER "Examples"
		POSITION_INDEPENDENT_CODE ON
)
