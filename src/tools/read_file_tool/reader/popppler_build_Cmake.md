find_package(PkgConfig REQUIRED)

pkg_check_modules(POPPLER REQUIRED poppler-cpp)
...
target_include_directories(app PRIVATE
    ${POPPLER_INCLUDE_DIRS}
)

target_link_libraries(app PRIVATE
    ${POPPLER_LIBRARIES}
)