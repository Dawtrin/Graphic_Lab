# BÁO CÁO BÀI THỰC HÀNH TẠI LỚP - LAB 2 (14/9)

**Trường:** Đại học Công nghệ Thông tin và Truyền thông Việt - Hàn (VKU)  
**Khoa:** Kỹ thuật Máy tính & Điện tử  
**Môn học:** Đồ họa máy tính (Computer Graphics)  
**Chủ đề:** Cài đặt các thuật toán vẽ đường cong cơ sở (Đường tròn, Ellipse, Parabol) bằng **Bresenham** và **Midpoint** với tương tác chuột hoàn chỉnh.

---

## 1. YÊU CẦU ĐỀ BÀI (14/9)
- Cài đặt 3 thuật toán vẽ đường cong cơ sở:
  1. **Vẽ đường tròn:** Thuật toán **Bresenham** (đối xứng 8 phần, hỗ trợ so sánh với **Midpoint**).
  2. **Vẽ Ellipse:** Thuật toán **Midpoint** (đối xứng 4 phần, 2 vùng tiếp tuyến).
  3. **Vẽ Parabol:** Thuật toán **Bresenham** (đối xứng 2 bên qua trục đứng, 2 vùng tiếp tuyến).
- **Yêu cầu tương tác chuột:** Click chuột để xác định trực quan các tham số:
  - Bán kính, tâm của đường tròn.
  - Tâm, bán trục lớn $a$, bán trục nhỏ $b$ của Ellipse.
  - Đỉnh, tiêu cự $p$, hướng mở của Parabol.
- **Yêu cầu nộp bài:** Gửi ảnh kết quả vào group Zalo gồm: **Hình tròn (Bresenham)**, **Ellipse (Midpoint)**, **Parabol (Bresenham)**.

---

## 2. CƠ SỞ TOÁN HỌC & CHI TIẾT THUẬT TOÁN

### 2.1. Thuật toán Bresenham vẽ đường tròn
- **Phương trình chính tắc:**
  $$(x - x_c)^2 + (y - y_c)^2 = R^2$$
- **Tính chất đối xứng 8 phần (8-way symmetry):**
  Chỉ cần tính toán cho $1/8$ cung tròn đầu tiên từ $(0, R)$ đến $(x \le y)$, sau đó phản xạ ra 8 điểm đối xứng:
  $$\begin{aligned}
  &(x_c \pm x, y_c \pm y) \\
  &(x_c \pm y, y_c \pm x)
  \end{aligned}$$
- **Thông số quyết định ban đầu:**
  $$d_0 = 3 - 2R$$
- **Công thức đệ quy bước lặp $k$:**
  - Nếu $d_k < 0$: Điểm kế tiếp là $(x_k + 1, y_k)$
    $$d_{k+1} = d_k + 4x_k + 6$$
  - Nếu $d_k \ge 0$: Điểm kế tiếp là $(x_k + 1, y_k - 1)$
    $$d_{k+1} = d_k + 4(x_k - y_k) + 10$$
- **Đặc trưng:** Thuần số nguyên, không dùng số thực, tốc độ tối ưu.
- *(Tùy chọn nâng cao)*: Hỗ trợ chuyển đổi sang **Midpoint Circle** ($p_0 = 1 - R$; $p < 0 \implies p_{k+1} = p_k + 2x_k + 3$; $p \ge 0 \implies p_{k+1} = p_k + 2(x_k - y_k) + 5$).

---

### 2.2. Thuật toán Midpoint vẽ Ellipse
- **Phương trình chính tắc:**
  $$\frac{(x - x_c)^2}{r_x^2} + \frac{(y - y_c)^2}{r_y^2} = 1 \iff r_y^2 (x - x_c)^2 + r_x^2 (y - y_c)^2 - r_x^2 r_y^2 = 0$$
- **Tính đối xứng 4 phần (4-way symmetry):** $(x_c \pm x, y_c \pm y)$.
- **Phân chia 2 vùng tiếp tuyến:**
  - **Vùng 1 (Độ dốc $|m| < 1 \iff 2r_y^2 x < 2r_x^2 y$):** Bước tăng theo trục $x$ ($x = x + 1$).
    - $p_{1_0} = r_y^2 - r_x^2 r_y + \frac{1}{4} r_x^2$.
    - Nếu $p_1 < 0$: $p_{1_{k+1}} = p_1 + 2r_y^2 x_{new} + r_y^2$.
    - Nếu $p_1 \ge 0$: $y = y - 1$, $p_{1_{k+1}} = p_1 + 2r_y^2 x_{new} - 2r_x^2 y_{new} + r_y^2$.
  - **Vùng 2 (Độ dốc $|m| \ge 1 \iff 2r_y^2 x \ge 2r_x^2 y$):** Bước giảm theo trục $y$ ($y = y - 1$).
    - $p_{2_0} = r_y^2 (x + 0.5)^2 + r_x^2 (y - 1)^2 - r_x^2 r_y^2$.
    - Nếu $p_2 > 0$: $p_{2_{k+1}} = p_2 - 2r_x^2 y_{new} + r_x^2$.
    - Nếu $p_2 \le 0$: $x = x + 1$, $p_{2_{k+1}} = p_2 + 2r_y^2 x_{new} - 2r_x^2 y_{new} + r_x^2$.
- **Đặc trưng:** Dùng kiểu dữ liệu `long long` 64-bit để tránh hoàn toàn hiện tượng tràn số nguyên khi bình phương bán trục lớn $r_x, r_y$.

---

### 2.3. Thuật toán Bresenham vẽ Parabol
- **Phương trình Parabol đỉnh $V(x_v, y_v)$:**
  $$(x - x_v)^2 = 4p(y - y_v) \implies y - y_v = \frac{(x - x_v)^2}{4p}$$
