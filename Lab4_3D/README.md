# LAB 4: QUAN SÁT VẬT THỂ 3D (3D VIEWING & PROJECTIONS) — VKU COMPUTER GRAPHICS

> **Trường Đại học Công nghệ Thông tin và Truyền thông Việt - Hàn (VKU)**  
> **Khoa Kỹ thuật Máy tính & Điện tử** · Môn học: **Đồ họa máy tính**  
> Ứng dụng mô phỏng và thực hành quan sát vật thể 3D tương tác cao bằng C++17, OpenGL và FreeGLUT.

---

## 1. Yêu cầu đề bài (Lab 4)

Chương trình cho phép quan sát vật thể trong không gian 3 chiều:
- **4.1. Sử dụng phép quay 3D quanh các trục $Ox, Oy, Oz$ để quan sát vật thể.**
- **4.2. Dùng ma trận biến đổi hệ quan sát để quan sát vật thể bằng cách:**
  - Thay đổi phép chiếu: **Song song (Orthographic)** hoặc **Phối cảnh (Perspective)**.
  - Thay đổi góc quan sát $\theta$ (Azimuth) và $\Phi$ (Elevation) để xoay hướng quan sát quanh vật thể.
  - Thay đổi vị trí quan sát (Điểm nhìn / Mắt quan sát $Eye$ và Tâm nhìn $Target$).
  - Phóng to, thu nhỏ vật thể (Scaling & Camera Distance / Zoom).
- **Yêu cầu kỹ thuật:**
  - Vật thể được biểu diễn theo **Mô hình mặt đa giác (Polygon Mesh Model)**.
  - Dùng **sự kiện bàn phím và chuột** để điều chỉnh góc quay, vị trí quan sát,...

---

## 2. Chuẩn toán học & Hệ tọa độ (VKU Computer Graphics)

Ứng dụng tuân thủ chuẩn quy ước trong giáo trình Đồ họa máy tính VKU:
- Biểu diễn tọa độ thuần nhất dạng **Vector hàng (Row Vector)**:
  $$P = \begin{bmatrix} x & y & z & 1 \end{bmatrix}, \quad P' = P \times M_{4 \times 4}$$
- Các phép biến đổi liên tiếp được nhân dồn từ trái sang phải:
  $$M = M_1 \times M_2 \times M_3 \times \dots$$

---

## 3. Cơ sở lý thuyết & Chi tiết các ma trận biến đổi

### 3.1. Phép quay 3D quanh các trục tọa độ $Ox, Oy, Oz$ (Mục 4.1)

