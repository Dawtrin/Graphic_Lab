# TÀI LIỆU ÔN VẤN ĐÁP — BT8: KHỬ KHUẤT & TÔ BÓNG HÌNH CHÓP SABC (LAB 16/11)

- **Môn học:** Đồ họa máy tính (Computer Graphics — VKU)
- **Đề bài thực hành (16/11):** Trong không gian 3D, cho hình chóp `SABC`, vẽ và quan sát hình chóp này. Yêu cầu:
  1. **Khử khuất hình chóp bằng:**
     - Thuật toán **Backface** (Khử mặt sau — Backface Culling)
     - Thuật toán **Deep-buffer** (Vùng đệm chiều sâu — Depth-buffer / Z-buffer)
  2. **Tô bóng hình chóp bằng phương pháp tô bóng Gouraud** (Gouraud Shading)
  3. Xuất 3 hình (Backface, Deep-buffer, Gouraud) nộp bài.
- **Mã nguồn:** `Lab_TH_1611/main.cpp` | **Chạy chương trình:** `Lab_TH_1611/run.bat`
- **Tài liệu đối chiếu:** Slide `CG.6-Advanced Graphics Techniques.pdf` (trang 8–20, 24–33).

---

## 1. BẢN ĐỒ CÁC HÀM QUAN TRỌNG TRONG `Lab_TH_1611/main.cpp` (THEO SỐ DÒNG)

*(File dài 1.582 dòng, nhưng **toàn bộ thuật toán cốt lõi cô sẽ hỏi nằm gọn ở 5 cụm dưới đây**!)*

| Khoảng dòng | Hàm quan trọng | Mục đề bài & Vai trò |
| :--- | :--- | :--- |
| `52 → 72` | `dot`, `cross`, `length`, `normalize` | **Toán học 3D** — Tích vô hướng (xét Backface) & Tích có hướng (tính pháp tuyến $\vec{N}$). |
| `97 → 153` | `PyramidSABC::initDefault`, `updateGeometry` | **Hình chóp SABC** — Khai báo 4 đỉnh `S, A, B, C`, 4 mặt ngược chiều kim đồng hồ (CCW), tính pháp tuyến mặt $\vec{N}$ và pháp tuyến đỉnh $\vec{N}_v$. |
| **`156 → 162`** | **`PyramidSABC::evaluateBackface`** | **Yêu cầu 1A (Backface)** — Tính tích vô hướng $\vec{N} \cdot \vec{V}$ để phân loại mặt trước (`HIỆN`) hay mặt sau (`KHUẤT`). |
| **`685 → 734`** | **`renderPyramidBackface`** | **Yêu cầu 1A (Backface)** — Tắt Depth Buffer, bật `GL_CULL_FACE` (`GL_BACK`, `GL_CCW`) để chứng minh riêng Backface khử khuất được khối lồi. |
| **`737 → 825`** | **`renderPyramidDeepBuffer`**, `renderDepthMapPictureInPicture` | **Yêu cầu 1B (Deep-buffer)** — Tắt Backface Culling, bật `GL_DEPTH_TEST` (`GL_LESS`), có chế độ đảo thứ tự vẽ để chứng minh Z-buffer + khung Depth Map mini. |
| **`828 → 978`** | **`drawSubdividedTriangleGouraud`**, **`renderPyramidGouraud`** | **Yêu cầu 2 (Gouraud)** — Thiết lập nguồn sáng `GL_LIGHT0`, vật liệu, chọn `glShadeModel(GL_SMOOTH)` (so sánh với `GL_FLAT`), gắn pháp tuyến đỉnh để nội suy ánh sáng mịn. |

---

## 2. GIẢI THÍCH TỪNG CỤM CODE QUAN TRỌNG (DỊCH THẲNG TỪNG LỆNH + CÁCH NÓI VỚI CÔ)

### 2.1. Toán học vector nền tảng (`Dòng 52 → 72`)

