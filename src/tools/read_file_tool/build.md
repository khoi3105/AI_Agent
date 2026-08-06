```bash
g++ -std=c++23 \
main.cpp \
read_file_tool.cpp \
loader/loader_file.cpp \
reader/textfile_reader.cpp \
reader/pdf_reader.cpp \
registry/file_reader_registry.cpp \
../../utils/read_write_textfile.cpp \
-o app \
$(pkg-config --cflags --libs poppler-cpp)
```