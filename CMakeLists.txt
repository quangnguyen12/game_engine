cmake_minimum_required(VERSION 3.20)

project(ShapeRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
)
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.5.zip
)
FetchContent_MakeAvailable(glm imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

find_program(GLSL_COMPILER glslc HINTS $ENV{VULKAN_SDK}/Bin REQUIRED)

set(VERT_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert)
set(FRAG_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag)
set(VERT_SPV ${CMAKE_CURRENT_BINARY_DIR}/vert.spv)
set(FRAG_SPV ${CMAKE_CURRENT_BINARY_DIR}/frag.spv)
set(SHADOW_VERT_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow.vert)
set(SHADOW_VERT_SPV ${CMAKE_CURRENT_BINARY_DIR}/shadow_vert.spv)

add_custom_command(
    OUTPUT ${VERT_SPV} ${FRAG_SPV}
    COMMAND ${GLSL_COMPILER} ${VERT_SHADER} -o ${VERT_SPV}
    COMMAND ${GLSL_COMPILER} ${FRAG_SHADER} -o ${FRAG_SPV}
    COMMAND ${GLSL_COMPILER} ${SHADOW_VERT_SHADER} -o ${SHADOW_VERT_SPV}
    DEPENDS ${VERT_SHADER} ${FRAG_SHADER} ${SHADOW_VERT_SHADER}
    COMMENT "Compiling GLSL shaders to SPIR-V..."
)

add_custom_target(CompileShaders ALL DEPENDS ${VERT_SPV} ${FRAG_SPV} ${SHADOW_VERT_SPV})

add_executable(ShapeRenderer
    src/main.cpp
    src/VulkanApp.cpp
    src/PhysicsSystem.cpp
    src/VulkanApp.h
    src/tinyfiledialogs.c
)

add_dependencies(ShapeRenderer CompileShaders)

add_custom_command(
    TARGET ShapeRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VERT_SPV} $<TARGET_FILE_DIR:ShapeRenderer>/vert.spv
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FRAG_SPV} $<TARGET_FILE_DIR:ShapeRenderer>/frag.spv
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADOW_VERT_SPV} $<TARGET_FILE_DIR:ShapeRenderer>/shadow_vert.spv
    COMMENT "Copying compiled shaders to output directory..."
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_include_directories(ShapeRenderer
    PRIVATE
    ${LUA_INCLUDE_DIR}
    ${glm_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(ShapeRenderer
    PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    imgui
    Jolt::Jolt
    ${LUA_LIBRARIES}
    sol2::sol2
    Comdlg32
    Ole32
)