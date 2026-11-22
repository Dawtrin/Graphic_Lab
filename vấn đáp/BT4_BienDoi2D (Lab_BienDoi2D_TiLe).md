# TÀI LIỆU ÔN VẤN ĐÁP — BT4: CÁC PHÉP BIẾN ĐỔI 2D (LAB_BIENDOI2D_TILE)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (Lab 3 — Biến đổi 2D):** Cài đặt 4 phép biến đổi 2D và áp dụng tìm ảnh của **tam giác $ABC$**:
  1. **Tịnh tiến** (Translation)
  2. **Tỉ lệ với tâm tỉ lệ $(x_0, y_0)$** (Scaling)
  3. **Quay với tâm quay $(x_0, y_0)$** (Rotation)
  4. **Đối xứng qua đường thẳng $y = mx + b$** (Reflection — *in đỏ trong đề, cực dễ bị hỏi!*)
  - **Yêu cầu:** Dùng sự kiện **click chuột** để xác định vector tịnh tiến, tâm tỉ lệ, tâm quay, đường thẳng đối xứng.
- **Mã nguồn:** `Lab_BienDoi2D_TiLe/scalling_lab.cpp` | **Chạy chương trình:** `Lab_BienDoi2D_TiLe/run.bat`
- **Tài liệu đối chiếu:** Slide `CG.3-2D Graphics.pdf` (trang 104–134).

---

## 1. BẢN ĐỒ TỔNG QUAN FILE `Lab_BienDoi2D_TiLe/scalling_lab.cpp` (THEO SỐ DÒNG)

*(File dài 1402 dòng nhưng phần vẽ UI/slider chiếm hơn 900 dòng; **phần cốt lõi cô sẽ hỏi nằm gọn ở `Dòng 43 → 141` và `Dòng 1211 → 1299`**!)*

| Khoảng dòng | Thành phần / Hàm | Vai trò chính |
| :--- | :--- | :--- |
| **`43 → 64`** | **`struct Pt`, `struct Mat3`, `ident()`, `operator*`** | **Cấu trúc điểm 2D, ma trận $3 \times 3$, nhân 2 ma trận và nhân điểm với ma trận ($Q = P \times M$).** |
| **`67 → 93`** | **`translation`, `scaling`, `rotation`, `reflectOx`** | **4 ma trận biến đổi cơ sở quanh gốc tọa độ $O$ (hệ tọa độ thuần nhất, quy ước vector hàng VKU).** |
| **`96 → 134`** | **`translateMat`, `scalePivotMat`, `rotatePivotMat`, `reflectLineMat`** | **4 phép biến đổi tổng hợp đúng đề bài (kèm công thức trực tiếp `Direct` để đối chiếu).** |
| `136 → 141` | `applyM` | Áp dụng ma trận `M` lên tất cả các đỉnh của tam giác $ABC$ (hoặc đa giác) để ra ảnh $A'B'C'$. |
| `180 → 188` | `presetPts` | Dữ liệu hình mẫu: mặc định số `0` là **Tam giác $ABC$** `{(1,1), (5,1), (3,4)}`. |
| `191 → 290` | `runTests` | 33 bài test toán học tự động kiểm chứng toàn bộ ma trận (chạy bằng `--test`, pass 33/33). |
| **`1211 → 1299`** | **`mouse`** | **Bắt sự kiện click chuột để chọn vector tịnh tiến, tâm tỉ lệ, tâm quay và 2 điểm tạo đường thẳng $y = mx + b$.** |
| `1326 → 1395` | `keyboard`, `special`, `main` | Phím tắt chuyển phép biến đổi, chế độ xem từng bước (`Space`/`H`) và vòng lặp GLUT. |

---

## 2. GIẢI THÍCH TỪNG CỤM CODE CỐT LÕI (DỊCH THẲNG TỪNG LỆNH + CÁCH NÓI VỚI CÔ)

### 2.1. Nhân ma trận $3 \times 3$ & Nhân điểm với ma trận (`Dòng 52 → 64`)

