KEYWORDS: file, read, write, đọc, ghi, lưu, danh sách, folder, directory, txt, json, md, log

# SKILL: File System Operations Protocol

## 1. QUY TRÌNH THỰC HIỆN (WORKFLOW)
Khi nhận các tác vụ liên quan đến tệp tin hoặc thư mục:
1. **Kiểm tra trước khi đọc (Validate Path)**:
   - Nếu không chắc chắn về tên file hoặc đường dẫn chính xác, hãy ưu tiên dùng công cụ `exec` (chạy lệnh `ls` hoặc `dir`) để kiểm tra danh sách file trước.
2. **Thực thi an toàn (Safe Execution)**:
   - Khi ghi file (`write_file`), ưu tiên kiểm tra nội dung cần ghi. Đảm bảo định dạng văn bản rõ ràng (UTF-8).
3. **Xác nhận kết quả (Verify Output)**:
   - Sau khi ghi thành công, thông báo đường dẫn file đã tạo/chỉnh sửa cho người dùng.

## 2. KỊCH BẢN XỬ LÝ (PATTERNS)
- **Tạo báo cáo/tóm tắt**: Sau khi thu thập thông tin từ web hoặc tính toán, nếu người dùng yêu cầu "lưu lại", hãy tự động định dạng dữ liệu thành cấu trúc Markdown hoặc JSON rồi mới gọi tool ghi file.
- **Xử lý tệp cấu hình/JSON**: Kiểm tra cú pháp JSON hợp lệ trước khi ghi nội dung vào đĩa.

## 3. RÀNG BUỘC (NEGATIVE CONSTRAINTS)
- KHÔNG tự ý ghi đè lên các file hệ thống quan trọng.
- KHÔNG đọc/ghi các file vượt ngoài phạm vi thư mục làm việc của dự án trừ khi người dùng chỉ định rõ ràng đường dẫn tuyệt đối.