cmake_minimum_required(VERSION 3.10)
project(HeadSoccer3D CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Dependencias externas ----
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(assimp REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

# ---- Fontes do jogo ----
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(headsoccer ${SOURCES})

target_include_directories(headsoccer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party   # stb_image.h
)

target_link_libraries(headsoccer
    OpenGL::GL
    OpenGL::GLU
    GLUT::GLUT
    assimp::assimp
    SDL2::SDL2
    SDL2_mixer::SDL2_mixer
)

# Copia a pasta assets/ para o diretorio de build, ao lado do executavel,
# para que os caminhos relativos em Config.h (ex: "assets/models/...")
# funcionem ao rodar direto da pasta de build.
add_custom_command(TARGET headsoccer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:headsoccer>/assets
)
