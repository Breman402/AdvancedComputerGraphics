cmake_minimum_required ( VERSION 3.10 )

project ( lab7-project )

# Find *all* shaders.
file(GLOB_RECURSE SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
)

# Find all textures and images
file(GLOB_RECURSE TEXTURES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.bmp"
)

# Separate filter for shaders.
source_group("Shaders" FILES ${SHADERS})

# Build and link executable.
add_executable ( ${PROJECT_NAME}
    lab7_main.cpp
    sphereMesh.cpp
    loadTexture.cpp
    createShadow.cpp
    gui.cpp
    applicationIcon.cpp
    ${SHADERS}
    ${TEXTURES}
)

target_link_libraries ( ${PROJECT_NAME} labhelper )
config_build_output()

target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native)