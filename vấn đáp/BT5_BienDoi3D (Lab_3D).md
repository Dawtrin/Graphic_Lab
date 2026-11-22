# TÀI LIỆU ÔN VẤN ĐÁP — BT5: HÌNH CHÓP ABCD, PHÉP CHIẾU & BIẾN ĐỔI 3D (LAB_3D)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (26/10):**
  1. Biểu diễn **hình chóp $ABCD$** theo **Mô hình Mặt đa giác (Polygon Mesh)** và **Khung kết nối (Wireframe)**.
  2. Thực hiện phép chiếu **Song song** và **Phối cảnh** lên các mặt phẳng phù hợp ($XOY, YOZ, ZOX$).
  3. Áp dụng các phép biến đổi 3D lên hình chóp: **Tịnh tiến**, **Tỉ lệ với tâm là 1 đỉnh của đối tượng**, **Quay quanh trục $Ox, Oy, Oz$**.
  4. Sự kiện phím/chuột: phím `T, S, R` đổi phép biến đổi; phím mũi tên `Up, Down, Left, Right` chỉnh góc quay; click chuột trái/phải tăng/giảm tỉ lệ hoặc xác định vị trí tịnh tiến.
- **Mã nguồn:** `Lab_3D/main.cpp` | **Chạy chương trình:** `Lab_3D/run.bat` hoặc `run_lab3D.bat`
- **Tài liệu đối chiếu:** Slide `CG.4-3D Graphics.pdf` (trang 5–12, 20–26, 28–36).

---

## 1. BẢN ĐỒ TỔNG QUAN FILE `Lab_3D/main.cpp` (THEO SỐ DÒNG)

*(File dài 1.693 dòng chủ yếu là vẽ khung 3D, thanh trượt slider và bảng hiển thị; **toàn bộ phần thuật toán & tương tác cô sẽ hỏi nằm gọn ở `Dòng 75 → 230`, `Dòng 369 → 392` và `Dòng 1437 → 1645`**!)*

