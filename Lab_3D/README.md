# VKU - BÀI THỰC HÀNH TẠI LỚP (26/10)
## BIỂU DIỄN HÌNH CHÓP ABCD & CÁC PHÉP BIẾN ĐỔI 3D

---

### I. YÊU CẦU ĐỀ BÀI
1. **Biểu diễn hình chóp $ABCD$** theo:
   - **Mô hình Khung kết nối (Wireframe model)**: Tập hợp các đỉnh và các cạnh nối giữa các đỉnh.
   - **Mô hình Mặt đa giác (Polygon face / Mesh model)**: Tập hợp các mặt tam giác $ABC, ACD, ADB, BCD$ được tô màu và đổ bóng.
2. **Thực hiện phép chiếu Song song / Phối cảnh** đối với hình chóp này:
   - Lựa chọn mặt phẳng chiếu phù hợp: **$XOY$ ($z=0$), $YOZ$ ($x=0$), $ZOX$ ($y=0$)** hoặc góc nhìn toàn cảnh 3D.
3. **Áp dụng các phép biến đổi 3D** lên hình chóp:
   - **Phép tịnh tiến (Translation)** theo vector $\vec{v} = (t_x, t_y, t_z)$.
   - **Phép tỉ lệ (Scaling)** với tâm tỉ lệ là **1 đỉnh của đối tượng** (ví dụ đỉnh $A$ hoặc đỉnh được chọn).
   - **Phép quay (Rotation)** quanh các trục **$Ox$, $Oy$, $Oz$** để quan sát vật thể.
4. **Sự kiện phím / chuột**:
   - Phím `T`, `S`, `R` để chuyển đổi các chế độ biến đổi: Tịnh tiến, Tỉ lệ, Quay.
   - Phím mũi tên (`Up`, `Down`, `Left`, `Right`) để tăng/giảm góc quay quanh các trục.
   - Click chuột (`Left`, `Right`) để tăng/giảm hệ số tỉ lệ $S$; click chuột để xác định vị trí tịnh tiến.
5. **Gửi ảnh kết quả**:
   - Chụp ảnh kết quả của **Phép Tỉ lệ** và **Phép Quay** gửi vào group Zalo và check nộp kết quả vào link pin trên group.

---

### II. CƠ SỞ TOÁN HỌC (CHUẨN GIÁO TRÌNH VKU)

Quy ước vector hàng (Row vector) trong tọa độ thuần nhất:
$$P = \begin{bmatrix} x & y & z & 1 \end{bmatrix}, \quad P' = P \times M_{4 \times 4}$$

#### 1. Mô hình hình chóp $ABCD$ (Tetrahedron)
- **Tọa độ 4 đỉnh:**
  - $A(0.0, 2.2, 0.0)$ (Đỉnh chóp - Apex)
  - $B(-1.8, -1.2, 1.4)$ (Đáy trước trái)
  - $C(1.8, -1.2, 1.4)$ (Đáy trước phải)
  - $D(0.0, -1.2, -1.8)$ (Đáy sau)
- **4 Mặt đa giác (Faces):** $ABC, ACD, ADB, BCD$.
- **6 Cạnh khung kết nối (Edges):** $AB, AC, AD, BC, CD, DB$.

#### 2. Phép Tịnh tiến 3D: $T(t_x, t_y, t_z)$
$$M_T = \begin{bmatrix}
1 & 0 & 0 & 0 \\
0 & 1 & 0 & 0 \\
0 & 0 & 1 & 0 \\
t_x & t_y & t_z & 1
\end{bmatrix}$$
Toạ độ trực tiếp:
$$x' = x + t_x, \quad y' = y + t_y, \quad z' = z + t_z$$

