# Cmake file for a sample cross-platform project with dear imgui.
# The aim here is to have most of the stuff (including cmake files)
# be the same throughout platforms.

cmake_minimum_required(VERSION 3.1)

project(TriangleBinNative)
set(CMAKE_C_STANDARD 99)

if (ANDROID)
    set(CMAKE_CXX_STANDARD 11)
else ()
    set(CMAKE_CXX_STANDARD 20)
endif ()
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if (ANDROID)
    find_package(SDL2 REQUIRED)
elseif (WIN32)
    set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/deps/SDL/VisualC/x64/Release/SDL2.lib)
    set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/deps/SDL/include)
else()
    find_package(SDL2 REQUIRED)
endif()

set(IMGUI_PATH ${PROJECT_SOURCE_DIR}/deps/imgui)
set(IMGUI_IMPL_PATH ${PROJECT_SOURCE_DIR}/src/imgui_impl)

# Use GLES2/GLES3 implementations for Android, GL3 for everything else

if (ANDROID)
    set(GL_PROFILES "GL_PROFILE_GLES2" "GL_PROFILE_GLES3")
else()
    set(GL_PROFILES "GL_PROFILE_GL3")
endif()

file(GLOB IMGUI_FILES
        ${IMGUI_PATH}/*.cpp
        ${IMGUI_IMPL_PATH}/*.cpp
)

file(GLOB DEMO_FILES
        src/*.cpp
)

if (NOT ANDROID)
    #set(GLLOAD_PATH src/glload)
    #file(GLOB GLLOAD_FILES
    #        ${GLLOAD_PATH}/*.c)
    set(GLEW_DIR ${PROJECT_SOURCE_DIR}/src/glew-2.1.0)
endif()


set(DEMO_SOURCES ${IMGUI_FILES} ${DEMO_FILES}
        #${GLLOAD_FILES}
)

# Build as a library for Android, as an executable otherwise

if (ANDROID)
    add_library(hidapi SHARED ${PROJECT_SOURCE_DIR}/deps/SDL/src/hidapi/android/hid.cpp)
    target_include_directories(hidapi PRIVATE ${PROJECT_SOURCE_DIR}/deps/SDL/src/hidapi/hidapi/hidapi.h)

    target_link_libraries(hidapi log ${SDL2_LIBRARY} android)
    target_include_directories(hidapi PRIVATE ${SDL2_INCLUDE_DIR})

    add_library(demo SHARED
            ${DEMO_SOURCES}
    )
    target_link_libraries(demo android GLESv2 GLESv3 EGL log m hidapi)
else()
    add_executable(demo
            ${DEMO_SOURCES}
    )
    if (WIN32)
        target_link_libraries(demo ${GLEW_DIR}/lib/Release/x64/glew32.lib opengl32.lib)
        target_include_directories(demo PRIVATE ${GLEW_DIR}/include)
    else()
        find_package(GLEW REQUIRED)
        set(OpenGL_GL_PREFERENCE GLVND)
        find_package(OpenGL REQUIRED)
        target_link_libraries(demo ${GLEW_LIBRARIES} ${OPENGL_LIBRARY})
        target_include_directories(demo PRIVATE ${GLEW_INCLUDE_DIR})
    endif()
endif()

target_link_libraries(demo ${SDL2_LIBRARY})
target_include_directories(demo PRIVATE ${SDL2_INCLUDE_DIR})
target_include_directories(demo PRIVATE ${IMGUI_PATH})
target_include_directories(demo PRIVATE ${IMGUI_IMPL_PATH})
target_include_directories(demo PRIVATE ${GLLOAD_PATH})
target_compile_definitions(demo PRIVATE ${GL_PROFILES})

target_include_directories(demo PRIVATE ${PROJECT_SOURCE_DIR}/deps/glm)
