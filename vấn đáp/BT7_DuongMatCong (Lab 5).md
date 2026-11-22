# TÀI LIỆU ÔN VẤN ĐÁP — BT7: ĐƯỜNG VÀ MẶT CONG 3D (LAB 5)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (Lab 5):** Viết chương trình vẽ các đường, mặt cong sau *(các mục **in đậm** là phần in đỏ trong đề, cô sẽ hỏi kỹ nhất!)*:
  - **5.1.** Các mặt có quy tắc: Cylinder, **Cone**, ...
  - **5.2.** Các mặt tròn xoay: Sphere, Ellipsoid, Hyperboloid, **Toroid**, ...
  - **5.3. Đường cong Bezier**
  - **5.4. Mặt cong Bezier**
  - **Yêu cầu:** Dùng sự kiện phím, chuột để điều chỉnh góc quay quan sát, phóng to/nhỏ, thay đổi màu sắc vật thể.
- **Mã nguồn:** `Lab 5/main.cpp` | **Chạy chương trình:** `Lab 5/run.bat` hoặc `run_lab5.bat`
- **Tài liệu đối chiếu:** Slide `CG.5-Curves and Surfaces.pdf` (trang 6–31).

---

## 1. BẢN ĐỒ CÁC HÀM QUAN TRỌNG TRONG `Lab 5/main.cpp` (THEO SỐ DÒNG)

*(File dài 2.243 dòng nhưng hơn 1.500 dòng là vẽ UI/slider/test; **toàn bộ thuật toán cô sẽ hỏi chỉ nằm gọn trong `Dòng 235 → 662`**!)*

| Khoảng dòng | Hàm quan trọng | Mục đề bài & Vai trò |
| :--- | :--- | :--- |
| `235 → 275` | `createCylinder` | **5.1** — Mặt trụ tròn (đường sinh thẳng song song trục $Oy$). |
| **`278 → 322`** | **`createCone`** *(in đỏ)* | **5.1** — **Mặt nón tròn** (đường sinh nối từ đáy lên 1 đỉnh cố định). |
| `394 → 433` | `createSphere`, `createEllipsoid` | **5.2** — Mặt cầu và Mặt Ellipsoid tròn xoay. |
| `436 → 508` | `createHyperboloidOneSheet`, `TwoSheets` | **5.2** — Mặt Hyperboloid 1 tầng & 2 tầng (và Yên ngựa ở `Dòng 325`). |
| **`511 → 529`** | **`createToroid`** *(in đỏ)* | **5.2** — **Mặt xuyến (Toroid / Donut)** quay đường tròn bán kính $r$ quanh trục cách $R$. |
| **`557 → 580`** | **`combination`, `bernstein`, `evaluateBezierCurveBernstein`** *(in đỏ)* | **5.3** — **Đường cong Bezier bằng Đa thức Bernstein** (khớp Slide trang 10 & 14). |
| **`583 → 596`** | **`deCasteljauSteps`** *(in đỏ)* | **5.3** — **Đường cong Bezier bằng thuật toán De Casteljau** (khớp Slide trang 7–8). |
| **`611 → 662`** | **`dBernstein3`, `evaluateBezierSurfacePoint`, `generateBezierSurfaceMesh`** *(in đỏ)* | **5.4** — **Mặt cong Bicubic Bezier $4 \times 4$** (16 điểm kiểm soát + tính pháp tuyến $\frac{\partial S}{\partial v} \times \frac{\partial S}{\partial u}$). |

---

## 2. GIẢI THÍCH TỪNG CỤM CODE QUAN TRỌNG (DỊCH THẲNG TỪNG LỆNH + CÁCH NÓI VỚI CÔ)

### 2.1. Mục 5.1 — Mặt có quy tắc: Mặt trụ (`Dòng 235 → 251`) & Mặt nón (`Dòng 278 → 301`)