```cpp
static Mat3 operator*(const Mat3& a, const Mat3& b) {
    Mat3 r{};                                       // Tạo ma trận kết quả 3x3 ban đầu toàn số 0
    for (int i = 0; i < 3; i++)                     // Duyệt từng hàng i của ma trận a
        for (int j = 0; j < 3; j++)                 // Duyệt từng cột j của ma trận b
            for (int k = 0; k < 3; k++)             // Tính tổng tích từng phần tử tương ứng
                r.m[i][j] += a.m[i][k] * b.m[k][j]; // Cộng dồn a[i][k] * b[k][j] vào ô r[i][j]
    return r;                                       // Trả về ma trận tích a * b
}

static Pt operator*(const Pt& p, const Mat3& M) {
    return {
        p.x * M.m[0][0] + p.y * M.m[1][0] + M.m[2][0], // Tính x' = [x y 1] nhân với cột 0 của M
        p.x * M.m[0][1] + p.y * M.m[1][1] + M.m[2][1]  // Tính y' = [x y 1] nhân với cột 1 của M
    };
}
```
> **Hiểu nhanh bản chất (để không bao giờ bị bí khi cô hỏi *"Nhân ma trận là sao?"*):**
> 1. **Nhân điểm với ma trận (`p * M`) = Hàng nhân Cột:** Viết điểm $P(x, y)$ thành hàng `[x, y, 1]`:
>    - $x'$ = lấy `[x, y, 1]` nhân lần lượt **Cột 0** rồi cộng lại: $x' = x \cdot m_{00} + y \cdot m_{10} + 1 \cdot m_{20}$.
>    - $y'$ = lấy `[x, y, 1]` nhân lần lượt **Cột 1** rồi cộng lại: $y' = x \cdot m_{01} + y \cdot m_{11} + 1 \cdot m_{21}$.
>    - *Ví dụ Tịnh tiến $(t_x=3, t_y=2)$:* Hàng cuối của ma trận là `[3, 2, 1]` → khi nhân với số `1` ở cuối `[x, y, 1]` sẽ ra đúng $x' = x + 3,\ y' = y + 2$. Đó là lý do phải thêm số `1` vào cuối!
> 2. **Tại sao nhân gộp nhiều ma trận (`M1 * M2 * M3`)?** Nhờ **tính kết hợp** $((P \times M_1) \times M_2) \times M_3 = P \times (M_1 \times M_2 \times M_3)$, ta nhân gộp sẵn tất cả các bước thành **1 ma trận tổng hợp `M` duy nhất**, rồi sau đó mỗi đỉnh $A, B, C$ chỉ cần nhân 1 phát `p * M` là ra ngay ảnh $A'B'C'$.
>
> **Cách nói với cô:**
> *"Dạ khi viết điểm dạng hàng $[x, y, 1]$ trong hệ tọa độ thuần nhất $3 \times 3$, cả Tịnh tiến, Tỉ lệ, Quay và Đối xứng đều quy về cùng một phép toán là **nhân ma trận** ạ. Nhờ tính kết hợp $((P \times M_1) \times M_2) \times M_3 = P \times (M_1 \times M_2 \times M_3)$, em nhân gộp các bước thành 1 ma trận tổng hợp `M` duy nhất trước, rồi chỉ cần lấy từng đỉnh $A, B, C$ nhân với `M` là ra ngay tam giác ảnh $A'B'C'$ ạ."*

---

### 2.2. 4 Ma trận cơ sở (`Dòng 67 → 93`)

```cpp
static Mat3 translation(double tx, double ty) {     // Ma trận Tịnh tiến theo vector (tx, ty)
    Mat3 r = ident();                               // Khởi tạo ma trận đơn vị I
    r.m[2][0] = tx;                                 // Đặt tx vào hàng cuối, cột 0 (quy ước vector hàng)
    r.m[2][1] = ty;                                 // Đặt ty vào hàng cuối, cột 1
    return r;                                       // Trả về ma trận T(tx, ty)
}

static Mat3 scaling(double sx, double sy) {         // Ma trận Tỉ lệ quanh gốc tọa độ O
    Mat3 r = ident();                               // Khởi tạo ma trận đơn vị I
    r.m[0][0] = sx;                                 // Đặt hệ số co giãn theo trục x là sx
    r.m[1][1] = sy;                                 // Đặt hệ số co giãn theo trục y là sy
    return r;                                       // Trả về ma trận S(sx, sy)
}

static Mat3 rotation(double rad) {                  // Ma trận Quay quanh gốc O một góc rad (ngược kim đồng hồ)
    Mat3 r = ident();                               // Khởi tạo ma trận đơn vị I
    double c = std::cos(rad), s = std::sin(rad);    // Tính sẵn cos(theta) và sin(theta)
    r.m[0][0] = c;  r.m[0][1] = s;                  // Hàng 0: [ cos, sin, 0]
    r.m[1][0] = -s; r.m[1][1] = c;                  // Hàng 1: [-sin, cos, 0] (đúng slide trang 121)
    return r;                                       // Trả về ma trận R(theta)
}

static Mat3 reflectOx() {                           // Ma trận Đối xứng qua trục hoành Ox
    Mat3 r = ident();                               // Khởi tạo ma trận đơn vị I
    r.m[1][1] = -1.0;                               // Đổi dấu tung độ y -> -y, giữ nguyên hoành độ x
    return r;                                       // Trả về ma trận đối xứng Ox (đúng slide trang 122)
}
```
> **Cách nói với cô:**
> *"Dạ đây là 4 ma trận cơ bản nhất quanh gốc tọa độ $O$ đúng chuẩn trang 121–122 trên slide ạ. Vì mình dùng vector hàng nên ở phép tịnh tiến, $(t_x, t_y)$ nằm ở hàng cuối cùng `m[2][0]` và `m[2][1]`; còn phép quay ngược chiều kim đồng hồ có hàng 0 là `cos, sin` và hàng 1 là `-sin, cos`; đối xứng qua trục $Ox$ thì chỉ cần đổi `m[1][1] = -1` để lật $y$ thành $-y$ ạ."*

