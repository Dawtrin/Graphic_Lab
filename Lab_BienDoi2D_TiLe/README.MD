# LAB 3: CÁC PHÉP BIẾN ĐỔI 2D & TÌM ẢNH TAM GIÁC ABC — VKU COMPUTER GRAPHICS

> **Trường Đại học Công nghệ Thông tin và Truyền thông Việt - Hàn (VKU)**  
> **Khoa Kỹ thuật Máy tính & Điện tử** · Môn học: **Đồ họa máy tính**  
> Ứng dụng thực hành tương tác đồ họa 2D (Interactive 2D Transformation Laboratory) bằng C++17, OpenGL và FreeGLUT.

---

## 1. Yêu cầu đề bài (Lab 3)

Cài đặt 4 phép biến đổi 2D cơ bản:
1. **Tịnh tiến (Translation):** Theo vector tịnh tiến $\vec{v} = (t_x, t_y)$.
2. **Tỉ lệ (Scaling):** Với tâm tỉ lệ $T(x_0, y_0)$ và hệ số $(S_x, S_y)$.
3. **Quay (Rotation):** Với tâm quay $T(x_0, y_0)$ một góc $\theta$.
4. **Đối xứng (Reflection):** Qua đường thẳng $d: y = mx + b$.

**Mục tiêu áp dụng:** Tìm ảnh $A'B'C'$ của tam giác $ABC$ gốc.

**Yêu cầu tương tác chuột:**
- **Vector tịnh tiến:** Click 2 điểm trên màn hình (Điểm gốc $\to$ Điểm ngọn) hoặc kéo thả để xác định vector $\vec{v} = (x_2 - x_1, y_2 - y_1)$.
- **Tâm tỉ lệ:** Click trực tiếp lên viewport để đặt tâm $T(x_0, y_0)$.
- **Tâm quay:** Click lên viewport để đặt tâm quay $T(x_0, y_0)$, chỉnh góc quay trực tiếp bằng slider/nút hoặc kéo chuột.
- **Đường thẳng đối xứng:** Click 2 điểm $P_1(x_1, y_1)$ và $P_2(x_2, y_2)$ trên viewport để xác định đường thẳng $y = mx + b$ ($m = \frac{y_2 - y_1}{x_2 - x_1}$, $b = y_1 - m \cdot x_1$).

---

## 2. Quy ước toán học (Chuẩn Slide VKU)

Biểu diễn bằng **vector hàng (row vector)**, phép nhân ma trận thực hiện ở **bên phải**:
$$P = \begin{bmatrix} x & y & 1 \end{bmatrix}, \quad Q = P \times M$$

### Ma trận cơ sở:
$$T(t_x, t_y) = \begin{bmatrix} 1 & 0 & 0 \\ 0 & 1 & 0 \\ t_x & t_y & 1 \end{bmatrix}, \quad S(S_x, S_y) = \begin{bmatrix} S_x & 0 & 0 \\ 0 & S_y & 0 \\ 0 & 0 & 1 \end{bmatrix}$$

$$R(\theta) = \begin{bmatrix} \cos\theta & \sin\theta & 0 \\ -\sin\theta & \cos\theta & 0 \\ 0 & 0 & 1 \end{bmatrix}, \quad M_{Ox} = \begin{bmatrix} 1 & 0 & 0 \\ 0 & -1 & 0 \\ 0 & 0 & 1 \end{bmatrix}$$

---

## 3. Chi tiết 4 phép biến đổi & Chuỗi ma trận (Pipeline)

### 3.1. Phép Tịnh tiến (Translation)
- **Công thức trực tiếp:**
  $$x' = x + t_x, \quad y' = y + t_y$$
- **Ma trận:** $M = T(t_x, t_y)$.
- **Tương tác chuột:** Click lần 1 chọn gốc vector, click lần 2 chọn ngọn vector $\to$ vẽ mũi tên vector $\vec{v}$ trực quan trên màn hình.

### 3.2. Phép Tỉ lệ với tâm tỉ lệ $T(x_0, y_0)$ (Scaling)
- **Chuỗi biến đổi (Pipeline 4 bước):**
  1. $P_1 = P \times T(-x_0, -y_0)$ (Dời tâm $T$ về gốc tọa độ $O$).
  2. $P_2 = P_1 \times S(S_x, S_y)$ (Co giãn quanh gốc $O$).
  3. $P' = P_2 \times T(x_0, y_0)$ (Dời về tâm ban đầu).
- **Ma trận tích lũy:**
  $$M = T(-x_0, -y_0) \times S(S_x, S_y) \times T(x_0, y_0)$$
- **Công thức trực tiếp:**
  $$x' = x_0 + (x - x_0) \cdot S_x, \quad y' = y_0 + (y - y_0) \cdot S_y$$

### 3.3. Phép Quay quanh tâm quay $T(x_0, y_0)$ góc $\theta$ (Rotation)
- **Chuỗi biến đổi (Pipeline 4 bước):**
  1. $P_1 = P \times T(-x_0, -y_0)$ (Dời tâm quay $T$ về gốc $O$).
  2. $P_2 = P_1 \times R(\theta)$ (Quay quanh $O$ một góc $\theta$).
  3. $P' = P_2 \times T(x_0, y_0)$ (Dời ngược về tâm quay $T$).
- **Ma trận tích lũy:**
  $$M = T(-x_0, -y_0) \times R(\theta) \times T(x_0, y_0)$$
- **Công thức trực tiếp:**
  $$x' = x_0 + (x - x_0)\cos\theta - (y - y_0)\sin\theta$$
  $$y' = y_0 + (x - x_0)\sin\theta + (y - y_0)\cos\theta$$