```cpp
// Tính tích vô hướng giữa 2 vector a và b: a.b = ax*bx + ay*by + az*bz
static double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;   // Trả về 1 số thực (dương nếu góc nhọn, âm nếu góc tù)
}

// Tính tích có hướng giữa 2 vector a và b theo quy tắc bàn tay phải
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,                  // Thành phần x của vector vuông góc
        a.z * b.x - a.x * b.z,                  // Thành phần y của vector vuông góc
        a.x * b.y - a.y * b.x                   // Thành phần z của vector vuông góc
    };
}

// Chuẩn hóa vector về độ dài bằng 1 (vector đơn vị)
static Vec3 normalize(const Vec3& v) {
    double l = length(v);                       // Tính độ dài vector v
    if (l < 1e-9) return { 0, 1, 0 };           // Nếu độ dài gần bằng 0 thì trả về trục y mặc định để tránh chia 0
    return v / l;                               // Trả về vector chia cho độ dài của nó
}
```

> **Cách nói với cô:**
> *"Dạ thưa cô, bài này em dùng hai phép toán vector cốt lõi đúng như trong Slide chương 6 ạ: **tích có hướng `cross`** để tìm vector pháp tuyến $\vec{N}$ vuông góc với từng mặt của hình chóp theo quy tắc bàn tay phải, còn **tích vô hướng `dot`** dùng để xét góc giữa pháp tuyến $\vec{N}$ và hướng nhìn $\vec{V}$ trong thuật toán Backface ạ."*

---

### 2.2. Dựng hình chóp `SABC` & Tính pháp tuyến mặt + pháp tuyến đỉnh (`Dòng 97 → 153`)

```cpp
// Khởi tạo 4 đỉnh và 4 mặt của hình chóp SABC (Dòng 97 -> 115)
vertices.push_back({  0.0,  2.2,  0.0 }); // Đỉnh 0: S nằm trên cao
vertices.push_back({ -1.8, -1.0,  1.2 }); // Đỉnh 1: A ở đáy (y = -1)
vertices.push_back({  1.8, -1.0,  1.2 }); // Đỉnh 2: B ở đáy (y = -1)
vertices.push_back({  0.0, -1.0, -1.8 }); // Đỉnh 3: C ở đáy (y = -1)

// Khai báo 4 mặt theo thứ tự ngược chiều kim đồng hồ (CCW) khi nhìn từ ngoài vào
faces.push_back({ "Mat ben SAB", { 0, 1, 2 }, ... }); // Mặt bên SAB
faces.push_back({ "Mat ben SBC", { 0, 2, 3 }, ... }); // Mặt bên SBC
faces.push_back({ "Mat ben SCA", { 0, 3, 1 }, ... }); // Mặt bên SCA
faces.push_back({ "Mat day ABC", { 1, 3, 2 }, ... }); // Mặt đáy ABC (đảo 3 rồi 2 để nhìn từ dưới lên là CCW)

// Bước 1: Tính vector pháp tuyến và trọng tâm cho từng mặt (Dòng 119 -> 127)
for (auto& f : faces) {                               // Lặp qua từng mặt của hình chóp
    Vec3 v0 = vertices[f.vertIdx[0]];                 // Lấy đỉnh thứ nhất của mặt
    Vec3 v1 = vertices[f.vertIdx[1]];                 // Lấy đỉnh thứ hai của mặt
    Vec3 v2 = vertices[f.vertIdx[2]];                 // Lấy đỉnh thứ ba của mặt
    Vec3 e1 = v1 - v0;                                // Tính vector cạnh thứ nhất (từ v0 đến v1)
    Vec3 e2 = v2 - v0;                                // Tính vector cạnh thứ hai (từ v0 đến v2)
    f.normal = normalize(cross(e1, e2));              // Tính pháp tuyến mặt bằng tích có hướng rồi chuẩn hóa
    f.centroid = (v0 + v1 + v2) * (1.0 / 3.0);        // Tính trọng tâm mặt bằng trung bình cộng 3 đỉnh
}

// Bước 2: Tính pháp tuyến tại từng đỉnh cho tô bóng Gouraud (Dòng 144 -> 152)
vertexNormals.assign(vertices.size(), { 0, 0, 0 });   // Khởi tạo pháp tuyến của 4 đỉnh bằng (0, 0, 0)
for (const auto& f : faces) {                         // Lặp qua tất cả các mặt
    for (int idx : f.vertIdx) {                       // Lặp qua 3 đỉnh thuộc mặt đó
        vertexNormals[idx] = vertexNormals[idx] + f.normal; // Cộng dồn pháp tuyến các mặt chung đỉnh lại
    }
}
for (auto& vn : vertexNormals) {                      // Lặp qua 4 pháp tuyến đỉnh vừa cộng
    vn = normalize(vn);                               // Chuẩn hóa về vector đơn vị (tương đương lấy trung bình rồi chuẩn hóa)
}
```