```cpp
// MẶT TRỤ (Cylinder - Dòng 241 -> 251): đường tròn đáy bán kính R quét thẳng dọc theo trục y
for (int j = 0; j <= stacks; j++) {
    double v = -H * 0.5 + H * ((double)j / stacks);     // Chia chiều cao từ -H/2 đến +H/2
    for (int i = 0; i <= slices; i++) {
        double u = i * 2.0 * PI / slices;               // Góc xoay u chạy từ 0 đến 2*pi
        Vec3 pos{ R * std::cos(u), v, R * std::sin(u) };// Điểm trên mặt trụ: x = R*cos(u), y = v, z = R*sin(u)
        Vec3 norm{ std::cos(u), 0, std::sin(u) };       // Pháp tuyến hướng ngang vuông góc trục y
    }
}

// MẶT NÓN (Cone - Dòng 288 -> 301): bán kính r giảm tuyến tính từ R (ở đáy s=0) về 0 (ở đỉnh s=1)
for (int j = 0; j <= stacks; j++) {
    double s = (double)j / stacks;                      // Tham số s chạy từ 0 (đáy) lên 1 (đỉnh nón)
    double r = (1.0 - s) * R;                           // Bán kính vòng tròn thu nhỏ dần về 0 tại đỉnh
    double y = -H * 0.5 + s * H;                        // Độ cao tăng đều từ -H/2 lên +H/2
    for (int i = 0; i <= slices; i++) {
        double u = i * 2.0 * PI / slices;               // Góc xoay quanh trục đứng
        Vec3 pos{ r * std::cos(u), y, r * std::sin(u) };// Điểm trên mặt nón: x = (1-s)R*cos(u), z = (1-s)R*sin(u)
        Vec3 norm{ nr * std::cos(u), ny, nr * std::sin(u) }; // Pháp tuyến nghiêng vuông góc mặt bên nón
    }
}
```
> **Cách nói với cô:**
> *"Dạ mặt có quy tắc (Ruled Surface) là mặt tạo ra khi quét một đường thẳng (đường sinh) trong không gian theo công thức tổng quát $P(u, v) = (1-v)P_0(u) + v P_1(u)$ trên slide trang 18 ạ. Với **Mặt trụ**, $P_0(u)$ và $P_1(u)$ là hai đường tròn đáy và nắp cùng bán kính $R$ nên bán kính không đổi; còn với **Mặt nón**, đầu trên co lại thành **1 điểm đỉnh cố định**, nên khi tham số `s` chạy từ `0` lên `1` thì bán kính `r = (1 - s) * R` thu nhỏ dần về `0` ạ. Trong OpenGL trục thẳng đứng là $Y$ (slide là $Z$) nên chỉ hoán đổi vị trí $y$ và $z$ thôi ạ."*

---

### 2.2. Mục 5.2 — Mặt tròn xoay: Mặt cầu (`Dòng 394 → 412`) & Mặt xuyến Toroid (`Dòng 511 → 529`)

```cpp
// MẶT CẦU (Sphere - Dòng 400 -> 410): quay nửa đường tròn bán kính R quanh trục Oy
for (int j = 0; j <= stacks; j++) {
    double v = -PI * 0.5 + PI * ((double)j / stacks);   // Góc vĩ tuyến v chạy từ -pi/2 (cực nam) đến +pi/2 (cực bắc)
    for (int i = 0; i <= slices; i++) {
        double u = i * 2.0 * PI / slices;               // Góc kinh tuyến u xoay vòng 0 -> 2*pi
        Vec3 pos{ R * cos(v) * cos(u), R * sin(v), R * cos(v) * sin(u) }; // Tọa độ điểm trên mặt cầu
    }
}

// MẶT XUYẾN (Toroid - Dòng 517 -> 527): quay đường tròn nhỏ bán kính r (tâm cách trục R) quanh trục Oy
for (int j = 0; j <= stacks; j++) {
    double v = j * 2.0 * PI / stacks;                   // Góc v chạy trọn vòng tiết diện ống (0 -> 2*pi)
    double cv = std::cos(v), sv = std::sin(v);
    for (int i = 0; i <= slices; i++) {
        double u = i * 2.0 * PI / slices;               // Góc u quay cả vòng ống quanh trục Oy (0 -> 2*pi)
        double cu = std::cos(u), su = std::sin(u);
        Vec3 pos{ (R + r * cv) * cu, r * sv, (R + r * cv) * su }; // Bán kính ngang tại góc v là (R + r*cos(v))
        Vec3 norm{ cv * cu, sv, cv * su };              // Pháp tuyến hướng từ tâm ống nhỏ ra ngoài
    }
}
```
> **Cách nói với cô:**
> *"Dạ mặt tròn xoay tạo ra khi quay một đường cong phẳng $C(v)$ quanh một trục cố định theo góc $u \in [0, 2\pi]$ (slide trang 26–29) ạ:*
> - ***Mặt cầu / Ellipsoid:** quay nửa đường tròn / nửa ellipse với $v \in [-\frac{\pi}{2}, \frac{\pi}{2}]$.*
> - ***Mặt xuyến (Toroid):** quay một **đường tròn nhỏ bán kính $r$** có tâm cách trục quay một khoảng $R$. Tại góc $v$ trên ống tròn nhỏ, điểm đó có độ cao là $r\sin(v)$ và khoảng cách tới trục quay là $(R + r\cos(v))$, khi quay góc $u$ quanh trục $Oy$ sẽ ra đúng công thức $x = (R + r\cos v)\cos u,\ y = r\sin v,\ z = (R + r\cos v)\sin u$ ạ."*

