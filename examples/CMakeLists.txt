cmake_minimum_required(VERSION 3.28.0)

set(NOIS_EXAMPLES_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(VST3_SDK_PATH "${NOIS_EXAMPLES_ROOT_DIR}/libs/vst3sdk")
set(VST3_SDK_ROOT "${NOIS_EXAMPLES_ROOT_DIR}/libs/vst3sdk")
set(public_sdk_SOURCE_DIR "${VST3_SDK_PATH}/public.sdk")


#-------------------------------------------------------------------------------------------------
#	Sub-directories
#--------------------------------------------------------------------------------------------------
# vst3sdk
add_subdirectory(${VST3_SDK_PATH})
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
set(SMTG_RUN_VST_VALIDATOR_DEFAULT OFF)
if(MSVC)
else()
target_compile_options(
	base
	PRIVATE
	-w
)
target_compile_options(
	pluginterfaces
	PRIVATE
	-w
)
target_compile_options(
	sdk
	PRIVATE
	-w
)
target_compile_options(
	sdk_common
	PRIVATE
	-w
)
target_compile_options(
	sdk_hosting
	PRIVATE
	-w
)
endif()

# nois-stretch-vst3
add_subdirectory("${NOIS_ROOT_DIR}/examples/nois-stretch/vst3")

# nois-distort-vst3
add_subdirectory("${NOIS_ROOT_DIR}/examples/nois-distort/vst3")
