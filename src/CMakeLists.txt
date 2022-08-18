cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 20)
set(PROJECT_NAME static)
Project(${PROJECT_NAME})
list(APPEND SOURCES main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

install(TARGETS static DESTINATION unamazing)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION unamazing)
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME unamazing)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)