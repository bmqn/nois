cmake_minimum_required(VERSION 3.28.0)

project(NzStretch VERSION 1.0.0)

set(NOIS_STRETCH_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_STRETCH_INC_DIR "${NOIS_STRETCH_ROOT_DIR}/include")
set(NOIS_STRETCH_SRC_DIR "${NOIS_STRETCH_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
set(FIRMWARE_NAME NzStretch)
set(FIRMWARE_SOURCES "${NOIS_STRETCH_SRC_DIR}/VersioStretch.cpp")
set(DAISY_GENERATE_BIN ON)
include(DaisyProject)

target_link_libraries(
	NzStretch
	PRIVATE
		nois
		umm_malloc
)

set_property(
	TARGET
	NzStretch
	PROPERTY
		FOLDER "Examples"
)
