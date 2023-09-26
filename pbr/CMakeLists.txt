project(pbr)

cmake_minimum_required(VERSION 3.1)

# Policies
if("${CMAKE_VERSION}" VERSION_GREATER 3.1)
	cmake_policy(SET CMP0054 NEW)
	cmake_policy(SET CMP0025 NEW)
endif()

# cmake modules directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../binocle-c/cmake")

# Enables CMake database for YCM
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Enables CMake debugging with VSCode
include(CMakeToolsHelpers OPTIONAL)

# Skip the platform compiler checks for cross compiling
set (CMAKE_CXX_COMPILER_WORKS TRUE)
set (CMAKE_C_COMPILER_WORKS TRUE)

# Enables CMake debugging with VSCode
include(CMakeToolsHelpers OPTIONAL)

# Add our own DEBUG define for Debug build types
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

include(BinocleUtils)

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "1")
SET(VERSION_PATCH "0")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(BINOCLE_LINK_LIBRARIES binocle-static)

if (MSVC)
	message("Including Windows defines")
	include(BinocleWindows)
elseif(APPLE)
	if(IOS)
		message("Including iOS defines")
		include(BinocleIOS)
	else()
		message("Including Mac defines")
		include(BinocleMac)
	endif()
elseif(ANDROID)
	message("Including Android defines")
	include(BinocleAndroid)
elseif(EMSCRIPTEN)
	message("Including Emscripten defines")
	include(BinocleWeb)
endif()

if (NOT BINOCLE_DATA_DIR)
    message("Setting BINOCLE_DATA_DIR to the default value")
    set(BINOCLE_DATA_DIR "${CMAKE_SOURCE_DIR}/assets/")
endif ()

message("Project source dir: ${PROJECT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE HEADERS ${PROJECT_SOURCE_DIR}/src/*.h)

message("Linking with the following libraries: ${BINOCLE_LINK_LIBRARIES}")

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps/sdl/include
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps/sdl_mixer
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps/stb_image
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps/physfs
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps/sokol
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps/luajit/src
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/deps/kazmath
    ${PROJECT_SOURCE_DIR}/../binocle-c/src/binocle/core
)

if (APPLE)
    if (IOS)
        include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/sdl/include/configs/default)
    else ()
        include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/sdl/include/configs/mac)
        include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/glew/include)
    endif ()
endif ()

if (ANDROID)
    include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/sdl/include/configs/default)
endif ()

if (WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/sdl/include/configs/windows)
    include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/glew/include)
endif ()

if (EMSCRIPTEN)
    include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/sdl/include/configs/default)
endif ()

