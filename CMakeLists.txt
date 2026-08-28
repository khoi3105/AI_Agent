cmake_minimum_required(VERSION 3.25)

# Tên dự án
project(MyAgentProject CXX)

# Bật tiêu chuẩn C++26 và chỉ định compiler g++-16 nếu chưa set môi trường
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tìm PkgConfig (dùng để tìm poppler-cpp, gumbo, glfw3)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)
pkg_check_modules(GUMBO REQUIRED gumbo)
pkg_check_modules(GLFW QUIET glfw3)

# Tìm thư viện libcurl, SQLite3, OpenGL trên hệ thống
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenGL REQUIRED)

# Gom tất cả các file .cpp từ các thư mục con lại (kèm Server, GUI & ImGui)
file(GLOB_RECURSE SOURCES
    "src/client/*.cpp"
    "src/main.cpp"
    "src/utils/*.cpp"
    "src/tools/*.cpp"
    "src/agent/*.cpp"
    "src/harness/*.cpp"
    "src/environment/*.cpp"
    "src/gui/*.cpp"
    "src/gui/imgui/*.cpp"
    "src/server/*.cpp"
)

# Tạo file thực thi build/main từ danh sách SOURCES
add_executable(main ${SOURCES})

# Thêm thư mục include cho các header file
target_include_directories(main PRIVATE 
    src
    src/gui/imgui
    src/server
    ${POPPLER_INCLUDE_DIRS}
    ${GUMBO_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

# Liên kết với các thư viện
target_link_libraries(main PRIVATE 
    CURL::libcurl
    SQLite::SQLite3
    OpenGL::GL
    ${POPPLER_LIBRARIES}
    ${GUMBO_LIBRARIES}
    glfw
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