---

### 2.3. Chuỗi ma trận cho 4 phép biến đổi đề bài (`Dòng 96 → 141`) *(CÔ HỎI KỸ NHẤT!)*

#### Cụm 3.1 — Phép Tịnh tiến (`Dòng 96 → 98` trong `scalling_lab.cpp`)
```cpp
static Mat3 translateMat(double tx, double ty) {
    return translation(tx, ty);                     // Chỉ cần 1 bước tịnh tiến theo vector (tx, ty)
}
```
> **Cách nói với cô:**
> *"Dạ phép tịnh tiến đơn giản nhất, chỉ cần gọi đúng 1 ma trận `translation(tx, ty)` với $(t_x, t_y)$ đặt ở hàng cuối cùng là dịch toàn bộ tam giác $ABC$ đi theo vector $\vec{v} = (t_x, t_y)$ ạ."*

#### Cụm 3.2 — Phép Tỉ lệ quanh tâm $(x_0, y_0)$ (`Dòng 104 → 106` trong `scalling_lab.cpp`)
```cpp
static Mat3 scalePivotMat(Pt t, double sx, double sy) {
    return translation(-t.x, -t.y)                  // Bước 1: Dời tâm tỉ lệ T về gốc tọa độ O
         * scaling(sx, sy)                          // Bước 2: Co giãn quanh gốc O theo hệ số (sx, sy)
         * translation(t.x, t.y);                   // Bước 3: Dời tâm trở lại vị trí cũ T(x0, y0)
}
```
> **Cách nói với cô:**
> *"Dạ vì công thức tỉ lệ cơ bản chỉ co giãn quanh gốc $O$, nên muốn tỉ lệ quanh một tâm $T(x_0, y_0)$ bất kỳ em nhân 3 ma trận: đầu tiên tịnh tiến `(-t.x, -t.y)` để dời tâm $T$ về gốc $O$, tiếp theo co giãn bằng `scaling(sx, sy)`, cuối cùng tịnh tiến `(t.x, t.y)` để trả hình về lại đúng tâm cũ ạ."*

#### Cụm 3.3 — Phép Quay quanh tâm $(x_0, y_0)$ (`Dòng 112 → 114` trong `scalling_lab.cpp`)
```cpp
static Mat3 rotatePivotMat(Pt t, double rad) {
    return translation(-t.x, -t.y)                  // Bước 1: Dời tâm quay T về gốc tọa độ O
         * rotation(rad)                            // Bước 2: Quay quanh gốc O một góc rad
         * translation(t.x, t.y);                   // Bước 3: Dời tâm quay trở lại vị trí cũ T(x0, y0)
}
```
> **Cách nói với cô:**
> *"Dạ phép quay quanh tâm $T(x_0, y_0)$ cũng làm y hệt 3 bước: dời tâm quay $T$ về gốc $O$ bằng `translation(-t.x, -t.y)`, quay quanh $O$ góc `rad` bằng `rotation(rad)`, rồi dời trả lại vị trí cũ bằng `translation(t.x, t.y)` ạ."*