> **Cách nói với cô:**
> *"Dạ khi khai báo 4 mặt của hình chóp `SABC`, em sắp xếp thứ tự 3 đỉnh của mỗi mặt theo **ngược chiều kim đồng hồ (quy tắc bàn tay phải)** khi nhìn từ bên ngoài vào. Nhờ vậy khi lấy tích có hướng `(v1 - v0) x (v2 - v0)` thì vector pháp tuyến $\vec{N}$ luôn **hướng ra ngoài vật thể**. Riêng mặt đáy `ABC` em để thứ tự `A -> C -> B` để pháp tuyến hướng thẳng xuống dưới (`Ny = -1`).*
> *Còn để chuẩn bị cho **tô bóng Gouraud**, ở đoạn dưới em tính **pháp tuyến tại từng đỉnh** bằng cách cộng dồn pháp tuyến của tất cả các mặt chung đỉnh đó rồi chuẩn hóa — đúng công thức trung bình cộng $\vec{n}_p = (\vec{n}_1 + \dots + \vec{n}_k)/k$ trong Slide trang 29 ạ. Ngoài ra em có tính thêm mảng `sideVertexNormals` chỉ gộp 3 mặt bên (không gộp mặt đáy) để khi chiếu sáng thân chóp không bị mặt đáy kéo tối xuống ạ."*

---

### 2.3. Yêu cầu 1A — Thuật toán khử khuất Backface (`Dòng 156 → 162` & `Dòng 685 → 734`)

```cpp
// Phần toán học: Xét mặt trước / mặt sau cho từng mặt (Dòng 156 -> 162)
void evaluateBackface(const Vec3& eyePos) {
    for (auto& f : faces) {                               // Lặp qua từng mặt của hình chóp
        Vec3 viewVec = normalize(eyePos - f.centroid);    // Tính vector hướng nhìn V từ trọng tâm mặt hướng về camera
        f.dotNV = dot(f.normal, viewVec);                 // Tính tích vô hướng giữa pháp tuyến N và hướng nhìn V
        f.isFrontFace = (f.dotNV > 0.0001);               // Nếu N.V > 0 thì là mặt trước (nhìn thấy), ngược lại là mặt sau (bị khuất)
    }
}

// Phần vẽ chế độ Backface (Dòng 685 -> 697)
static void renderPyramidBackface(const Vec3& eyePos) {
    glDisable(GL_LIGHTING);                               // Tắt chiếu sáng để nhìn rõ màu từng mặt
    glDisable(GL_DEPTH_TEST);                             // Cố tình TẮT Depth Buffer để chứng minh riêng Backface đủ khử khuất

    if (g.enableBackfaceCulling) {                        // Nếu đang bật chế độ khử mặt sau
        glEnable(GL_CULL_FACE);                           // Bật chức năng loại bỏ mặt của OpenGL
        glCullFace(GL_BACK);                              // Chỉ định loại bỏ mặt sau (Back-face)
        glFrontFace(GL_CCW);                              // Quy ước mặt có đỉnh ngược chiều kim đồng hồ là mặt trước
    } else {                                              // Ngược lại (khi bấm B để tắt đi so sánh)
        glDisable(GL_CULL_FACE);                          // Tắt khử mặt sau để nhìn thấy cả các mặt phía sau bị vẽ đè
    }
    ...
}
```

> **Cách nói với cô:**
> *"Dạ ở chế độ 1 — **Khử khuất Backface**, em làm theo đúng thuật toán tiếp cận **không gian đối tượng (object-space)** trong Slide trang 11–12 ạ. Với mỗi mặt, em lấy vector hướng nhìn $\vec{V}$ từ trọng tâm mặt về mắt camera rồi tính tích vô hướng $\vec{N} \cdot \vec{V}$. Nếu $\vec{N} \cdot \vec{V} > 0$ (tức góc giữa $\vec{N}$ và $\vec{V}$ nhọn hơn $90^\circ$) thì mặt đó đang hướng về mắt nên **hiển thị (màu xanh)**; còn nếu $\vec{N} \cdot \vec{V} \le 0$ thì mặt quay lưng lại nên **bị loại bỏ (màu đỏ)**.*
> *Đặc biệt ở hàm `renderPyramidBackface`, em **cố tình tắt `GL_DEPTH_TEST`** và chỉ bật `glEnable(GL_CULL_FACE)` với `GL_BACK` và `GL_CCW` để chứng minh cho cô thấy là chỉ riêng Backface đã đủ khử khuất hoàn chỉnh cho khối đa diện lồi như hình chóp SABC rồi ạ."*

