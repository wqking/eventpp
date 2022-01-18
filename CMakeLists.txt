cmake_minimum_required(VERSION 3.11)
project(eventpp VERSION 0.1.1)

add_library(eventpp INTERFACE)
target_compile_features(eventpp INTERFACE cxx_std_11)

target_include_directories(
  eventpp INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(eventpp::eventpp ALIAS eventpp)

# Installation
# ------------
include(GNUInstallDirs)

# Install the library
install(
  TARGETS eventpp
  EXPORT eventppTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install the headers
install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# (Generate and) install the target import file
install(
  EXPORT eventppTargets
  NAMESPACE eventpp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eventpp
)

# Generate the package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/eventppConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Generate the package configuration file, that allows other
# CMake projects to find the library with find_package()
configure_package_config_file(
  cmake/eventppConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/eventppConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eventpp
)

# Install the package version and configuration files
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/eventppConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/eventppConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eventpp
)

# Install readme and license
install(
  FILES
  readme.md
  license
  DESTINATION ${CMAKE_INSTALL_DATADIR}/eventpp
)
