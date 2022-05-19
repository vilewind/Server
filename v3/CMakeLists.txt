CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
# 括号内大小写敏感
project(webserver VERSION 1.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MINIMUM_REQUIRED true)

aux_source_directory(. SOURCES)

add_executable(${PROJECT_NAME} ${SOURCES})
