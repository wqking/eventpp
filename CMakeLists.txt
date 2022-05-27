cmake_minimum_required(VERSION 3.2)
project(eventpp VERSION 0.1.2)

add_library(eventpp INTERFACE)
set(CMAKE_CXX_STANDARD 11)

target_include_directories(
	eventpp INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(eventpp::eventpp ALIAS eventpp)

# Installation
# ------------
include(GNUInstallDirs)
if (POLICY CMP0077)
	cmake_policy(SET CMP0077 NEW)
endif (POLICY CMP0077)
option(EVENTPP_INSTALL "Enable installation (Projects embedding eventpp may want to turn this OFF)." ON)

if (EVENTPP_INSTALL)
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
endif()