file(GLOB SOURCE_FILES
${CMAKE_SOURCE_DIR}/src/*.c
${CMAKE_SOURCE_DIR}/src/*.h
        )

message("List of source files: ${SOURCE_FILES}")
message("List of include directories follows")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

if (APPLE)
    #if(IOS)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    file(GLOB_RECURSE RESOURCES "${CMAKE_SOURCE_DIR}/assets/*")
    foreach(RES_FILE ${RESOURCES})
        message("RES_FILE: ${RES_FILE}")
        file(RELATIVE_PATH RES_PATH "${CMAKE_SOURCE_DIR}/assets" ${RES_FILE})
        message("RES_PATH: ${RES_PATH}")
        get_filename_component(RES_FOLDER ${RES_PATH} DIRECTORY)
        message("RES_FOLDER: ${RES_FOLDER}")
        set_property(SOURCE ${RES_FILE} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${RES_FOLDER}")
    endforeach(RES_FILE)
    #set_property(SOURCE ${RESOURCES} PROPERTY MACOSX_PACKAGE_LOCATION "Resources")
    message("Resources list: ${RESOURCES}")
    #endif()
endif ()


if (ANDROID)
    include_directories(${CMAKE_SOURCE_DIR}/binocle-c/src/deps/sdl/include)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/binocle-c/src/deps/sdl/src/main/android/SDL_android_main.c)
else ()
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${EXE_TYPE} ${SOURCE_FILES} ${BINOCLE_ICON} ${RESOURCES})
endif ()

target_link_libraries(${PROJECT_NAME} ${BINOCLE_LINK_LIBRARIES})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "${PROJECT_NAME}")
endif ()

# Build the shared library next to sdl-endless-runner
set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/src
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/src
        XCODE_ATTRIBUTE_PRODUCT_NAME "${PROJECT_NAME}"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        )

if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)

    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_RPATH 1
            BUILD_WITH_INSTALL_RPATH 1
            INSTALL_RPATH "@loader_path/../Frameworks"
            MACOSX_BUNDLE_GUI_IDENTIFIER "it.altralogica.${PROJECT_NAME}"
            )
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    #set_target_properties(${PROJECT_NAME} PROPERTIES RESOURCE "${RESOURCES}")
    # file(GLOB_RECURSE RESOURCES
    # 			${CMAKE_SOURCE_DIR}/data/*
    # )
    # set_source_files_properties(
    # 			${RESOURCES}
    # 			PROPERTIES
    # 			MACOSX_PACKAGE_LOCATION Resources
    # )
    set_source_files_properties(
            ${RESOURCES}
            PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
    if (IOS)
        set_target_properties(
                ${PROJECT_NAME}
                PROPERTIES
                MACOSX_BUNDLE YES
                MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/../binocle-c/build/ios/Info.plist"
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
                XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
                XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 8.0
                RESOURCE "${RESOURCES}"
        )

        # Copy resources
        #add_custom_command(
        #    TARGET ${NAME}
        #    POST_BUILD
        #    COMMAND /Developer/Library/PrivateFrameworks/DevToolsCore.framework/Resources/pbxcp -exclude .DS_Store -exclude CVS -exclude .svn -resolve-src-symlinks ${CMAKE_SOURCE_DIR}/data ${APP_NAME}
        #)
    endif ()
endif ()


if (MSVC)
    set(EXE_TYPE WIN32)
    #    set (SOURCE_FILES ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/build/cmake/Binocle.rc)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    file(GLOB_RECURSE RESOURCES "${BINOCLE_DATA_DIR}*")
    foreach(RES_FILE ${RESOURCES})
        message("RES_FILE: ${RES_FILE}")
        file(RELATIVE_PATH RES_PATH "${BINOCLE_DATA_DIR}" ${RES_FILE})
        message("RES_PATH: ${RES_PATH}")
        get_filename_component(RES_FOLDER ${RES_PATH} DIRECTORY)
        message("RES_FOLDER: ${RES_FOLDER}")
        add_custom_command(TARGET ${PROJECT_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${RES_FILE} ${CMAKE_CURRENT_BINARY_DIR}/src/assets/${RES_PATH}
        )
    endforeach(RES_FILE)
elseif (APPLE)
    #ICNS
    set(MACOSX_BUNDLE_ICON_FILE ${PROJECT_NAME}.icns)
    if (NOT IOS)
        set(BINOCLE_ICON ${CMAKE_SOURCE_DIR}/build/cmake/Binocle.icns)
    endif ()
    set_source_files_properties(${BINOCLE_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(EXE_TYPE MACOSX_BUNDLE)
endif ()

if (ANDROID)
    message("Post build will copy to ${CMAKE_CURRENT_BINARY_DIR}/../../../android-project/libs/${ANDROID_ABI}/libmain.so")
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/../../../android-project/libs/${ANDROID_ABI}/libmain.so")
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/../../../android-project/libs/${ANDROID_ABI}/libmain.so")
    #	file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/../android-project/libs/${ANDROID_ABI}/lib${PROJECT_NAME}.so ${CMAKE_CURRENT_BINARY_DIR}/../android-project/libs/${ANDROID_ABI}/libmain.so)
endif ()

add_subdirectory(../binocle-c/src ${CMAKE_BINARY_DIR}/binocle-c/src)
