# TÀI LIỆU ÔN VẤN ĐÁP — BT1: BRESENHAM & MIDPOINT (LAB 2)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (14/9):** Sử dụng thuật toán **Bresenham** và **Midpoint** để cài đặt:
  1. Vẽ **Đường tròn** (Bresenham — có kèm Midpoint để so sánh)
  2. Vẽ **Ellipse** (Midpoint)
  3. Vẽ **Parabol** (Bresenham)
  - **Yêu cầu:** Sử dụng sự kiện **click chuột** để xác định các tham số (tâm, bán kính, trục lớn/nhỏ, đỉnh, tiêu cự).
- **Mã nguồn:** `Lab_2/lab2.cpp` | **Chạy chương trình:** `run_lab2.bat` hoặc `Lab_2/run.bat`
- **Tài liệu đối chiếu:** Slide `CG.3-2D Graphics.pdf` (trang 19–47).

---

## 1. BẢN ĐỒ TỔNG QUAN FILE `Lab_2/lab2.cpp` (THEO SỐ DÒNG)

| Khoảng dòng | Thành phần / Hàm | Vai trò chính |
| :--- | :--- | :--- |
| `17 → 55` | `enum ToolMode`, `struct Point2D`, `struct Shape`, `struct UIButton` | Định nghĩa cấu trúc dữ liệu: 1 điểm pixel `(x, y)`, 1 hình hoàn chỉnh (tâm/đỉnh, tham số `p1, p2`, `vector<Point2D> points`). |
| `60 → 80` | Biến toàn cục (`shapes`, `currentMode`, `isDrawing`, `startX/Y`, `currentX/Y`) | Lưu danh sách hình đang vẽ và trạng thái tương tác chuột. |
| `91 → 150` | `drawText`, `drawRect`, `drawGlowMarker` | Hàm phụ trợ vẽ chữ, khung giao diện, dấu tâm/đỉnh. |
| **`162 → 202`** | **`computeCircleBresenham`** | **Thuật toán Bresenham vẽ Đường tròn (8-way symmetry).** |
| `205 → 243` | `computeCircleMidpoint` | Thuật toán Midpoint vẽ Đường tròn (8-way symmetry). |
| **`253 → 312`** | **`computeEllipseMidpoint`** | **Thuật toán Midpoint vẽ Ellipse (2 vùng, 4-way symmetry).** |
| **`329 → 373`** | **`computeParabolaBresenham`** | **Thuật toán Bresenham vẽ Parabol (2 vùng, 2-way symmetry).** |
| `378 → 403` | `renderShape` | Duyệt `s.points` gọi `glVertex2i` vẽ lên màn hình (`GL_LINE_LOOP`/`GL_LINE_STRIP` hoặc `GL_POINTS`). |
| `408 → 474` | `loadPresetMode` | Tạo sẵn các hình mẫu chuẩn cho nút `[1]`, `[2]`, `[3]`, `[4]`. |
| `479 → 703` | `initButtons`, `renderUI`, `drawCardForShape`, `drawGridAndLayout` | Vẽ thanh công cụ, lưới tọa độ và thẻ công thức. |
| `708 → 796` | `display` | Hàm vẽ chính của OpenGL (`gluOrtho2D`, vẽ hình đã chốt + live preview khi đang vẽ + UI). |
| `813 → 848` | `handleButtonClick` | Xử lý bấm nút / phím tắt đổi hình, xóa hết, bật tắt lưới & thông số. |
| **`854 → 956`** | **`mouseClick`** | **Bắt sự kiện click chuột chọn tâm/đỉnh và chốt hình.** |
| `958 → 985` | `mouseMotion`, `mousePassiveMotion` | Cập nhật tọa độ chuột liên tục để vẽ live preview và hover nút. |
| `990 → 1060` | `keyboard`, `specialKeys` | Bắt phím tắt `1..4, C, G, I, +, -, ESC` và `F1..F7`. |
| `1065 → 1131` | `init`, `main` | Khởi tạo cửa sổ GLUT, đăng ký callback và chạy `glutMainLoop()`. |

---

## 2. GIẢI THÍCH CHI TIẾT TỪNG CỤM CODE CỦA 3 HÀM THUẬT TOÁN CHÍNH

