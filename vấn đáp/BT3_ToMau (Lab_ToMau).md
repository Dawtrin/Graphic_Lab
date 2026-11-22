# TÀI LIỆU ÔN VẤN ĐÁP — BT3: TÔ MÀU THEO ĐƯỜNG BIÊN (COLORING)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (Coloring – 05/10):** Cài đặt thuật toán **tô màu theo đường biên (Boundary Fill)**:
  1. Dùng phương pháp **Đệ quy** (Slide trang 84)
  2. Dùng phương pháp **Khử đệ quy** (dùng **Stack** hoặc **Queue** — Slide trang 85–88)
  - **Yêu cầu:** Vùng tô là **1 vùng phức tạp** (giao của các hình) và **điểm khởi tạo (seed)** được xác định qua **click chuột**.
- **Mã nguồn:** `Lab_Graphic_ToMau/main.cpp` | **Chạy chương trình:** `Lab_Graphic_ToMau/run.bat`
- **Tài liệu đối chiếu:** Slide `CG.3-2D Graphics.pdf` (trang 78–89).

---

## 1. BẢN ĐỒ TỔNG QUAN FILE `Lab_Graphic_ToMau/main.cpp` (THEO SỐ DÒNG)

| Khoảng dòng | Thành phần / Hàm | Vai trò chính |
| :--- | :--- | :--- |
| `58 → 134` | Cấu hình cửa sổ, `PALETTE[16]`, `enum FillAlgo`, biến toàn cục | Khai báo bảng 16 màu, 3 chế độ thuật toán (`STACK_DFS`, `QUEUE_BFS`, `RECURSIVE`), danh sách đỉnh và thống kê. |
| **`139 → 169`** | **`pixelBuffer`, `colorsEqual`, `isInsideCanvas`, `getPixel`, `putPixel`** | **Bộ đệm ảnh lưu màu từng pixel trên RAM để đọc/ghi màu nhanh thay vì gọi `glReadPixels`.** |
| **`174 → 203`** | **`boundaryFillRecursiveHelper` & `boundaryFillRecursive`** | **Thuật toán Tô màu theo đường biên — Phương pháp Đệ quy (khớp Slide trang 84).** |
| **`208 → 270`** | **`boundaryFill`** | **Thuật toán Tô màu theo đường biên — Khử đệ quy bằng Stack (DFS) & Queue (BFS) (khớp Slide trang 85–88).** |
| `275 → 289` | `drawLineBresenham` | Vẽ các cạnh đường biên đa giác lên `pixelBuffer` bằng thuật toán Bresenham. |
| `349 → 673` | `buildCircle`, `buildShatteredSword`, `loadPreset` | Tạo các vùng tô phức tạp (giao 2 tròn, ngôi sao 8 cánh, Olympic 5 vòng, bông hoa 6 cánh, thanh kiếm vỡ). |
| `715 → 855` | `drawOverlay`, `drawHud`, `display` | Vẽ `pixelBuffer` ra màn hình bằng `glDrawPixels`, vẽ thanh công cụ, bảng 16 màu và thông số. |
| **`875 → 927`** | **`mouse`** | **Bắt sự kiện click chuột: chọn nút, chọn màu, thêm đỉnh vẽ đa giác hoặc click chọn điểm hạt giống (seed) để tô.** |
| `936 → 994` | `keyboard`, `main` | Phím tắt (`B` đổi thuật toán, `1..9, S` nạp hình mẫu, `A` bật/tắt animation), khởi tạo GLUT. |

---

## 2. GIẢI THÍCH TỪNG CỤM CODE CỐT LÕI (DỊCH THẲNG TỪNG LỆNH + CÁCH NÓI VỚI CÔ)

### 2.1. Bộ đệm điểm ảnh & Đọc/Ghi màu — `pixelBuffer`, `colorsEqual`, `getPixel`, `putPixel` (`Dòng 139 → 169`)

