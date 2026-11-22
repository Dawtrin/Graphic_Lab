# TÀI LIỆU ÔN VẤN ĐÁP — BT2: THUẬT TOÁN XÉN HÌNH (LAB 3)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (Slide 80):** Cài đặt các thuật toán xén hình:
  1. **Cohen-Sutherland**
  2. **Chia nhị phân (Midpoint Subdivision)**
  - **Yêu cầu:** Sử dụng sự kiện **click chuột** để xác định đoạn thẳng $AB$ cần xén.
- **Mã nguồn:** `Lab_3/lab3.cpp` | **Chạy chương trình:** `run_lab3.bat` hoặc `Lab_3/run.bat`
- **Tài liệu đối chiếu:** Slide `CG.3-2D Graphics.pdf` (trang 54–70).

---

## 1. BẢN ĐỒ TỔNG QUAN FILE `Lab_3/lab3.cpp` (THEO SỐ DÒNG)

| Khoảng dòng | Thành phần / Hàm | Vai trò chính |
| :--- | :--- | :--- |
| `19 → 63` | `INSIDE/LEFT/RIGHT/BOTTOM/TOP`, `struct Point2D`, `ClipWindow`, `ClippedLine` | Định nghĩa mã 4-bit OutCode (`1, 2, 4, 8`), cửa sổ xén `[xmin, xmax] × [ymin, ymax]` và cấu trúc lưu 1 đoạn thẳng trước/sau xén. |
| `68 → 88` | Biến toàn cục (`clipWin`, `currentAlgo`, `currentDisp`, `isDrawing`, `lines`) | Lưu cửa sổ xén, thuật toán đang chọn, trạng thái vẽ chuột và danh sách các đoạn thẳng. |
| **`168 → 177`** | **`computeOutCode`** | **Hàm gán mã vùng 4-bit (TBRL) cho 1 điểm `(x, y)` (khớp Slide trang 56).** |
| **`189 → 247`** | **`clipCohenSutherland`** | **Thuật toán xén đoạn thẳng Cohen-Sutherland (khớp Slide trang 57–61).** |
| **`263 → 336`** | **`subdivideRecursive` & `clipMidpointSubdivision`** | **Thuật toán Chia nhị phân (Midpoint Subdivision) (khớp Slide trang 62–70).** |
| `341 → 359` | `processLine` | Gọi thuật toán đang chọn để xén đoạn $AB$ và đóng gói kết quả vào `ClippedLine`. |
| `367 → 386` | `loadPresetLines` | Tạo sẵn 6 đoạn thẳng mẫu test đủ mọi trường hợp (trong, ngoài cùng phía, cắt 2 biên, cắt góc, chéo ngoài). |
| `391 → 464` | `drawGridAndClipWindow`, `drawLinePart`, `renderClippedLine` | Vẽ lưới, khung cửa sổ xén, phần giữ lại (xanh lá đậm) và phần bị xén (đỏ). |
| `469 → 531` | `initButtons`, `renderUI` | Vẽ thanh công cụ nút bấm và thanh trạng thái. |
| `536 → 590` | `display` | Hàm vẽ chính của OpenGL (vẽ khung, vẽ các đoạn đã chốt, live preview khi kéo chuột, UI). |
| `612 → 640` | `handleButtonClick` | Xử lý bấm nút đổi thuật toán (`[1]`/`[2]`), đổi chế độ xem (`[V]`), nạp mẫu (`[P]`), xóa (`[C]`), lưới (`[G]`). |
| **`645 → 706`** | **`mouseClick`** | **Bắt sự kiện click chuột trái lần 1 (chọn $A$), lần 2 (chọn $B$ & xén), chuột phải (hủy).** |
| `708 → 735` | `mousePassiveMotion`, `mouseMotion` | Cập nhật vị trí con trỏ liên tục để xem trước kết quả xén thời gian thực. |
| `740 → 831` | `keyboard`, `initGL`, `main` | Phím tắt, khởi tạo OpenGL nền trắng, đăng ký callback GLUT. |

