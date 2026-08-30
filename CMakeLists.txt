cmake_minimum_required(VERSION 3.25)

# Tên dự án
project(MyAgentProject CXX)

# Set compiler to g++-16
set(CMAKE_CXX_COMPILER g++-16)

# Bật tiêu chuẩn C++26 và chỉ định compiler g++-16 nếu chưa set môi trường
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tìm PkgConfig (dùng để tìm poppler-cpp, gumbo)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)
pkg_check_modules(GUMBO REQUIRED gumbo)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Tìm thư viện libcurl trên hệ thống
find_package(CURL REQUIRED)

# Gom tất cả các file .cpp từ các thư mục con lại (kèm Server, Tools, Agent, Harness)
file(GLOB_RECURSE SOURCES
    "src/client/*.cpp"
    "src/main.cpp"
    "src/utils/*.cpp"
    "src/tools/*.cpp"
    "src/agent/*.cpp"
    "src/harness/*.cpp"
    "src/environment/*.cpp"
    "src/config/*.cpp"
    "src/server/*.cpp"
)

# Tạo file thực thi build/main từ danh sách SOURCES
add_executable(main ${SOURCES})

# Thêm thư mục include cho các header file
target_include_directories(main PRIVATE 
    src
    src/server
    ${POPPLER_INCLUDE_DIRS}
    ${GUMBO_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

# Liên kết với các thư viện
target_link_libraries(main PRIVATE 
    CURL::libcurl
    ${SQLITE3_LIBRARIES}
    ${POPPLER_LIBRARIES}
    ${GUMBO_LIBRARIES}
    xdo
    dl
    pthread
)

# Precompiled headers
target_precompile_headers(main PRIVATE
    <iostream>
    <vector>
    <string>
    <curl/curl.h>
    <poppler-document.h>
    <sqlite3.h>
    <gumbo.h>
    "src/utils/exprtk.hpp"
)