### 3.4. Phép Đối xứng qua đường thẳng bất kỳ $y = mx + b$ (Reflection)
- Đặt $\alpha = \arctan(m)$ (góc nghiêng của đường thẳng so với trục $Ox$).
- **Chuỗi biến đổi chuẩn (Pipeline 6 bước / 5 phép toán):**
  1. **Bước 1:** Tịnh tiến dời đường thẳng về đi qua gốc $O$: $T(0, -b)$.
  2. **Bước 2:** Quay quanh $O$ góc $-\alpha$ để đường thẳng trùng trục $Ox$: $R(-\alpha)$.
  3. **Bước 3:** Lấy đối xứng qua trục $Ox$: $M_{Ox}$ ($y \to -y$).
  4. **Bước 4:** Quay ngược lại góc $+\alpha$: $R(\alpha)$.
  5. **Bước 5:** Tịnh tiến ngược lại về vị trí cũ: $T(0, b)$.
- **Ma trận tích lũy:**
  $$M = T(0, -b) \times R(-\alpha) \times M_{Ox} \times R(\alpha) \times T(0, b)$$
- **Công thức giải tích trực tiếp:**
  $$x' = \frac{(1 - m^2)x + 2my - 2mb}{m^2 + 1}$$
  $$y' = \frac{2mx + (m^2 - 1)y + 2b}{m^2 + 1}$$

---

## 4. Áp dụng tìm ảnh của Tam giác ABC

Chương trình mặc định khởi động với **Preset 0: Tam giác ABC**:
- Đỉnh gốc: $A(1, 1), \quad B(5, 1), \quad C(3, 4)$
- Hiển thị trực quan:
  - Tam giác gốc $ABC$ (màu xám xanh, nét đậm).
  - Tam giác ảnh $A'B'C'$ (màu xanh ngọc phát sáng).
  - Bảng tọa độ hiển thị rõ $A \to A', B \to B', C \to C'$.
  - Đường đứt nét nối các cặp đỉnh tương ứng $A-A', B-B', C-C'$.
  - Vẽ đường thẳng đối xứng $y = mx + b$, mũi tên vector tịnh tiến $\vec{v}$, tâm tỉ lệ và tâm quay $T(x_0, y_0)$.

---

## 5. Hướng dẫn sử dụng & Phím tắt

### 5.1. Tương tác chuột
- **Click chuột trái trên viewport:**
  - *Tịnh tiến:* Click 2 điểm để xác định vector $\vec{v}$ (có xem trước live preview).
  - *Tỉ lệ:* Click 1 điểm để đặt tâm tỉ lệ $T(x_0, y_0)$.
  - *Quay:* Click 1 điểm để đặt tâm quay $T(x_0, y_0)$.
  - *Đối xứng:* Click 2 điểm để tạo đường thẳng $y = mx + b$ (vẽ đường thẳng kéo dài toàn màn hình).
- **Click chuột phải:** Chọn đỉnh tam giác gần nhất để xem tọa độ chi tiết.
- **Cuộn chuột (Wheel):** Phóng to / Thu nhỏ khung nhìn (Zoom quanh con trỏ chuột).
- **Chuột giữa (hoặc Shift + Kéo chuột trái):** Di chuyển góc nhìn (Pan viewport).

### 5.2. Phím tắt bàn phím

| Phím | Chức năng |
|---|---|
| **T** | Chuyển sang phép **Tịnh tiến (Translation)** |
| **S** | Chuyển sang phép **Tỉ lệ (Scaling)** |
| **R** / **Q** | Chuyển sang phép **Quay (Rotation)** |
| **M** / **D** | Chuyển sang phép **Đối xứng (Reflection)** qua $y = mx + b$ |
| **1 – 5** | Chọn hình học: **1**: Tam giác ABC, **2**: Nhà Slide 113, **3**: Chữ nhật, **4**: Đa giác lõm, **5**: Tự vẽ |
| **Space** / **Backspace** | Xem bước tiếp theo / lùi bước trong Pipeline |
| **F** | Fit View (Tự động canh góc nhìn vừa vặn) |
| **G** / **C** / **B** / **L** | Bật/tắt Lưới, Đường nối đỉnh, Bounding Box, Nhãn đỉnh |
| **H** | Bật/tắt chế độ Trình chiếu (Presentation Mode) |
| **Esc** | Hủy thao tác click dở dang / Thoát chế độ xem từng bước |

---

## 6. Biên dịch và Chạy thử

### Cách 1: Chạy file script
Nhấp đúp chuột vào file `run.bat` trong thư mục này.

### Cách 2: Bằng CMake (Khuyên dùng)
```bash
# Đứng tại thư mục Lab gốc
cmake --build build --target Lab_BienDoi2D_TiLe_scalling_lab
build\Lab_BienDoi2D_TiLe_scalling_lab.exe
```

### Cách 3: Chạy bộ kiểm thử tự động (33 Tests)
```bash
build\Lab_BienDoi2D_TiLe_scalling_lab.exe --test
```
Bộ test kiểm tra tự động 33 trường hợp bao gồm:
- Tính đúng đắn của ma trận và công thức trực tiếp cho cả 4 phép biến đổi.
- Bất biến tâm quay, tâm tỉ lệ, và điểm trên đường thẳng đối xứng.
- Quy tắc bảo toàn diện tích (tịnh tiến, quay, đối xứng) và biến thiên diện tích theo $|S_x S_y|$ (tỉ lệ).
- Ma trận tích lũy Pipeline $\equiv$ công thức giải tích trực tiếp (sai số $< 10^{-7}$).
- Kiểm chứng kết quả ảnh của tam giác ABC cho từng phép biến đổi.