| Khoảng dòng | Thành phần / Hàm | Vai trò chính |
| :--- | :--- | :--- |
| **`75 → 103`** | **`struct Mat4`, `ident4()`, `operator*`** | **Ma trận thuần nhất $4 \times 4$, nhân 2 ma trận và nhân điểm 3D với ma trận ($P' = P \times M$).** |
| **`106 → 137`** | **`translation4`, `scaling4`, `scalePivotMat4`** | **Phép Tịnh tiến 3D và Phép Tỉ lệ 3D với tâm là 1 đỉnh $P_0(x_0, y_0, z_0)$ (3 bước).** |
| **`140 → 167`** | **`rotX4`, `rotY4`, `rotZ4`, `rotationEuler4`** | **3 ma trận Quay quanh trục $Ox, Oy, Oz$ và quay tổng hợp (khớp Slide trang 12).** |
| **`183 → 229`** | **`parallelProjMat`, `projectParallel`, `projectPerspective`** | **Phép chiếu Song song trực giao và Phép chiếu Phối cảnh lên $XOY, YOZ, ZOX$ (khớp Slide trang 31–35).** |
| `240 → 335` | `runTests` | 17 test toán học tự động kiểm chứng toàn bộ ma trận và thể tích (chạy `--test`, pass 17/17). |
| **`369 → 392`** | **`origVerts`, `faces`, `edges`** | **Biểu diễn Hình chóp $ABCD$: 4 đỉnh, 4 mặt tam giác (Polygon Mesh) và 6 cạnh (Wireframe).** |
| `778 → 860` | Vẽ hình chóp $ABCD$ | Duyệt `g.edges` vẽ Khung kết nối và duyệt `g.faces` vẽ Mặt đa giác tô màu. |
| **`1437 → 1645`** | **`mouse`, `keyboard`, `special`** | **Bắt sự kiện chuột trái/phải và phím `T, S, R`, mũi tên, `M`, `P`, `1..4`, `V`, `X/Y/Z`.** |

---

## 2. GIẢI THÍCH TỪNG CỤM CODE CỐT LÕI (DỊCH THẲNG TỪNG LỆNH + CÁCH NÓI VỚI CÔ)

### 2.1. Biểu diễn Hình chóp $ABCD$ — Mô hình Khung kết nối & Mặt đa giác (`Dòng 369 → 392`)

```cpp
// Danh sách 4 đỉnh của hình chóp ABCD (tứ diện)
{  0.0,  2.2,  0.0 }, // Đỉnh A (đỉnh chóp trên cùng)
{ -1.8, -1.2,  1.4 }, // Đỉnh B (đáy trước trái)
{  1.8, -1.2,  1.4 }, // Đỉnh C (đáy trước phải)
{  0.0, -1.2, -1.8 }, // Đỉnh D (đáy sau)

// Mô hình MẶT ĐA GIÁC (Polygon Mesh): danh sách 4 mặt tam giác (lưu chỉ số 3 đỉnh của mỗi mặt)
std::vector<std::array<int, 3>> faces = { {0,1,2}, {0,2,3}, {0,3,1}, {1,3,2} }; // ABC, ACD, ADB, BCD

// Mô hình KHUNG KẾT NỐI (Wireframe): danh sách 6 cạnh (lưu chỉ số đỉnh đầu và đỉnh cuối)
std::vector<std::pair<int, int>> edges = { {0,1}, {0,2}, {0,3}, {1,2}, {2,3}, {3,1} }; // AB, AC, AD, BC, CD, DB
```
> **Cách nói với cô:**
> *"Dạ đúng như slide trang 20–26, để biểu diễn hình chóp $ABCD$ em lưu 1 danh sách tọa độ 4 đỉnh $A, B, C, D$. Khi vẽ theo **Mô hình Khung kết nối (Wireframe)** thì em duyệt mảng `edges` gồm 6 cạnh nối từng cặp đỉnh đầu–cuối bằng đoạn thẳng. Còn khi vẽ theo **Mô hình Mặt đa giác (Polygon Mesh)** thì em duyệt mảng `faces` gồm 4 mặt tam giác $ABC, ACD, ADB, BCD$ để tô màu và đổ bóng từng mặt ạ."*

---

### 2.2. Nhân ma trận $4 \times 4$ & Quy ước tọa độ thuần nhất 3D (`Dòng 85 → 103`)

```cpp
static Vec3 operator*(const Vec3& p, const Mat4& M) {
    // Nhân vector hàng [x, y, z, 1] với từng cột 0, 1, 2, 3 của ma trận M
    double x = p.x * M.m[0][0] + p.y * M.m[1][0] + p.z * M.m[2][0] + M.m[3][0];
    double y = p.x * M.m[0][1] + p.y * M.m[1][1] + p.z * M.m[2][1] + M.m[3][1];
    double z = p.x * M.m[0][2] + p.y * M.m[1][2] + p.z * M.m[2][2] + M.m[3][2];
    double w = p.x * M.m[0][3] + p.y * M.m[1][3] + p.z * M.m[2][3] + M.m[3][3];
    if (std::fabs(w) > 1e-9 && std::fabs(w - 1.0) > 1e-7) { // Nếu hệ số w khác 1 (khi chiếu phối cảnh)
        return { x / w, y / w, z / w };                     // -> Chia chuẩn hóa cho w để về tọa độ 3D thực
    }
    return { x, y, z };                                     // Ngược lại (w == 1) -> trả về thẳng (x, y, z)
}
```
> **Cách nói với cô:**
> *"Dạ cũng giống bài 2D, trong không gian 3D em nâng lên hệ tọa độ thuần nhất $4 \times 4$ với điểm là vector hàng $[x,\ y,\ z,\ 1]$ và biến đổi bằng $P' = P \times M$ đúng quy ước slide trang 5 ạ. Nhờ vậy Tịnh tiến, Tỉ lệ, Quay đều gộp thành phép nhân ma trận; còn nếu thành phần thứ 4 là `w` khác 1 (như trong phép chiếu phối cảnh) thì mình chia lại cho `w` để chuẩn hóa tọa độ ạ."*

---

### 2.3. 3 Phép biến đổi 3D (`Dòng 106 → 167`)

#### Cụm 3.1 — Phép Tịnh tiến 3D (`Dòng 106 → 112`)
```cpp
static Mat4 translation4(double tx, double ty, double tz) {
    Mat4 r = ident4();                          // Khởi tạo ma trận đơn vị 4x4
    r.m[3][0] = tx;                             // Đặt tx vào hàng cuối (hàng 3), cột 0
    r.m[3][1] = ty;                             // Đặt ty vào hàng cuối, cột 1
    r.m[3][2] = tz;                             // Đặt tz vào hàng cuối, cột 2
    return r;                                   // Trả về ma trận T(tx, ty, tz)
}
```
> **Cách nói với cô:**
> *"Dạ phép tịnh tiến 3D đặt vector độ dời $(t_x, t_y, t_z)$ vào hàng cuối cùng của ma trận đơn vị $4 \times 4$ (đúng slide trang 6), khi nhân với $[x, y, z, 1]$ sẽ cho ra đúng $(x + t_x,\ y + t_y,\ z + t_z)$ ạ."*

#### Cụm 3.2 — Phép Tỉ lệ quanh 1 đỉnh $P_0$ của hình chóp (`Dòng 118 → 130`)
```cpp
static Mat4 scaling4(double sx, double sy, double sz) {
    Mat4 r = ident4();                          // Khởi tạo ma trận đơn vị 4x4
    r.m[0][0] = sx; r.m[1][1] = sy; r.m[2][2] = sz; // Đặt hệ số tỉ lệ sx, sy, sz trên đường chéo chính
    return r;
}

static Mat4 scalePivotMat4(const Vec3& p0, double sx, double sy, double sz) {
    return translation4(-p0.x, -p0.y, -p0.z)    // Bước 1: Tịnh tiến dời đỉnh tâm P0 về gốc tọa độ O
         * scaling4(sx, sy, sz)                 // Bước 2: Co giãn quanh gốc O theo hệ số (sx, sy, sz)
         * translation4(p0.x, p0.y, p0.z);      // Bước 3: Tịnh tiến dời đỉnh P0 trở lại vị trí ban đầu
}
```
> **Cách nói với cô:**
> *"Dạ đề yêu cầu tỉ lệ với tâm là **1 đỉnh của đối tượng** (ví dụ đỉnh $A$, hoặc bấm `V` để đổi sang $B, C, D$). Theo đúng slide trang 10, em nhân 3 ma trận: dời đỉnh $P_0$ về gốc $O$ → co giãn quanh $O$ → dời trả lại $P_0$. Khi đó đỉnh $P_0$ sẽ đứng yên bất biến, còn cả hình chóp phóng to/thu nhỏ xung quanh đỉnh đó ạ."*

#### Cụm 3.3 — Phép Quay quanh 3 trục $Ox, Oy, Oz$ (`Dòng 140 → 167`)
```cpp
static Mat4 rotX4(double rad) {                 // Quay quanh trục Ox -> giữ nguyên x (m[0][0] = 1)
    Mat4 r = ident4(); double c = std::cos(rad), s = std::sin(rad);
    r.m[1][1] = c;  r.m[1][2] = s;              // Xoay cặp tọa độ (y, z)
    r.m[2][1] = -s; r.m[2][2] = c;
    return r;
}
static Mat4 rotY4(double rad) {                 // Quay quanh trục Oy -> giữ nguyên y (m[1][1] = 1)
    Mat4 r = ident4(); double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][2] = -s;             // Xoay cặp tọa độ (x, z) — chú ý dấu -sin ở hàng 0 cột 2!
    r.m[2][0] = s;  r.m[2][2] = c;
    return r;
}
static Mat4 rotZ4(double rad) {                 // Quay quanh trục Oz -> giữ nguyên z (m[2][2] = 1)
    Mat4 r = ident4(); double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][1] = s;              // Xoay cặp tọa độ (x, y) giống hệt quay 2D
    r.m[1][0] = -s; r.m[1][1] = c;
    return r;
}
```
> **Cách nói với cô:**
> *"Dạ đây là 3 ma trận quay cơ bản quanh 3 trục tọa độ đúng slide trang 12 ạ. Nguyên tắc rất dễ nhớ: quay quanh trục nào thì tọa độ trục đó giữ nguyên (đặt `1` ở cột/hàng trục đó), còn 2 trục còn lại quay ngược chiều kim đồng hồ theo quy tắc bàn tay phải khi nhìn từ nửa trục dương về gốc $O$ ạ."*

---

### 2.4. Phép chiếu Song song & Phép chiếu Phối cảnh (`Dòng 190 → 229`)

#### Cụm 4.1 — Phép chiếu Song song trực giao (`Dòng 190 → 196`)
```cpp
static Mat4 parallelProjMat(ProjPlane pl) {
    Mat4 r = ident4();                          // Khởi tạo ma trận đơn vị 4x4
    if (pl == PLANE_XOY)      r.m[2][2] = 0.0;  // Nếu chiếu lên mặt XOY -> triệt tiêu z (cho z = 0)
    else if (pl == PLANE_YOZ) r.m[0][0] = 0.0;  // Ngược lại nếu chiếu lên YOZ -> triệt tiêu x (cho x = 0)
    else if (pl == PLANE_ZOX) r.m[1][1] = 0.0;  // Ngược lại nếu chiếu lên ZOX -> triệt tiêu y (cho y = 0)
    return r;                                   // Trả về ma trận chiếu song song
}
```
> **Cách nói với cô:**
> *"Dạ theo slide trang 31, phép chiếu song song trực giao chiếu vuông góc điểm xuống mặt phẳng chiếu, nên chiếu lên mặt $XOY$ thì chỉ cần gán `z = 0` (đặt `m[2][2] = 0`), tương tự $YOZ$ cho `x = 0` và $ZOX$ cho `y = 0` ạ."*

#### Cụm 4.2 — Phép chiếu Phối cảnh (`Dòng 199 → 221`)
```cpp
static Vec3 projectPerspective(const Vec3& p, ProjPlane pl, double d = 6.0) {
    Vec3 r = p;
    if (pl == PLANE_XOY) {                      // Nếu chiếu phối cảnh lên mặt XOY (z = 0), mắt đặt tại (0, 0, d)
        double denom = (d - p.z) / d;           // Tính mẫu số (1 - z/d) đúng công thức slide trang 35
        if (std::fabs(denom) < 1e-4) denom = 1e-4; // Nếu điểm quá sát mắt (denom ~ 0) -> chặn tránh chia cho 0
        r.x = p.x / denom;                      // Tính x' = x / (1 - z/d)
        r.y = p.y / denom;                      // Tính y' = y / (1 - z/d)
        r.z = 0.0;                              // Ép ảnh nằm trên mặt phẳng chiếu z = 0
    } // ... tương tự cho PLANE_YOZ (chia theo 1 - x/d) và PLANE_ZOX (chia theo 1 - y/d)
    return r;
}
```
> **Cách nói với cô:**
> *"Dạ phép chiếu phối cảnh em cài đúng công thức tam giác đồng dạng ở slide trang 35: với mặt phẳng chiếu $z = 0$ và tâm chiếu (mắt nhìn) đặt trên trục $Oz$ cách gốc khoảng $d$, ta có $x' = \frac{x}{1 - z/d},\ y' = \frac{y}{1 - z/d},\ z' = 0$. Điểm nào càng gần mắt ($z$ càng lớn) thì mẫu số càng nhỏ nên ảnh càng to; điểm nào ở xa ($z$ âm) thì mẫu số lớn nên ảnh thu nhỏ lại — tạo hiệu ứng xa–gần thực tế ạ."*

---

### 2.5. Tương tác Chuột & Phím (`mouse`: `Dòng 1463 → 1487`, `keyboard/special`: `Dòng 1535 → 1645`)

> **Cách nói với cô nếu hỏi phần chuột/phím:**
> *"Dạ em làm đúng lưu ý đề bài: dùng hàm `keyboard` bắt phím `T, S, R` để đổi giữa 3 chế độ Tịnh tiến, Tỉ lệ, Quay; phím `M` đổi mô hình Khung kết nối / Mặt đa giác; phím `P` đổi chiếu Song song / Phối cảnh; phím `1, 2, 3, 4` đổi mặt phẳng chiếu $XOY, YOZ, ZOX$ và 3D; phím `V` đổi đỉnh làm tâm tỉ lệ ($A, B, C, D$). Trong hàm `special`, các phím mũi tên `Up, Down, Left, Right` dùng để tăng/giảm góc quay (kết hợp `X, Y, Z` chọn trục). Còn trong hàm `mouse`, ở chế độ Tỉ lệ thì click chuột trái tăng hệ số tỉ lệ `+0.2`, click chuột phải giảm `-0.2`; ở chế độ Tịnh tiến thì click chuột trái để xác định vị trí tịnh tiến mới của hình chóp ạ."*

---

## 3. BỘ HỎI – ĐÁP NHANH KHI VẤN ĐÁP

1. **Nếu cô hỏi: *"Khác nhau giữa phép chiếu Song song và phép chiếu Phối cảnh là gì?"***
   > *"Dạ ở **chiếu Song song**, các tia chiếu song song với nhau (coi như tâm chiếu ở vô cực), nên nó giữ nguyên tỉ lệ kích thước giữa các cạnh song song, hay dùng trong vẽ kỹ thuật CAD nhưng không có cảm giác xa–gần. Còn ở **chiếu Phối cảnh**, các tia chiếu hội tụ về 1 tâm chiếu (mắt nhìn), tạo hiệu ứng vật ở gần thì to, ở xa thì nhỏ giống mắt người thật, nên hay dùng trong game và mô phỏng 3D ạ."*

2. **Nếu cô hỏi: *"Khác nhau giữa Mô hình Khung kết nối (Wireframe) và Mặt đa giác (Polygon Mesh)?"***
   > *"Dạ **Khung kết nối** chỉ lưu danh sách đỉnh và danh sách cạnh nên vẽ rất nhẹ, nhưng nhìn xuyên thấu không phân biệt được mặt nào ở trước, mặt nào bị khuất và không tô màu được. **Mặt đa giác** lưu danh sách đỉnh và danh sách các mặt tam giác nên tô màu, chiếu sáng và ẩn mặt khuất (Z-buffer) được ạ."*

3. **Nếu cô hỏi: *"Tại sao khi tỉ lệ hình chóp với hệ số $S$, thể tích lại tăng $S^3$ lần?"***
   > *"Dạ vì trong không gian 3D, cả 3 chiều dài, rộng, cao đều nhân lên $S$ lần nên thể tích mới $V' = V \cdot S_x \cdot S_y \cdot S_z = V \cdot S^3$ ạ (còn phép Tịnh tiến và Quay là phép dời hình nên thể tích giữ nguyên không đổi, em có viết sẵn hàm `tetrahedronVolume` dùng tích hỗn tạp $|(\vec{AB} \times \vec{AC}) \cdot \vec{AD}| / 6$ để kiểm chứng luôn ở bộ `--test` ạ)."*

4. **Nếu cô hỏi: *"Trong `rotY4`, sao dấu `-sin` lại nằm ở hàng 0 cột 2 (`m[0][2] = -s`), ngược với `rotX4` và `rotZ4`?"***
   > *"Dạ vì theo quy tắc bàn tay phải cho thứ tự vòng $X \to Y \to Z \to X$: quanh $Ox$ xoay từ $Y \to Z$, quanh $Oz$ xoay từ $X \to Y$, còn quanh $Oy$ thì chiều dương xoay từ **$Z \to X$** nên dấu `-sin` đảo lên `m[0][2]` và `+sin` ở `m[2][0]` giống đúng slide trang 12 ạ."*

---

## 4. KỊCH BẢN DEMO 60 GIÂY TRƯỚC MẶT CÔ (KÈM LỜI NÓI)

1. **Demo Mô hình Khung kết nối & Mặt đa giác (10 giây):**
   - Bấm phím **`M`** vài lần để chuyển qua lại giữa *Khung kết nối (Wireframe)*, *Mặt đa giác (Polygon)* và *Cả hai*.
   - **Lời nói:** *"Dạ đây là hình chóp $ABCD$ biểu diễn theo cả mô hình khung kết nối 6 cạnh và mô hình mặt đa giác 4 mặt tam giác ạ."*
2. **Demo Phép chiếu Song song & Phối cảnh (15 giây):**
   - Bấm phím **`1`** ($XOY$), **`2`** ($YOZ$), **`3`** ($ZOX$) để hiện hình chiếu phẳng lên từng mặt tọa độ, rồi bấm phím **`P`** chuyển qua lại *Song song ↔ Phối cảnh*.
   - **Lời nói:** *"Dạ em có thể chiếu hình chóp lên từng mặt $XOY, YOZ, ZOX$ và đổi giữa chiếu song song (bỏ 1 tọa độ) với chiếu phối cảnh (chia theo $1 - z/d$) ạ."* → Bấm **`4`** quay lại chế độ 3D.
3. **Demo Phép Tỉ lệ quanh 1 đỉnh (15 giây):**
   - Bấm **`S`** (Tỉ lệ), bấm **`V`** đổi đỉnh làm tâm tỉ lệ (ví dụ đỉnh $A$).
   - **Click chuột trái** vài lần để phóng to, **click chuột phải** để thu nhỏ.
   - **Lời nói:** *"Dạ ở chế độ tỉ lệ em click chuột trái tăng, chuột phải giảm hệ số tỉ lệ; cô thấy đỉnh $A$ đứng yên làm tâm bất biến, còn toàn bộ hình chóp co giãn quanh đỉnh $A$ theo chuỗi 3 ma trận $T(-A) \cdot S \cdot T(A)$ ạ."*
4. **Demo Phép Quay quanh $Ox, Oy, Oz$ & Tịnh tiến (20 giây):**
   - Bấm **`R`** (Quay) → bấm **`X` / `Y` / `Z`** chọn trục rồi bấm **phím mũi tên** để quay hình chóp (hoặc kéo chuột trái xoay camera quan sát).
   - Bấm **`T`** (Tịnh tiến) → **click chuột trái** vào vị trí mới trên khung 3D để dời hình chóp tới đó.
   - **Lời nói:** *"Dạ bấm `R` + mũi tên để quay quanh từng trục $Ox, Oy, Oz$, còn bấm `T` + click chuột để tịnh tiến hình chóp tới vị trí mới ạ."*
   - *(Bấm **`Space`** bất cứ lúc nào để Reset về hình chóp gốc, phím **`0`** để Reset góc nhìn camera).*
