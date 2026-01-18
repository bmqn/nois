cmake_minimum_required(VERSION 3.28.0)

project(verb-vst3 VERSION 1.0.0)

set(NOIS_VERB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_VERB_INC_DIR "${NOIS_VERB_ROOT_DIR}/include")
set(NOIS_VERB_SRC_DIR "${NOIS_VERB_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
smtg_add_vst3plugin(
	verb-vst3
	"${NOIS_VERB_SRC_DIR}/Vst3Verb.cpp"
)

smtg_target_configure_version_file(
	verb-vst3
)

target_link_libraries(
	verb-vst3
	nois-common-vst3
)

set_target_properties(
	verb-vst3
	PROPERTIES
	FOLDER "Examples"
	POSITION_INDEPENDENT_CODE ON
)
