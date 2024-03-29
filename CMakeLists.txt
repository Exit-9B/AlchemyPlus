cmake_minimum_required(VERSION 3.24)

project(
	AlchemyPlus
	VERSION 1.1.0
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_IGNORE_PATH "${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
include(SKSEPlugin)

set(SKSE_SUPPORT_VR ON)

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	SOURCES
		.clang-format
		.editorconfig
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/PCH/PCH.h
)

find_package(jsoncpp CONFIG REQUIRED)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		JsonCpp::JsonCpp
)

install(
	FILES
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_CZECH.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_ENGLISH.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_FRENCH.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_GERMAN.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_ITALIAN.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_JAPANESE.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_POLISH.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_RUSSIAN.txt"
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_SPANISH.txt"
	DESTINATION
		"Interface/Translations"
)

install(
	FILES
		"${PROJECT_SOURCE_DIR}/data/SKSE/Plugins/${PROJECT_NAME}.json"
	DESTINATION
		"SKSE/Plugins"
)

set(CPACK_GENERATOR "7Z")
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION})
if(BUILD_SKYRIMVR)
	string(APPEND CPACK_PACKAGE_FILE_NAME "-VR")
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
include(CPack)
