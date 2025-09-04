cmake_minimum_required(VERSION 3.28.0)

#--------------------------------------------------------------------------------------------------
#	Libraries
#--------------------------------------------------------------------------------------------------


#--------------------------------------------------------------------------------------------------
#	Build
#--------------------------------------------------------------------------------------------------
add_executable(
	nois-example-scripting
		"scripting/Main.cpp"
)

target_include_directories(
	nois-example-scripting
	PRIVATE
)

target_link_libraries(
	nois-example-scripting
	PRIVATE
		nois
)