> **Khung chung của cả 3 hàm:**
> 1. Đưa hình về gốc `(0, 0)` và khởi tạo biến quyết định ban đầu.
> 2. Chạy vòng lặp thuật toán trên **phần nhỏ nhất** ($1/8$ tròn, $1/4$ ellipse, $1/2$ parabol), mỗi bước chỉ cộng/trừ sai phân nhỏ để chọn 1 trong 2 pixel kề nhau.
> 3. Nhân bản đối xứng sang các phần còn lại, cộng tọa độ tâm/đỉnh `(xc, yc)` rồi trả về `std::vector<Point2D>`.

---

### 2.1. Đường tròn Bresenham (`Dòng 162 → 202`)

#### Cụm 1.1 — Khởi tạo (`Dòng 166 → 168`)
```cpp
int x = 0, y = R;
int d = 3 - 2 * R;
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ đây là bước khởi tạo thuật toán Bresenham vẽ đường tròn đưa về tâm `(0, 0)`: điểm xuất phát ở đỉnh trên cùng `(0, R)`, còn `d = 3 - 2*R` là giá trị ban đầu $d_0$ của biến quyết định để chọn pixel tiếp theo thuần bằng số nguyên, không dùng số thực ạ.

#### Cụm 1.2 — Vòng lặp $1/8$ cung & đối xứng 8 phần (`Dòng 170 → 188`)
```cpp
while (x <= y) {
    oct1.push_back({xc + x, yc + y}); ... oct8.push_back({xc - x, yc + y});

    if (d < 0) {
        d += 4 * x + 6;
    } else {
        d += 4 * (x - y) + 10;
        y--;
    }
    x++;
}
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ đây là lõi thuật toán Bresenham (khớp slide trang 24):
> - `while (x <= y)`: chỉ cần tính **$1/8$ cung tròn** (từ $90^\circ$ đến $45^\circ$), rồi từ 1 điểm `(x, y)` lấy đối xứng ra **8 điểm** `(xc ± x, yc ± y)` và `(xc ± y, yc ± x)` lưu vào 8 vector `oct1..oct8`.
> - `if (d < 0)`: đường tròn nằm gần điểm trên `(x+1, y)` hơn nên giữ nguyên `y`, cập nhật `d += 4*x + 6`.
> - `else` (`d >= 0`): đường tròn gần điểm dưới `(x+1, y-1)` hơn nên cập nhật `d += 4*(x - y) + 10` (dùng `y` cũ) trước rồi mới giảm `y--`. Cuối vòng luôn tăng `x++` ạ.

#### Cụm 1.3 — Ghép 8 cung (`Dòng 191 → 201`)
```cpp
for (...) fullCircle.push_back(oct2[i]);
for (...) fullCircle.push_back(oct1[oct1.size() - 1 - i]);
...
return fullCircle;
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ cụm này ghép 8 cung đối xứng `oct1..oct8` theo đúng thứ tự liên tục ngược chiều kim đồng hồ ($0^\circ \to 360^\circ$) vào `fullCircle` để khi vẽ bằng `GL_LINE_LOOP` đường tròn khép kín, không bị nối chéo dây ạ.

*(Ghi chú: Hàm `computeCircleMidpoint` ở `Dòng 205 → 243` có cấu trúc y hệt, chỉ đổi `p = 1 - R`, `p += 2*x + 3`, `p += 2*(x - y) + 5` theo slide trang 28.)*

---

### 2.2. Ellipse Midpoint (`Dòng 253 → 312`)

#### Cụm 2.1 — Khởi tạo (`Dòng 257 → 269`)
```cpp
long long rx2 = (long long)rx * rx, ry2 = (long long)ry * ry;
long long twoRx2 = 2 * rx2, twoRy2 = 2 * ry2;
int x = 0, y = ry;
double p1 = ry2 - rx2 * ry + 0.25 * rx2;
long long dx = 0, dy = twoRx2 * y;
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ cụm này tính trước $a^2, b^2, 2a^2, 2b^2$ bằng kiểu `long long` để tránh tràn số nguyên khi bán trục lớn. Điểm bắt đầu là `(0, ry)`, `p1` là giá trị hàm Midpoint ban đầu tại `(1, ry - 0.5)`, còn `dx` ($= 2b^2x$) và `dy` ($= 2a^2y$) dùng để kiểm tra độ dốc tiếp tuyến phân chia 2 vùng ạ.