---

## 2. GIẢI THÍCH CHI TIẾT TỪNG CỤM CODE CỦA 3 HÀM THUẬT TOÁN CHÍNH

### 2.1. Hàm gán mã vùng 4-bit — `computeOutCode` (`Dòng 168 → 177`)

```cpp
const int INSIDE = 0; // 0000: nằm trong khung
const int LEFT   = 1; // 0001: nằm bên trái khung
const int RIGHT  = 2; // 0010: nằm bên phải khung
const int BOTTOM = 4; // 0100: nằm bên dưới khung
const int TOP    = 8; // 1000: nằm bên trên khung

int computeOutCode(double x, double y, const ClipWindow& win) {
    int code = INSIDE;                      // Ban đầu cho mã bằng 0000 (coi như nằm trong)
    if (x < win.xmin)      code |= LEFT;    // Nếu x nhỏ hơn cạnh trái -> bật bit Trái lên 1
    else if (x > win.xmax) code |= RIGHT;   // Ngược lại nếu x lớn hơn cạnh phải -> bật bit Phải lên 1
    if (y < win.ymin)      code |= BOTTOM;  // Nếu y nhỏ hơn cạnh dưới -> bật bit Dưới lên 1
    else if (y > win.ymax) code |= TOP;     // Ngược lại nếu y lớn hơn cạnh trên -> bật bit Trên lên 1
    return code;                            // Trả về mã 4-bit vừa tính được
}
```
> **Cách nói với cô:**
> *"Dạ hàm này dùng để gán mã vùng 4-bit cho một điểm `(x, y)` theo đúng quy ước trên slide là 9 vùng xung quanh cửa sổ xén ạ. Em kiểm tra xem điểm đó có bị lệch sang trái, phải, dưới hay trên so với khung hình chữ nhật không, nếu có thì em dùng phép OR bit `|=` để bật bit tương ứng lên 1 — ví dụ vừa ở trên vừa ở trái thì nó tự ghép thành `1001`, còn nằm gọn bên trong thì giữ nguyên là `0000` ạ."*

---

### 2.2. Thuật toán Cohen-Sutherland — `clipCohenSutherland` (`Dòng 189 → 247`)

#### Cụm 2.1 — Khởi tạo & 2 điều kiện dừng nhanh (`Dòng 191 → 208`)
```cpp
int code0 = computeOutCode(x0, y0, win);    // Tính mã 4-bit cho điểm đầu A
int code1 = computeOutCode(x1, y1, win);    // Tính mã 4-bit cho điểm cuối B
while (true) {                              // Lặp liên tục cho tới khi xén xong
    steps++;                                // Tăng biến đếm số bước lặp lên 1
    if ((code0 | code1) == 0) {             // Nếu OR 2 mã bằng 0 (cả A và B đều nằm trong khung)
        accept = true;                      // -> Chấp nhận giữ nguyên đoạn thẳng
        break;                              // -> Thoát vòng lặp
    } else if (code0 & code1) {             // Ngược lại nếu AND 2 mã khác 0 (cả 2 cùng nằm 1 phía ngoài)
        accept = false;                     // -> Loại bỏ cả đoạn thẳng
        break;                              // -> Thoát vòng lặp
    }
```
> **Cách nói với cô:**
> *"Dạ đầu tiên em tính mã 4-bit cho hai đầu mút A, B rồi vào vòng lặp kiểm tra nhanh 2 trường hợp dễ trước ạ. Nếu lấy `code0 | code1` mà bằng 0, tức là cả hai đầu đều có mã `0000`, nằm trọn trong khung rồi nên em giữ nguyên cả đoạn luôn. Còn nếu lấy `code0 & code1` mà khác 0, nghĩa là hai đầu cùng bị lệch về một phía bên ngoài — ví dụ cùng ở bên trái khung — thì chắc chắn không cắt khung nên em loại bỏ ngay ạ."*

