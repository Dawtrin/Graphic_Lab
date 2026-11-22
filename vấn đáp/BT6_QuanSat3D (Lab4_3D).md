# TÀI LIỆU ÔN VẤN ĐÁP — BT6: QUAN SÁT VẬT THỂ 3D & HỆ QUAN SÁT (LAB4_3D)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (Lab 4):** Viết chương trình cho phép quan sát vật thể 3D:
  - **4.1.** Sử dụng phép **quay 3D quanh trục $Ox, Oy, Oz$** để quan sát vật thể.
  - **4.2.** Dùng **ma trận biến đổi hệ quan sát** để quan sát vật thể bằng cách:
    - Thay đổi phép chiếu: **Song song** hoặc **Phối cảnh**.
    - Thay đổi góc quan sát **$\theta, \Phi$** để xoay vật thể.
    - Thay đổi vị trí quan sát (**$R$**).
    - Phóng to, thu nhỏ vật thể (**$D$**).
  - **Yêu cầu:** Vật thể biểu diễn theo **mô hình mặt đa giác**; dùng sự kiện phím, chuột để điều chỉnh.
- **Mã nguồn:** `Lab4_3D/main.cpp` | **Chạy chương trình:** `Lab4_3D/run.bat` hoặc `run_lab4.bat`
- **Tài liệu đối chiếu:** Slide `CG.4-3D Graphics.pdf` (trang 11–12, 38–49).

---

## 1. BẢN ĐỒ TỔNG QUAN FILE `Lab4_3D/main.cpp` (THEO SỐ DÒNG)

| Khoảng dòng | Thành phần / Hàm | Vai trò chính |
| :--- | :--- | :--- |
| **`59 → 79`** | **`dot`, `cross`, `length`, `normalize`** | **Tích vô hướng (`dot`), Tích có hướng (`cross`) và chuẩn hóa vector đơn vị.** |
| `82 → 110` | `struct Mat4`, `ident4`, `operator*` | Ma trận thuần nhất $4 \times 4$, quy ước vector hàng $P' = P \times M$. |
| **`114 → 146`** | **`rotX4`, `rotY4`, `rotZ4`, `rotationEuler4`** | **Mục 4.1: Quay 3D quanh trục $Ox, Oy, Oz$ để quan sát vật thể.** |
| **`168 → 176`** | **`computeEyePosition`** | **Mục 4.2: Tính tọa độ mắt nhìn `eye(x, y, z)` từ 3 tham số cầu $(\theta, \Phi, R)$.** |
| **`177 → 197`** | **`viewingMatrix4`** | **Mục 4.2: Lập ma trận biến đổi hệ quan sát $4 \times 4$ (dựng 3 trục $\vec{u}, \vec{v}, \vec{n}$ bằng `cross` & `dot`).** |
| **`200 → 209`** | **`projectToScreen`** | **Mục 4.2: Chiếu Song song & Chiếu Phối cảnh (dùng tham số $D$ phóng to/thu nhỏ).** |
| **`229 → 240`** | **`updateMeshNormals`** | **Tính vector pháp tuyến cho từng mặt đa giác bằng `cross(v1 - v0, v2 - v0)`.** |
| `243 → 550` | `createPyramidMesh`, Cube, House, Commando | 4 mô hình mặt đa giác 3D (Hình chóp, Lập phương, Ngôi nhà, Lính đặc nhiệm cầm súng). |
| `1409 → 1600` | `mouseButton`, `mouseMotion`, `keyboard`, `specialKey` | Bắt chuột kéo xoay camera, cuộn zoom và phím `1, 2, P, M, W, X/Y/Z, +/-, mũi tên`. |

---

## 2. GIẢI THÍCH TỪNG CỤM CODE CỐT LÕI (DỊCH THẲNG TỪNG LỆNH + CÁCH NÓI VỚI CÔ)

### 2.1. Tích vô hướng (`dot`) & Tích có hướng (`cross`) (`Dòng 59 → 79`) *(CÔ RẤT HAY HỎI!)*