---

### 2.4. Yêu cầu 1B — Thuật toán khử khuất Deep-buffer / Z-buffer (`Dòng 737 → 825`)

```cpp
// Phần vẽ chế độ Deep-buffer (Dòng 737 -> 760)
static void renderPyramidDeepBuffer(const Vec3& eyePos) {
    glDisable(GL_LIGHTING);                               // Tắt chiếu sáng để nhìn rõ màu từng mặt
    glDisable(GL_CULL_FACE);                              // Cố tình TẮT Backface Culling để Z-buffer làm toàn bộ khử khuất

    if (g.enableDepthBuffer) {                            // Nếu đang bật Depth Buffer (Z-buffer)
        glEnable(GL_DEPTH_TEST);                          // Bật kiểm tra độ sâu theo từng pixel
        glDepthFunc(GL_LESS);                             // Chỉ giữ lại pixel nào có độ sâu Z nhỏ hơn (gần camera hơn)
    } else {                                              // Ngược lại (khi bấm D để tắt đi so sánh)
        glDisable(GL_DEPTH_TEST);                         // Tắt kiểm tra độ sâu
    }

    // Thí nghiệm đảo thứ tự vẽ để chứng minh tác dụng của Z-buffer (Dòng 750 -> 759)
    std::vector<size_t> drawOrder = { 0, 1, 2, 3 };       // Thứ tự vẽ mặc định 4 mặt
    if (g.simulateReverseDraw) {                          // Nếu bật chế độ đảo thứ tự vẽ
        std::sort(drawOrder.begin(), drawOrder.end(), [&](size_t a, size_t b) {
            double distA = length(eyePos - g.pyramid.faces[a].centroid); // Tính khoảng cách từ camera đến mặt a
            double distB = length(eyePos - g.pyramid.faces[b].centroid); // Tính khoảng cách từ camera đến mặt b
            return distA < distB;                         // Sắp xếp mặt gần camera vẽ TRƯỚC, mặt xa vẽ SAU
        });
    }
    ...
}
```
*(Ghi chú: Ở hàm `display()` dòng 1226 có lệnh `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);` — đây chính là bước khởi tạo `Depth(x,y) = max` và `Fresh(x,y) = Background` trong Slide trang 19.)*

> **Cách nói với cô:**
> *"Dạ ở chế độ 2 — **Khử khuất Deep-buffer (Z-buffer)**, đây là tiếp cận theo **không gian ảnh (image-space)** ở Slide trang 17–20 ạ. Đầu mỗi khung hình em xóa bộ đệm sâu bằng `glClear(GL_DEPTH_BUFFER_BIT)` (khởi tạo toàn bộ độ sâu về giá trị lớn nhất `max`). Sau đó em **cố tình tắt Backface Culling** (`glDisable(GL_CULL_FACE)`) và bật `glEnable(GL_DEPTH_TEST)` với điều kiện `GL_LESS`: tại mỗi pixel, mặt nào có tọa độ $Z$ nhỏ hơn (gần camera hơn) thì sẽ ghi đè màu và cập nhật độ sâu mới vào Z-buffer.*
> *Em có làm thêm nút **Đảo thứ tự vẽ** (`simulateReverseDraw`): khi em cố tình vẽ mặt gần trước, mặt xa sau, nếu **tắt Depth Buffer** (phím `D`) thì mặt xa sẽ vẽ đè lên mặt gần bị sai 3D ngay; còn khi **bật Depth Buffer** lại thì dù vẽ theo thứ tự nào hình chóp vẫn khử khuất chính xác tuyệt đối ạ. Góc trên bên trái còn có khung **Depth Map mini** hiển thị độ sâu từng đỉnh theo thang xám (trắng = gần, đen = xa) ạ."*