- **Tính đối xứng 2 phần (2-way symmetry):** $(x_v \pm x, y_v + \text{dir} \cdot y)$.
- **Hệ số góc tiếp tuyến:** $\frac{dy}{dx} = \frac{x}{2p}$.
  - **Vùng 1 ($x \le 2p$, độ dốc $\le 1$):** Bước theo trục $x$ ($x = x + 1$).
    - $d_{1_0} = 1 - 2p$.
    - Nếu $d_1 < 0$: $d_{1_{k+1}} = d_1 + 2x_k + 3$.
    - Nếu $d_1 \ge 0$: $y = y + 1$, $d_{1_{k+1}} = d_1 + 2x_k + 3 - 4p$.
    - Sau đó tăng $x = x + 1$.
  - **Vùng 2 ($x > 2p$, độ dốc $> 1$):** Bước theo trục $y$ ($y = y + 1$).
    - $d_{2_0} = (x + 0.5)^2 - 4p(y + 1)$.
    - Nếu $d_2 > 0$: $d_{2_{k+1}} = d_2 - 4p$.
    - Nếu $d_2 \le 0$: $d_{2_{k+1}} = d_2 + 2x_k + 2 - 4p$, tăng $x = x + 1$.
    - Sau đó tăng $y = y + 1$.
- **Độ chính xác:** Đã được kiểm chứng toán học đạt sai số tối ưu $\le 0.5$ pixel trên cả 2 vùng.

---

## 3. THIẾT KẾ GIAO DIỆN & TÍNH NĂNG VƯỢT TRỘI

### 3.1. Bố cục màn hình cân đối (Layout 3 Cột CAD)
Khi nhấn nút **`[* MAU ZALO *]`** hoặc bấm phím **`[D]`**:
- Cửa sổ tự động chia thành 3 phân vùng độc lập cách biệt bởi nét đứt kỹ thuật:
  - **Cột 1:** Hình tròn Bresenham (Neon Cyan, $R = 120\text{ px}$, tâm $I(200, 390)$).
  - **Cột 2:** Ellipse Midpoint (Sunset Gold, $a = 150, b = 95\text{ px}$, tâm $I(600, 390)$).
  - **Cột 3:** Parabol Bresenham (Magenta, $p = 32\text{ px}$, đỉnh $V(1000, 240)$, tiêu điểm $F(1000, 272)$).
- Bên dưới mỗi hình là **Thẻ thông số (Info Card)** phong cách kính mờ Dark Glassmorphism, hiển thị:
  - Tên thuật toán & tính chất đối xứng.
  - Phương trình toán học chính tắc.
  - Công thức đệ quy biến quyết định $d_0 / p_0$.
  - Tọa độ tâm/đỉnh, bán kính, tiêu cự và tổng số điểm pixel được tính toán.

### 3.2. Chế độ nét vẽ kép (Dual Render Modes)
- **Nét mượt (Smooth Curves):** Vẽ đường liên tục, chống răng cưa (`GL_LINE_SMOOTH`), cực kỳ sắc nét khi chụp màn hình gửi Zalo.
- **Điểm ảnh (Pixel Dots):** Vẽ từng điểm ảnh rời rạc (`GL_POINTS`) chuẩn mô hình raster đồ họa máy tính, có thể điều chỉnh độ to nhỏ của điểm bằng phím `+` / `-`.

---

## 4. HƯỚNG DẪN THAO TÁC

### 4.1. Cách chạy chương trình
- **Cách 1:** Chạy file `run_lab2.bat` (tại thư mục gốc) hoặc `Lab_2/run.bat`.
- **Cách 2:** Chạy lệnh terminal:
  ```powershell
  cmake --build build --target lab2
  .\build\lab2.exe
  ```

### 4.2. Thao tác vẽ bằng chuột
- **Chọn công cụ:** Click trực tiếp vào các nút trên thanh công cụ Top Bar (`[1. H.Tron]`, `[2. Ellipse]`, `[3. Parabol]`).
- **Click chuột trái lần 1:** Đặt vị trí Tâm $I$ hoặc Đỉnh $V$.
- **Rê chuột:** Xem trước trực quan nét vẽ với huy hiệu kích thước thực hiển thị kế bên con trỏ chuột ($R$, $a, b$, $p$).
- **Click chuột trái lần 2:** Chốt hình vào bảng vẽ.
- **Click chuột phải:** Hủy thao tác vẽ dở bất cứ lúc nào.

### 4.3. Bảng tra cứu phím tắt
| Phím tắt | Phím chức năng (Chống Unikey) | Chức năng |
| :---: | :---: | :--- |
| `1` | `F1` | Chuyển chế độ: Vẽ Đường tròn |
| `2` | `F2` | Chuyển chế độ: Vẽ Ellipse |
| `3` | `F3` | Chuyển chế độ: Vẽ Parabol |
| `D` | `F4` | **Nạp Bộ 3 Hình Mẫu Chuẩn Zalo** |
| `U` | - | Hoàn tác hình vừa vẽ (Undo) |
| `C` | `F5` | Xóa toàn bộ màn hình (Clear) |
| `S` | `F6` | Chuyển đổi Nét mượt $\leftrightarrow$ Điểm ảnh |
| `T` | `F7` | Bật / Tắt thẻ thông số và đường gióng kích thước |
| `G` | `F8` | Bật / Tắt lưới tọa độ |
| `M` | - | Đổi thuật toán Đường tròn: Bresenham $\leftrightarrow$ Midpoint |
| `+` / `-` | - | Tăng / Giảm kích thước điểm ảnh pixel |
| `ESC` | - | Thoát chương trình |
