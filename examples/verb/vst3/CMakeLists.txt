cmake_minimum_required(VERSION 3.28.0)

project(NzVerb VERSION 1.0.0)

set(NOIS_VERB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_VERB_INC_DIR "${NOIS_VERB_ROOT_DIR}/include")
set(NOIS_VERB_SRC_DIR "${NOIS_VERB_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
smtg_add_vst3plugin(
	NzVerb
	"${NOIS_VERB_SRC_DIR}/Vst3Verb.cpp"
)

smtg_target_configure_version_file(
	NzVerb
)

target_link_libraries(
	NzVerb
	PRIVATE
		NzCommon
)

set_target_properties(
	NzVerb
	PROPERTIES
		FOLDER "Examples"
		POSITION_INDEPENDENT_CODE ON
)