```cpp
// TÍCH VÔ HƯỚNG (Dot Product): kết quả là 1 CON SỐ thực
static double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;   // Trả về ax*bx + ay*by + az*bz
}

// TÍCH CÓ HƯỚNG (Cross Product): kết quả là 1 VECTOR mới vuông góc với cả a và b
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,                  // Thành phần x của vector vuông góc
        a.z * b.x - a.x * b.z,                  // Thành phần y của vector vuông góc
        a.x * b.y - a.y * b.x                   // Thành phần z của vector vuông góc
    };
}
```
> **Cách nói với cô (nếu hỏi *"Tích vô hướng và tích có hướng là gì? Em dùng ở đâu?"*):**
> - **Tích vô hướng (`dot`, `Dòng 59`):** *"Dạ tích vô hướng của 2 vector cho ra một **con số** (`ax*bx + ay*by + az*bz`), dùng để tính **độ dài chiếu** hoặc **góc giữa 2 vector**. Trong bài này em dùng `dot` ở hàm `viewingMatrix4` (`Dòng 182` kiểm tra trục nhìn có bị trùng trục `up` không, và `Dòng 193–195` tính hàng tịnh tiến `-dot(eye, u/v/n)` để dời gốc tọa độ về mắt nhìn), ngoài ra còn dùng để tính độ sáng mặt theo góc ánh sáng ạ."*
> - **Tích có hướng (`cross`, `Dòng 63`):** *"Dạ tích có hướng của 2 vector cho ra một **vector mới vuông góc** với cả 2 vector ban đầu (theo quy tắc bàn tay phải). Trong bài này em dùng `cross` ở **2 chỗ cực quan trọng**:*
>   1. *Ở hàm `viewingMatrix4` (`Dòng 186–187`): dựng trục ngang `u = cross(up, n)` và trục đứng `v = cross(n, u)` để tạo đủ 3 trục vuông góc cho hệ tọa độ quan sát (camera).*
>   2. *Ở hàm `updateMeshNormals` (`Dòng 235`): lấy `cross(v1 - v0, v2 - v0)` của 2 cạnh mặt tam giác để tìm **vector pháp tuyến vuông góc với mặt đa giác** phục vụ đổ bóng ạ."*

---

### 2.2. Mục 4.1 — Phép quay 3D quanh 3 trục $Ox, Oy, Oz$ (`Dòng 114 → 146`)

```cpp
static Mat4 rotX4(double rad) {                 // Quay quanh trục Ox -> giữ nguyên x
    Mat4 r = ident4(); double c = std::cos(rad), s = std::sin(rad);
    r.m[1][1] = c;  r.m[1][2] = s;
    r.m[2][1] = -s; r.m[2][2] = c;
    return r;
}
static Mat4 rotY4(double rad) {                 // Quay quanh trục Oy -> giữ nguyên y (chú ý dấu -sin ở hàng 0 cột 2)
    Mat4 r = ident4(); double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][2] = -s;
    r.m[2][0] = s;  r.m[2][2] = c;
    return r;
}
static Mat4 rotZ4(double rad) {                 // Quay quanh trục Oz -> giữ nguyên z
    Mat4 r = ident4(); double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][1] = s;
    r.m[1][0] = -s; r.m[1][1] = c;
    return r;
}
static Mat4 rotationEuler4(double degX, double degY, double degZ) {
    return rotZ4(degZ * PI / 180.0) * rotY4(degY * PI / 180.0) * rotX4(degX * PI / 180.0); // Nhân gộp 3 ma trận quay
}
```
> **Cách nói với cô:**
> *"Dạ đây là mục 4.1 đúng slide trang 12 ạ: em cài 3 ma trận quay $4 \times 4$ quanh $Ox, Oy, Oz$ (quay quanh trục nào thì trục đó giữ nguyên bằng `1`), rồi hàm `rotationEuler4` nhân gộp 3 ma trận lại thành 1 ma trận duy nhất để xoay trực tiếp vật thể ạ."*

---

### 2.3. Mục 4.2 — 4 Tham số quan sát $\theta, \Phi, R, D$ (Slide trang 49)