#### Cụm 3.4 — Phép Đối xứng qua đường thẳng $y = mx + b$ (`Dòng 123 → 126` trong `scalling_lab.cpp`) *(QUAN TRỌNG NHẤT!)*
```cpp
static Mat3 reflectLineMat(double m, double b) {
    double alpha = std::atan(m);                    // Tính góc nghiêng alpha của đường thẳng so với trục Ox
    return translation(0, -b)                       // Bước 1: Tịnh tiến xuống -b để đường thẳng đi qua gốc O
         * rotation(-alpha)                         // Bước 2: Quay góc -alpha để đường thẳng nằm trùng lên trục Ox
         * reflectOx()                              // Bước 3: Lấy đối xứng cơ bản qua trục Ox (lật y -> -y)
         * rotation(alpha)                          // Bước 4: Quay ngược góc +alpha trả lại góc nghiêng ban đầu
         * translation(0, b);                       // Bước 5: Tịnh tiến lên lại +b trả về vị trí cũ
}
```
> **Cách nói với cô:**
> *"Dạ phép đối xứng qua đường thẳng $y = mx + b$ em dùng chuỗi **5 ma trận**: bước 1 tịnh tiến `translation(0, -b)` để kéo đường thẳng đi qua gốc $O$; bước 2 tính góc nghiêng $\alpha = \arctan(m)$ rồi quay `rotation(-alpha)` để xoay đường thẳng nằm trùng khít lên trục $Ox$; bước 3 lấy đối xứng qua trục $Ox$ bằng `reflectOx()`; bước 4 và 5 làm ngược lại là quay trả `rotation(alpha)` và tịnh tiến lên lại `translation(0, b)` để đưa hình về đúng hệ trục ban đầu ạ."*

#### Cụm 3.5 — Áp ma trận lên Tam giác $ABC$ (`Dòng 136 → 141` trong `scalling_lab.cpp`)
```cpp
static std::vector<Pt> applyM(const std::vector<Pt>& v, const Mat3& M) {
    std::vector<Pt> r;                              // Tạo mảng lưu tọa độ các đỉnh ảnh A'B'C'
    for (const auto& p : v) r.push_back(p * M);     // Với mỗi đỉnh p của tam giác gốc -> nhân p * M rồi lưu vào r
    return r;                                       // Trả về danh sách đỉnh ảnh A'B'C'
}
```
> **Cách nói với cô:**
> *"Dạ sau khi tính xong ma trận tổng hợp `M` ở trên, em chỉ cần gọi `applyM` duyệt qua từng đỉnh $A, B, C$ của tam giác gốc, nhân `p * M` rồi lưu vào mảng `r` để vẽ ra tam giác ảnh $A'B'C'$ ạ."*

---

### 2.4. Tương tác click chuột (`mouse`: `Dòng 1241 → 1285`)

```cpp
Pt w = s2w(g.mx, g.my);                             // Đổi tọa độ pixel màn hình sang tọa độ toán học (x, y)

if (g.transType == TT_TRANSLATE) {                  // Nếu đang ở chế độ Tịnh tiến
    if (g.clickStep == 0) {                         // Nếu là click lần 1 -> lưu điểm gốc vector
        g.clickTempPt = w; g.vStart = w; g.clickStep = 1;
    } else {                                        // Ngược lại (click lần 2) -> tính vector v = điểm ngọn - điểm gốc
        g.tx = std::round((w.x - g.clickTempPt.x) * 20.0) / 20.0;
        g.ty = std::round((w.y - g.clickTempPt.y) * 20.0) / 20.0;
        g.clickStep = 0;
    }
} else if (g.transType == TT_SCALE) {               // Nếu đang ở chế độ Tỉ lệ -> click 1 lần đặt tâm tỉ lệ (x0, y0)
    g.scalePivX = w.x; g.scalePivY = w.y;
} else if (g.transType == TT_ROTATE) {              // Nếu đang ở chế độ Quay -> click 1 lần đặt tâm quay (x0, y0)
    g.rotPivX = w.x; g.rotPivY = w.y;
} else if (g.transType == TT_REFLECT) {             // Nếu đang ở chế độ Đối xứng qua y = mx + b
    if (g.clickStep == 0) {                         // Nếu là click lần 1 -> lưu điểm P1 của đường thẳng
        g.clickTempPt = w; g.lineP1 = w; g.clickStep = 1;
    } else {                                        // Ngược lại (click lần 2) -> lưu P2 rồi tính hệ số m và b
        g.lineP2 = w;
        if (std::fabs(g.lineP2.x - g.clickTempPt.x) < 1e-3) g.lineP2.x += 0.25; // Nếu thẳng đứng thì dịch nhẹ tránh chia 0
        g.lineM = (g.lineP2.y - g.clickTempPt.y) / (g.lineP2.x - g.clickTempPt.x); // m = (y2 - y1) / (x2 - x1)
        g.lineB = g.clickTempPt.y - g.lineM * g.clickTempPt.x;                     // b = y1 - m * x1
        g.clickStep = 0;
    }
}
```
> **Cách nói với cô:**
> *"Dạ ở hàm `mouse`, trước hết em đổi tọa độ chuột màn hình sang tọa độ toán học thực bằng hàm `s2w` (screen-to-world). Tùy phép biến đổi đang chọn: Tịnh tiến thì click 2 điểm lấy hiệu ra $(t_x, t_y)$; Tỉ lệ và Quay thì click 1 điểm đặt trực tiếp tâm $(x_0, y_0)$; còn Đối xứng thì click 2 điểm $P_1, P_2$ rồi tính $m = \frac{y_2 - y_1}{x_2 - x_1}$ và $b = y_1 - m x_1$ để ra phương trình đường thẳng $y = mx + b$ (có chặn trường hợp 2 điểm thẳng đứng khỏi bị chia cho 0) ạ."*