---

### 2.3. Mục 5.3 — Đường cong Bezier: Bernstein & De Casteljau (`Dòng 557 → 596`) *(CÔ HỎI RẤT KỸ!)*

#### Cụm 3.1 — Cách 1: Dùng Đa thức Bernstein (`Dòng 557 → 580`)
```cpp
static double combination(int n, int k) {               // Tính tổ hợp C(n, k) = n! / (k! * (n-k)!)
    if (k < 0 || k > n) return 0.0;                     // Nếu k ngoài [0, n] -> trả về 0
    if (k == 0 || k == n) return 1.0;                   // Nếu k = 0 hoặc k = n -> trả về 1
    double res = 1.0;
    for (int i = 1; i <= k; i++) res = res * (n - i + 1) / i; // Tính tích lũy tránh tràn giai thừa
    return res;
}

static double bernstein(int i, int n, double t) {       // Đa thức Bernstein B_i^n(t) (slide trang 10)
    return combination(n, i) * std::pow(1.0 - t, n - i) * std::pow(t, i); // C(n, i) * (1-t)^(n-i) * t^i
}

static Vec3 evaluateBezierCurveBernstein(const std::vector<Vec3>& ctrlPts, double t) {
    int n = (int)ctrlPts.size() - 1;                    // Bậc n = (số điểm kiểm soát) - 1
    Vec3 pt{ 0, 0, 0 };                                 // Khởi tạo điểm tổng bằng (0, 0, 0)
    for (int i = 0; i <= n; i++) {                      // Duyệt từng điểm kiểm soát P_i (i = 0..n)
        double b = bernstein(i, n, t);                  // Tính trọng số (hàm trộn) B_i^n(t)
        pt = pt + ctrlPts[i] * b;                       // Cộng dồn P_i * B_i^n(t) vào điểm kết quả
    }
    return pt;                                          // Trả về điểm P(t) nằm trên đường cong Bezier
}
```
> **Cách nói với cô:**
> *"Dạ cách 1 em cài đúng công thức đa thức Bernstein trên slide trang 10 và 14 ạ: với $n+1$ điểm kiểm soát $P_0 \dots P_n$, mỗi điểm tại tham số $t \in [0, 1]$ là tổng có trọng số $P(t) = \sum_{i=0}^{n} P_i \cdot B_i^n(t)$, trong đó hàm trộn $B_i^n(t) = C_n^i (1-t)^{n-i} t^i$. Em tính tổ hợp $C_n^i$ bằng vòng `for` nhân chia xen kẽ ở hàm `combination` để tránh bị tràn số nguyên như khi tính `n!` ạ."*

#### Cụm 3.2 — Cách 2: Thuật toán De Casteljau (`Dòng 583 → 596`)
```cpp
static std::vector<std::vector<Vec3>> deCasteljauSteps(const std::vector<Vec3>& ctrlPts, double t) {
    std::vector<std::vector<Vec3>> levels;              // Lưu tất cả các thế hệ điểm trung gian để vẽ minh họa
    levels.push_back(ctrlPts);                          // Thế hệ 0 chính là các điểm kiểm soát gốc P0..Pn
    int n = (int)ctrlPts.size();
    for (int k = 1; k < n; k++) {                       // Lặp qua từng thế hệ k = 1 .. n-1
        std::vector<Vec3> nextLevel;                    // Tạo mảng điểm cho thế hệ mới
        const auto& prev = levels.back();               // Lấy mảng điểm của thế hệ trước đó (k - 1)
        for (size_t i = 0; i + 1 < prev.size(); i++) {  // Với mỗi cặp điểm kề nhau P_i và P_{i+1}
            nextLevel.push_back(lerp(prev[i], prev[i + 1], t)); // Nội suy tuyến tính: (1-t)*P_i + t*P_{i+1}
        }
        levels.push_back(nextLevel);                    // Lưu thế hệ mới vào danh sách
    }
    return levels;                                      // Phần tử cuối cùng (chỉ còn 1 điểm) chính là điểm P(t)!
}
```
> **Cách nói với cô:**
> *"Dạ cách 2 em cài thuật toán **De Casteljau** đúng slide trang 7–8 ạ: thay vì tính lũy thừa và tổ hợp, ở mỗi giá trị $t \in [0, 1]$, em cứ nội suy tuyến tính `lerp(P_i, P_{i+1}, t) = (1-t)*P_i + t*P_{i+1}` trên từng cặp điểm kề nhau để tạo ra thế hệ điểm mới ít hơn 1 điểm. Lặp đến khi chỉ còn đúng **1 điểm cuối cùng** thì đó chính là điểm $P(t)$ trên đường cong Bezier — kết quả trùng khớp 100% với Bernstein và còn trực quan hóa được các đoạn nối trung gian chạy động trên màn hình ạ."*

