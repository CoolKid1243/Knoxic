include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.11.0)

set(NAME KnoxicEngine)

message(STATUS "using ${CMAKE_GENERATOR}")

if (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
    if (NOT MINGW_PATH)
        message(FATAL_ERROR "MINGW_PATH not set in .env.cmake")
    endif()
    set(USE_MINGW "True")
    set(CMAKE_C_COMPILER ${MINGW_PATH}/bin/gcc.exe)
    set(CMAKE_CXX_COMPILER ${MINGW_PATH}/bin/g++.exe)
endif()

project(${NAME} VERSION 0.23.0)

if (DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
    set(Vulkan_FOUND "True")
else()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

if (DEFINED GLFW_PATH)
    message(STATUS "Using GLFW path specified in .env")
    set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
    if (MSVC)
        set(GLFW_LIB "${GLFW_PATH}/lib-vc2019")
    elseif (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
        message(STATUS "USING MINGW")
        set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64")
    endif()
else()
    find_package(glfw3 3.3 REQUIRED)
    set(GLFW_LIB glfw)
    message(STATUS "Found GLFW")
endif()

if (NOT GLFW_LIB)
    message(FATAL_ERROR "Could not find glfw library!")
else()
    message(STATUS "Using glfw lib at: ${GLFW_LIB}")
endif()

if (DEFINED ASSIMP_PATH)
    message(STATUS "Using ASSIMP path specified in .env")
    set(ASSIMP_INCLUDE_DIRS "${ASSIMP_PATH}/include")
    if (MSVC)
        set(ASSIMP_LIBRARIES "${ASSIMP_PATH}/lib/assimp-vc142-mt.lib")
    elseif (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
        set(ASSIMP_LIBRARIES "${ASSIMP_PATH}/lib/libassimp.dll.a")
    endif()
    set(ASSIMP_FOUND TRUE)
else()
    find_package(assimp REQUIRED)
    if (assimp_FOUND)
        set(ASSIMP_LIBRARIES assimp::assimp)
        set(ASSIMP_FOUND TRUE)
        message(STATUS "Found Assimp using find_package")
    endif()
endif()

if (NOT ASSIMP_FOUND)
    message(FATAL_ERROR "Could not find Assimp library!")
else()
    message(STATUS "Using Assimp lib at: ${ASSIMP_LIBRARIES}")
endif()

if (NOT STBIMAGE_PATH)
    message(STATUS "STBIMAGE_PATH not specified in .env.cmake, using external/stbimage")
    set(STBIMAGE_PATH external/stbimage)
endif()

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/external/imgui")
set(IMGUIZMO_DIR "${PROJECT_SOURCE_DIR}/external/ImGuizmo")

file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

file(GLOB IMGUIZMO_SOURCES
    ${IMGUIZMO_DIR}/ImGuizmoWrapper.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUIZMO_SOURCES})

target_include_directories(imgui PUBLIC
    ${PROJECT_SOURCE_DIR}/external
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIZMO_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(imgui PUBLIC glfw ${Vulkan_LIBRARIES})

if (WIN32)
    message(STATUS "CREATING BUILD FOR WINDOWS")

    if (USE_MINGW)
        target_include_directories(${PROJECT_NAME} PUBLIC
            ${MINGW_PATH}/include
        )
        target_link_directories(${PROJECT_NAME} PUBLIC
            ${MINGW_PATH}/lib
        )
    endif()

    target_include_directories(${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${STBIMAGE_PATH}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_PATH}
        ${ASSIMP_INCLUDE_DIRS}
        ${IMGUI_DIR}
        ${IMGUIZMO_DIR}
    )

    target_link_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_LIBRARIES}
        ${GLFW_LIB}
    )

    target_link_libraries(${PROJECT_NAME}
        glfw3
        vulkan-1
        ${ASSIMP_LIBRARIES}
        imgui
    )

elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")

    target_include_directories(${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${STBIMAGE_PATH}
        ${IMGUI_DIR}
        ${IMGUIZMO_DIR}
    )

    target_link_libraries(${PROJECT_NAME}
        glfw
        ${Vulkan_LIBRARIES}
        ${ASSIMP_LIBRARIES}
        imgui
    )
endif()

find_program(GLSL_VALIDATOR glslangValidator HINTS 
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
    /usr/bin 
    /usr/local/bin 
    ${VULKAN_SDK_PATH}/Bin 
    ${VULKAN_SDK_PATH}/Bin32 
    $ENV{VULKAN_SDK}/Bin/ 
    $ENV{VULKAN_SDK}/Bin32/
)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/vulkan/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/vulkan/*.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(
    shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)