cmake_minimum_required(VERSION 3.25.0)

project(nois-stretch-vst3 VERSION 1.0.0)

set(NOIS_VST3_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_VST3_INC_DIR "${NOIS_VST3_ROOT_DIR}/include")
set(NOIS_VST3_SRC_DIR "${NOIS_VST3_ROOT_DIR}/src")

set(public_sdk_SOURCE_DIR "${VST3_SDK_PATH}/public.sdk")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
smtg_add_vst3plugin(
	nois-stretch-vst3
	"${NOIS_VST3_SRC_DIR}/NoisVst3Plugin.cpp"
	"${NOIS_VST3_SRC_DIR}/NoisVst3Plugin.hpp"
	"${NOIS_VST3_SRC_DIR}/NoisVst3Factory.cpp"
)

smtg_target_configure_version_file(
	nois-stretch-vst3
)

target_link_libraries(nois-stretch-vst3
	PRIVATE sdk nois
)

if(MSVC)
target_compile_options(nois-stretch-vst3
	PRIVATE
	$<$<CONFIG:Release>:/Ox>
	$<$<CONFIG:Release>:/GL>
)
target_link_options(nois-stretch-vst3
	PRIVATE
	$<$<CONFIG:Release>:/LTCG>
)
else()
target_compile_options(nois-stretch-vst3
	PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	$<$<CONFIG:Debug>:-g>
	$<$<CONFIG:Release>:-g>
	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Release>:-funroll-loops>
	
)
target_link_options(nois-stretch-vst3
	PRIVATE
	$<$<CONFIG:Release>:-flto>
)
endif()
