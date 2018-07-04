cmake_minimum_required(VERSION 3.5)

project(thunderbolt-user-space)

set(VERSION "0.9.3")

include(GNUInstallDirs)

include(FindPkgConfig)
pkg_get_variable(PKG_CONFIG_UDEV_DIR udev udevdir)

set(UDEV_RULES_DIR "${PKG_CONFIG_UDEV_DIR}/rules.d" CACHE PATH "Install path for udev rules")
set(UDEV_BIN_DIR   "${PKG_CONFIG_UDEV_DIR}"         CACHE PATH "Install path for udev-triggered executables")
set(RULES_PREFIX   "60"                             CACHE PATH "The numeric prefix for udev rules file")

set(TBT_CXXFLAGS ${CXX_FLAGS} -Wall -Wextra)

add_subdirectory(common)
add_subdirectory(tbtacl)
add_subdirectory(tbtxdomain)
add_subdirectory(tbtadm)
add_subdirectory(docs)

configure_file(tests/test_integration_mock.py tests/test_integration_mock.py COPYONLY)
configure_file(tests/test.py tests/test.py COPYONLY)
configure_file(tests/Dockerfile tests/Dockerfile COPYONLY)

add_custom_target(check
	COMMAND python3 tests/test.py
	DEPENDS tests/test_integration_mock.py tests/test.py tbtadm
)

set(DOCKER_IMAGE "thunderbolt-tools")

set(DOCKER_BUILD_CMD
	docker build
	--tag ${DOCKER_IMAGE}
	--file tests/Dockerfile
	.
)
add_custom_target(docker-build
	COMMAND ${DOCKER_BUILD_CMD}
	DEPENDS tests/Dockerfile
)

set(DOCKER_RUN_CMD
	docker run
	--env
	--tty
	--volume ${CMAKE_CURRENT_LIST_DIR}:/usr/local/src/thunderbolt-tools
	${DOCKER_IMAGE}
	/bin/bash tests/docker-build.sh
)
add_custom_target(docker-run
	COMMAND ${DOCKER_RUN_CMD}
	DEPENDS tests tbtadm
)

set(PACKAGE_SUMMARY "User-space components for handling Thunderbolt controller and devices")

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Description")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PACKAGE_SUMMARY})
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Thunderbolt Software mailing list <thunderbolt-software@lists.01.org>")
set(CPACK_PACKAGE_VENDOR "Intel")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

# RPM specific
set(CPACK_PACKAGE_RELOCATABLE OFF)
set(CPACK_RPM_PACKAGE_REQUIRES "boost-filesystem")

function(ALL_ANCESTOR_DIRS result dir)
    while(NOT dir STREQUAL "/")
      list(APPEND LOCAL_LIST "${dir}")
      get_filename_component(dir "${dir}/.." ABSOLUTE)
    endwhile()
    set(${result} "${LOCAL_LIST}" PARENT_SCOPE)
endfunction()

foreach(dir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
            "${UDEV_RULES_DIR}"
            "${UDEV_BIN_DIR}"
            "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR}/man1"
            "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/bash-completion/completions")
  ALL_ANCESTOR_DIRS(LIST_FOR_RPM "${dir}")
  list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${LIST_FOR_RPM}")
  list(REMOVE_DUPLICATES CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION)
endforeach()

# DEB specific
file(READ "${CPACK_PACKAGE_DESCRIPTION_FILE}" PACKAGE_DESCRIPTION)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION ${PACKAGE_DESCRIPTION})

# Main customization points
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

# For DEB
install(FILES ${CPACK_RESOURCE_FILE_LICENSE}
        DESTINATION share/doc/${CPACK_PACKAGE_NAME}
        RENAME copyright)

include(CPack)
