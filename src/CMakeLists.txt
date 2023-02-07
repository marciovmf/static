cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 20)
set(PROJECT_NAME static)
Project(${PROJECT_NAME})
list(APPEND SOURCES 
  main.cpp
  markdown.cpp
  markdown.h
  parser_utils.cpp
  parser_utils.h)

add_executable(${PROJECT_NAME} ${SOURCES})

set(VERSION_MAJOR 0)
set(VERSION_PATCH 0)
set(VERSION_MINOR 1)
set(VERSION_STRING ${VERSION_MAJOR}_${VERSION_PATCH}_${VERSION_MINOR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

install(TARGETS static DESTINATION static)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../README.md" DESTINATION static)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../demo/" DESTINATION static/demo)
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME static_${VERSION_STRING})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)

