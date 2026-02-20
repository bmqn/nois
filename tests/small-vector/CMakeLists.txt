cmake_minimum_required(VERSION 3.28.0)

set(NOIS_TEST_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_TEST_SRC_DIR "${NOIS_TEST_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_executable(
	small-vector
	"${NOIS_TEST_SRC_DIR}/Main.cpp"
)

target_link_libraries(
	small-vector
	PRIVATE
		nois
)

set_target_properties(
	small-vector
	PROPERTIES
		FOLDER "Tests"
)
