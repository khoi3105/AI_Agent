#ifndef BASE64_ENCODER_H
#define BASE64_ENCODER_H

#include <string>
#include <vector>
#include <expected>

class Base64Encoder {
private:
    static constexpr char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

public:
    /**
     * @brief Mã hóa chuỗi byte thô (binary data) sang chuỗi Base64
     */
    static std::string encode(const std::vector<unsigned char>& data);

    /**
     * @brief Đọc file ảnh từ đĩa và mã hóa trực tiếp sang chuỗi Base64
     * @param filepath Đường dẫn đến file ảnh (VD: "screenshot.png")
     * @return std::expected<std::string, std::string> 
     * - Thành công: Chuỗi Base64 của ảnh.
     * - Thất bại: Chuỗi thông báo lỗi (file không tồn tại, rỗng...).
     */
    static std::expected<std::string, std::string> encodeFile(const std::string& filepath);
};

#endif // BASE64_ENCODER_H