| Tham số | Tên gọi & Vị trí trong code | Khi tăng / giảm tham số này thì hình thay đổi thế nào? |
| :---: | :--- | :--- |
| **$\theta$** (`thetaDeg`) | Góc phương vị ngang — `computeEyePosition` (`Dòng 169`) | Xoay điểm nhìn **ngang quanh vật thể** (trái ↔ phải). |
| **$\Phi$** (`phiDeg`) | Góc ngẩng dọc — `computeEyePosition` (`Dòng 170`) | Xoay điểm nhìn **lên trên / xuống dưới** vật thể. |
| **$R$** (`radius` / `eyeRadius`) | Khoảng cách từ gốc $O$ (vật thể) đến mắt nhìn $O'$ (`Dòng 172–174`) | Thay đổi **vị trí quan sát xa hay gần** vật thể. |
| **$D$** (`d` / `projD`) | Khoảng cách từ mắt nhìn đến mặt phẳng chiếu — `projectToScreen` (`Dòng 200, 206`) | **Phóng to / thu nhỏ ảnh** chiếu trên màn hình (như chỉnh tiêu cự ống kính zoom máy ảnh: $D$ càng lớn ảnh càng to). |

---

### 2.4. Tính vị trí mắt nhìn từ $(\theta, \Phi, R)$ — `computeEyePosition` (`Dòng 168 → 176`)

```cpp
static Vec3 computeEyePosition(const Vec3& target, double thetaDeg, double phiDeg, double radius) {
    double theta = thetaDeg * PI / 180.0;                   // Đổi góc ngang theta từ độ sang radian
    double phi   = phiDeg   * PI / 180.0;                   // Đổi góc ngẩng phi từ độ sang radian

    double x = target.x + radius * std::cos(phi) * std::sin(theta); // Tọa độ x của mắt trên mặt cầu bán kính R
    double y = target.y + radius * std::sin(phi);                   // Độ cao y của mắt theo góc ngẩng phi
    double z = target.z + radius * std::cos(phi) * std::cos(theta); // Tọa độ z của mắt trên mặt cầu bán kính R
    return { x, y, z };                                             // Trả về tọa độ điểm nhìn Eye(x, y, z)
}
```
> **Cách nói với cô:**
> *"Dạ hàm này đổi 3 tham số tọa độ cầu $(\theta, \Phi, R)$ sang tọa độ Đề-các `(x, y, z)` của mắt nhìn `eye` nằm trên mặt cầu bán kính $R$ bao quanh vật thể ạ. Khi mình thay đổi $\theta$ thì mắt chạy vòng ngang, thay đổi $\Phi$ thì mắt ngẩng lên/hạ xuống, còn thay đổi $R$ thì mắt tiến lại gần hoặc lùi ra xa vật thể ạ."*

---

### 2.5. Ma trận biến đổi hệ quan sát — `viewingMatrix4` (`Dòng 177 → 197`) *(TRỌNG TÂM 4.2)*

```cpp
static Mat4 viewingMatrix4(const Vec3& eye, const Vec3& target, const Vec3& upGuide = { 0, 1, 0 }) {
    Vec3 n = normalize(eye - target);           // Trục chiều sâu n (hướng từ vật thể về mắt nhìn, chuẩn hóa độ dài = 1)

    Vec3 up = upGuide;                          // Vector hướng lên mặc định là trục Oy (0, 1, 0)
    if (std::fabs(dot(n, up)) > 0.999) {        // Nếu hướng nhìn n gần trùng thẳng đứng với up (lúc nhìn thẳng từ đỉnh xuống)
        up = { 0, 0, -1 };                      // -> Đổi vector phụ sang (0, 0, -1) để tích có hướng không bị bằng 0
    }

    Vec3 u = normalize(cross(up, n));           // Trục ngang u (sang phải camera) = tích có hướng của up và n
    Vec3 v = cross(n, u);                       // Trục đứng v (lên trên camera) = tích có hướng của n và u

    Mat4 M = ident4();                          // Tạo ma trận biến đổi hệ quan sát 4x4 (chuẩn vector hàng)
    M.m[0][0] = u.x;  M.m[0][1] = v.x;  M.m[0][2] = n.x; // Cột 0 là trục u, cột 1 là trục v, cột 2 là trục n (phần quay)
    M.m[1][0] = u.y;  M.m[1][1] = v.y;  M.m[1][2] = n.y;
    M.m[2][0] = u.z;  M.m[2][1] = v.z;  M.m[2][2] = n.z;
    M.m[3][0] = -dot(eye, u);                   // Hàng cuối (hàng 3): tịnh tiến đưa điểm nhìn Eye về gốc (0, 0, 0)
    M.m[3][1] = -dot(eye, v);
    M.m[3][2] = -dot(eye, n);
    return M;                                   // Trả về ma trận hệ quan sát M (WCS -> VCS)
}
```
> **Cách nói với cô:**
> *"Dạ hàm `viewingMatrix4` xây dựng ma trận biến đổi điểm từ hệ tọa độ thế giới (WCS) sang hệ tọa độ quan sát (VCS) có gốc đặt ngay tại mắt nhìn `eye` ạ. Em dựng 3 trục trực chuẩn vuông góc nhau là trục sâu `n`, trục ngang `u = cross(up, n)` và trục đứng `v = cross(n, u)`, xếp vào 3 cột đầu của ma trận để xoay hệ trục, còn hàng cuối đặt `-dot(eye, u/v/n)` để tịnh tiến mắt nhìn về gốc `(0, 0, 0)`. Về bản chất nó tương đương hoàn toàn với tích 4 ma trận $T = A \cdot B \cdot C \cdot D$ trên slide trang 39–45 nhưng gọn và không bị lỗi khóa trục (gimbal lock) khi nhìn thẳng từ trên xuống nhờ đoạn `if (fabs(dot(n, up)) > 0.999)` ở dòng 182 ạ."*