#### Cụm 2.2 — Vùng 1: Đường cong thoải (`Dòng 271 → 282`)
```cpp
while (dx < dy) {
    q1.push_back({x, y});
    x++;
    dx += twoRy2;
    if (p1 < 0) {
        p1 += dx + ry2;
    } else {
        y--;
        dy -= twoRx2;
        p1 += dx - dy + ry2;
    }
}
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ đây là **Vùng 1** của thuật toán Midpoint Ellipse (khi hệ số góc $|m| < 1 \iff 2b^2x < 2a^2y$, tức `dx < dy`). Vì đường cong còn thoải nên mỗi bước em cho tăng `x++`:
> - Nếu `p1 < 0` (trung điểm nằm trong ellipse) → chọn điểm trên `(x+1, y)`, giữ nguyên `y`, cộng `p1 += dx + ry2`.
> - Nếu `p1 >= 0` (trung điểm nằm ngoài ellipse) → hạ `y--`, cập nhật `dy -= twoRx2` và `p1 += dx - dy + ry2`. Toàn bộ điểm góc phần tư 1 lưu vào mảng `q1` ạ.

#### Cụm 2.3 — Vùng 2: Đường cong dốc đứng (`Dòng 286 → 298`)
```cpp
double p2 = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
while (y >= 0) {
    q1.push_back({x, y});
    y--;
    dy -= twoRx2;
    if (p2 > 0) {
        p2 += rx2 - dy;
    } else {
        x++;
        dx += twoRy2;
        p2 += dx - dy + rx2;
    }
}
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ đây là **Vùng 2** khi độ dốc $|m| \ge 1$ (đường cong dựng đứng), phải đổi sang **giảm `y--`** cho đến khi `y >= 0` để nét vẽ không bị đứt đoạn. `p2` được tính lại tại trung điểm `(x + 0.5, y - 1)` nối tiếp ngay từ điểm cuối Vùng 1:
> - Nếu `p2 > 0` (trung điểm nằm ngoài ellipse) → giữ nguyên `x`, cộng `p2 += rx2 - dy`.
> - Nếu `p2 <= 0` (trung điểm nằm trong ellipse) → đẩy `x++` ra ngoài và cộng `p2 += dx - dy + rx2` ạ.

#### Cụm 2.4 — Đối xứng 4 phần (`Dòng 301 → 311`)
```cpp
// 4 vòng for phản xạ q1 sang: (xc+x, yc+y), (xc+x, yc-y), (xc-x, yc-y), (xc-x, yc+y)
return fullEllipse;
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ Ellipse có tính đối xứng 4 phần qua 2 trục $Ox, Oy$, nên cụm này lấy mảng điểm góc phần tư 1 (`q1`) phản xạ sang cả 4 góc quanh tâm `(xc, yc)` thành vòng kín `fullEllipse` ạ.

---

### 2.3. Parabol Bresenham (`Dòng 329 → 373`)

#### Cụm 3.1 — Khởi tạo & Vùng 1 (`Dòng 333 → 347`)
```cpp
int x = 0, y = 0;
int d1 = 1 - 2 * p;
while (x <= 2 * p && y <= maxY) {
    half.push_back({x, y});
    if (d1 < 0) {
        d1 += 2 * x + 3;
    } else {
        y++;
        d1 += 2 * x + 3 - 4 * p;
    }
    x++;
}
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ đây là **Vùng 1** của Parabol $x^2 = 4py$ bắt đầu từ đỉnh `(0, 0)`. Đạo hàm là $y' = \frac{x}{2p}$, nên khi $y' \le 1 \iff x \le 2p$, đường cong thoải nên bước theo `x++`, biến quyết định ban đầu là `d1 = 1 - 2*p`:
> - Nếu `d1 < 0` → giữ nguyên `y`, cộng `d1 += 2*x + 3`.
> - Nếu `d1 >= 0` → tăng `y++`, cộng `d1 += 2*x + 3 - 4*p`. Các điểm nhánh phải lưu vào `half` ạ.

