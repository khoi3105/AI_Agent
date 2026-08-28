cmake_minimum_required(VERSION 3.25) # Đảm bảo CMake đủ mới để hỗ trợ C++26 tốt

# Tên dự án
project(MyAgentProject CXX)

# Bật tiêu chuẩn C++26 và chỉ định compiler g++-16 nếu chưa set môi trường
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tìm PkgConfig (dùng để tìm poppler-cpp và gumbo)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)
pkg_check_modules(GUMBO REQUIRED gumbo) # Thêm Gumbo qua PkgConfig

# Tìm thư viện libcurl và SQLite3 trên hệ thống
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED) # Thêm SQLite3 (CMake hỗ trợ sẵn)

# Gom tất cả các file .cpp từ các thư mục con lại
file(GLOB_RECURSE SOURCES
    "src/client/*.cpp"
    "src/main.cpp"
    "src/utils/*.cpp"
    "src/tools/*.cpp"
    "src/agent/*.cpp"
    "src/harness/*.cpp"
    "src/environment/*.cpp"
    "src/config/*.cpp"
)

# Tạo file thực thi build/main từ danh sách SOURCES
add_executable(main ${SOURCES})

# Thêm thư mục include cho các header file
target_include_directories(main PRIVATE 
    src
    ${POPPLER_INCLUDE_DIRS}
    ${GUMBO_INCLUDE_DIRS}     # Include headers của Gumbo
    ${SQLITE3_INCLUDE_DIRS}   # Include headers của SQLite3
)

# Liên kết với các thư viện
target_link_libraries(main PRIVATE 
    CURL::libcurl
    SQLite::SQLite3          # Target SQLite3 chính thức của CMake
    ${POPPLER_LIBRARIES}
    ${GUMBO_LIBRARIES}        # Thư viện Gumbo
)

# Precompiled headers
target_precompile_headers(main PRIVATE
    <iostream>
    <vector>
    <string>
    <curl/curl.h>
    <poppler-document.h> # Thêm header poppler vào precompiled headers
    <sqlite3.h>          # Thêm header sqlite3 (tùy chọn nhưng giúp compile nhanh hơn)
    <gumbo.h>            # Thêm header gumbo (tùy chọn)
    "src/utils/exprtk.hpp" # Đường dẫn tương đối tới file exprtk.hpp của bạn
)