---

### 2.5. Yêu cầu 2 — Tô bóng Gouraud (`Dòng 828 → 978`)

```cpp
// Thiết lập nguồn sáng, vật liệu và chế độ tô bóng Gouraud (Dòng 871 -> 898)
static void renderPyramidGouraud(const Vec3& eyePos) {
    glEnable(GL_DEPTH_TEST);                              // Bật khử khuất Z-buffer
    glEnable(GL_LIGHTING);                                // Bật hệ thống chiếu sáng 3D
    glEnable(GL_LIGHT0);                                  // Bật nguồn sáng số 0

    // Cấu hình vị trí và 3 thành phần ánh sáng: môi trường (Ambient), khuếch tán (Diffuse), phản xạ gương (Specular)
    glLightfv(GL_LIGHT0, GL_POSITION, lPos);              // Gán tọa độ nguồn sáng
    glLightfv(GL_LIGHT0, GL_AMBIENT, lAmb);               // Gán ánh sáng môi trường Ia
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lDiff);              // Gán ánh sáng khuếch tán Id (Lambert)
    glLightfv(GL_LIGHT0, GL_SPECULAR, lSpec);             // Gán ánh sáng bóng gương Is

    // Bật chất liệu vật thể và độ bóng (Shininess)
    glEnable(GL_COLOR_MATERIAL);                          // Cho phép lấy màu glColor làm hệ số phản xạ ka, kd
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float)g.shininess); // Gán độ bóng bề mặt

    // Chọn mô hình tô bóng: Gouraud (GL_SMOOTH) hay Flat/Lambert (GL_FLAT)
    if (g.shadingMode == SHADE_GOURAUD) {                 // Nếu đang ở chế độ tô bóng Gouraud
        glShadeModel(GL_SMOOTH);                          // Bật nội suy tuyến tính màu sắc giữa các đỉnh (Gouraud)
    } else {                                              // Ngược lại (khi bấm G chuyển sang Flat)
        glShadeModel(GL_FLAT);                            // Tô bóng hằng: mỗi mặt chỉ có 1 màu đồng nhất (Lambert/Flat)
    }
    ...
}

// Trong hàm vẽ từng mặt (Dòng 916 -> 928):
if (g.shadingMode == SHADE_FLAT) {                        // Nếu là Flat Shading
    glNormal3d(f.normal.x, f.normal.y, f.normal.z);       // Chỉ dùng 1 pháp tuyến chung cho cả mặt tam giác
    // ... vẽ 3 đỉnh v0, v1, v2 ...
} else {                                                  // Ngược lại (Gouraud Shading)
    const auto& n0 = vNorms[i0];                          // Lấy pháp tuyến riêng tại đỉnh 0
    const auto& n1 = vNorms[i1];                          // Lấy pháp tuyến riêng tại đỉnh 1
    const auto& n2 = vNorms[i2];                          // Lấy pháp tuyến riêng tại đỉnh 2
    drawSubdividedTriangleGouraud(v0, v1, v2, n0, n1, n2, subdiv); // Vẽ tam giác gắn pháp tuyến từng đỉnh để nội suy mịn
}
```

> **Cách nói với cô:**
> *"Dạ ở chế độ 3 — **Tô bóng Gouraud**, nguyên lý đúng như Slide trang 28–30 gồm 3 bước ạ:*
> *1. Tính **vector pháp tuyến tại từng đỉnh** bằng trung bình cộng pháp tuyến các mặt kề nhau (`updateGeometry`).*
> *2. Tính **cường độ sáng tại mỗi đỉnh** dựa trên nguồn sáng `GL_LIGHT0` (gồm ánh sáng môi trường Ambient, khuếch tán Lambert $I_s k_d \cos\theta$, và phản xạ bóng Specular).*
> *3. Bật `glShadeModel(GL_SMOOTH)` để OpenGL **nội suy tuyến tính cường độ sáng** từ 3 đỉnh vào toàn bộ các pixel bên trong mặt tam giác.*
> *Em có làm phím `G` để chuyển qua lại giữa **Gouraud (Smooth)** và **Flat Shading (tô bóng hằng)** cho cô so sánh trực tiếp: bên Flat thì mỗi mặt là một mảng màu phẳng tách biệt rõ cạnh, còn bên Gouraud thì ánh sáng chuyển mượt qua các cạnh của hình chóp ạ. Ngoài ra, vì hình chóp chỉ có 4 đỉnh nên tam giác rất lớn, em có thêm thanh trượt `Subdiv` (nếu kéo về `1x` là Gouraud thuần đúng 4 đỉnh, tăng lên `4x` để chia nhỏ lưới tam giác giúp thấy rõ vệt bóng sáng trên thân chóp hơn ạ)."*