```cpp
Color pixelBuffer[WINDOW_HEIGHT][WINDOW_WIDTH]; // Mảng 2 chiều lưu màu RGB của từng pixel trên cửa sổ

bool colorsEqual(const Color& a, const Color& b) {
    const float epsilon = 0.01f;                // Sai số nhỏ để so sánh 2 màu kiểu số thực float
    return (std::abs(a.r - b.r) < epsilon &&    // Trả về true nếu hiệu kênh Đỏ < 0.01
            std::abs(a.g - b.g) < epsilon &&    // VÀ hiệu kênh Xanh lá < 0.01
            std::abs(a.b - b.b) < epsilon);     // VÀ hiệu kênh Xanh dương < 0.01
}

Color getPixel(int x, int y) {
    if (isInsideBuffer(x, y)) return pixelBuffer[y][x]; // Nếu (x,y) nằm trong màn hình -> trả về màu tại ô đó
    return BACKGROUND_COLOR;                            // Ngược lại -> trả về màu nền mặc định
}

void putPixel(int x, int y, Color color) {
    if (isInsideBuffer(x, y)) pixelBuffer[y][x] = color; // Nếu nằm trong màn hình -> gán màu mới cho ô (x,y)
}
```
> **Cách nói với cô:**
> *"Dạ thay vì gọi `glReadPixels` trực tiếp từ card đồ họa rất chậm mỗi lần kiểm tra màu pixel, em tạo một mảng 2 chiều `pixelBuffer` trên RAM để lưu màu của từng điểm ảnh ạ. Hàm `getPixel` đọc màu tại tọa độ `(x, y)` từ mảng này, `putPixel` tô màu vào mảng, rồi hàm `display()` chỉ cần gọi `glDrawPixels` đẩy cả mảng ra màn hình nên chạy rất nhanh ạ. Còn `colorsEqual` dùng sai số `epsilon = 0.01` để so sánh 2 màu số thực `float` cho chính xác."*

---

### 2.2. Thuật toán Tô màu theo đường biên — ĐỆ QUY (`Dòng 174 → 203`)

```cpp
void boundaryFillRecursiveHelper(int x, int y, Color fillColor, Color boundaryColor, int depth) {
    if (!isInsideCanvas(x, y)) return;              // Nếu điểm (x,y) vượt ra ngoài vùng vẽ -> dừng luôn
    if (depth > 15000) {                            // Nếu gọi đệ quy sâu quá 15.000 lớp
        g_recursiveCapped = true;                   // -> Đánh dấu chạm giới hạn để không bị tràn Call Stack
        return;                                     // -> Dừng nhánh này lại
    }
    if ((std::size_t)depth > g_stats.peakStack) g_stats.peakStack = (std::size_t)depth; // Lưu độ sâu lớn nhất

    Color c = getPixel(x, y);                       // Đọc màu hiện tại của pixel (x, y)
    if (!colorsEqual(c, boundaryColor) && !colorsEqual(c, fillColor)) { // Nếu KHÔNG phải màu biên VÀ CHƯA tô màu fill
        putPixel(x, y, fillColor);                  // -> Tô pixel (x, y) bằng màu fillColor
        ++g_stats.filled;                           // -> Tăng biến đếm số pixel đã tô lên 1

        boundaryFillRecursiveHelper(x - 1, y, fillColor, boundaryColor, depth + 1); // Gọi đệ quy loang sang Trái
        boundaryFillRecursiveHelper(x, y + 1, fillColor, boundaryColor, depth + 1); // Gọi đệ quy loang lên Trên
        boundaryFillRecursiveHelper(x + 1, y, fillColor, boundaryColor, depth + 1); // Gọi đệ quy loang sang Phải
        boundaryFillRecursiveHelper(x, y - 1, fillColor, boundaryColor, depth + 1); // Gọi đệ quy loang xuống Dưới
    }
}
```
> **Cách nói với cô:**
> *"Dạ đây là thuật toán tô màu đường biên bằng đệ quy 4-lân cận giống đúng slide trang 84 ạ. Tại mỗi điểm `(x, y)`, em kiểm tra trước xem có nằm trong vùng vẽ không, rồi đọc màu hiện tại của nó. Nếu điểm đó chưa phải màu biên và cũng chưa được tô màu mới thì em tô màu nó luôn, xong gọi đệ quy lan tiếp ra 4 ô xung quanh là Trái, Trên, Phải, Dưới. Em có thêm dòng `if (depth > 15000)` để chặn không cho chương trình bị văng (crash) do tràn bộ nhớ Stack nếu lỡ click vào vùng quá lớn ạ."*

---

### 2.3. Thuật toán Tô màu theo đường biên — KHỬ ĐỆ QUY bằng STACK & QUEUE (`Dòng 208 → 270`)