#### 3. Phép Tỉ lệ với tâm là 1 đỉnh $P_0(x_0, y_0, z_0)$
Quy trình biến đổi phức hợp qua 3 bước:
$$M_S = T(-P_0) \times S(S_x, S_y, S_z) \times T(P_0)$$
Trong đó:
$$S(S_x, S_y, S_z) = \begin{bmatrix}
S_x & 0 & 0 & 0 \\
0 & S_y & 0 & 0 \\
0 & 0 & S_z & 0 \\
0 & 0 & 0 & 1
\end{bmatrix}$$
Toạ độ trực tiếp:
$$x' = x_0 + (x - x_0) \cdot S_x, \quad y' = y_0 + (y - y_0) \cdot S_y, \quad z' = z_0 + (z - z_0) \cdot S_z$$
*Tính chất: Đỉnh tâm $P_0$ bất biến qua phép biến đổi ($P_0' = P_0$).*

#### 4. Phép Quay quanh các trục $Ox, Oy, Oz$
- **Quay quanh trục $Ox$ góc $\alpha$:**
  $$R_x(\alpha) = \begin{bmatrix}
  1 & 0 & 0 & 0 \\
  0 & \cos\alpha & \sin\alpha & 0 \\
  0 & -\sin\alpha & \cos\alpha & 0 \\
  0 & 0 & 0 & 1
  \end{bmatrix} \implies \begin{cases} x' = x \\ y' = y\cos\alpha - z\sin\alpha \\ z' = y\sin\alpha + z\cos\alpha \end{cases}$$

- **Quay quanh trục $Oy$ góc $\beta$:**
  $$R_y(\beta) = \begin{bmatrix}
  \cos\beta & 0 & -\sin\beta & 0 \\
  0 & 1 & 0 & 0 \\
  \sin\beta & 0 & \cos\beta & 0 \\
  0 & 0 & 0 & 1
  \end{bmatrix} \implies \begin{cases} x' = x\cos\beta + z\sin\beta \\ y' = y \\ z' = -x\sin\beta + z\cos\beta \end{cases}$$

- **Quay quanh trục $Oz$ góc $\gamma$:**
  $$R_z(\gamma) = \begin{bmatrix}
  \cos\gamma & \sin\gamma & 0 & 0 \\
  -\sin\gamma & \cos\gamma & 0 & 0 \\
  0 & 0 & 1 & 0 \\
  0 & 0 & 0 & 1
  \end{bmatrix} \implies \begin{cases} x' = x\cos\gamma - y\sin\gamma \\ y' = x\sin\gamma + y\cos\gamma \\ z' = z \end{cases}$$

#### 5. Phép Chiếu Song song & Phối cảnh
- **Chiếu Song song (Orthographic):**
  - Chiếu lên $XOY$ ($z=0$): $(x, y, z) \to (x, y, 0)$
  - Chiếu lên $YOZ$ ($x=0$): $(x, y, z) \to (0, y, z)$
  - Chiếu lên $ZOX$ ($y=0$): $(x, y, z) \to (x, 0, z)$
- **Chiếu Phối cảnh (Perspective):**
  - Tâm chiếu cách mặt phẳng khoảng $d$:
  - Chiếu lên $XOY$: $x' = \frac{x}{1 - z/d}, \ y' = \frac{y}{1 - z/d}, \ z' = 0$

---

### III. HƯỚNG DẪN SỬ DỤNG PHÍM VÀ CHUỘT

| Phím / Chuột | Chức năng |
| :--- | :--- |
| **`T`** | Chế độ **Tịnh tiến (Translation)** - Hiển thị Vector tịnh tiến $\vec{v}$ màu cam trực quan |
| **`S`** | Chế độ **Tỉ lệ (Scaling)** - Hiển thị các tia tỉ lệ dãn nở phát ra từ tâm đỉnh $P_0$ |
| **`R`** | Chế độ **Quay (Rotation)** - Hiển thị vòng tròn góc quay Gizmo quanh trục $Ox, Oy, Oz$ |
| **`K`** / **`H`** | **Bật / Tắt vòng tròn xoay Gizmo** quanh trục để khung hình gọn gàng, sạch sẽ |
| **`A`** | **Bật/Tắt Tự động xoay 3D (Auto-Spin)** mượt mà 60 FPS để quan sát toàn diện |
| **`0`** | Góc nhìn 3D Phối cảnh chuẩn (Isometric 3D View) |
| **`7` / `8` / `9`** | Chuyển nhanh 3 hình chiếu kỹ thuật: **Hình chiếu Bằng (Top)**, **Đứng (Front)**, **Cạnh (Side)** |
| **`[` / `]`** | Tăng / Giảm độ trong suốt (Opacity) của các mặt đa giác |
| **`O` / `G`** | Bật / Tắt khung nét đứt của hình gốc $ABCD$ ban đầu |
| **`Up` / `Down`** | Tăng / giảm góc quay quanh trục $Ox$ hoặc tăng/giảm tham số |
| **`Left` / `Right`** | Tăng / giảm góc quay quanh trục đang chọn ($Ox, Oy, Oz$) |
| **Click chuột Trái** | Ở mode `S`: **Tăng hệ số tỉ lệ $S$ (+0.2)**<br>Ở mode `T`: **Đặt vị trí tịnh tiến mới** |
| **Click chuột Phải** | Ở mode `S`: **Giảm hệ số tỉ lệ $S$ (-0.2)**<br>Ở mode `R`: Pan camera |
| **Kéo chuột Trái** | Xoay tự do góc nhìn 3D (Orbit Camera) |
| **Cuộn chuột** | Phóng to / Thu nhỏ khung nhìn (Zoom Camera) |
| **`X` / `Y` / `Z`** | Đổi trục quay đang kích hoạt ($Ox, Oy, Oz$) |
| **`V`** | Đổi đỉnh tâm tỉ lệ ($A \to B \to C \to D$) |
| **`M`** | Đổi mô hình: Khung kết nối $\leftrightarrow$ Mặt đa giác $\leftrightarrow$ Cả hai |
| **`P`** | Đổi phép chiếu: Chiếu Song song $\leftrightarrow$ Chiếu Phối cảnh |
| **`1` / `2` / `3` / `4`** | Chọn mặt phẳng chiếu ($XOY, YOZ, ZOX$, hoặc 3D toàn cảnh) |
| **`Space`** | Reset biến đổi về mặc định |

---

### IV. CÁCH CHẠY VÀ NỘP BÀI

#### 1. Khởi chạy ứng dụng
Chạy file `run.bat` trong thư mục `Lab_3D` hoặc chạy:
```bash
cmake -B build
cmake --build build --target Lab_3D_main
build\Lab_3D_main.exe
```

#### 2. Kiểm thử toán học tự động (Self-tests)
```bash
build\Lab_3D_main.exe --test
```
Kết quả kiểm tra 17/17 test pass.

#### 3. Hướng dẫn chụp ảnh nộp Zalo:
- **Ảnh 1 (Phép Quay):**
  1. Nhấn phím `R` (chế độ Quay).
  2. Dùng các phím mũi tên hoặc nút `Y: Truc Oy` / `X: Truc Ox` để xoay hình chóp một góc đẹp.
  3. Bảng bên phải hiển thị rõ các góc quay và ma trận $4 \times 4$.
  4. Nhấn `PrintScreen` (hoặc `Win + Shift + S`) để chụp ảnh.
- **Ảnh 2 (Phép Tỉ Lệ):**
  1. Nhấn phím `S` (chế độ Tỉ lệ).
  2. Click chuột trái vào vùng đồ họa để phóng to hình chóp (ví dụ $S = 1.6$).
  3. Chú ý đỉnh tâm tỉ lệ (ví dụ đỉnh $A$ màu vàng) giữ nguyên vị trí, các đỉnh khác mở rộng đều ra.
  4. Chụp ảnh màn hình và gửi vào group Zalo!
