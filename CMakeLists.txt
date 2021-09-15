cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

include("${CMAKE_CURRENT_SOURCE_DIR}/extern/core/manifest.cmake")

string(TOLOWER ${cross_target} project_name)
project(${project_name} VERSION ${cross_version})
set(CPACK_PACKAGE_VENDOR "${cross_identifier}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_BINARY_DIR}/icon/app.ico")

set(CPACK_GENERATOR "WIX")
set(CPACK_WIX_UPGRADE_GUID "${cross_guid}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_VENDOR}\\\\${CMAKE_PROJECT_NAME}")

include(CPack)

add_compile_definitions(PROJECT_NAME="${CMAKE_PROJECT_NAME}")
add_compile_definitions(PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
add_compile_definitions(APPLICATION_ID="${CPACK_PACKAGE_VENDOR}")

add_executable("${CMAKE_PROJECT_NAME}")
target_include_directories("${CMAKE_PROJECT_NAME}" PRIVATE "extern/core/src")
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
file(GLOB sources "./*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${sources})
file(GLOB core_sources "extern/core/src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${core_sources})
file(GLOB main_sources "../../src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${main_sources})
target_link_options("${CMAKE_PROJECT_NAME}" BEFORE PRIVATE
    "-Wl,-subsystem,windows")
target_link_options("${CMAKE_PROJECT_NAME}" BEFORE PRIVATE
    -static-libstdc++ -static-libgcc)
target_link_libraries("${CMAKE_PROJECT_NAME}"
    mingw32 win_resource soundio pthread stdc++fs)
install(TARGETS "${CMAKE_PROJECT_NAME}" DESTINATION "bin")
install(DIRECTORY "../../assets" DESTINATION "share/${CMAKE_PROJECT_NAME}")

add_subdirectory("icon")