---

## 3. BỘ HỎI – ĐÁP NHANH KHI VẤN ĐÁP

1. **Nếu cô hỏi: *"Tại sao phải dùng hệ tọa độ thuần nhất $3 \times 3$ mà không dùng ma trận $2 \times 2$?"***
   > *"Dạ vì trong không gian 2D, phép Quay và Tỉ lệ viết được thành phép nhân ma trận $2 \times 2$, nhưng phép Tịnh tiến lại là phép cộng $Q = P + T$. Khi nâng lên hệ tọa độ thuần nhất $3 \times 3$ với điểm $[x\ y\ 1]$, cả Tịnh tiến cũng trở thành phép nhân ma trận, nhờ đó mình gộp nhiều phép biến đổi liên tiếp (như 5 bước đối xứng qua $y = mx + b$) thành duy nhất 1 ma trận tích $M$ rồi nhân 1 lần với từng đỉnh ạ."*

2. **Nếu cô hỏi: *"Thứ tự nhân các ma trận có đổi chỗ cho nhau được không? Ví dụ đổi $T(-x_0, -y_0) \times R(\theta) \times T(x_0, y_0)$?"***
   > *"Dạ **không** ạ, vì phép nhân ma trận không có tính giao hoán ($M_1 \times M_2 \neq M_2 \times M_1$). Với quy ước vector hàng $P \times M_1 \times M_2 \times M_3$, phép nào thực hiện trước phải đặt bên trái, phép thực hiện sau đặt bên phải. Nếu đảo thứ tự thì tâm quay sẽ bị lệch sai vị trí ngay ạ."*

3. **Nếu cô hỏi: *"Tại sao trong phép đối xứng, bước 2 phải quay góc $-\alpha$ rồi bước 4 mới quay $+\alpha$?"***
   > *"Dạ vì góc quay dương trong hệ tọa độ Descartes là ngược chiều kim đồng hồ. Đường thẳng $y = mx + b$ đang tạo với trục $Ox$ một góc $\alpha = \arctan(m)$, nên muốn xoay nó nằm trùng xuống trục $Ox$ thì phải quay theo chiều ngược lại một góc $-\alpha$; sau khi đối xứng xong mới quay lại $+\alpha$ để trả về góc nghiêng ban đầu ạ."*

4. **Nếu cô hỏi: *"Nếu $S_x = -1, S_y = 1$ trong phép Tỉ lệ thì ra hình gì?"***
   > *"Dạ nó trở thành phép đối xứng (lật) qua trục tung đi qua tâm tỉ lệ ạ; tương tự $S_x = 1, S_y = -1$ là đối xứng qua trục ngang, còn $|S_x|, |S_y| < 1$ là thu nhỏ, $> 1$ là phóng to ạ."*

---

## 4. KỊCH BẢN DEMO 60 GIÂY TRƯỚC MẶT CÔ (KÈM LỜI NÓI)

