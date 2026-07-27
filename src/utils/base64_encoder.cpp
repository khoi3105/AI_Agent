#include "base64_encoder.h"
#include <fstream>

std::string Base64Encoder::encode(const std::vector<unsigned char>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = data.size();
    size_t pos = 0;

    while (in_len--) {
        char_array_3[i++] = data[pos++];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++) {
            ret += base64_chars[char_array_4[j]];
        }

        while (i++ < 3) {
            ret += '=';
        }
    }

    return ret;
}

std::expected<std::string, std::string> Base64Encoder::encodeFile(const std::string& filepath) {
    // 1. Mở file ở chế độ Binary
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        return std::unexpected("Lỗi: Không thể mở file ảnh tại đường dẫn: " + filepath);
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        return std::unexpected("Lỗi: File ảnh rỗng hoặc không có dữ liệu!");
    }

    file.seekg(0, std::ios::beg);

    // 2. Đọc toàn bộ nội dung file vào buffer
    std::vector<unsigned char> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return std::unexpected("Lỗi: Không thể đọc dữ liệu binary từ file ảnh!");
    }

    file.close();

    // 3. Trả về kết quả thành công
    return encode(buffer);
}