#### 1. Quay quanh trục $Ox$ góc $\alpha$ (rad):
Trục $Ox$ bất biến ($x' = x$), các tọa độ $y, z$ quay trong mặt phẳng $YOZ$:
$$R_x(\alpha) = \begin{bmatrix}
1 & 0 & 0 & 0 \\
0 & \cos\alpha & \sin\alpha & 0 \\
0 & -\sin\alpha & \cos\alpha & 0 \\
0 & 0 & 0 & 1
\end{bmatrix}$$

#### 2. Quay quanh trục $Oy$ góc $\beta$ (rad):
Trục $Oy$ bất biến ($y' = y$), các tọa độ $z, x$ quay trong mặt phẳng $ZOX$:
$$R_y(\beta) = \begin{bmatrix}
\cos\beta & 0 & -\sin\beta & 0 \\
0 & 1 & 0 & 0 \\
\sin\beta & 0 & \cos\beta & 0 \\
0 & 0 & 0 & 1
\end{bmatrix}$$

#### 3. Quay quanh trục $Oz$ góc $\gamma$ (rad):
Trục $Oz$ bất biến ($z' = z$), các tọa độ $x, y$ quay trong mặt phẳng $XOY$:
$$R_z(\gamma) = \begin{bmatrix}
\cos\gamma & \sin\gamma & 0 & 0 \\
-\sin\gamma & \cos\gamma & 0 & 0 \\
0 & 0 & 1 & 0 \\
0 & 0 & 0 & 1
\end{bmatrix}$$

#### 4. Ma trận quay Euler tổng hợp 3 trục:
$$M_{rot} = R_z(\gamma) \times R_y(\beta) \times R_x(\alpha)$$

---

### 3.2. Ma trận biến đổi hệ quan sát (Viewing Transformation Matrix - Mục 4.2)

Hệ quan sát chuyển đổi từ **Hệ tọa độ thế giới (WCS - World Coordinate System)** sang **Hệ tọa độ quan sát (VCS - Viewing Coordinate System)**:

#### 1. Tọa độ cầu của điểm nhìn (Eye Position $E$):
Điểm nhìn $E(x_E, y_E, z_E)$ xoay trên mặt cầu bán kính $R$ quanh điểm nhìn mục tiêu $C(x_c, y_c, z_c)$:
- $\theta$: Góc phương vị nằm ngang ($0^\circ \le \theta < 360^\circ$).
- $\Phi$: Góc ngẩng theo phương đứng ($-89^\circ \le \Phi \le +89^\circ$).
- $R$: Bán kính / Khoảng cách quan sát.
$$x_E = x_c + R \cos\Phi \sin\theta$$
$$y_E = y_c + R \sin\Phi$$
$$z_E = z_c + R \cos\Phi \cos\theta$$

#### 2. Xây dựng hệ trục quan sát $(\vec{u}, \vec{v}, \vec{n})$:
- Vector trục ngắm $\vec{n}$ (trục $z_v$ hướng từ vật đến mắt):
  $$\vec{n} = \frac{E - C}{\|E - C\|}$$
- Vector trục hoành $\vec{u}$ (trục $x_v$ song song mặt phẳng nhìn):
  $$\vec{u} = \frac{\vec{Up} \times \vec{n}}{\|\vec{Up} \times \vec{n}\|}$$
- Vector trục tung thực $\vec{v}$ (trục $y_v$ hướng lên trên trong mặt phẳng nhìn):
  $$\vec{v} = \vec{n} \times \vec{u}$$

#### 3. Ma trận hệ quan sát $M_{view}$:
$$M_{view} = T(-E) \times R_{view} = \begin{bmatrix}
u_x & v_x & n_x & 0 \\
u_y & v_y & n_y & 0 \\
u_z & v_z & n_z & 0 \\
-\vec{E} \cdot \vec{u} & -\vec{E} \cdot \vec{v} & -\vec{E} \cdot \vec{n} & 1
\end{bmatrix}$$
Sau biến đổi $P_{VCS} = P_{WCS} \times M_{view}$, gốc quan sát nằm tại mắt nhìn $E$, trục nhìn trùng với trục $z_v$.

---

### 3.3. Các phép chiếu quan sát (Projections)

#### 1. Phép chiếu Song song (Orthographic Projection):
- Các tia chiếu song song vuông góc với mặt phẳng quan sát:
  $$x_{screen} = x_v \cdot s_p, \quad y_{screen} = y_v \cdot s_p$$
- **Đặc điểm:** Tỉ lệ kích thước vật thể không thay đổi theo độ sâu xa gần. Thích hợp cho bản vẽ kỹ thuật CAD, mặt cắt chiếu đứng, bằng, cạnh.

#### 2. Phép chiếu Phối cảnh (Perspective Projection):
- Các tia chiếu hội tụ về tâm mắt $E$. Với khoảng cách tiêu cự $d$:
  $$x_{screen} = \frac{x_v \cdot d}{z_v}, \quad y_{screen} = \frac{y_v \cdot d}{z_v}$$
- Ma trận thuần nhất 4x4:
  $$M_{persp} = \begin{bmatrix}
  1 & 0 & 0 & 0 \\
  0 & 1 & 0 & 0 \\
  0 & 0 & 1 & 1/d \\
  0 & 0 & 0 & 0
  \end{bmatrix}$$
- **Đặc điểm:** Vật thể ở xa sẽ nhỏ hơn vật ở gần (luật xa gần), các đường thẳng song song hội tụ về điểm tụ, phản ánh thị giác mắt người thực tế.

---

### 3.4. Phóng to, thu nhỏ vật thể (Scaling & Zooming)

Hỗ trợ 2 phương pháp thu phóng:
1. **Thu phóng đối tượng (Object Scaling):** Dùng ma trận tỉ lệ $S(s)$:
   $$S(s) = \begin{bmatrix} s & 0 & 0 & 0 \\ 0 & s & 0 & 0 \\ 0 & 0 & s & 0 \\ 0 & 0 & 0 & 1 \end{bmatrix}$$
2. **Thu phóng khoảng cách quan sát (Camera Dolly Zoom):** Thay đổi bán kính $R$ và tiêu cự $d$ qua lăn chuột hoặc phím tắt.

---

## 4. Mô hình mặt đa giác (Polygon Mesh Model)

Chương trình lưu trữ dữ liệu hình học theo cấu trúc chuẩn:
- **Danh sách đỉnh (Vertex List):** Tọa độ thực $(x, y, z)$ của từng đỉnh trong WCS.
- **Danh sách mặt (Face List):** Tập hợp các chỉ số đỉnh cấu thành mặt đa giác (tam giác, tứ giác) theo chiều ngược chiều kim đồng hồ (CCW).
- **Danh sách cạnh (Edge List):** Các đoạn thẳng liên kết dùng cho chế độ khung dây (Wireframe).
- **Vector pháp tuyến mặt (Face Normals):** Dùng để tính toán ánh sáng và đổ bóng phong phú:
  $$\vec{N} = \frac{(V_1 - V_0) \times (V_2 - V_0)}{\|(V_1 - V_0) \times (V_2 - V_0)\|}$$

**Các mô hình có sẵn:**
1. **Hình chóp tứ diện ABCD (Tetrahedron):** 4 đỉnh, 4 mặt tam giác, 6 cạnh.
2. **Khối lập phương (Cube 6 mặt):** 8 đỉnh, 6 mặt tứ giác, 12 cạnh, màu sắc riêng biệt cho từng mặt.
3. **Mô hình Ngôi nhà 3D (VKU 3D House):** 10 đỉnh, 9 mặt (thân nhà và mái lăng trụ).
4. **Khối bát diện đều (Octahedron):** 6 đỉnh, 8 mặt tam giác đều.
5. **Mô hình Lính đặc nhiệm cầm súng (Special Forces Commando with Rifle):**
   - Đầu trùm mặt nạ balaclava & Mũ bảo hiểm chiến thuật (Combat Helmet).
   - Kính nhìn đêm / Kính râm chiến thuật (Night-vision Visor).
   - Thân trang bị áo giáp chống đạn (Tactical Body Armor) & Hộp tiếp đạn trước ngực.
   - Thắt lưng chiến thuật & Ba lô quân sự sau lưng (Backpack).
   - Quần rằn ri đặc nhiệm & 2 ủng quân sự (Combat Boots).
   - Hai cánh tay giương tư thế ngắm bắn chắc chắn.
   - Khẩu súng trường tấn công chi tiết: Báng súng (Stock), Thân súng (Receiver), Băng đạn cong (Curved Magazine), Tay cầm cò (Pistol Grip), Nòng súng dài vươn về phía trước (Barrel), Đầu nòng giảm giật (Muzzle Brake), Ống ngắm quang học Laser đỏ (Tactical Scope), và Ốp lót tay sa mạc (Desert Tan Handguard).

**Chế độ hiển thị:**
- `Khung dây (Wireframe)`: Chỉ vẽ các cạnh lưới không gian.
- `Mặt đa giác (Polygon)`: Tô màu mặt phẳng có nguồn sáng đa hướng.
- `Kết hợp (Both)`: Mặt đa giác tô màu viền khung trắng nổi bật.

---

## 5. Hướng dẫn sử dụng & Phím tắt

### 5.1. Thao tác chuột
| Thao tác | Chức năng |
| :--- | :--- |
| **Kéo chuột trái (Left Drag)** | Trong 4.1: Xoay vật thể quanh $Ox, Oy$. Trong 4.2: Xoay góc quan sát $\theta, \Phi$. |
| **Kéo chuột phải (Right Drag)**| Di chuyển vị trí quan sát (Pan Target $X, Y$). |
| **Cuộn chuột (Wheel)** | Phóng to / Thu nhỏ (Zoom $R$ hoặc $Scale$). |
| **Click chuột trái UI** | Chọn nút bấm, kéo slider, đổi mô hình trực tiếp. |

### 5.2. Phím tắt bàn phím
| Phím | Chức năng |
| :---: | :--- |
| `1` | Chuyển sang chuyên đề **4.1: Quay quanh Ox, Oy, Oz** |
| `2` | Chuyển sang chuyên đề **4.2: Ma trận hệ quan sát & Phép chiếu** |
| `P` | Chuyển đổi giữa **Chiếu Song song (Orthographic)** $\leftrightarrow$ **Chiếu Phối cảnh (Perspective)** |
| `M` | Đổi mô hình vật thể (Chóp ABCD $\to$ Lập phương $\to$ Ngôi nhà $\to$ Bát diện) |
| `W` | Đổi kiểu vẽ: Khung dây $\to$ Mặt đa giác $\to$ Cả hai |
| `X`, `Y`, `Z` | Chọn trục quay tích cực để điều khiển trong mục 4.1 |
| `Left` / `Right` | Giảm / Tăng góc quay trục đã chọn hoặc góc quan sát $\theta$ |
| `Up` / `Down` | Tăng / Giảm góc quay $Ox$ hoặc góc ngẩng $\Phi$ |
| `+` / `-` | Phóng to / Thu nhỏ |
| `Space` | Bật / Tắt chế độ tự động xoay demo |
| `R` | Reset toàn bộ các thông số về góc nhìn mặc định |
| `ESC` | Thoát chương trình |

---

## 6. Hướng dẫn biên dịch và chạy

### Cách 1: Chạy trực tiếp file batch (Khuyên dùng)
- Click đúp vào file `run.bat` nằm ngay trong thư mục `Lab4_3D/`.
- Hoặc từ thư mục gốc `Lab/`, click đúp file `run_lab4.bat`.

### Cách 2: Biên dịch qua CMake (Command Line)
```bash
cd F:\GraphicsLab\Lab
cmake -B build
cmake --build build --target Lab4_3D_main
.\build\Lab4_3D_main.exe
```

### Cách 3: Chạy bộ kiểm thử tự động (Headless Self-Tests)
```bash
.\build\Lab4_3D_main.exe --test
```

---

## 7. Kết quả kiểm thử tự động (Self-Tests)

Toàn bộ **36/36 kiểm thử toán học** đã được kiểm tra và vượt qua chính xác 100%:
- **13/13 kiểm thử 4.1:** Kiểm tra tính bảo toàn tọa độ trục quay, phép quay góc vuông $90^\circ$ quanh $Ox, Oy, Oz$, ma trận quay tổng hợp Euler và bảo toàn khoảng cách Euclid.
- **23/23 kiểm thử 4.2:** Kiểm tra công thức tọa độ cầu $E(\theta, \Phi, R)$, ma trận hệ quan sát $M_{view}$ biến đổi Target về gốc VCS, công thức chiếu song song không phụ thuộc $z$, công thức phối cảnh tỉ lệ $d/z$, tỉ lệ thu phóng $S(s)$, và tính nhất quán topology của cả 5 mô hình mặt đa giác (bao gồm mô hình Lính đặc nhiệm cầm súng).
