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
set(CPACK_WIX_PRODUCT_ICON "${CPACK_PACKAGE_ICON}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_VENDOR}\\\\${CMAKE_PROJECT_NAME}")

include(CPack)

add_subdirectory("icon")

add_compile_definitions(PROJECT_NAME="${CMAKE_PROJECT_NAME}")
add_compile_definitions(PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
add_compile_definitions(APPLICATION_ID="${CPACK_PACKAGE_VENDOR}")

exec_program(nuget.exe
    ARGS install "Microsoft.Web.WebView2" -Version 1.0.961.33 -ExcludeVersion -OutputDirectory ${CMAKE_BINARY_DIR}/packages)
install(FILES "${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2/build/native/x64/WebView2Loader.dll" DESTINATION "bin")

add_library("platform" STATIC)
set_property(TARGET "platform" PROPERTY CXX_STANDARD 17)
target_include_directories("platform" PUBLIC
    "extern/core/src")
target_include_directories("platform" PRIVATE
    "${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2/build/native/include")
file(GLOB sources "./*.cpp")
target_sources("platform" PRIVATE ${sources})
file(GLOB core_sources "extern/core/src/*.cpp")
target_sources("platform" PRIVATE ${core_sources})
target_link_libraries("platform" INTERFACE
    "${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2/build/native/Microsoft.Web.WebView2.targets")

add_executable("${CMAKE_PROJECT_NAME}" WIN32)
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
add_dependencies("${CMAKE_PROJECT_NAME}" "app_icon")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE "res.rc")
file(GLOB main_sources "../../src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${main_sources})
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE "platform")
install(TARGETS "${CMAKE_PROJECT_NAME}" DESTINATION "bin")
install(DIRECTORY "../../assets" DESTINATION "share")