---

## 3. BỘ CÂU HỎI PHẢN BIỆN CÔ HAY HỎI & CÁCH TRẢ LỜI

1. **Cô hỏi: *"So sánh thuật toán Backface và Deep-buffer? Khi nào Backface bị sai?"***
   - **Trả lời:**
     - **Backface** làm việc trong **không gian đối tượng (object-space)**, tính rất nhanh bằng tích vô hướng $\vec{N} \cdot \vec{V}$, loại bỏ khoảng 50% số mặt ngay từ đầu. Tuy nhiên nó **chỉ khử khuất đúng cho 1 khối đa diện lồi đơn lẻ** (như hình chóp `SABC`). Nếu có **vật thể lõm** hoặc **nhiều vật thể che khuất nhau**, Backface không biết mặt trước nào đứng gần mắt hơn nên sẽ bị vẽ đè sai.
     - **Deep-buffer (Z-buffer)** làm việc trong **không gian ảnh (image-space)**, so sánh độ sâu $Z$ tại **từng pixel**, nên xử lý đúng cho **mọi loại vật thể** (kể cả vật lõm, nhiều vật giao cắt nhau) và không phụ thuộc thứ tự vẽ; nhược điểm là tốn thêm bộ nhớ cho mảng Z-buffer. Trong thực tế người ta kết hợp cả hai: dùng Backface loại bớt mặt sau cho nhẹ, rồi dùng Z-buffer khử khuất chính xác.

2. **Cô hỏi: *"Trong thuật toán Backface, làm sao em biết vector pháp tuyến hướng ra ngoài chứ không hướng vào trong?"***
   - **Trả lời:** Dạ em sắp xếp thứ tự 3 đỉnh của mỗi mặt theo **quy tắc bàn tay phải (ngược chiều kim đồng hồ — CCW)** khi nhìn từ ngoài vào (ví dụ `SAB`, `SBC`, `SCA`, và mặt đáy nhìn từ dưới lên là `ACB`). Khi lấy tích có hướng `(v1 - v0) x (v2 - v0)`, ngón cái theo quy tắc bàn tay phải luôn chỉ thẳng ra ngoài vật thể ạ.

3. **Cô hỏi: *"Phân biệt tô bóng hằng (Flat/Lambert), tô bóng Gouraud và tô bóng Phong?"***
   - **Trả lời:**
     - **Flat (Lambert):** Dùng 1 pháp tuyến chung cho cả mặt → tính sáng 1 lần cho cả mặt → nhanh nhưng bị lộ rõ từng ô đa giác.
     - **Gouraud:** Dùng pháp tuyến tại từng đỉnh → tính cường độ sáng tại các đỉnh rồi **nội suy màu sắc (cường độ sáng)** vào bên trong mặt → mịn hơn hẳn Flat và vẫn chạy nhanh, nhưng nếu mặt quá to thì có thể bị mất điểm sáng chói (specular) ở giữa mặt.
     - **Phong:** **Nội suy vector pháp tuyến** tại từng pixel bên trong mặt rồi mới tính công thức chiếu sáng cho từng pixel → đẹp và bắt điểm sáng chói chuẩn nhất, nhưng tốn tính toán hơn Gouraud.

4. **Cô hỏi: *"Trong code Gouraud em có nút `PT: Than chuan` (`useSmoothSides`) và `PT: Gop day`, hai cái đó khác nhau thế nào?"***
   - **Trả lời:** Dạ nếu gộp chung pháp tuyến của mặt đáy `ABC` (đang chỉ thẳng xuống dưới `y = -1`) vào các đỉnh `A, B, C` thì pháp tuyến ở chân hình chóp bị kéo chúc xuống đất làm phần dưới thân chóp bị tối đi. Nên em làm thêm tùy chọn `sideVertexNormals` tách riêng pháp tuyến 3 mặt bên với mặt đáy để thân chóp chuyển sáng mượt đẹp hơn, và vẫn giữ nút chuyển sang `Gop day` để đối chiếu đúng công thức gộp toàn bộ 4 mặt ạ.