---

### 2.4. Mục 5.4 — Mặt cong Bezier $4 \times 4$ (`Dòng 611 → 662`) *(CÔ HỎI RẤT KỸ!)*

```cpp
static VertexUV evaluateBezierSurfacePoint(const std::array<std::array<Vec3, 4>, 4>& net, double u, double v) {
    Vec3 pt{ 0, 0, 0 }, du{ 0, 0, 0 }, dv{ 0, 0, 0 };   // Khởi tạo điểm S(u,v) và 2 vector tiếp tuyến dS/du, dS/dv

    double bu[4], bv[4], dbu[4], dbv[4];
    for (int i = 0; i < 4; i++) {                       // Tính sẵn 4 đa thức Bernstein bậc 3 và đạo hàm của chúng
        bu[i]  = bernstein(i, 3, u);  bv[i]  = bernstein(i, 3, v);
        dbu[i] = dBernstein3(i, u);   dbv[i] = dBernstein3(i, v);
    }

    for (int i = 0; i < 4; i++) {                       // Tổng kép 2 chiều qua lưới 4x4 = 16 điểm kiểm soát P_{i,j}
        for (int j = 0; j < 4; j++) {
            const Vec3& P = net[i][j];                  // Lấy điểm kiểm soát P_{i, j}
            pt = pt + P * (bu[i] * bv[j]);              // Tọa độ mặt cong: sum P_{i,j} * B_i^3(u) * B_j^3(v)
            du = du + P * (dbu[i] * bv[j]);             // Vector tiếp tuyến theo chiều u: dS/du
            dv = dv + P * (bu[i] * dbv[j]);             // Vector tiếp tuyến theo chiều v: dS/dv
        }
    }

    Vec3 norm = normalize(cross(dv, du));               // Vector pháp tuyến N = chuẩn hóa tích có hướng (dv x du)
    return { pt, norm, u, v };                          // Trả về điểm + pháp tuyến để tô bóng mượt
}
```
> **Cách nói với cô:**
> *"Dạ mặt cong Bezier là mở rộng 2 chiều của đường cong Bezier đúng công thức slide trang 11 ạ: $S(u, v) = \sum_{i=0}^{3}\sum_{j=0}^{3} P_{i,j} \cdot B_i^3(u) \cdot B_j^3(v)$ với lưới kiểm soát $4 \times 4 = 16$ điểm và hai tham số $u, v \in [0, 1]$. Ngoài tọa độ `pt`, em tính thêm đạo hàm riêng theo $u$ (`du`) và theo $v$ (`dv`) rồi lấy tích có hướng `normalize(cross(dv, du))` để ra **vector pháp tuyến chính xác tại từng điểm** cho ánh sáng chiếu mượt trên mặt cong ạ. Hàm `generateBezierSurfaceMesh` (`Dòng 648`) chỉ việc chạy 2 vòng `for` cho $u, v$ từ `0 → 1` để lấy mẫu thành lưới đa giác vẽ lên màn hình ạ."*

---

## 3. BỘ HỎI – ĐÁP NHANH KHI VẤN ĐÁP

1. **Nếu cô hỏi: *"Đường cong Bezier có những tính chất quan trọng nào?"***
   > *"Dạ có 4 tính chất nổi bật ạ: (1) Luôn đi qua **điểm đầu $P_0$** (khi $t=0$) và **điểm cuối $P_n$** (khi $t=1$), còn các điểm ở giữa chỉ kéo cong đường chứ đường cong không nhất thiết đi qua; (2) Tổng các đa thức Bernstein luôn bằng 1 ($\sum B_i^n(t) = 1$) nên đường cong luôn nằm gọn trong **bao lồi** của đa giác kiểm soát; (3) Tiếp tuyến tại $P_0$ cùng hướng $\vec{P_0 P_1}$, tại $P_n$ cùng hướng $\vec{P_{n-1} P_n}$; (4) Khi sửa 1 điểm kiểm soát sẽ làm thay đổi **toàn cục** cả đường cong ạ."*

