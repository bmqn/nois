cmake_minimum_required(VERSION 3.28.0)

set(NOIS_EXAMPLES_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_EXAMPLES_LIB_DIR "${NOIS_EXAMPLES_ROOT_DIR}/libs")


#-------------------------------------------------------------------------------------------------
#	Sub-directories
#--------------------------------------------------------------------------------------------------
if(NOIS_EXAMPLES_PLATFORM STREQUAL "VST3")
	set(VST3_SDK_PATH "${NOIS_EXAMPLES_LIB_DIR}/vst3sdk")
	set(VST3_SDK_ROOT "${NOIS_EXAMPLES_LIB_DIR}/vst3sdk")
	set(public_sdk_SOURCE_DIR "${VST3_SDK_PATH}/public.sdk")

	# vst3sdk
	set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
	set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
	set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
	set(SMTG_CREATE_PLUGIN_LINK OFF)
	add_subdirectory("${NOIS_EXAMPLES_LIB_DIR}/vst3sdk")

	# NzCommon
	add_subdirectory("${NOIS_EXAMPLES_ROOT_DIR}/common/vst3")

	# NzStretch
	add_subdirectory("${NOIS_EXAMPLES_ROOT_DIR}/nois-stretch/vst3")

	# NzDistort
	add_subdirectory("${NOIS_EXAMPLES_ROOT_DIR}/nois-distort/vst3")

	# NzVerb
	add_subdirectory("${NOIS_EXAMPLES_ROOT_DIR}/verb/vst3" "Examples")
elseif(NOIS_EXAMPLES_PLATFORM STREQUAL "Versio")
	# libDaisy
	add_subdirectory("${NOIS_EXAMPLES_LIB_DIR}/libDaisy")

	# umm_malloc
	add_library(
		umm_malloc
		STATIC
		"${NOIS_EXAMPLES_LIB_DIR}/umm_malloc/src/umm_malloc.c"
	)
	target_include_directories(
		umm_malloc
		PUBLIC
		"${NOIS_EXAMPLES_LIB_DIR}/umm_malloc/src"
		"${NOIS_EXAMPLES_LIB_DIR}/umm_malloc/src/config"
		"${NOIS_EXAMPLES_LIB_DIR}/umm_malloc/src/config/host"
	)
	target_compile_definitions(
		umm_malloc
		PUBLIC
		UMM_MALLOC_CFGFILE="${NOIS_EXAMPLES_ROOT_DIR}/common/versio/include/umm_malloc_cfgport.h"
	)

	# NzStretch
	add_subdirectory("${NOIS_EXAMPLES_ROOT_DIR}/nois-stretch/versio")
endif()


#-------------------------------------------------------------------------------------------------
#	Sub-directories build
#--------------------------------------------------------------------------------------------------
if(NOIS_EXAMPLES_PLATFORM STREQUAL "VST3")
	# vst3sdk
	if(MSVC)
	else()
		target_compile_options(
			base
			PRIVATE
			-w
			$<$<CONFIG:Release>:-flto=thin>
		)

		target_compile_options(
			pluginterfaces
			PRIVATE
			-w
			$<$<CONFIG:Release>:-flto=thin>
		)

		target_compile_options(
			sdk
			PRIVATE
			-w
			$<$<CONFIG:Release>:-flto=thin>
		)

		target_compile_options(
			sdk_common
			PRIVATE
			-w
			$<$<CONFIG:Release>:-flto=thin>
		)

		target_compile_options(
			sdk_hosting
			PRIVATE
			-w
			$<$<CONFIG:Release>:-flto=thin>
		)

		target_compile_options(
			validator
			PRIVATE
			-w
			$<$<CONFIG:Release>:-flto=thin>
		)
endif()

endif()
