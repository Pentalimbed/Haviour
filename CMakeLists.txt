cmake_minimum_required(VERSION 3.12)
set(CXX_STANDARD 23)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")

project(Haviour
VERSION 0.7.1
LANGUAGES CXX)

find_package(spdlog CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(unofficial-nativefiledialog CONFIG REQUIRED)
find_package(eventpp CONFIG REQUIRED)
find_package(robin_hood CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

include("cmake/headerlist.cmake")
include("cmake/sourcelist.cmake")

add_executable(${PROJECT_NAME} ${headers} ${sources})

target_link_libraries(
    ${PROJECT_NAME} 
    PRIVATE
        spdlog::spdlog
        pugixml::pugixml
        unofficial::nativefiledialog::nfd
        eventpp::eventpp
        robin_hood::robin_hood
        ${OPENGL_LIBRARIES}
        glfw
        imgui::imgui
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
		cxx_std_23
)

if(MSVC)
    add_compile_definitions(NOMINMAX)
endif()

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:${PROJECT_NAME}>)