#### Cụm 3.1 — Khởi tạo & Lấy điểm từ Stack / Queue (`Dòng 208 → 233`)
```cpp
if (g_algo == FillAlgo::RECURSIVE) {            // Nếu đang chọn chế độ Đệ quy
    boundaryFillRecursive(sx, sy, fillColor, boundaryColor); // -> Gọi hàm đệ quy ở trên rồi thoát
    return;
}

bool useBFS = (g_algo == FillAlgo::QUEUE_BFS);  // Kiểm tra xem đang dùng Queue (BFS) hay Stack (DFS)
std::stack<Point> st;                           // Khai báo ngăn xếp Stack (vào sau ra trước - LIFO)
std::queue<Point> q;                            // Khai báo hàng đợi Queue (vào trước ra trước - FIFO)

if (useBFS) q.push({sx, sy});                   // Nếu dùng Queue -> đẩy điểm hạt giống (sx, sy) vào hàng đợi
else        st.push({sx, sy});                  // Ngược lại -> đẩy điểm hạt giống vào ngăn xếp Stack

while (useBFS ? !q.empty() : !st.empty()) {     // Lặp liên tục khi Queue (hoặc Stack) vẫn còn phần tử
    Point p;
    if (useBFS) { p = q.front(); q.pop(); }     // Nếu dùng Queue -> lấy điểm ở đầu hàng đợi ra xét rồi xóa đi
    else        { p = st.top();  st.pop(); }    // Ngược lại dùng Stack -> lấy điểm trên đỉnh Stack ra xét rồi xóa đi

    if (!isInsideCanvas(p.x, p.y)) continue;    // Nếu điểm p ra ngoài khung vẽ -> bỏ qua, sang vòng lặp tiếp
```
> **Cách nói với cô:**
> *"Dạ hàm `boundaryFill` này em cài khử đệ quy hỗ trợ cả 2 cấu trúc dữ liệu là **Stack (DFS)** và **Queue (BFS)** (chuyển đổi bằng phím `B`) ạ. Bước đầu em đưa điểm hạt giống `(sx, sy)` người dùng vừa click vào Stack hoặc Queue. Sau đó em cho vòng lặp `while` chạy tới khi nào Stack/Queue rỗng thì dừng; mỗi vòng lặp lấy 1 điểm `p` ra để kiểm tra, nếu vượt khỏi vùng vẽ thì bỏ qua ạ."*

#### Cụm 3.2 — Kiểm tra màu, Tô điểm & Đẩy 4 lân cận vào Stack / Queue (`Dòng 235 → 265`)
```cpp
    Color c = getPixel(p.x, p.y);                                           // Đọc màu tại điểm p
    if (!colorsEqual(c, boundaryColor) && !colorsEqual(c, fillColor)) {     // Nếu p KHÔNG phải biên VÀ CHƯA tô màu fill
        putPixel(p.x, p.y, fillColor);                                      // -> Tô màu cho điểm p
        ++g_stats.filled;                                                   // -> Tăng bộ đếm pixel đã tô

        if (g_animate && (g_stats.filled % 100 == 0)) {                     // Nếu bật hiệu ứng -> cứ tô 100 pixel thì vẽ lại 1 lần
            glRasterPos2i(0, 0);
            glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_FLOAT, pixelBuffer);
            glutSwapBuffers();
        }

        if (useBFS) {                                                       // Nếu dùng Queue (BFS - loang đều như sóng nước)
            q.push({p.x - 1, p.y});                                         // Đẩy ô Trái vào hàng đợi
            q.push({p.x, p.y + 1});                                         // Đẩy ô Trên vào hàng đợi
            q.push({p.x + 1, p.y});                                         // Đẩy ô Phải vào hàng đợi
            q.push({p.x, p.y - 1});                                         // Đẩy ô Dưới vào hàng đợi
        } else {                                                            // Ngược lại dùng Stack (DFS - đi sâu từng nhánh)
            st.push({p.x, p.y - 1});                                        // Đẩy Dưới, Phải, Trên, Trái (LIFO nên Trái sẽ lấy ra trước)
            st.push({p.x + 1, p.y});
            st.push({p.x, p.y + 1});
            st.push({p.x - 1, p.y});
        }
    }
```
> **Cách nói với cô:**
> *"Dạ tại mỗi điểm `p` lấy ra, em đọc màu của nó. Nếu nó không phải màu biên (màu đen) và chưa mang màu tô mới thì em tô nó bằng `putPixel`, rồi đẩy tiếp 4 điểm lân cận (Trái, Trên, Phải, Dưới) vào Stack hoặc Queue để các vòng sau xét tiếp ạ. Nhờ điều kiện `!colorsEqual(c, fillColor)` mà mỗi pixel chỉ bị tô tối đa 1 lần, nên thuật toán chắc chắn dừng khi Stack/Queue rỗng. Em có thêm đoạn `if (g_animate)` cứ 100 pixel cập nhật màn hình 1 lần để cô nhìn rõ sự khác nhau giữa loang theo chiều sâu (Stack) và loang đều như sóng nước (Queue) ạ."*

---

## 3. TƯƠNG TÁC CLICK CHUỘT CHỌN ĐIỂM SEED (`mouse`: `Dòng 875 → 927`)