#### Cụm 2.2 — Tìm giao điểm với cạnh cửa sổ & cập nhật (`Dòng 209 → 241`)
```cpp
    } else {                                // Còn không thì có ít nhất 1 đầu đang ở ngoài, phải đi tìm giao điểm
        int codeOut = code0 ? code0 : code1;    // Nếu A ở ngoài (code0 != 0) thì lấy A, ngược lại lấy B
        double x = 0, y = 0;

        if (codeOut & TOP) {                    // Nếu điểm đó nằm phía trên cạnh ymax
            x = x0 + (x1 - x0) * (win.ymax - y0) / (y1 - y0); // Tính hoành độ x giao với cạnh trên
            y = win.ymax;                                     // Tung độ y nằm đúng trên cạnh ymax
        } else if (codeOut & BOTTOM) {          // Ngược lại nếu nằm phía dưới cạnh ymin
            x = x0 + (x1 - x0) * (win.ymin - y0) / (y1 - y0); // Tính x giao với cạnh dưới
            y = win.ymin;                                     // y nằm đúng trên cạnh ymin
        } else if (codeOut & RIGHT) {           // Ngược lại nếu nằm bên phải cạnh xmax
            y = y0 + (y1 - y0) * (win.xmax - x0) / (x1 - x0); // Tính y giao với cạnh phải
            x = win.xmax;                                     // x nằm đúng trên cạnh xmax
        } else if (codeOut & LEFT) {            // Ngược lại nếu nằm bên trái cạnh xmin
            y = y0 + (y1 - y0) * (win.xmin - x0) / (x1 - x0); // Tính y giao với cạnh trái
            x = win.xmin;                                     // x nằm đúng trên cạnh xmin
        }

        if (codeOut == code0) {                 // Nếu nãy mình vừa cắt đầu A thì cập nhật lại tọa độ & mã của A
            x0 = x; y0 = y; code0 = computeOutCode(x0, y0, win);
        } else {                                // Ngược lại thì cập nhật tọa độ & mã của đầu B
            x1 = x; y1 = y; code1 = computeOutCode(x1, y1, win);
        }
    }
```
> **Cách nói với cô:**
> *"Dạ rơi vào nhánh `else` này là lúc đoạn thẳng chưa kết luận được ngay, có ít nhất một đầu đang nằm ngoài khung. Ở dòng 210 em dùng `code0 ? code0 : code1` để chọn lấy đầu mút nằm ngoài đó. Tiếp theo em kiểm tra bằng phép AND bit xem nó đang vượt cạnh nào — trên, dưới, phải hay trái — rồi thay vào phương trình đường thẳng để tính tọa độ giao điểm với cạnh đó (như dòng 215 là cắt cạnh trên `ymax`). Tính xong thì ở dòng 232 em kiểm tra xem nãy mình cắt đầu A hay đầu B để thay tọa độ mới vào, tính lại mã OutCode rồi lặp lại cho tới khi xén xong ạ."*

---

### 2.3. Thuật toán Chia nhị phân — `subdivideRecursive` & `clipMidpointSubdivision` (`Dòng 263 → 336`)

