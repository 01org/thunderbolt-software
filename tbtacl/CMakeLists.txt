set(TBTACL "tbtacl")
project(${TBTACL}-write VERSION 0.1 LANGUAGES CXX)
set(TBTACL_RULES "${RULES_PREFIX}-${TBTACL}.rules")

add_executable(${PROJECT_NAME} "write.cpp")

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${TBT_CXXFLAGS}>)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

configure_file("${TBTACL}.in"       ${TBTACL}       @ONLY)
configure_file("${TBTACL}.rules.in" ${TBTACL_RULES} @ONLY)

install(TARGETS              ${PROJECT_NAME}
        RUNTIME DESTINATION  ${UDEV_BIN_DIR})
install(PROGRAMS            "${CMAKE_CURRENT_BINARY_DIR}/${TBTACL}"
        DESTINATION          ${UDEV_BIN_DIR})
install(FILES               "${CMAKE_CURRENT_BINARY_DIR}/${TBTACL_RULES}"
        DESTINATION          ${UDEV_RULES_DIR})