> **Cách nói với cô nếu hỏi phần chuột:**
> *"Dạ em bắt sự kiện chuột ở hàm `mouse` đăng ký qua `glutMouseFunc` ạ. Đầu tiên ở dòng 878 em lật tọa độ `glY = WINDOW_HEIGHT - 1 - y` cho khớp hệ tọa độ OpenGL gốc dưới-trái. Nếu người dùng click vào các nút bấm hoặc 16 ô màu trên thanh HUD thì em đổi màu/chế độ tương ứng. Còn khi click vào trong khung vẽ ở chế độ `FILL`, em lấy màu tại đúng chỗ vừa click: nếu bấm nhầm lên đường biên hoặc vùng đã tô màu đó rồi thì báo lên thanh trạng thái; còn nếu bấm vào vùng hợp lệ thì em lấy tọa độ `(x, glY)` làm điểm khởi tạo (seed) rồi gọi hàm `boundaryFill` để tô loang luôn ạ."*

---

## 4. BỘ HỎI – ĐÁP NHANH KHI VẤN ĐÁP

1. **Nếu cô hỏi: *"Nhược điểm của phương pháp Đệ quy là gì? Tại sao phải khử đệ quy bằng Stack/Queue?"***
   > *"Dạ nhược điểm lớn nhất của đệ quy là mỗi lần gọi đệ quy sẽ tốn bộ nhớ trên **Call Stack** của hệ thống (lưu địa chỉ trả về, tham số...). Với vùng tô có diện tích lớn (vài chục nghìn pixel), Call Stack rất nhanh bị đầy gây lỗi **tràn Stack (Stack Overflow)** làm văng chương trình. Khi khử đệ quy bằng `std::stack` hoặc `std::queue` thì dữ liệu được cấp phát trên **Heap** (bộ nhớ RAM rất lớn) nên tô thoải mái vùng rộng mà không bao giờ bị crash ạ."*

2. **Nếu cô hỏi: *"So sánh tô bằng Stack (DFS) và Queue (BFS) có gì khác nhau?"***
   > *"Dạ kết quả vùng tô cuối cùng là hoàn toàn giống nhau, chỉ khác thứ tự loang thôi ạ. Dùng **Stack** (vào sau ra trước — DFS) thì màu sẽ chạy sâu theo một hướng tới khi đụng biên rồi mới quay lại. Còn dùng **Queue** (vào trước ra trước — BFS) thì màu loang đều ra xung quanh theo từng lớp giống như giọt nước rơi xuống mặt hồ ạ — bật nút `ANIM (A)` lên là thấy rất rõ."*

3. **Nếu cô hỏi: *"Em dùng tô loang 4-lân cận hay 8-lân cận? Tại sao?"***
   > *"Dạ em dùng **4-lân cận** (Trái, Phải, Trên, Dưới) ạ. Vì đường biên em vẽ bằng thuật toán Bresenham là đường nét mảnh 1 pixel dạng **8-liên thông** (các pixel có thể nối chéo góc với nhau). Nếu dùng tô 8-lân cận thì màu có thể đi chéo **lách qua khe hở giữa 2 pixel biên** và tràn ra ngoài, còn dùng 4-lân cận thì chắc chắn bị chặn kín hoàn toàn ạ."*

4. **Nếu cô hỏi: *"Nếu đường biên bị hở thì chuyện gì xảy ra?"***
   > *"Dạ màu sẽ loang qua chỗ hở tràn ra toàn bộ vùng vẽ, nhưng em đã có điều kiện `isInsideCanvas(p.x, p.y)` chặn lại ở 4 mép khung vẽ nên nó chỉ tràn hết khung canvas chứ không tràn đè lên thanh công cụ HUD phía trên ạ (bấm phím `3` là có sẵn hình mẫu biên hở để test luôn ạ)."*

---

## 5. PHÍM TẮT DEMO NHANH TRƯỚC MẶT CÔ
- **`B` (hoặc nút `ALGO (B)`):** Chuyển qua lại 3 thuật toán: **DFS (Stack)** → **BFS (Queue)** → **Recursive (Đệ quy)**.
- **`P4`, `P5`, `P7`, `P8`, `P9`, `S` (hoặc phím `4, 5, 7, 8, 9, S`):** Nạp các **vùng phức tạp** (giao 2 tròn, tam giác + vuông, ngôi sao 8 cánh, Olympic 5 vòng, bông hoa 6 cánh, thanh kiếm vỡ).
- **`3`:** Nạp hình mẫu **biên hở** để minh họa màu tràn ra ngoài.
- **`A`:** Bật/tắt hiệu ứng Animation tô loang để cô nhìn rõ sự khác biệt giữa Stack và Queue.
- **Click 16 ô màu ở trên + Click vào từng mảnh trên hình:** Chọn màu và đặt điểm hạt giống (seed) tô trực tiếp.
- **`D` / `ENTER` / `C`:** Vẽ đa giác tự do / Đóng đa giác / Xóa hết.
