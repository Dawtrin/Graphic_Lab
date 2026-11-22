# BÁO CÁO THỰC HÀNH TẠI LỚP (NGÀY 16/11): HÌNH CHÓP SABC TRONG KHÔNG GIAN 3D
**Học phần:** Đồ họa máy tính (Computer Graphics)  
**Trường Đại học Công nghệ Thông tin và Truyền thông Việt - Hàn (VKU)**  
**Thư mục bài làm:** `Lab_TH_1611`

---

## 1. ĐỀ BÀI VÀ CÁC YÊU CẦU TRỌNG TÂM

> **Đề bài:** Trong không gian 3D, cho hình chóp $SABC$, vẽ và quan sát hình chóp này.  
> **Yêu cầu:**
> 1. Khử khuất hình chóp bằng:
>    - Thuật toán Backface (Backface Culling)
>    - Thuật toán Deep-buffer (Depth-buffer / Z-buffer)
> 2. Tô bóng hình chóp bằng phương pháp tô bóng Gouraud (Gouraud Shading)
> 3. Nộp 3 hình (*khử khuất Backface*, *khử khuất Deep-buffer*, *tô bóng Gouraud*) vào group Zalo và check cột "16/11".

---

## 2. CƠ SỞ LÝ THUYẾT VÀ NGUYÊN LÝ THUẬT TOÁN

### 2.1. Biểu diễn hình học hình chóp 3D $SABC$
Hình chóp gồm 4 đỉnh và 4 mặt tam giác:
- **Đỉnh $S$ (Apex):** Tọa độ trên cao: $S = (0.0, 2.2, 0.0)$
- **Các đỉnh đáy $A, B, C$:** Tạo thành tam giác trên mặt phẳng đáy $y = -1.0$:
  - $A = (-1.8, -1.0, 1.2)$
  - $B = ( 1.8, -1.0, 1.2)$
  - $C = ( 0.0, -1.0, -1.8)$
- **4 mặt tam giác (định hướng ngược chiều kim đồng hồ - CCW khi nhìn từ ngoài vào):**
  - Mặt bên $SAB$: Thứ tự đỉnh $[S, A, B]$
  - Mặt bên $SBC$: Thứ tự đỉnh $[S, B, C]$
  - Mặt bên $SCA$: Thứ tự đỉnh $[S, C, A]$
  - Mặt đáy $ABC$: Thứ tự đỉnh $[A, C, B]$ (Pháp tuyến hướng xuống dưới $-Y$)

Pháp tuyến ngoài của từng mặt được tính bằng tích có hướng:
$$\vec{N} = \frac{(V_1 - V_0) \times (V_2 - V_0)}{\|(V_1 - V_0) \times (V_2 - V_0)\|}$$

---

### 2.2. Yêu cầu 1A: Thuật toán Khử khuất Backface (Backface Culling)
- **Nguyên lý:**
  - Xác định vector tia nhìn $\vec{V}$ từ trọng tâm mặt $C_{face}$ tới mắt người quan sát (Camera Eye):
    $$\vec{V} = \frac{\text{Eye} - C_{face}}{\|\text{Eye} - C_{face}\|}$$
  - Tính tích vô hướng giữa pháp tuyến ngoài $\vec{N}$ và vector tia nhìn $\vec{V}$:
    $$s = \vec{N} \cdot \vec{V}$$
    - Nếu $s > 0$: Góc giữa $\vec{N}$ và $\vec{V} < 90^\circ \implies$ Mặt quay về phía mắt người xem $\to$ **Mặt trước (Front-face) $\implies$ HIỂN THỊ**.
    - Nếu $s \le 0$: Góc $\ge 90^\circ \implies$ Mặt quay lưng lại với mắt người xem $\to$ **Mặt sau (Back-face) $\implies$ KHỬ KHUẤT (CULLED / LOẠI BỎ)**.
- **Đặc điểm nổi bật trong chương trình:**
  - Ở chế độ này, **Depth Buffer được cố ý tắt** (`glDisable(GL_DEPTH_TEST)`) để chứng minh nguyên lý toán học kinh điển: **Với vật thể đa diện lồi (Convex Polyhedron) như hình chóp SABC, chỉ riêng thuật toán Backface Culling đã đủ để khử khuất hoàn hảo 100% tất cả các mặt sau!**
  - Vẽ trực quan các vector pháp tuyến $\vec{N}$ từ trọng tâm mỗi mặt (Màu xanh lá = Mặt trước đang hiển thị; Màu đỏ = Mặt sau bị loại bỏ).
  - Có bảng HUD hiển thị thời gian thực giá trị $\vec{N} \cdot \vec{V}$ của cả 4 mặt $SAB, SBC, SCA, ABC$.

---