- **Bước 0 — Chuẩn bị (2 giây):**
  - Ở mục **3. CHỌN HÌNH HỌC**, bấm **`Tam giac ABC`** (phím `1`) cho đúng đề bài.
  - Ở mục **4. XEM TỪNG BƯỚC PIPELINE**, bấm **`Tat (Off)`** để hiện đồng thời Tam giác gốc $ABC$ (xám) và Tam giác ảnh $A'B'C'$ (xanh ngọc).

- **Bước 1 — Demo Tịnh tiến (10 giây):**
  - Bấm **`Tinh Tien`** (phím `T`) → **Click 2 điểm** trên lưới (điểm 1 là gốc mũi tên, điểm 2 là ngọn mũi tên).
  - **Lời nói:** *"Dạ đây là vector tịnh tiến em vừa click, tam giác $A'B'C'$ dịch theo đúng vector đó; ma trận $M$ ở mục 6 có $(t_x, t_y)$ ở hàng cuối và tọa độ ảnh $A', B', C'$ cập nhật trực tiếp ở mục 5 ạ."*

- **Bước 2 — Demo Tỉ lệ quanh tâm $(x_0, y_0)$ (15 giây):**
  - Bấm **`Ti Le (x0,y0)`** (phím `S`) → **Click 1 điểm** trên lưới (hoặc bấm **`Tam la dinh A`**) → kéo slider `Sx, Sy` (ví dụ `1.5`).
  - Bấm **`Space`** đi qua từng bước rồi bấm **`Tat (Off)`**.
  - **Lời nói:** *"Dạ khi bấm Space sẽ thấy rõ 3 bước nhân ma trận: dời tâm $T$ về gốc $O$ → co giãn quanh $O$ → dời trả lại đúng tâm $T$ ạ."*

- **Bước 3 — Demo Quay quanh tâm $(x_0, y_0)$ (15 giây):**
  - Bấm **`Quay (x0,y0)`** (phím `R`) → **Click 1 điểm** đặt tâm quay $T(x_0, y_0)$ → kéo slider **Góc quay** (ví dụ `90°`).
  - **Lời nói:** *"Dạ tam giác quay quanh đúng tâm em vừa click, tọa độ ảnh $A'B'C'$ và ma trận quay $3 \times 3$ hiển thị ngay bên phải ạ."*

- **Bước 4 — Demo Đối xứng qua đường thẳng $y = mx + b$ (20 giây — *ăn điểm nhất!*):**
  - Bấm **`Doi Xung y=mx+b`** (phím `M`) → **Click 2 điểm** trên lưới để vẽ đường thẳng nghiêng bất kỳ.
  - Bấm **`Space`** đi từng bước từ `1/6` → `6/6` vừa bấm vừa nói:
  - **Lời nói:** *"Dạ đây là chuỗi 5 ma trận đối xứng: bước 1 kéo đường thẳng qua gốc $O$ → bước 2 xoay $-\alpha$ trùng trục $Ox$ → bước 3 lật đối xứng qua $Ox$ → bước 4 xoay trả $+\alpha$ → bước 5 đẩy lên lại $+b$ là ra đúng ảnh $A'B'C'$ ạ."*

- *(Mẹo: Nếu hình bị chạy ra ngoài khung nhìn, bấm **`Fit View`** ở góc trên phải hoặc phím **`F`** để tự căn giữa lại.)*

---

## 5. PHÍM TẮT DEMO NHANH TRƯỚC MẶT CÔ
- **`T`**: Tịnh tiến (Click 2 điểm tạo vector $\vec{v}$).
- **`S`**: Tỉ lệ quanh tâm $(x_0, y_0)$ (Click đặt tâm, kéo slider chỉnh $S_x, S_y$).
- **`Q` / `R`**: Quay quanh tâm $(x_0, y_0)$ (Click đặt tâm, kéo slider chỉnh góc $\theta$).
- **`M` / `D`**: Đối xứng qua đường thẳng $y = mx + b$ (Click 2 điểm tạo đường thẳng).
- **`Space` / `Backspace` / `H`**: **Chế độ xem từng bước (Step-by-step)** — cực kỳ ấn tượng khi demo cho cô xem từng bước trong 3 bước Quay/Tỉ lệ hoặc 5 bước Đối xứng!
- **`1` → `5`**: Đổi hình mẫu (`1` là Tam giác $ABC$ đề bài, `2` là ngôi nhà Slide 113).
- **`F`**: Căn giữa lại khung nhìn | **Cuộn chuột**: Zoom | **`Shift` + kéo chuột**: Di chuyển khung nhìn.