#### Cụm 3.2 — Vùng 2 (`Dòng 350 → 360`)
```cpp
double d2 = (x + 0.5) * (x + 0.5) - 4.0 * p * (y + 1);
while (y <= maxY && (xv + x < windowWidth + 50)) {
    half.push_back({x, y});
    if (d2 > 0) {
        d2 -= 4.0 * p;
    } else {
        d2 += 2.0 * x + 2.0 - 4.0 * p;
        x++;
    }
    y++;
}
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ đây là **Vùng 2** khi $x > 2p$ (độ dốc $y' > 1$, nhánh Parabol dựng đứng) nên phải đổi sang bước theo `y++` cho đến giới hạn chiều cao `maxY` để không bị hở pixel. `d2` tính lại tại trung điểm `(x + 0.5, y + 1)`:
> - Nếu `d2 > 0` → giữ nguyên `x`, trừ `d2 -= 4*p`.
> - Nếu `d2 <= 0` → tăng `x++`, cộng `d2 += 2*x + 2 - 4*p` ạ.

#### Cụm 3.3 — Đối xứng 2 nhánh (`Dòng 363 → 372`)
```cpp
for (size_t i = half.size(); i > 0; --i)
    fullParabola.push_back({xv - half[i - 1].x, yv + direction * half[i - 1].y});
for (size_t i = 1; i < half.size(); ++i)
    fullParabola.push_back({xv + half[i].x, yv + direction * half[i].y});