#### Cụm 3.1 — Hàm đệ quy chia đôi đoạn thẳng (`Dòng 263 → 298`)
```cpp
void subdivideRecursive(Point2D p1, Point2D p2, int code1, int code2, ...) {
    steps++;                                                                   // Đếm số lần gọi đệ quy
    if ((code1 | code2) == 0) { acceptedList.push_back({ p1, p2 }); return; }  // Nếu cả 2 đầu nằm trong -> lưu đoạn con rồi dừng
    if (code1 & code2)        { return; }                                      // Nếu cả 2 đầu cùng 1 phía ngoài -> bỏ luôn rồi dừng

    // Nếu đã chia quá 22 lần HOẶC độ dài đoạn nhỏ hơn 0.1 pixel thì dừng
    if (depth > 22 || std::hypot(p1.x - p2.x, p1.y - p2.y) < 0.1) {
        Point2D mid = { (p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5 };            // Lấy trung điểm làm giao điểm
        if (computeOutCode(mid.x, mid.y, win) == INSIDE)                       // Nếu trung điểm nằm trong khung
            acceptedList.push_back({ mid, mid });                              // -> Lưu điểm này vào danh sách
        return;                                                                // -> Dừng nhánh đệ quy này
    }

    // Nếu chưa dừng thì lấy trung điểm mid để chia đôi đoạn [p1, p2]
    Point2D mid = { (p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5 };
    int codeMid = computeOutCode(mid.x, mid.y, win);                           // Tính mã 4-bit cho trung điểm mid

    subdivideRecursive(p1, mid, code1, codeMid, ...);                          // Gọi đệ quy chia tiếp nửa đầu [p1, mid] trước
    subdivideRecursive(mid, p2, codeMid, code2, ...);                          // Sau đó gọi đệ quy chia tiếp nửa sau [mid, p2]
}
```
> **Cách nói với cô:**
> *"Dạ hàm đệ quy này là phần chính của thuật toán chia nhị phân ạ. Ban đầu em cũng dùng mã 4-bit để kiểm tra nhanh giống Cohen-Sutherland: nằm trọn trong khung thì lưu lại vào `acceptedList`, cùng phía ngoài thì bỏ qua luôn. Còn nếu đoạn con vẫn cắt biên, thay vì giải phương trình tìm giao điểm như Cohen-Sutherland, em lấy trung điểm `mid` ở giữa bằng cách cộng 2 đầu rồi chia đôi (dòng 289), xong gọi đệ quy chia tiếp nửa đầu `[p1, mid]` và nửa sau `[mid, p2]`. Cứ chia nhỏ dần như vậy đến khi độ dài đoạn con nhỏ hơn 0.1 pixel (em đo bằng `std::hypot` ở dòng 280) hoặc quá 22 lần thì dừng, lúc đó trung điểm đã sát ngay cạnh cửa sổ rồi ạ."*

#### Cụm 3.2 — Ghép kết quả đoạn sau khi xén (`Dòng 300 → 336`)
```cpp
std::vector<std::pair<Point2D, Point2D>> acceptedList;  // Tạo mảng lưu các đoạn con nằm trong khung
subdivideRecursive(A, B, codeA, codeB, win, acceptedList, 0, steps, sampledMidpoints); // Chạy đệ quy chia đôi
if (acceptedList.empty()) return false;                 // Nếu mảng rỗng (không có phần nào trong khung) -> trả về false

outA = acceptedList.front().first;                      // Lấy điểm đầu của đoạn con đầu tiên làm giao điểm I1
outB = acceptedList.back().second;                      // Lấy điểm cuối của đoạn con cuối cùng làm giao điểm I2
return true;                                            // Trả về true (chấp nhận đoạn thẳng đã xén)
```
> **Cách nói với cô:**
> *"Dạ vì nãy em gọi đệ quy nửa đầu `[p1, mid]` trước rồi mới tới nửa sau `[mid, p2]`, nên tất cả các đoạn con nằm trong khung được lưu vào `acceptedList` theo đúng thứ tự từ A tới B. Vậy nên ở đây em chỉ cần lấy điểm đầu tiên của đoạn đầu (`front().first`) làm giao điểm $I_1$ và điểm cuối của đoạn cuối (`back().second`) làm giao điểm $I_2$ là ra trọn vẹn phần đoạn thẳng giữ lại ạ."*

---

## 3. TƯƠNG TÁC CHUỘT (`mouseClick`: `Dòng 645 → 706`)

> **Cách nói với cô nếu hỏi phần chuột:**
> *"Dạ em xử lý sự kiện chuột ở hàm `mouseClick` đăng ký qua `glutMouseFunc` ạ. Đầu tiên ở dòng 647 em phải lật lại tọa độ `worldY = windowHeight - y` vì gốc tọa độ chuột của GLUT nằm ở góc trên-trái, ngược với gốc tọa độ vẽ OpenGL ở dưới-trái. Khi người dùng click trái lần 1 thì em lưu điểm A vào `mouseStart`; lúc rê chuột thì hàm `mousePassiveMotion` cập nhật vị trí con trỏ để vẽ xem trước đoạn nào giữ lại màu xanh, đoạn nào bị xén màu đỏ ngay lập tức; đến click trái lần 2 thì chốt điểm B, chạy thuật toán đang chọn rồi lưu vào mảng `lines`. Còn bấm chuột phải là hủy nét đang vẽ dở ạ."*