---

## 4. KỊCH BẢN DEMO 60 GIÂY TRƯỚC MẶT CÔ

Mở chương trình bằng `Lab_TH_1611/run.bat`:

1. **Bước 1 — Khử khuất Backface (mặc định đang ở Tab 1 — phím `1`):**
   - **Thao tác:** Kéo chuột trái xoay nhẹ hình chóp `SABC`, chỉ vào bảng trạng thái 4 mặt bên phải và các mũi tên pháp tuyến xanh (`HIỆN`) / đỏ (`KHUẤT`). Bấm phím **`B`** để tắt rồi bật lại Backface Culling.
   - **Lời nói:** *"Dạ đầu tiên là chế độ khử khuất Backface ạ. Em đã tắt Depth Buffer, chỉ dùng tích vô hướng `dot(N, V)` để xét mặt trước/sau. Khi em xoay chóp, các mặt có `dot > 0` hiện màu xanh, `dot <= 0` hiện màu đỏ và bị loại bỏ. Bấm `B` tắt culling đi thì sẽ thấy các mặt sau lộ ra ngay ạ."*

2. **Bước 2 — Khử khuất Deep-buffer (bấm phím `2`):**
   - **Thao tác:** Bấm nút **`Dao thu tu: [BAT]`**, sau đó bấm phím **`D`** để tắt Depth Buffer (thấy mặt xa vẽ đè lên mặt gần sai 3D), rồi bấm **`D`** bật lại để hình đúng trở lại. Chỉ vào khung **Depth Map mini** góc trên trái.
   - **Lời nói:** *"Sang phím `2` là chế độ Deep-buffer, em tắt hẳn Backface Culling đi. Khi em bật đảo thứ tự vẽ (vẽ mặt gần trước, mặt xa sau) rồi bấm `D` tắt Z-buffer thì hình bị sai do mặt xa đè mặt gần; bật lại `D` thì Z-buffer so sánh độ sâu từng pixel nên hình hiển thị đúng hoàn toàn ạ."*

3. **Bước 3 — Tô bóng Gouraud (bấm phím `3`):**
   - **Thao tác:** Xoay nhẹ hình chóp hoặc kéo thanh trượt **Nguồn sáng X/Y/Z** để thấy vùng sáng dịch chuyển mượt. Bấm phím **`G`** để so sánh **Flat Shading** (mảng phẳng) và **Gouraud Shading** (mịn). Kéo thử `Subdiv` về `1x` và `4x`.
   - **Lời nói:** *"Cuối cùng phím `3` là tô bóng Gouraud ạ. Em tính pháp tuyến đỉnh `Nv` (mũi tên vàng), chiếu sáng từ `Light0` rồi nội suy tuyến tính bằng `GL_SMOOTH`. Bấm `G` có thể thấy rõ sự khác biệt giữa tô bóng hằng Flat và tô bóng mượt Gouraud ạ."*

### Bảng phím tắt nhanh
| Phím | Chức năng |
| :--- | :--- |
| **`1` / `2` / `3`** | Chuyển chế độ: `1` Backface / `2` Deep-buffer / `3` Tô bóng Gouraud |
| **`B`** | Bật / Tắt Backface Culling (ở chế độ 1) |
| **`D`** | Bật / Tắt Depth Buffer / Z-buffer (ở chế độ 2) |
| **`G`** | Chuyển đổi qua lại giữa **Gouraud Shading** và **Flat Shading** (ở chế độ 3) |
| **`N`** | Bật / Tắt hiển thị vector pháp tuyến mặt `N` / pháp tuyến đỉnh `Nv` |
| **`Space`** | Bật / Tắt tự động xoay camera |
| **`P` / `F12`** | Chụp ảnh màn hình chế độ hiện tại ra file `.BMP` |
| **`R`** | Đặt lại góc nhìn và thông số mặc định |
| **Chuột trái / phải (cuộn)** | Kéo chuột trái: Xoay camera | Kéo chuột phải hoặc cuộn: Phóng to / thu nhỏ |
