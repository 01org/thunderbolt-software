project(tbtadm VERSION 0.1 LANGUAGES CXX)

add_executable(${PROJECT_NAME} "main.cpp" "controller.cpp")

if(use-std-filesystem)
target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
else(use-std-filesystem)
find_package(Boost REQUIRED COMPONENTS filesystem)
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
endif(use-std-filesystem)

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${TBT_CXXFLAGS}>)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

install(TARGETS            ${PROJECT_NAME}
        DESTINATION        ${CMAKE_INSTALL_BINDIR})
install(FILES              ${PROJECT_NAME}.bash_completion
        DESTINATION        ${CMAKE_INSTALL_DATAROOTDIR}/bash-completion/completions
        RENAME             ${PROJECT_NAME})
