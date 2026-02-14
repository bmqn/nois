cmake_minimum_required(VERSION 3.28.0)

set(NOIS_TEST_SPAN_ALLOCATOR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NOIS_TEST_SPAN_ALLOCATOR_SRC_DIR "${NOIS_TEST_SPAN_ALLOCATOR_ROOT_DIR}/src")


#-------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_executable(
	test-span-allocator
	"${NOIS_TEST_SPAN_ALLOCATOR_SRC_DIR}/Main.cpp"
)

target_link_libraries(
	test-span-allocator
	PRIVATE
		nois
)

set_target_properties(
	test-span-allocator
	PROPERTIES
		FOLDER "Tests"
)