---

## 4. BỘ HỎI – ĐÁP NHANH KHI VẤN ĐÁP

1. **Nếu cô hỏi: *"So sánh Cohen-Sutherland và Chia nhị phân khác nhau chỗ nào?"***
   > *"Dạ Cohen-Sutherland thì tính thẳng giao điểm bằng phương trình đường thẳng nên ít bước lặp hơn (tối đa 4 lần cắt), nhưng phải dùng phép nhân chia số thực. Còn Chia nhị phân tuy lặp nhiều bước hơn nhưng chỉ cần phép cộng và chia đôi (trên phần cứng tương đương dịch bit phải), không phải chia số thực nên rất nhẹ với phần cứng không có bộ xử lý số thực ạ."*

2. **Nếu cô hỏi: *"Có khi nào `codeA & codeB == 0` mà đoạn thẳng vẫn nằm ngoài khung không?"***
   > *"Dạ có ạ, ví dụ đoạn đi chéo cắt qua góc ngoài của khung: một đầu ở góc trên-trái (`1001`), đầu kia ở dưới-phải (`0110`), AND lại vẫn bằng `0000` dù không đi qua khung. Nhưng khi thuật toán cắt lần đầu theo cạnh trên hoặc dưới, điểm giao mới sẽ cùng phía với đầu kia nên vòng lặp sau `code0 & code1` sẽ khác 0 và tự loại bỏ ngay ạ. Trong dữ liệu mẫu của em có đoạn số 6 ở dòng 385 test đúng trường hợp này luôn."*

3. **Nếu cô hỏi: *"Ở dòng 215 chia cho `(y1 - y0)` thì lỡ đoạn nằm ngang `y1 == y0` có bị lỗi chia cho 0 không?"***
   > *"Dạ không ạ, vì nếu đoạn nằm ngang (`y1 == y0`) thì chỉ có hai khả năng: một là cả hai đầu cùng nằm trên hoặc cùng nằm dưới khung — lúc đó `code0 & code1 != 0` đã loại từ trước rồi; hai là cả hai cùng nằm giữa `ymin` và `ymax` thì bit `TOP` và `BOTTOM` đều bằng 0 nên code sẽ không bao giờ nhảy vào nhánh chia cho `(y1 - y0)` ạ. Đoạn thẳng đứng cũng tương tự như vậy ạ."*

4. **Nếu cô hỏi: *"Mấy cái `vector` trong bài là vector gì?"***
   > *"Dạ bài này là xén hình 2D nên không có vector pháp tuyến hay ma trận 3D đâu ạ, `std::vector` ở đây chỉ là mảng động của C++ để em lưu danh sách các đoạn thẳng `lines` và các đoạn con `acceptedList` thôi ạ."*

---

## 5. PHÍM TẮT DEMO NHANH TRƯỚC MẶT CÔ
- `1`: Chuyển sang **Cohen-Sutherland** (tự động xén lại toàn bộ các đoạn đang có).
- `2`: Chuyển sang **Chia nhị phân (Midpoint Subdivision)**.
- `V`: Đổi chế độ xem (*Hiện cả đoạn bị xén màu đỏ + đoạn giữ lại màu xanh* ↔ *Chỉ hiện đoạn sau khi xén*).
- `P`: Nạp 6 đoạn thẳng mẫu test đủ mọi trường hợp cắt xén.
- `C`: Xóa hết để demo click chuột vẽ đoạn thẳng $AB$ mới.
- `G`: Bật/tắt lưới tọa độ | `ESC`: Hủy vẽ / Thoát.