---

### 2.6. Chiếu Song song & Phối cảnh trong hệ quan sát — `projectToScreen` (`Dòng 200 → 209`)

```cpp
static Vec3 projectToScreen(const Vec3& pv, bool isPerspective, double d = 4.0, double zoom = 1.0) {
    if (!isPerspective) {                       // Nếu đang chọn Phép chiếu Song song (slide trang 48)
        return { pv.x * zoom, pv.y * zoom, 0.0 }; // -> Bỏ độ sâu z, giữ nguyên (x0, y0)
    } else {                                    // Ngược lại là Phép chiếu Phối cảnh (slide trang 46 & 48)
        double z = -pv.z;                       // Lấy độ sâu z0 dương (khoảng cách từ điểm đến mắt)
        if (z < 0.2) z = 0.2;                   // Nếu điểm sát mắt quá (< 0.2) -> chặn lại để tránh chia cho 0
        double factor = (d / z) * zoom;         // Tính tỉ lệ phóng đại D / z0
        return { pv.x * factor, pv.y * factor, z }; // Trả về x' = x0 * D / z0 và y' = y0 * D / z0
    }
}
```
> **Cách nói với cô:**
> *"Dạ sau khi đưa vật thể về hệ quan sát có tọa độ `pv(x0, y0, z0)`, hàm `projectToScreen` chiếu lên màn hình 2D đúng theo slide trang 46–48 ạ: nếu là **Song song** thì lấy thẳng `(x0, y0)`; còn nếu là **Phối cảnh** thì nhân thêm tỉ lệ `D / z0`, tức $x' = x_0 \cdot \frac{D}{z_0}$ và $y' = y_0 \cdot \frac{D}{z_0}$. Do đó khi tăng/giảm $D$, ảnh trên màn hình sẽ phóng to hoặc thu nhỏ đúng yêu cầu đề bài ạ."*

---

### 2.7. Tính vector pháp tuyến cho từng mặt đa giác — `updateMeshNormals` (`Dòng 229 → 240`)

```cpp
static void updateMeshNormals(Mesh3D& mesh) {
    for (auto& f : mesh.faces) {                            // Duyệt từng mặt đa giác f trong mô hình
        if (f.vertexIndices.size() >= 3) {                  // Nếu mặt có từ 3 đỉnh trở lên
            Vec3 v0 = mesh.vertices[f.vertexIndices[0]];    // Lấy đỉnh 0
            Vec3 v1 = mesh.vertices[f.vertexIndices[1]];    // Lấy đỉnh 1
            Vec3 v2 = mesh.vertices[f.vertexIndices[2]];    // Lấy đỉnh 2
            f.normal = normalize(cross(v1 - v0, v2 - v0));  // Pháp tuyến = chuẩn hóa tích có hướng 2 cạnh (v1-v0) và (v2-v0)
        } else {
            f.normal = { 0, 1, 0 };                         // Ngược lại gán pháp tuyến mặc định hướng lên
        }
    }
}
```
> **Cách nói với cô:**
> *"Dạ mỗi mặt đa giác cần có 1 **vector pháp tuyến** (vuông góc mặt, độ dài = 1) để OpenGL tính ánh sáng đổ bóng sáng/tối. Ở đây em lấy 3 đỉnh đầu tiên `v0, v1, v2` của mặt tạo ra 2 vector cạnh `(v1 - v0)` và `(v2 - v0)`, lấy tích có hướng `cross` rồi `normalize` về độ dài 1 ạ."*