2. **Nếu cô hỏi: *"So sánh cách tính Bezier bằng Bernstein và De Casteljau?"***
   > *"Dạ hai cách cho ra **cùng một đường cong 100%**. **Bernstein** viết gọn theo công thức tổng trực tiếp, nhưng với bậc $n$ lớn dễ bị sai số số thực khi tính tổ hợp và lũy thừa. **De Casteljau** chỉ dùng phép nội suy tuyến tính `(1-t)A + tB` lặp lại nên **ổn định số học hơn** và rất tiện để chia nhỏ đường cong hoặc vẽ minh họa các đoạn trung gian ạ."*

3. **Nếu cô hỏi: *"Làm sao phân biệt Mặt có quy tắc (5.1) và Mặt tròn xoay (5.2)?"***
   > *"Dạ **Mặt có quy tắc** là mặt mà qua mỗi điểm trên mặt luôn có ít nhất **1 đường thẳng** (đường sinh) nằm trọn trên mặt (như Mặt trụ, Mặt nón, Yên ngựa). Còn **Mặt tròn xoay** được tạo bằng cách lấy một đường cong phẳng **quay quanh 1 trục cố định** (như Mặt cầu, Ellipsoid, Toroid). Có những mặt vừa là mặt có quy tắc vừa là mặt tròn xoay như Mặt trụ, Mặt nón và Hyperboloid 1 tầng ạ."*

4. **Nếu cô hỏi: *"Tại sao công thức mặt trụ/mặt cầu trong code em lại đặt trục đứng là `y` còn trong slide là `z`?"***
   > *"Dạ trong hệ tọa độ mặc định của OpenGL 3D, trục $Y$ là trục hướng lên trên còn trục $Z$ hướng ra màn hình, nên em đặt chiều cao theo `y` để hình dựng thẳng đứng đẹp mắt; bản chất toán học vẫn hoàn toàn giống công thức trong slide chỉ đổi tên 2 trục `y` và `z` thôi ạ."*

---

## 4. KỊCH BẢN DEMO 60 GIÂY TRƯỚC MẶT CÔ (KÈM LỜI NÓI)

- **Chuyển nhanh qua 4 mục bằng phím `Tab`**, **kéo chuột trái xoay góc nhìn**, **cuộn chuột phóng to/nhỏ**, bấm **`C`** đổi 8 màu sắc, bấm **`M`** đổi Solid / Wireframe:
  1. **5.1 — Mặt có quy tắc (10 giây):** Bấm phím **`1`** (Cylinder), **`2`** (**Cone** — *mục in đỏ!*), **`3`** (Yên ngựa) — chỉ rõ các đường sinh thẳng nổi trên mặt.
  2. **5.2 — Mặt tròn xoay (15 giây):** Bấm **`Tab`** sang 5.2 → bấm **`1`** (Sphere), **`2`** (Ellipsoid), **`3`** (Hyperboloid), **`4`** (**Toroid** — *mục in đỏ!*), kéo chuột xoay và bấm **`C`** đổi màu vật thể.
  3. **5.3 — Đường cong Bezier (15 giây):** Bấm **`Tab`** sang 5.3 → bấm **`Space`** cho điểm $B(t)$ và các đoạn nội suy **De Casteljau** chạy động dọc theo đường cong từ $P_0 \to P_3$.
     - **Lời nói:** *"Dạ đây là đường cong Bezier, cô thấy các đoạn nối trung gian của thuật toán De Casteljau chạy theo tham số $t \in [0, 1]$ luôn tiếp tuyến với đường cong ạ."*
  4. **5.4 — Mặt cong Bezier $4 \times 4$ (20 giây):** Bấm **`Tab`** sang 5.4 → bấm **`1..4`** xem 4 mặt mẫu (Sóng, Yên ngựa, Chóp, Máng) rồi click chọn 1 điểm trên bảng $4 \times 4$ kéo slider độ cao $Y$ để uốn mặt cong trực tiếp.
     - **Lời nói:** *"Dạ đây là mặt cong Bicubic Bezier 16 điểm kiểm soát $4 \times 4$, khi em thay đổi tọa độ 1 điểm kiểm soát thì mặt cong uốn theo ngay lập tức và pháp tuyến $\frac{\partial S}{\partial v} \times \frac{\partial S}{\partial u}$ cũng tự cập nhật để ánh sáng vẫn mượt ạ."*
