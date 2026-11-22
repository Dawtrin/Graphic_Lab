# BÁO CÁO BÀI TẬP LAB 5: ĐƯỜNG VÀ MẶT CONG TRONG KHÔNG GIAN 3D
**Học phần:** Đồ họa máy tính (Computer Graphics)  
**Trường Đại học Công nghệ Thông tin và Truyền thông Việt - Hàn (VKU)**

---

## 1. MỤC TIÊU VÀ NỘI DUNG YÊU CẦU

Chương trình triển khai đầy đủ 4 nội dung trọng tâm của **LAB 5**:
1. **5.1. Các mặt có quy tắc (Ruled Surfaces):**
   - **Mặt trụ tròn (Cylinder):** Phương trình tham số với đáy tròn, nắp trên và hệ đường sinh thẳng song song.
   - **Mặt nón tròn (Cone):** Phương trình tham số nối từ đỉnh nón xuống vành tròn đáy, thể hiện các đường sinh.
   - **Mặt yên ngựa (Hyperbolic Paraboloid):** Mặt kẻ đôi kinh điển với 2 họ đường thẳng sinh cắt nhau trên mặt cong.
   - **Mặt Conoid:** Mặt cong sinh bởi đường thẳng chuyển động song song mặt phẳng đáy và lượn sóng.

2. **5.2. Các mặt tròn xoay (Surfaces of Revolution):**
   - **Mặt cầu (Sphere):** Lưới kinh vĩ tuyến chuẩn xác trong không gian 3D.
   - **Mặt Ellipsoid:** 3 bán trục $a, b, c$ tùy chỉnh độc lập.
   - **Mặt Hyperboloid 1 tầng (One-sheet Hyperboloid):** Dạng eo thắt sinh bởi đường cong hyperbolic tròn xoay (đồng thời cũng là mặt kẻ đôi).
   - **Mặt xuyến (Toroid / Torus):** Đường tròn bán kính $r$ quay quanh trục tâm cách khoảng $R$.
   - **Mặt Paraboloid tròn xoay:** Bát hứng sóng tạo bởi đường parabol quay quanh trục đối xứng.

3. **5.3. Đường cong Bezier (Bezier Curves):**
   - Đa thức cơ sở Bernstein (Bernstein Polynomials) tổng quát.
   - Thuật toán nội suy đệ quy De Casteljau.
   - Trực quan hóa đa giác kiểm soát (Control Polygon) và các đỉnh kiểm soát $P_i$.
   - Hiển thị từng bước phân đoạn De Casteljau và điểm chạy động $B(t)$ khi $t \in [0, 1]$.
   - Chọn và di chuyển tọa độ các điểm kiểm soát trong không gian 3D.

4. **5.4. Mặt cong Bezier (Bezier Surfaces):**
   - Mặt cong Bicubic Bezier tạo từ lưới $4 \times 4$ gồm 16 điểm kiểm soát $P_{i,j}$.
   - Tính toán chính xác vector tiếp tuyến và vector pháp tuyến $\vec{N}(u, v) = \frac{\partial S}{\partial u} \times \frac{\partial S}{\partial v}$ để áp dụng mô hình chiếu sáng Gouraud bóng mượt.
   - Trực quan hóa Lưới kiểm soát 3D (Control Net Grid).
   - Bảng ma trận $4 \times 4$ trong giao diện cho phép chọn điểm bất kỳ và thay đổi độ cao $Y$ hoặc vị trí $X, Z$ để uốn nắn mặt cong trong thời gian thực.
   - 4 Presets mặt cong mẫu: Sóng uốn lượn (Wave), Yên ngựa (Saddle), Chóp nón mềm (Dome), Máng trũng đôi (Valley).

---

## 2. HỆ THỐNG ĐIỀU KHIỂN & TƯƠNG TÁC

### Tương tác Chuột:
- **Kéo chuột trái:** Xoay tự do góc quan sát xung quanh vật thể (Góc phương vị $\theta$ và Góc ngẩng $\phi$).
- **Kéo chuột phải:** Phóng to / Thu nhỏ khoảng cách nhìn (Zoom).
- **Cuộn con lăn chuột:** Phóng to / Thu nhỏ nhanh chóng (Scroll Zoom In/Out).
- **Click & Kéo Sliders / Buttons:** Thao tác trên thanh điều khiển bên phải.

### Phím tắt Bàn phím:
- `Phím Space`: Bật/Tắt tự động xoay quanh vật thể (Auto Orbit) hoặc Bật/Tắt animation tham số $t$ trong đường cong Bezier.
- `Phím Tab`: Chuyển đổi nhanh giữa các bài `5.1` $\to$ `5.2` $\to$ `5.3` $\to$ `5.4`.
- `Phím 1, 2, 3, 4, 5`: Chọn nhanh đối tượng trong chuyên mục hiện tại.
- `Phím M`: Chuyển đổi chế độ hiển thị giữa `Solid Shaded` $\to$ `Wireframe` $\to$ `Solid + Wire`.
- `Phím C`: Đổi sang preset màu tiếp theo trong bảng 8 màu sắc.
- `Phím + / =` và `- / _`: Phóng to / Thu nhỏ tỉ lệ vật thể (Scaling).
- `Phím [ / ]`: Tăng / Giảm khoảng cách Camera (Camera Distance).
- `Phím mũi tên (Left, Right, Up, Down)` hoặc `A, D, W, S`: Xoay góc nhìn theo từng nấc 5 độ.
- `Phím R`: Reset mọi thông số và camera về mặc định ban đầu.
- `Phím ESC`: Thoát ứng dụng.

---

## 3. CÁCH BIÊN DỊCH VÀ KHỞI CHẠY

### Cách 1: Chạy file Batch tự động (Khuyên dùng)
- Click đúp vào `F:\GraphicsLab\Lab\run_lab5.bat` hoặc chạy từ dòng lệnh:
  ```cmd
  cd /d "F:\GraphicsLab\Lab"
  run_lab5.bat
  ```

### Cách 2: Biên dịch thủ công bằng CMake
```cmd
cd /d "F:\GraphicsLab\Lab"
cmake -B build
cmake --build build --target Lab_5_main
build\Lab_5_main.exe
```

### Cách 3: Chạy bộ kiểm thử tự động (Math Self-Tests)
```cmd
build\Lab_5_main.exe --test
```
Bộ kiểm thử sẽ kiểm tra 33 trường hợp toán học cho cả 4 phần (phương trình chính tắc, bán kính, điểm mút, tính chất Bernstein, De Casteljau, 4 góc Bicubic Bezier, tính chuẩn hóa pháp tuyến) và in kết quả `PASS` 100%.
