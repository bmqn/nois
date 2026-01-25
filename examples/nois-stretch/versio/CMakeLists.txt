cmake_minimum_required(VERSION 3.28.0)

project(nois-stretch-versio VERSION 1.0.0)

set(NOIS_STRETCH_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_STRETCH_INC_DIR "${NOIS_STRETCH_ROOT_DIR}/include")
set(NOIS_STRETCH_SRC_DIR "${NOIS_STRETCH_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
set(FIRMWARE_NAME nois-stretch-versio)
set(FIRMWARE_SOURCES "${NOIS_STRETCH_SRC_DIR}/VersioStretch.cpp")
include(DaisyProject)

target_link_libraries(
	nois-stretch-versio
	PRIVATE
		nois
)

target_compile_options(
	nois-stretch-versio
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
	nois-stretch-versio
	PRIVATE
		$<$<CONFIG:Release>:-flto>
		$<$<CONFIG:Distribution>:-flto>
)

set_property(
	TARGET
	nois-stretch-versio
	PROPERTY
		FOLDER "Examples"
		POSITION_INDEPENDENT_CODE ON
)
