cmake_minimum_required(VERSION 3.18)
set(PROJECT_NAME static)
Project(${PROJECT_NAME})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND SOURCES 
  common.h common.c
  filesystem.h filesystem.c
  config.h config.c
  ast.h ast.c
  interpreter.h interpreter.c
  parser.h parser.c
  main.c
  version.rc)

add_executable(${PROJECT_NAME} ${SOURCES})

set(VERSION_MAJOR 1)
set(VERSION_PATCH 0)
set(VERSION_MINOR 0)
set(VERSION_STRING ${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_PATCH})
set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION ${VERSION_MAJOR}.${VERSION_MINOR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
  configure_file(version.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/version.rc)

else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

install(TARGETS static DESTINATION static)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../README.md" DESTINATION "/")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../demo/" DESTINATION "static/demo")
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME static_${VERSION_STRING})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)