return fullParabola;
```
> **Nếu cô hỏi cụm này làm gì:**
> Dạ Parabol đối xứng 2 bên qua trục đứng đi qua đỉnh `x = xv`. Vòng `for` đầu lật ngược `half` sang nhánh trái (`xv - x`), vòng `for` sau vẽ tiếp nhánh phải (`xv + x`, bắt đầu từ `i = 1` để không bị trùng điểm đỉnh), còn nhân với `direction` (`+1` hoặc `-1`) để vẽ được cả Parabol mở lên và mở xuống ạ.

---

## 3. KIẾN THỨC GIẢI TÍCH CHIA 2 VÙNG (ĐẠO HÀM & HỆ SỐ GÓC TIẾP TUYẾN)

- **Nguyên lý chung:**
  - Đường cong **thoải** ($|m| \le 1$) → bước theo trục $x$ (`x++`).
  - Đường cong **dốc đứng** ($|m| > 1$) → đổi sang bước theo trục $y$ (`y++` / `y--`) để không bị hở pixel.
  - **Ranh giới chuyển vùng** là điểm có độ dốc $|m| = 1$ (góc $45^\circ$).
- **Với Ellipse** ($f(x, y) = b^2x^2 + a^2y^2 - a^2b^2 = 0$):
  $$m = -\frac{f_x}{f_y} = -\frac{2b^2x}{2a^2y} \implies |m| = 1 \iff 2b^2x = 2a^2y \quad (\text{trong code là } \texttt{dx < dy})$$
- **Với Parabol** ($x^2 = 4py \iff y = \frac{x^2}{4p}$):
  $$m = y' = \frac{x}{2p} \implies y' = 1 \iff x = 2p \quad (\text{trong code là } \texttt{x <= 2 * p})$$

*(Lưu ý: Bài Lab 2 là Đồ họa 2D cơ sở nên **không có vector pháp tuyến 3D**; `std::vector` trong code chỉ là mảng động của C++ lưu danh sách điểm và danh sách hình).*

---

## 4. TƯƠNG TÁC CHUỘT & OPENGL (`mouseClick`: `Dòng 854 → 956`)

- **Lật tọa độ Y (`Dòng 856`):** `int worldY = windowHeight - y;`
  - *Giải thích:* Tọa độ chuột GLUT có gốc `(0, 0)` ở **góc trên-trái** (trục Y hướng xuống), ngược với `gluOrtho2D(0, windowWidth, 0, windowHeight)` có gốc ở **góc dưới-trái** (trục Y hướng lên).
- **Cách tính tham số từ 2 lần click chuột:**
  - **Click lần 1 (`!isDrawing`):** Lưu tọa độ tâm/đỉnh vào `(startX, startY)`.
  - **Rê chuột (`mousePassiveMotion`, `Dòng 966 → 985`):** Cập nhật `(currentX, currentY)` và gọi `glutPostRedisplay()` để vẽ hình xem trước (live preview).
  - **Click lần 2 (`isDrawing`):**
    - **Đường tròn:** $R = \sqrt{(x_2 - x_1)^2 + (y_2 - y_1)^2}$
    - **Ellipse:** $a = |x_2 - x_1|,\quad b = |y_2 - y_1|$
    - **Parabol:** Hướng mở `direction = (dy >= 0) ? 1 : -1`, tiêu cự $p = \frac{\Delta x^2}{4|\Delta y|}$
  - **Chuột phải:** Hủy thao tác vẽ hiện tại.

---

## 5. BỘ HỎI – ĐÁP NHANH KHI VẤN ĐÁP

1. **Tại sao không dùng $y = \sqrt{R^2 - x^2}$ hay $\sin, \cos$ mà dùng Bresenham/Midpoint?**
   → Vì căn bậc hai và lượng giác dùng nhiều phép tính số thực (chậm) và dễ bị đứt đoạn khi độ dốc $|m| > 1$. Bresenham/Midpoint chỉ dùng phép cộng/trừ sai phân nên rất nhanh và luôn liền nét.
2. **Khác nhau giữa Bresenham và Midpoint là gì?**
   → Bresenham so sánh hiệu khoảng cách $d_1 - d_2$ tới 2 pixel kề nhau; Midpoint xét dấu hàm $f(x, y)$ tại trung điểm giữa 2 pixel đó. Cả hai đều quy về công thức đệ quy tương đương nhau.
3. **Tại sao Đường tròn chỉ tính $1/8$ cung còn Ellipse phải tính $1/4$ cung?**
   → Đường tròn đối xứng qua cả 2 trục tọa độ lẫn 2 đường phân giác $y = \pm x$ (8 phần). Ellipse chỉ đối xứng qua 2 trục $Ox, Oy$ (4 phần), không đối xứng qua $y = x$.
4. **Khác nhau giữa `glutMotionFunc` và `glutPassiveMotionFunc`?**
   → `glutMotionFunc` chạy khi di chuyển chuột **có giữ nút chuột**; `glutPassiveMotionFunc` chạy khi di chuyển chuột **không giữ nút** (dùng cho hover nút bấm và xem trước giữa 2 lần click).
5. **Sao nét vẽ mượt quá, có đúng là vẽ điểm pixel Bresenham/Midpoint không? (Hoặc cô muốn xem các điểm pixel rời rạc `GL_POINTS`)**
   → Dạ thuật toán vẫn tính từng tọa độ điểm nguyên lưu vào mảng `s.points` ạ. Trong `renderShape` (`Dòng 378–403`) em có 2 chế độ: `smoothMode = true` nối các điểm đó bằng `GL_LINE_LOOP`/`GL_LINE_STRIP` cho rõ khi chụp ảnh nộp, còn đổi `bool smoothMode = false;` ở `Dòng 75` là hiện ngay từng điểm pixel `GL_POINTS` (bấm `+`/`-` để phóng to từng pixel) ạ.
6. **Thế đường tròn Midpoint đâu?**
   → Dạ em có cài sẵn đầy đủ hàm `computeCircleMidpoint` ở `Dòng 205–243`, nhưng vì đề 14/9 yêu cầu nộp ảnh Hình tròn bằng **Bresenham** nên em để mặc định `currentCircleAlgo = ALGO_CIRCLE_BRESENHAM` ở `Dòng 63` (đổi sang `ALGO_CIRCLE_MIDPOINT` là chạy ngay bản Midpoint) ạ.

---

## 6. PHÍM TẮT DEMO NHANH TRƯỚC MẶT CÔ
- `1`, `2`, `3`: Xem hình mẫu chuẩn (Tròn / Ellipse / Parabol) hoặc đổi công cụ khi đang vẽ tự do.
- `4`: Hiện đồng thời cả 3 hình chia 3 cột.
- `C`: Xóa sạch bảng vẽ để demo vẽ tự do bằng click chuột.
- `G`: Bật/tắt lưới tọa độ | `I`: Bật/tắt thẻ thông số | `ESC`: Thoát.
- *(Hỗ trợ thêm `F1 → F7` để không bị lỗi bộ gõ tiếng Việt Unikey).*
