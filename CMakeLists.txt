cmake_minimum_required(VERSION 3.25) # Đảm bảo CMake đủ mới để hỗ trợ C++26 tốt

# Tên dự án
project(MyAgentProject CXX)

# Bật tiêu chuẩn C++26 và chỉ định compiler g++-16 nếu chưa set môi trường
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tìm PkgConfig (dùng để tìm poppler-cpp)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)

# Tìm thư viện libcurl trên hệ thống
find_package(CURL REQUIRED)

# Gom tất cả các file .cpp từ các thư mục con lại
file(GLOB_RECURSE SOURCES
    "src/client/*.cpp"
    "src/main.cpp"
    "src/utils/*.cpp"
    "src/tools/*.cpp"
    "src/agent/*.cpp"
)

# Tạo file thực thi build/main từ danh sách SOURCES
add_executable(main ${SOURCES})

# Thêm thư mục include nếu dự án có các file .h/.hpp trong src + include của poppler
target_include_directories(main PRIVATE 
    src
    ${POPPLER_INCLUDE_DIRS}
)

# Liên kết với thư viện libcurl và poppler-cpp
target_link_libraries(main PRIVATE 
    CURL::libcurl
    ${POPPLER_LIBRARIES}
)

target_precompile_headers(main PRIVATE
    <iostream>
    <vector>
    <string>
    <curl/curl.h>
    <poppler-document.h> # Thêm header poppler vào precompiled headers (nếu muốn)
    "src/utils/exprtk.hpp" # Đường dẫn tương đối tới file exprtk.hpp của bạn
)