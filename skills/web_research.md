KEYWORDS: search, web, tìm, tra cứu, google, tin tức, thông tin, thời sự, giá cả, cập nhật

# SKILL: Information Retrieval & Fact-Checking Protocol

## 1. QUY TRÌNH THỰC HIỆN (WORKFLOW)
Khi nhận nhiệm vụ tìm kiếm thông tin trực tuyến:
1. **Phân tích & Trích xuất Từ khóa (Keyword Extraction)**:
   - Rút gọn câu hỏi của người dùng thành câu truy vấn (query) ngắn gọn từ 2-4 từ khóa quan trọng.
   - Bỏ các từ nối không cần thiết.
2. **Thực thi Tìm kiếm (Execute Search)**:
   - Gọi công cụ `web_search` với từ khóa đã trích xuất.
3. **Tổng hợp & Kiểm chứng (Synthesis)**:
   - Trích xuất các ý chính từ kết quả trả về.
   - Nếu thông tin mâu thuẫn hoặc chưa rõ ràng, thực hiện lượt tìm kiếm thứ 2 với từ khóa mở rộng hơn hoặc bằng tiếng Anh.

## 2. KỊCH BẢN XỬ LÝ (PATTERNS)
- **Tìm tin tức thời gian thực / Sự kiện mới**: Luôn dùng `web_search` thay vì tự suy luận bằng kiến thức cũ của mô hình.
- **So sánh / Bảng biểu**: Khi được yêu cầu so sánh (ví dụ: giá cả, thông số kỹ thuật), tổng hợp kết quả dưới dạng bảng Markdown.

## 3. RÀNG BUỘC (NEGATIVE CONSTRAINTS)
- KHÔNG tự bịa ra thông tin, số liệu hoặc đường liên kết (URL) nếu tool `web_search` không cung cấp.
- KHÔNG gọi tool `web_search` liên tục quá 3 lần cho cùng một câu hỏi.