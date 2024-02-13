cmake_minimum_required(VERSION 3.28.0)

set(NOIS_ROOT_DIR	${CMAKE_CURRENT_SOURCE_DIR})
set(NOIS_INC_DIR	${NOIS_ROOT_DIR}/include)
set(NOIS_SRC_DIR	${NOIS_ROOT_DIR}/src)
set(NOIS_LIBS_DIR	${NOIS_ROOT_DIR}/libs)

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

set(NOIS_SRC
	"${NOIS_SRC_DIR}/NoisPrefix.pch"

	"${NOIS_SRC_DIR}/NoisLog.h"
	"${NOIS_SRC_DIR}/NoisMixer.cpp"
)

source_group("Include" FILES ${NOIS_HSP})
source_group("Source" FILES ${NOIS_SRC})


#--------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_library(
	nois STATIC
	${NOIS_SRC}
)

target_include_directories(
	nois
	PUBLIC ${NOIS_INC_DIR}
	PRIVATE ${NOIS_SRC_DIR} ${NOIS_LIBS_DIR}/debugbreak
)

target_precompile_headers(
	nois
	PRIVATE "${NOIS_SRC_DIR}/NoisPrefix.pch"
)


#--------------------------------------------------------------------------------------------------
#	Sources
#--------------------------------------------------------------------------------------------------
# add_subdirectory(examples)
