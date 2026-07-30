cmake_minimum_required(VERSION 3.10)

set(PACE_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(PACE_IS_TOP_LEVEL ON)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/parse_version.cmake)
parse_version(PACE_VERSION PACE_STAGE)

set(PACE_PROJECT
    pace
    VERSION ${PACE_VERSION}
    LANGUAGES CXX
    DESCRIPTION "A powerful and high-performance terminal progress bar for Modern C++.")
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.12")
  list(APPEND PACE_PROJECT HOMEPAGE_URL "https://github.com/Konvt/pace")
endif()

project(${PACE_PROJECT})

option(PACE_INSTALL "Enable install target" ${PACE_IS_TOP_LEVEL})
option(PACE_PACKAGE "Enable packaging support" ${PACE_IS_TOP_LEVEL})
option(PACE_BUILD_DEMO "Build demo programs from demo/ directory using Makefile" OFF)

include(CMakePackageConfigHelpers)

add_library(pace INTERFACE)
add_library(pace::pace ALIAS pace)

target_compile_features(pace INTERFACE cxx_std_11)

target_include_directories(
  pace INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(PACE_INSTALL)
  include(GNUInstallDirs)
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pace-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/pace-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/pace-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pace.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/pace.pc
    @ONLY)

  install(TARGETS pace EXPORT pace-targets)
  install(EXPORT pace-targets
          FILE pace-targets.cmake
          NAMESPACE pace::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pace.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  install(FILES README.md DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/docs/${PROJECT_NAME})
  install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pace-config.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/pace-config-version.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
  install(DIRECTORY include/
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(DIRECTORY docs/
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/docs/${PROJECT_NAME})
endif()

if(PACE_PACKAGE)
  if(NOT DEFINED PACE_PKG_NAME OR PACE_PKG_NAME STREQUAL "")
    set(PACE_PKG_NAME "${PROJECT_NAME}-${PACE_VERSION}-${PACE_STAGE}")
  endif()

  set(PACE_PKG_ROOT "${CMAKE_CURRENT_BINARY_DIR}/_pkg_root")
  set(PACE_PKG_DIR "${PACE_PKG_ROOT}/${PROJECT_NAME}")
  set(PACE_PKG_FMT "zip")
  set(PACE_PKG_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PACE_PKG_NAME}.${PACE_PKG_FMT}")

  add_custom_target(artifact COMMAND ${CMAKE_COMMAND} -E echo ${PACE_PKG_FILE})

  set(PACE_INS_STAMP "installed")
  add_custom_command(
    OUTPUT "${PACE_PKG_ROOT}/${PACE_INS_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACE_PKG_DIR}
    COMMAND ${CMAKE_COMMAND} -E env
            DESTDIR=${PACE_PKG_ROOT}
            ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR} --prefix "/${PROJECT_NAME}"
    COMMAND ${CMAKE_COMMAND} -E touch "${PACE_PKG_ROOT}/${PACE_INS_STAMP}"
    COMMENT "Installing ${PROJECT_NAME} into staging directory")

  # Adjust the directory tree structure to conform to the release package structure
  # instead of the GNU installation structure
  set(PACE_MOVEUP "${CMAKE_SOURCE_DIR}/cmake/moveup.cmake")
  set(PACE_ADJT_STAMP "adjusted")
  add_custom_command(
    OUTPUT "${PACE_PKG_ROOT}/${PACE_ADJT_STAMP}"
    DEPENDS "${PACE_PKG_ROOT}/${PACE_INS_STAMP}"

    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/module"
            "${PACE_PKG_DIR}/module"

    COMMAND ${CMAKE_COMMAND} -DDIR="${PACE_PKG_ROOT}/${PROJECT_NAME}/${CMAKE_INSTALL_DATAROOTDIR}/docs/${PROJECT_NAME}" -P ${PACE_MOVEUP}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${PACE_PKG_ROOT}/${PROJECT_NAME}/${CMAKE_INSTALL_DATAROOTDIR}/docs/${PROJECT_NAME}"

    COMMAND ${CMAKE_COMMAND} -DDIR="${PACE_PKG_ROOT}/${PROJECT_NAME}/${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}" -P ${PACE_MOVEUP}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${PACE_PKG_ROOT}/${PROJECT_NAME}/${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"

    COMMAND ${CMAKE_COMMAND} -DDIR="${PACE_PKG_ROOT}/${PROJECT_NAME}/${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" -P ${PACE_MOVEUP}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${PACE_PKG_ROOT}/${PROJECT_NAME}/${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"

    COMMAND ${CMAKE_COMMAND} -E touch "${PACE_PKG_ROOT}/${PACE_ADJT_STAMP}"
    COMMENT "Adjusting directory tree")

  add_custom_command(
    OUTPUT ${PACE_PKG_FILE}
    DEPENDS "${PACE_PKG_ROOT}/${PACE_ADJT_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E tar
            cf ${PACE_PKG_FILE} --format=${PACE_PKG_FMT}
            -- ${PROJECT_NAME}
    WORKING_DIRECTORY ${PACE_PKG_ROOT}
    COMMENT "Packaging ${PROJECT_NAME}")
  add_custom_target(pack DEPENDS ${PACE_PKG_FILE})
endif()

if(PACE_BUILD_DEMO)
  file(GLOB DEMO_SOURCES CONFIGURE_DEPENDS "demo/*.cpp")
  foreach(source IN LISTS DEMO_SOURCES)
    get_filename_component(demo_name ${source} NAME_WE)
    add_executable(${demo_name} ${source})
    target_link_libraries(${demo_name} PRIVATE pace)

    target_compile_options(${demo_name} PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/Za;/permissive->
      $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-pedantic-errors>)

    set_target_properties(${demo_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    list(APPEND DEMO_TARGETS ${demo_name})
  endforeach()
  add_custom_target(demos DEPENDS ${DEMO_TARGETS})
endif()

if(NOT TARGET uninstall)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    COMMENT "Uninstalling files from install_manifest.txt")
endif()
