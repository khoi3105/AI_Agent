KEYWORDS: error, failed, exception, timeout, invalid, 404, 500, retry, fallback, error_recovery, lỗi, thất bại

# SKILL: Tool Execution Error Recovery & Fallback Protocol

## 1. MỤC TIÊU (OBJECTIVE)
Kỹ năng này hướng dẫn Agent xử lý linh hoạt khi các công cụ (Tools) trả về thông báo lỗi, không thể thực thi hoặc cho ra kết quả rỗng. Agent KHÔNG ĐƯỢC bỏ cuộc ngay lập tức hoặc trả lỗi trực tiếp cho người dùng nếu chưa thử các chiến lược khôi phục bên dưới.

---

## 2. QUY TRÌNH XỬ LÝ LỖI TỔNG QUÁT (GENERAL RECOVERY LOOP)
Khi nhận phản hồi có dấu hiệu LỖI từ bất kỳ Tool nào:
1. **Phân tích nguyên nhân (Analyze)**: Xác định loại lỗi thuộc nhóm nào: *Lỗi tham số đầu vào (Input Error)*, *Lỗi hệ thống/mạng (System/Network Error)*, hay *Lỗi không tìm thấy dữ liệu (Data Not Found)*.
2. **Điều chỉnh tham số (Refine)**: Sửa lại tên hàm, kiểu dữ liệu, định dạng JSON hoặc câu lệnh theo gợi ý của thông báo lỗi.
3. **Thử lại có chiến lược (Retry Strategy)**: Gọi lại Tool với tham số mới. Tối đa 2 lần thử lại cho cùng 1 mục đích.
4. **Chuyển đổi phương án dự phòng (Fallback)**: Nếu thử lại vẫn thất bại, đổi sang gọi Tool khác có chức năng tương đương hoặc báo cho người dùng hướng giải quyết thay thế.

---

## 3. KỊCH BẢN KHÔI PHỤC THEO TỪNG CÔNG CỤ (TOOL-SPECIFIC PATTERNS)

### 🔴 A. Web Search Tool (`web_search`)
* **Trường hợp 1: Kết quả rỗng (No results found) hoặc Từ khóa quá chi tiết**
  * *Hành động*: Rút gọn câu truy vấn. Loại bỏ các từ nối, chỉ giữ lại 2–3 từ khóa chính. 
  * *Thử lại*: Nếu tìm bằng tiếng Việt không ra, hãy tự động dịch từ khóa sang tiếng Anh để tìm kiếm lại.
* **Trường hợp 2: Lỗi HTTP 429 (Rate Limit) hoặc Timeout / Network Error**
  * *Hành động*: Không gọi lại ngay lập tức. Chuyển sang tìm kiếm với từ khóa khác biệt hơn hoặc tóm tắt dựa trên ngữ cảnh đã có.

### 🔴 B. File Tools (`read_file` / `write_file` / `exec`)
* **Trường hợp 1: File Not Found (Lỗi đường dẫn)**
  * *Hành động*: Kiểm tra xem đường dẫn có bị thừa/thiếu dấu `/` hoặc sai thư mục tương đối hay không.
  * *Thử lại*: Gọi lệnh danh sách file (`ls` hoặc `list_dir`) để xác nhận vị trí chính xác của file trước khi đọc lại.
* **Trường hợp 2: Permission Denied hoặc File Busy**
  * *Hành động*: Không thử ghi lại vào cùng một file. Hãy thử tạo file mới với tên khác (ví dụ: `result_v2.txt`).

### 🔴 C. Calculator Tool (`calculator`)
* **Trường hợp 1: Syntax Error / Invalid Expression**
  * *Hành động*: Kiểm tra lại biểu thức toán học. Đảm bảo sử dụng đúng toán tử C++ (`*` cho phép nhân, `/` cho phép chia, `^` hoặc `pow` đúng cú pháp).
  * *Ví dụ sửa lỗi*: Chuyển `"15 x 17"` thành `"15 * 17"`.

---

## 4. QUY TẮC RÀNG BUỘC KHI BÁO LỖI VỀ USER (NEGATIVE CONSTRAINTS)
- **Chủ động đổi chiến lược khi gặp lỗi**: Nếu 1 Tool bị lỗi lần đầu tiên, hãy thử lại bằng phương án dự phòng (fallback) ngay ở lượt tiếp theo. Tránh gọi lại cùng một công cụ với cùng một tham số đã bị lỗi.