---

## 3. BỘ HỎI – ĐÁP NHANH KHI VẤN ĐÁP

1. **Nếu cô hỏi: *"Khác nhau giữa mục 4.1 (Quay vật thể) và mục 4.2 (Biến đổi hệ quan sát) là gì?"***
   > *"Dạ ở **mục 4.1**, mắt nhìn đứng yên, còn **bản thân vật thể** xoay quanh các trục $Ox, Oy, Oz$ bằng ma trận `rotX4/rotY4/rotZ4`. Còn ở **mục 4.2**, vật thể đứng yên trong không gian, còn **mắt nhìn (camera)** di chuyển xung quanh vật thể theo các tham số $(\theta, \Phi, R, D)$ thông qua ma trận hệ quan sát `viewingMatrix4` ạ."*

2. **Nếu cô hỏi: *"Trong hệ quan sát, tăng $R$ và tăng $D$ khác nhau thế nào?"***
   > *"Dạ theo đúng slide trang 49: tăng/giảm **$R$** là di chuyển **vị trí mắt nhìn** ra xa hoặc lại gần vật thể (làm thay đổi góc nhìn phối cảnh). Còn tăng/giảm **$D$** là thay đổi **khoảng cách từ mắt đến mặt phẳng chiếu** — tương tự như vặn ống kính **zoom** máy ảnh để phóng to hoặc thu nhỏ ảnh trên màn hình ạ."*

3. **Nếu cô hỏi: *"Tại sao trong `viewingMatrix4` hàng cuối lại là `-dot(eye, u)` có dấu trừ?"***
   > *"Dạ vì mình đang biến đổi hệ tọa độ để đưa vị trí mắt `eye` về gốc `(0, 0, 0)` nên phải **tịnh tiến ngược** một vector `-eye`, rồi chiếu vector `-eye` đó lên 3 trục mới `u, v, n` bằng phép tích vô hướng `dot` ạ."*

---

## 4. KỊCH BẢN DEMO 60 GIÂY TRƯỚC MẶT CÔ (KÈM LỜI NÓI)

1. **Demo 4.1 — Quay 3D quanh $Ox, Oy, Oz$ (15 giây):**
   - Bấm phím **`1`** chọn chế độ `4.1: Quay 3D`.
   - Bấm **`X`, `Y`, `Z`** + **phím mũi tên** (hoặc `Space` tự xoay) để xoay vật thể quanh từng trục; bấm **`+` / `-`** phóng to/thu nhỏ vật thể.
   - **Lời nói:** *"Dạ đây là mục 4.1: em xoay trực tiếp vật thể quanh các trục $Ox, Oy, Oz$ bằng ma trận `rotX4, rotY4, rotZ4` ạ."*
2. **Demo 4.2 — Hệ quan sát với $\theta, \Phi, R, D$ & Chiếu Song song/Phối cảnh (30 giây):**
   - Bấm phím **`2`** chuyển sang chế độ `4.2: Hệ quan sát`.
   - **Kéo chuột trái** (hoặc phím mũi tên) thay đổi góc quan sát **$\theta, \Phi$**; **cuộn chuột** (hoặc `+`/`-`) thay đổi khoảng cách **$R$**; kéo slider chỉnh **$D$** để phóng to/thu nhỏ ảnh.
   - Bấm phím **`P`** chuyển qua lại giữa **Chiếu Phối cảnh** và **Chiếu Song song**.
   - **Lời nói:** *"Dạ sang mục 4.2, em giữ vật thể đứng yên và thay đổi hệ quan sát qua 4 tham số $\theta, \Phi, R, D$ đúng slide trang 49, đồng thời bấm `P` để so sánh ngay phép chiếu phối cảnh (gần to xa nhỏ) và chiếu song song ạ."*
3. **Demo Mô hình Mặt đa giác (15 giây):**
   - Bấm phím **`M`** đổi lần lượt 4 mô hình mặt đa giác (Hình chóp → Lập phương → Ngôi nhà 3D → **Lính đặc nhiệm cầm súng**); bấm **`W`** chuyển qua lại chế độ Mặt đa giác / Khung kết nối / Cả hai.
   - *(Bấm **`R`** để Reset về mặc định bất cứ lúc nào).*