### 2.3. Yêu cầu 1B: Thuật toán Khử khuất Deep-buffer (Depth-buffer / Z-buffer)
- **Nguyên lý:**
  - Sử dụng một ma trận bộ đệm độ sâu $Z_{buffer}(x, y)$ có kích thước bằng khung hình.
  - Khởi tạo giá trị lớn nhất: $Z_{buffer}(x, y) = 1.0$ (khoảng cách mặt phẳng xa nhất).
  - Khi vẽ bất kỳ tam giác nào, tại mỗi điểm ảnh $(x, y)$, nội suy độ sâu $z \in [0, 1]$:
    - Nếu $z < Z_{buffer}(x, y)$ (gần mắt hơn vật thể đã vẽ trước đó):
      - Cập nhật: $Z_{buffer}(x, y) = z$
      - Ghi màu: $\text{Color}(x, y) = \text{Màu bề mặt hiện tại}$
    - Ngược lại: Bỏ qua (pixel bị che khuất bởi đối tượng phía trước).
- **Đặc điểm nổi bật trong chương trình:**
  - Tắt Backface culling (`glDisable(GL_CULL_FACE)`) và bật `glEnable(GL_DEPTH_TEST)` để kiểm nghiệm độc lập sức mạnh của Z-buffer.
  - **Khung phụ trực quan hóa Depth Map (Z-Buffer Visualization):** Thể hiện thang độ xám Grayscale từ gần (trắng/sáng) đến xa (tối/đen), hiển thị khoảng cách và giá trị $Z_{norm}$ thực tế của từng đỉnh $S, A, B, C$.
  - **Nút thí nghiệm "Đảo thứ tự vẽ" + "Tắt Depth Buffer":** Giúp quan sát hiện tượng sai lệch thứ tự không gian khi không có Z-buffer (mặt ở xa vẽ đè lên mặt ở gần).

---

### 2.4. Yêu cầu 2: Phương pháp Tô bóng Gouraud (Gouraud Shading)
- **Nguyên lý:**
  1. **Tính vector pháp tuyến tại các đỉnh (Vertex Normals):**
     Với mỗi đỉnh $V \in \{S, A, B, C\}$, vector pháp tuyến đỉnh $\vec{N}_V$ được tính bằng trung bình cộng chuẩn hóa của các pháp tuyến mặt kề với đỉnh đó:
     $$\vec{N}_V = \text{normalize}\left(\sum_{F \in Faces(V)} \vec{N}_F\right)$$
  2. **Tính toán cường độ ánh sáng tại từng đỉnh:**
     $$I_V = I_a k_a + I_d k_d (\vec{N}_V \cdot \vec{L}) + I_s k_s (\vec{R} \cdot \vec{V})^n$$
  3. **Nội suy màu tuyến tính (Bilinear Interpolation):**
     Nội suy màu sắc từ các đỉnh vào từng điểm bên trong mặt tam giác theo tọa độ trọng tâm (Barycentric coordinates). Trong OpenGL, kích hoạt thông qua:
     `glShadeModel(GL_SMOOTH);`

- **Các điểm tinh chỉnh quan trọng giúp tránh lỗi hiển thị khi tô bóng Gouraud:**
  1. **Tránh lỗi chân chóp bị tối đen (Tách pháp tuyến thân và đáy):**
     - Mặt đáy $ABC$ có pháp tuyến hướng xuống đất $(0, -1, 0)$. Nếu gộp chung cả mặt đáy vào các đỉnh $A, B, C$, vector pháp tuyến tại chân chóp sẽ bị kéo cụp xuống sàn. Khi nguồn sáng ở trên cao, $\vec{N} \cdot \vec{L} \le 0$ làm cho chân chóp bị tối sầm bất thường.
     - Chương trình cung cấp tùy chọn **`[PT: Thân chuẩn]`** (pháp tuyến đỉnh thân chóp tính riêng từ 3 mặt bên, đáy giữ phẳng chuẩn đồ họa 3D) và cho phép đối chiếu với **`[PT: Gộp đáy]`**.
  2. **Giải quyết nhược điểm đốm sáng (Specular Highlight) trên tam giác lớn:**
     - Lý thuyết chỉ ra: Gouraud Shading chỉ tính toán ánh sáng tại 3 đỉnh góc; nếu vệt sáng bóng (Specular) rơi vào giữa mặt tam giác lớn thì Gouraud cổ điển sẽ bỏ sót đốm sáng đó.
     - Chương trình tích hợp thanh trượt **`Độ mịn lưới Subdiv (1x - 8x)`** chia nhỏ mặt tam giác thành lưới con, giúp ánh sáng và điểm bóng loáng (Specular) lướt chuyển động cực kỳ mịn màng trên mặt chóp.
  3. **Chế độ màu đồng nhất (Uniform Material):**
     - Cho phép chọn **`[Màu: Đồng nhất]`** (màu xanh ngọc / cyan chuẩn) để dễ quan sát dải sáng tối (shading gradient) thay vì bị rối mắt bởi 4 màu sắc sặc sỡ khác nhau của từng mặt.
  4. **Tắt viền Wireframe khi chụp ảnh:**
     - Nút **`[Viền Wireframe: TẮT]`** giúp hình chóp Gouraud khi xuất ảnh nộp bài Zalo trông mượt mà, bóng bẩy và chuyên nghiệp 100%.

---

## 3. HƯỚNG DẪN BIÊN DỊCH VÀ CHẠY ỨNG DỤNG

### Cách 1: Khởi chạy nhanh bằng File Batch (Khuyên dùng)
Tại thư mục `F:\GraphicsLab\Lab`:
- Nhấp đúp vào file `run_lab_1611.bat`
- Hoặc vào thư mục `Lab_TH_1611` và chạy `run.bat`

### Cách 2: Biên dịch thủ công bằng CMake & MinGW
```powershell
cd F:\GraphicsLab\Lab
cmake -B build
cmake --build build --target Lab_TH_1611_main
.\build\Lab_TH_1611_main.exe
```

### Cách 3: Chạy bộ kiểm thử tự động (Self-Tests)
```powershell
.\build\Lab_TH_1611_main.exe --test
```
*(Kết quả kiểm tra 10/10 tiêu chí toán học, tọa độ, pháp tuyến và chụp ảnh đều PASS 100%)*

---

## 4. HƯỚNG DẪN XUẤT 3 HÌNH ẢNH NỘP VÀO GROUP ZALO

Đề bài yêu cầu:
> *"Nộp 3 hình (khử khuất Backface, khử khuất Deep-buffer, tô bóng Gouraud) vào group zalo và check cột “16/11”"*

Chương trình đã tích hợp sẵn tính năng tự động xuất ảnh BMP độ nét cao:
1. **Cách tự động (1 Click):**
   - Trên thanh công cụ bên phải, nhấp vào nút: **`⭐ CHỤP CẢ 3 HÌNH`**
   - Chương trình sẽ tự động chụp và lưu lần lượt 3 file ảnh chuẩn vào thư mục làm việc:
     - `Hinh1_KhuKhuat_Backface.bmp`: Chế độ khử khuất Backface (kèm vector pháp tuyến và bảng trạng thái Front/Back)
     - `Hinh2_KhuKhuat_DeepBuffer.bmp`: Chế độ khử khuất Deep-buffer (kèm khung Z-buffer Depth Map mini)
     - `Hinh3_ToBong_Gouraud.bmp`: Chế độ tô bóng Gouraud (kèm nguồn sáng và ánh sáng mượt mà)
2. **Cách thủ công:**
   - Chọn Tab `[1]`, chỉnh góc nhìn vừa ý $\to$ bấm **`📸 Chụp ảnh hiện tại`** (hoặc phím `F12` / `P`).
   - Chọn Tab `[2]` $\to$ bấm chụp.
   - Chọn Tab `[3]` $\to$ bấm chụp.
3. Kéo thả 3 file ảnh BMP vừa sinh ra vào group Zalo của lớp và thông báo check cột "16/11"!

---

## 5. BẢNG PHÍM TẮT VÀ ĐIỀU KHIỂN

| Thao tác | Chức năng |
| :--- | :--- |
| **Phím `1`** | Chuyển sang chế độ **1. Khử khuất Backface** |
| **Phím `2`** | Chuyển sang chế độ **2. Khử khuất Deep-buffer** |
| **Phím `3`** | Chuyển sang chế độ **3. Tô bóng Gouraud** |
| **Phím `Space`** | Bật / Tắt tự động xoay quanh vật thể (Auto Orbit) |
| **Phím `B`** | Bật / Tắt Backface Culling (để đối chiếu) |
| **Phím `D`** | Bật / Tắt Depth Buffer (để quan sát lỗi chồng lấn) |
| **Phím `G`** | Chuyển đổi giữa Gouraud Shading (`GL_SMOOTH`) và Flat Shading (`GL_FLAT`) |
| **Phím `N`** | Bật / Tắt hiển thị các vector pháp tuyến |
| **Phím `P` hoặc `F12`**| Chụp ảnh màn hình hiện tại (.BMP) |
| **Phím `R`** | Đặt lại góc nhìn và thông số mặc định |
| **Phím `Mũi tên`** | Xoay góc quan sát theo từng bước 5 độ |
| **Kéo Chuột trái** | Xoay tự do góc nhìn 3D (Góc phương vị $\theta$ và góc ngẩng $\phi$) |
| **Kéo Chuột phải** | Phóng to / Thu nhỏ khoảng cách nhìn |
| **Cuộn con lăn** | Zoom In / Zoom Out nhanh chóng |
