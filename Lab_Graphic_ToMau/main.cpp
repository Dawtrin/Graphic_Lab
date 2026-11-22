/*
============================================================
CHỨNG MINH TÍNH ĐÚNG ĐẮN
BOUNDARY FILL — ITERATIVE USING EXPLICIT STACK
============================================================

1. PHÁT BIỂU THUẬT TOÁN
   Thuật toán khởi tạo bằng cách đưa điểm hạt giống (seed) vào một Stack.
   Trong quá trình lặp (khi Stack chưa rỗng), thuật toán lấy một điểm từ Stack ra xét.
   Nếu điểm đó không phải là biên (boundaryColor) và chưa được tô màu (fillColor),
   thuật toán sẽ tô màu điểm đó bằng fillColor, sau đó đưa 4 điểm láng giềng (trái, trên, phải, dưới) vào Stack để tiếp tục.

2. CHỨNG MINH THUẬT TOÁN DỪNG
   Mỗi pixel sau khi lấy ra từ Stack và thoả mãn điều kiện sẽ được tô thành fillColor.
   Khi một điểm đã tô vô tình được đẩy lại vào Stack và pop ra xét ở vòng lặp sau, màu của nó lúc này đã là fillColor.
   Thuật toán lập tức bỏ qua (không tô và không push thêm láng giềng).
   Vì số lượng pixel của cửa sổ đồ họa là hữu hạn (N pixel), mỗi pixel được xử lý tô màu tối đa 1 lần, nên số lượt push/pop vào Stack là hữu hạn. Cuối cùng Stack chắc chắn sẽ rỗng và thuật toán dừng.

3. CHỨNG MINH KHÔNG TÔ RA NGOÀI VÙNG KÍN
   Pixel biên bị chặn: nó không được tô và không được lan tiếp.
   Thuật toán chỉ di chuyển 4 hướng. Đường biên Bresenham là 8-liên thông
   (các điểm kề nhau có thể chéo góc), nên một bước đi 4 hướng không thể
   "lách" qua khe chéo của biên. Nếu biên khép kín thì mọi đường đi 4 hướng
   từ trong ra ngoài đều phải cắt biên, tức không có đường thoát.
   (Nếu biên bị hở, màu sẽ lan ra ngoài — đó là hành vi đúng.)

4. CHỨNG MINH TÔ HẾT VÙNG
   Giả sử khi stack rỗng vẫn còn pixel P thuộc vùng liên thông 4 hướng
   (không chứa biên) với seed S mà chưa được tô.
   Lấy một đường đi 4 hướng S -> ... -> P. Gọi B là pixel đầu tiên chưa tô
   trên đường đi, A là pixel đứng ngay trước B (A đã tô).
   Khi tô A, thuật toán đã push B. Stack rỗng nên B đã được pop.
   Lúc pop, B không ngoài canvas, không phải biên, không mang fillColor
   (vì B chưa tô) nên B phải được tô — mâu thuẫn.
   Vậy toàn bộ vùng liên thông 4 hướng chứa seed đều được tô.

5. ĐỘ PHỨC TẠP
   - Thời gian: O(N) (với N là số lượng thao tác pop từ Stack, giới hạn bởi 4 lần số lượng pixel cần tô). Mỗi pixel trong vùng được xử lý trạng thái thực tế đúng 1 lần.
   - Bộ nhớ:
     - pixelBuffer: O(W×H)
     - stack: O(N) (Kích thước lớn nhất của stack)
============================================================
*/

#include <GL/glut.h>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <stack>
#include <queue>
#include <string>
#include <vector>
#include <chrono>

// =========================================================
// CẤU HÌNH CỐ ĐỊNH
// =========================================================
constexpr int WINDOW_WIDTH  = 1000;
constexpr int WINDOW_HEIGHT = 700;

constexpr int HUD_HEIGHT    = 96;   // Thanh HUD bên trên
constexpr int STATUS_HEIGHT = 28;   // Thanh trạng thái bên dưới

constexpr int CANVAS_Y_MIN = STATUS_HEIGHT;
constexpr int CANVAS_Y_MAX = WINDOW_HEIGHT - HUD_HEIGHT - 1;

struct Point {
    int x;
    int y;
};

struct Color {
    float r, g, b;
};

const Color BOUNDARY_COLOR   = {0.0f, 0.0f, 0.0f}; // Đen
const Color BACKGROUND_COLOR = {0.98f, 0.98f, 1.00f}; // Trắng ngà nhẹ (canvas)

// Palette 16 màu phong phú (Cyberpunk / Fantasy)
constexpr int NUM_COLORS = 16;
const Color PALETTE[NUM_COLORS] = {
    {0.92f, 0.20f, 0.25f}, // 0: Đỏ Ruby (Ruby Red)
    {0.98f, 0.45f, 0.15f}, // 1: Cam Lửa (Flame Orange)
    {1.00f, 0.82f, 0.10f}, // 2: Vàng Hoàng Kim (Golden Yellow)
    {0.55f, 0.85f, 0.15f}, // 3: Xanh Lá Non (Lime Green)
    {0.12f, 0.75f, 0.45f}, // 4: Xanh Ngọc Lục Bảo (Emerald Green)
    {0.05f, 0.85f, 0.82f}, // 5: Xanh Ngọc Bích / Cyan (Turquoise / Cyan)
    {0.20f, 0.65f, 0.98f}, // 6: Xanh Da Trời (Sky Blue)
    {0.15f, 0.38f, 0.92f}, // 7: Xanh Lam Hoàng Gia (Royal Blue)
    {0.50f, 0.22f, 0.90f}, // 8: Tím Ma Thuật (Arcane Purple)
    {0.78f, 0.22f, 0.85f}, // 9: Tím Hoa Cà (Magenta Violet)
    {0.95f, 0.30f, 0.65f}, // 10: Hồng Neon (Hot Pink)
    {0.96f, 0.62f, 0.72f}, // 11: Hồng Phấn (Pastel Rose)
    {0.72f, 0.45f, 0.22f}, // 12: Nâu Đồng (Bronze / Leather)
    {0.52f, 0.58f, 0.68f}, // 13: Xám Bạc Kim Loại (Steel Silver)
    {0.22f, 0.25f, 0.32f}, // 14: Xám Đen Tro (Dark Slate)
    {0.85f, 0.92f, 0.98f}  // 15: Băng Tuyết Sáng (Frost Ice / Light Silver)
};

// Màu giao diện
const Color UI_PANEL   = {0.12f, 0.14f, 0.19f}; // Xanh đen đậm
const Color UI_TEXT    = {0.95f, 0.96f, 0.98f};
const Color UI_MUTED   = {0.60f, 0.65f, 0.75f};
const Color UI_DRAW    = {0.20f, 0.52f, 0.95f}; // Xanh dương
const Color UI_FILLCLR = {0.18f, 0.72f, 0.45f}; // Xanh lá
const Color UI_STATUS  = {0.94f, 0.95f, 0.97f}; // Xám sáng
const Color UI_STATUSTX= {0.15f, 0.17f, 0.22f};
const Color UI_BORDER  = {0.80f, 0.80f, 0.85f};

// =========================================================
// TRẠNG THÁI CHƯƠNG TRÌNH
// =========================================================
enum class Mode { DRAW, FILL };
enum class FillAlgo { STACK_DFS = 0, QUEUE_BFS = 1, RECURSIVE = 2 };

Mode               g_mode = Mode::DRAW;
std::vector<Point> g_vertices;
bool               g_closed = false;
bool               g_hasSeed = false;
Point              g_seed = {0, 0};
int                g_mouseX = 0, g_mouseY = 0;
bool               g_mouseValid = false;
std::string        g_status = "Welcome! Click on the canvas to draw a polygon.";
int                g_paletteIdx = 0;
FillAlgo           g_algo = FillAlgo::STACK_DFS;
bool               g_animate = true;
bool               g_recursiveCapped = false;

struct FillStats {
    std::size_t filled    = 0;
    std::size_t peakStack = 0;
    long long   timeMs    = 0;
};
FillStats g_stats;

// =========================================================
// PIXEL BUFFER
// =========================================================
Color pixelBuffer[WINDOW_HEIGHT][WINDOW_WIDTH];

bool colorsEqual(const Color& a, const Color& b) {
    const float epsilon = 0.01f;
    return (std::abs(a.r - b.r) < epsilon &&
            std::abs(a.g - b.g) < epsilon &&
            std::abs(a.b - b.b) < epsilon);
}

// Kiểm tra điểm có nằm trong TOÀN BỘ buffer không (dành cho getPixel/putPixel chung)
bool isInsideBuffer(int x, int y) {
    return (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT);
}

// Kiểm tra điểm có nằm trong CANVAS không (dành RIÊNG cho thuật toán fill để chặn tràn)
bool isInsideCanvas(int x, int y) {
    return (x >= 0 && x < WINDOW_WIDTH && y >= CANVAS_Y_MIN && y <= CANVAS_Y_MAX);
}

Color getPixel(int x, int y) {
    if (isInsideBuffer(x, y)) {
        return pixelBuffer[y][x];
    }
    return BACKGROUND_COLOR;
}

void putPixel(int x, int y, Color color) {
    if (isInsideBuffer(x, y)) {
        pixelBuffer[y][x] = color;
    }
}

// =========================================================
// BOUNDARY FILL — PHƯƠNG PHÁP ĐỆ QUY (RECURSIVE, SLIDE TRANG 84)
// =========================================================
void boundaryFillRecursiveHelper(int x, int y, Color fillColor, Color boundaryColor, int depth) {
    if (!isInsideCanvas(x, y)) return;
    if (depth > 15000) {
        g_recursiveCapped = true; // Chặn tràn Call Stack khi vùng tô quá lớn
        return;
    }
    if ((std::size_t)depth > g_stats.peakStack) g_stats.peakStack = (std::size_t)depth;

    Color c = getPixel(x, y);
    if (!colorsEqual(c, boundaryColor) && !colorsEqual(c, fillColor)) {
        putPixel(x, y, fillColor);
        ++g_stats.filled;

        boundaryFillRecursiveHelper(x - 1, y, fillColor, boundaryColor, depth + 1); // Trái
        boundaryFillRecursiveHelper(x, y + 1, fillColor, boundaryColor, depth + 1); // Trên
        boundaryFillRecursiveHelper(x + 1, y, fillColor, boundaryColor, depth + 1); // Phải
        boundaryFillRecursiveHelper(x, y - 1, fillColor, boundaryColor, depth + 1); // Dưới
    }
}

void boundaryFillRecursive(int sx, int sy, Color fillColor, Color boundaryColor) {
    g_stats = FillStats{};
    g_recursiveCapped = false;
    auto start_time = std::chrono::high_resolution_clock::now();

    boundaryFillRecursiveHelper(sx, sy, fillColor, boundaryColor, 1);

    auto end_time = std::chrono::high_resolution_clock::now();
    g_stats.timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
}

// =========================================================
// BOUNDARY FILL — KHỬ ĐỆ QUY (STACK / QUEUE, SLIDE TRANG 85-88)
// =========================================================
void boundaryFill(int sx, int sy, Color fillColor, Color boundaryColor) {
    if (g_algo == FillAlgo::RECURSIVE) {
        boundaryFillRecursive(sx, sy, fillColor, boundaryColor);
        return;
    }

    bool useBFS = (g_algo == FillAlgo::QUEUE_BFS);
    g_stats = FillStats{};
    g_recursiveCapped = false;
    auto start_time = std::chrono::high_resolution_clock::now();

    std::stack<Point> st;
    std::queue<Point> q;

    if (useBFS) q.push({sx, sy});
    else st.push({sx, sy});

    while (useBFS ? !q.empty() : !st.empty()) {
        Point p;
        if (useBFS) {
            p = q.front(); q.pop();
        } else {
            p = st.top(); st.pop();
        }

        // Chặn vùng fill không cho tràn vào HUD/Status bar
        if (!isInsideCanvas(p.x, p.y)) continue;
        
        Color c = getPixel(p.x, p.y);
        
        // Điều kiện dừng nguyên bản: chỉ tô nếu pixel không phải biên và chưa tô
        if (!colorsEqual(c, boundaryColor) && !colorsEqual(c, fillColor)) {
            putPixel(p.x, p.y, fillColor);
            ++g_stats.filled;

            // ANIMATION HACK: Cập nhật màn hình mỗi 100 pixels để tạo hiệu ứng tô loang
            if (g_animate && (g_stats.filled % 100 == 0)) {
                glRasterPos2i(0, 0);
                glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_FLOAT, pixelBuffer);
                glutSwapBuffers();
            }

            if (useBFS) {
                q.push({p.x - 1, p.y});
                q.push({p.x, p.y + 1});
                q.push({p.x + 1, p.y});
                q.push({p.x, p.y - 1});
            } else {
                // Stack LIFO -> push ngược để Trái pop trước
                st.push({p.x, p.y - 1}); // Dưới
                st.push({p.x + 1, p.y}); // Phải
                st.push({p.x, p.y + 1}); // Trên
                st.push({p.x - 1, p.y}); // Trái
            }
        }

        std::size_t currentSize = useBFS ? q.size() : st.size();
        if (currentSize > g_stats.peakStack) g_stats.peakStack = currentSize;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    g_stats.timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
}

// =========================================================
// BRESENHAM & QUẢN LÝ POLYGON
// =========================================================
void drawLineBresenham(int x1, int y1, int x2, int y2, Color color) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (isInsideCanvas(x1, y1)) putPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void initBuffer() {
    for (int y = 0; y < WINDOW_HEIGHT; ++y)
        for (int x = 0; x < WINDOW_WIDTH; ++x)
            pixelBuffer[y][x] = BACKGROUND_COLOR;
}

void fullClear() {
    initBuffer();
    g_vertices.clear();
    g_closed  = false;
    g_hasSeed = false;
    g_mode    = Mode::DRAW;
    g_stats   = FillStats{};
    g_status  = "Canvas cleared. Click to draw.";
}

void startNewShape() {
    g_vertices.clear();
    g_closed  = false;
    g_hasSeed = false;
    g_mode    = Mode::DRAW;
    g_status  = "New shape. Draw the next polygon.";
}

void addVertex(Point p) {
    if (!g_vertices.empty()) {
        Point prev = g_vertices.back();
        if (prev.x == p.x && prev.y == p.y) return;
        drawLineBresenham(prev.x, prev.y, p.x, p.y, BOUNDARY_COLOR);
    } else {
        if (isInsideCanvas(p.x, p.y)) putPixel(p.x, p.y, BOUNDARY_COLOR);
    }
    g_vertices.push_back(p);
}

void closePolygon() {
    if (g_vertices.size() < 3) {
        g_status = "Need at least 3 vertices to close a polygon.";
        return;
    }
    Point a = g_vertices.back();
    Point b = g_vertices.front();
    drawLineBresenham(a.x, a.y, b.x, b.y, BOUNDARY_COLOR);
    g_closed = true;
    g_mode   = Mode::FILL;
    g_status = "Polygon closed. Click INSIDE to choose a seed point.";
}

void finishOpen() {
    if (g_vertices.size() < 2) {
        g_status = "Need at least 2 vertices.";
        return;
    }
    g_closed = false;
    g_mode   = Mode::FILL;
    g_status = "Boundary OPEN. Click a seed to watch it leak.";
}

void buildCircle(int cx, int cy, int r, int n) {
    const double pi = 3.14159265358979323846;
    startNewShape();
    for (int i = 0; i < n; ++i) {
        double a = 2.0 * pi * i / n;
        addVertex({(int)std::lround(cx + r * std::cos(a)),
                   (int)std::lround(cy + r * std::sin(a))});
    }
    closePolygon();
}

void buildShatteredSword(int cx, int cy) {
    fullClear();

    // 1. Pommel (Chuôi kiếm - Lục giác)
    startNewShape();
    addVertex({cx, 45});
    addVertex({cx + 16, 58});
    addVertex({cx + 16, 72});
    addVertex({cx, 85});
    addVertex({cx - 16, 72});
    addVertex({cx - 16, 58});
    closePolygon();

    // 2. Tay cầm (Grip) - 3 đoạn sọc
    startNewShape();
    addVertex({cx - 11, 85}); addVertex({cx + 11, 85});
    addVertex({cx + 10, 105}); addVertex({cx - 10, 105});
    closePolygon();

    startNewShape();
    addVertex({cx - 10, 105}); addVertex({cx + 10, 105});
    addVertex({cx + 10, 125}); addVertex({cx - 10, 125});
    closePolygon();

    startNewShape();
    addVertex({cx - 10, 125}); addVertex({cx + 10, 125});
    addVertex({cx + 11, 145}); addVertex({cx - 11, 145});
    closePolygon();

    // 3. Bảo hộ thủ (Crossguard) - Cánh hộ vệ tay cong nhọn
    startNewShape();
    addVertex({cx - 11, 145});
    addVertex({cx - 65, 135});
    addVertex({cx - 115, 160});
    addVertex({cx - 125, 180});
    addVertex({cx - 75, 175});
    addVertex({cx - 25, 165});
    addVertex({cx, 160});
    addVertex({cx - 15, 150});
    closePolygon();

    startNewShape();
    addVertex({cx + 11, 145});
    addVertex({cx + 65, 135});
    addVertex({cx + 115, 160});
    addVertex({cx + 125, 180});
    addVertex({cx + 75, 175});
    addVertex({cx + 25, 165});
    addVertex({cx, 160});
    addVertex({cx + 15, 150});
    closePolygon();

    // Viên ngọc tâm hộ thủ (Magic Gem)
    startNewShape();
    addVertex({cx, 148});
    addVertex({cx + 14, 162});
    addVertex({cx, 176});
    addVertex({cx - 14, 162});
    closePolygon();

    // 4. Lưỡi kiếm nứt vỡ (Shattered Blade Shards)
    // TẦNG 1: Gốc lưỡi kiếm
    startNewShape();
    addVertex({cx - 36, 175});
    addVertex({cx - 38, 230});
    addVertex({cx - 12, 215});
    addVertex({cx, 176});
    addVertex({cx - 25, 165});
    closePolygon();

    startNewShape();
    addVertex({cx, 176});
    addVertex({cx - 12, 215});
    addVertex({cx + 6, 240});
    addVertex({cx + 15, 205});
    closePolygon();

    startNewShape();
    addVertex({cx, 176});
    addVertex({cx + 15, 205});
    addVertex({cx + 6, 240});
    addVertex({cx + 38, 230});
    addVertex({cx + 36, 175});
    addVertex({cx + 25, 165});
    closePolygon();

    // TẦNG 2: Thân dưới
    startNewShape();
    addVertex({cx - 38, 230});
    addVertex({cx - 39, 290});
    addVertex({cx - 20, 275});
    addVertex({cx - 5, 285});
    addVertex({cx - 12, 215});
    closePolygon();

    startNewShape();
    addVertex({cx - 12, 215});
    addVertex({cx - 5, 285});
    addVertex({cx + 18, 270});
    addVertex({cx + 6, 240});
    closePolygon();

    startNewShape();
    addVertex({cx + 6, 240});
    addVertex({cx + 18, 270});
    addVertex({cx + 40, 305});
    addVertex({cx + 38, 230});
    closePolygon();

    // TẦNG 3: Tâm nứt vỡ (Shatter Core)
    startNewShape();
    addVertex({cx - 39, 290});
    addVertex({cx - 40, 360});
    addVertex({cx - 18, 345});
    addVertex({cx - 20, 275});
    closePolygon();

    startNewShape();
    addVertex({cx - 20, 275});
    addVertex({cx - 18, 345});
    addVertex({cx, 330});
    addVertex({cx - 5, 285});
    closePolygon();

    startNewShape();
    addVertex({cx - 5, 285});
    addVertex({cx, 330});
    addVertex({cx + 12, 350});
    addVertex({cx + 22, 320});
    addVertex({cx + 18, 270});
    closePolygon();

    startNewShape();
    addVertex({cx + 18, 270});
    addVertex({cx + 22, 320});
    addVertex({cx + 40, 370});
    addVertex({cx + 40, 305});
    closePolygon();

    // TẦNG 4: Thân trên
    startNewShape();
    addVertex({cx - 40, 360});
    addVertex({cx - 36, 435});
    addVertex({cx - 15, 410});
    addVertex({cx - 18, 345});
    closePolygon();

    startNewShape();
    addVertex({cx - 18, 345});
    addVertex({cx - 15, 410});
    addVertex({cx - 2, 435});
    addVertex({cx, 330});
    closePolygon();

    startNewShape();
    addVertex({cx, 330});
    addVertex({cx - 2, 435});
    addVertex({cx + 18, 420});
    addVertex({cx + 12, 350});
    closePolygon();

    startNewShape();
    addVertex({cx + 12, 350});
    addVertex({cx + 18, 420});
    addVertex({cx + 35, 440});
    addVertex({cx + 40, 370});
    closePolygon();

    // TẦNG 5: Mũi kiếm nứt gãy
    startNewShape();
    addVertex({cx - 36, 435});
    addVertex({cx - 24, 495});
    addVertex({cx - 10, 480});
    addVertex({cx - 15, 410});
    closePolygon();

    startNewShape();
    addVertex({cx + 18, 420});
    addVertex({cx + 12, 475});
    addVertex({cx + 25, 495});
    addVertex({cx + 35, 440});
    closePolygon();

    startNewShape();
    addVertex({cx - 15, 410});
    addVertex({cx - 10, 480});
    addVertex({cx, 515});
    addVertex({cx + 12, 475});
    addVertex({cx + 18, 420});
    addVertex({cx - 2, 435});
    closePolygon();

    startNewShape();
    addVertex({cx - 24, 495});
    addVertex({cx, 575});
    addVertex({cx, 515});
    addVertex({cx - 10, 480});
    closePolygon();

    startNewShape();
    addVertex({cx + 25, 495});
    addVertex({cx, 575});
    addVertex({cx, 515});
    addVertex({cx + 12, 475});
    closePolygon();

    g_mode = Mode::FILL;
    g_status = "SHATTERED BLADE loaded! Click on shards to fill with magic colors.";
}

void loadPreset(int which) {
    fullClear();
    int cx = WINDOW_WIDTH / 2;
    int cy = CANVAS_Y_MIN + (CANVAS_Y_MAX - CANVAS_Y_MIN) / 2;

    switch (which) {
        case 1: // Lục giác
            buildCircle(cx, cy, 200, 6);
            g_status = "TEST 1: Hexagon.";
            break;
        case 2: // Đường tròn (48 đỉnh)
            buildCircle(cx, cy, 200, 48);
            g_status = "TEST 2: Circle.";
            break;
        case 3: // Biên hở
            startNewShape();
            addVertex({cx - 150, cy - 100});
            addVertex({cx + 150, cy - 100});
            addVertex({cx + 150, cy + 150});
            addVertex({cx - 150, cy + 150});
            addVertex({cx - 150, cy}); // Hở phần cạnh trái
            finishOpen();
            g_status = "TEST 3: Open boundary.";
            break;
        case 4: // Giao 2 hình tròn
            buildCircle(cx - 100, cy, 150, 32);
            buildCircle(cx + 100, cy, 150, 32);
            g_status = "TEST 4: Intersecting circles.";
            break;
        case 5: // Tam giác + vuông
            startNewShape();
            addVertex({cx, cy + 200});
            addVertex({cx + 150, cy - 50});
            addVertex({cx - 150, cy - 50});
            closePolygon();
            startNewShape();
            addVertex({cx - 100, cy - 150});
            addVertex({cx + 100, cy - 150});
            addVertex({cx + 100, cy + 50});
            addVertex({cx - 100, cy + 50});
            closePolygon();
            g_status = "TEST 5: Triangle and Square.";
            break;
        case 6: // Grid 3x3
            for(int i = 0; i < 3; ++i) {
                for(int j = 0; j < 3; ++j) {
                    startNewShape();
                    addVertex({cx - 180 + i*120, cy - 180 + j*120});
                    addVertex({cx - 180 + i*120 + 100, cy - 180 + j*120});
                    addVertex({cx - 180 + i*120 + 100, cy - 180 + j*120 + 100});
                    addVertex({cx - 180 + i*120, cy - 180 + j*120 + 100});
                    closePolygon();
                }
            }
            g_status = "TEST 6: 3x3 Grid.";
            break;
        case 7: { // Star Mandala (Ngôi sao 8 cánh lồng nhau)
            int s = 140;
            startNewShape();
            addVertex({cx - s, cy - s});
            addVertex({cx + s, cy - s});
            addVertex({cx + s, cy + s});
            addVertex({cx - s, cy + s});
            closePolygon();

            int d = (int)(s * 1.414);
            startNewShape();
            addVertex({cx, cy - d});
            addVertex({cx + d, cy});
            addVertex({cx, cy + d});
            addVertex({cx - d, cy});
            closePolygon();

            buildCircle(cx, cy, 90, 36);
            g_status = "TEST 7: 8-Point Star Mandala (16 intersecting regions).";
            break;
        }
        case 8: { // Olympic 5 Rings (5 vòng tròn lồng nhau)
            int r = 68;
            int dx = 90;
            int dy = 55;
            buildCircle(cx - dx, cy + dy/2, r, 32);
            buildCircle(cx, cy + dy/2, r, 32);
            buildCircle(cx + dx, cy + dy/2, r, 32);
            buildCircle(cx - dx/2, cy - dy/2, r, 32);
            buildCircle(cx + dx/2, cy - dy/2, r, 32);
            g_status = "TEST 8: Olympic Rings (Interlocking circles).";
            break;
        }
        case 9: { // Flower of Life (Bông hoa 6 cánh)
            const double pi = 3.14159265358979323846;
            int r = 110;
            buildCircle(cx, cy, r, 36);
            for (int i = 0; i < 6; ++i) {
                double a = 2.0 * pi * i / 6.0;
                int ox = cx + (int)std::lround(r * std::cos(a));
                int oy = cy + (int)std::lround(r * std::sin(a));
                buildCircle(ox, oy, r, 36);
            }
            g_status = "TEST 9: Flower of Life (Geometric petals).";
            break;
        }
    }
}

// =========================================================
// HIỂN THỊ
// =========================================================
void setColor(const Color& c) { glColor3f(c.r, c.g, c.b); }

void drawText(float x, float y, const std::string& s, void* font, const Color& c) {
    setColor(c);
    glRasterPos2f(x, y);
    for (char ch : s) glutBitmapCharacter(font, ch);
}

int textWidth(const std::string& s, void* font) {
    int w = 0;
    for (char ch : s) w += glutBitmapWidth(font, ch);
    return w;
}

void fillRect(float x0, float y0, float x1, float y1, const Color& c) {
    setColor(c);
    glRectf(x0, y0, x1, y1);
}

struct Button {
    std::string label;
    int x, y, w, h;
    char key;
    bool active;
};
std::vector<Button> g_buttons;

void drawButton(const Button& b, bool isHovered) {
    Color bg = b.active ? UI_DRAW : Color{0.25f, 0.28f, 0.35f};
    if (isHovered) { bg.r += 0.1f; bg.g += 0.1f; bg.b += 0.1f; }
    Color fg = b.active ? UI_TEXT : UI_MUTED;
    
    fillRect(b.x, b.y, b.x + b.w, b.y + b.h, bg);
    int tw = textWidth(b.label, GLUT_BITMAP_HELVETICA_12);
    drawText(b.x + (b.w - tw)/2, b.y + b.h/2 - 4, b.label, GLUT_BITMAP_HELVETICA_12, fg);
}

void drawOverlay() {
    glLineWidth(1.0f);
    setColor(BOUNDARY_COLOR);
    if (g_vertices.size() >= 2) {
        glBegin(g_closed ? GL_LINE_LOOP : GL_LINE_STRIP);
        for (const Point& p : g_vertices) glVertex2f(p.x + 0.5f, p.y + 0.5f);
        glEnd();
    }

    if (g_mode == Mode::DRAW) {
        if (!g_vertices.empty() && g_mouseValid) {
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(2, 0xAAAA);
            glColor3f(0.45f, 0.45f, 0.5f);
            glBegin(GL_LINES);
            glVertex2f(g_vertices.back().x + 0.5f, g_vertices.back().y + 0.5f);
            glVertex2f(g_mouseX + 0.5f, g_mouseY + 0.5f);
            glEnd();
            glDisable(GL_LINE_STIPPLE);
        }
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        for (std::size_t i = 0; i < g_vertices.size(); ++i) {
            if (i == 0) setColor(UI_FILLCLR); else setColor(UI_DRAW);
            glVertex2f(g_vertices[i].x + 0.5f, g_vertices[i].y + 0.5f);
        }
        glEnd();
    }

    if (g_hasSeed) {
        glPointSize(8.0f);
        glColor3f(0.1f, 0.2f, 0.85f);
        glBegin(GL_POINTS);
        glVertex2f(g_seed.x + 0.5f, g_seed.y + 0.5f);
        glEnd();
    }

    // Vẽ viền canvas
    glLineWidth(1.0f);
    setColor(UI_BORDER);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.5f, CANVAS_Y_MIN + 0.5f);
    glVertex2f(WINDOW_WIDTH - 0.5f, CANVAS_Y_MIN + 0.5f);
    glVertex2f(WINDOW_WIDTH - 0.5f, CANVAS_Y_MAX + 0.5f);
    glVertex2f(0.5f, CANVAS_Y_MAX + 0.5f);
    glEnd();
}

void drawHud() {
    void* big   = GLUT_BITMAP_HELVETICA_18;
    void* small = GLUT_BITMAP_HELVETICA_12;
    const float top = (float)WINDOW_HEIGHT;
    const float hudY = top - HUD_HEIGHT;
    const bool  draw = (g_mode == Mode::DRAW);
    const Color accent = draw ? UI_DRAW : UI_FILLCLR;

    // HUD Panel
    fillRect(0, hudY, WINDOW_WIDTH, top, UI_PANEL);
    fillRect(0, hudY, WINDOW_WIDTH, hudY + 3, accent); // Viền phân cách

    // Hàng 1: Tiêu đề
    drawText(20, top - 30, "BOUNDARY FILL STUDIO", big, UI_TEXT);

    // Bảng 16 ô màu trực quan (Click chọn trực tiếp)
    int swatchStartX = 280;
    int swatchY = top - 38;
    int swatchSize = 20;
    int swatchGap = 4;
    for (int i = 0; i < NUM_COLORS; ++i) {
        int sx = swatchStartX + i * (swatchSize + swatchGap);
        if (i == g_paletteIdx) {
            fillRect(sx - 3, swatchY - 3, sx + swatchSize + 3, swatchY + swatchSize + 3, UI_TEXT);
            fillRect(sx - 2, swatchY - 2, sx + swatchSize + 2, swatchY + swatchSize + 2, UI_DRAW);
        } else {
            fillRect(sx - 1, swatchY - 1, sx + swatchSize + 1, swatchY + swatchSize + 1, Color{0.3f, 0.33f, 0.4f});
        }
        fillRect(sx, swatchY, sx + swatchSize, swatchY + swatchSize, PALETTE[i]);
    }

    // Huy hiệu Mode
    fillRect(20, top - 70, 110, top - 45, accent);
    drawText(30, top - 62, draw ? "DRAW MODE" : "FILL MODE", small, UI_TEXT);

    // Lời khuyên
    std::string hint = draw ? "Click to add vertex (ENTER: close, O: leave open)" 
                            : "Click on canvas to fill. Click any color above to pick!";
    drawText(130, top - 62, hint, small, UI_MUTED);

    // Trạng thái thuật toán (Stack / Queue / Recursive)
    std::string traversalInfo = (g_algo == FillAlgo::STACK_DFS) ? "Algo: DFS (Stack)"
                              : (g_algo == FillAlgo::QUEUE_BFS) ? "Algo: BFS (Queue)"
                                                                : "Algo: Recursive";
    drawText((float)(WINDOW_WIDTH - 20 - textWidth(traversalInfo, small)), top - 30, traversalInfo, small, UI_MUTED);

    // Vẽ các nút ảo
    g_buttons.clear();
    int bx = WINDOW_WIDTH - 930;
    int by = top - 80;
    g_buttons.push_back({"DRAW (D)", bx, by, 70, 24, 'd', g_mode == Mode::DRAW}); bx += 74;
    g_buttons.push_back({"FILL (L)", bx, by, 65, 24, 'l', g_mode == Mode::FILL}); bx += 69;
    g_buttons.push_back({"ANIM (A)", bx, by, 65, 24, 'a', g_animate}); bx += 69;
    g_buttons.push_back({"NEW (N)", bx, by, 65, 24, 'n', false}); bx += 69;
    g_buttons.push_back({"COLOR (F)", bx, by, 75, 24, 'f', false}); bx += 79;
    g_buttons.push_back({"ALGO (B)", bx, by, 85, 24, 'b', g_algo != FillAlgo::STACK_DFS}); bx += 89;
    g_buttons.push_back({"P1", bx, by, 25, 24, '1', false}); bx += 28;
    g_buttons.push_back({"P4", bx, by, 25, 24, '4', false}); bx += 28;
    g_buttons.push_back({"P5", bx, by, 25, 24, '5', false}); bx += 28;
    g_buttons.push_back({"P6", bx, by, 25, 24, '6', false}); bx += 28;
    g_buttons.push_back({"P7", bx, by, 25, 24, '7', false}); bx += 28;
    g_buttons.push_back({"P8", bx, by, 25, 24, '8', false}); bx += 28;
    g_buttons.push_back({"P9", bx, by, 25, 24, '9', false}); bx += 29;
    g_buttons.push_back({"SWORD (S)", bx, by, 78, 24, 's', false}); bx += 82;
    g_buttons.push_back({"CLEAR (C)", bx, by, 75, 24, 'c', false});

    for (const auto& btn : g_buttons) {
        bool hover = (g_mouseX >= btn.x && g_mouseX <= btn.x + btn.w &&
                      g_mouseY >= btn.y && g_mouseY <= btn.y + btn.h);
        drawButton(btn, hover);
    }

    // Status bar
    fillRect(0, 0, WINDOW_WIDTH, STATUS_HEIGHT, UI_STATUS);
    fillRect(0, STATUS_HEIGHT - 1, WINDOW_WIDTH, STATUS_HEIGHT, UI_BORDER);
    
    drawText(10, 8, g_status, small, UI_STATUSTX);

    char stats[200];
    snprintf(stats, sizeof(stats), "Vertices: %zu  |  Filled: %zu px  |  Peak stack: %zu  |  Time: %lld ms",
             g_vertices.size(), g_stats.filled, g_stats.peakStack, g_stats.timeMs);
    std::string s_stats = stats;
    drawText((float)(WINDOW_WIDTH - 10 - textWidth(s_stats, small)), 8, s_stats, small, UI_STATUSTX);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(0, 0);
    glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_FLOAT, pixelBuffer);
    drawOverlay();
    drawHud();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (w != WINDOW_WIDTH || h != WINDOW_HEIGHT) {
        glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
        return;
    }
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// =========================================================
// TƯƠNG TÁC
// =========================================================
void keyboard(unsigned char key, int x, int y);

void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

    int glY = WINDOW_HEIGHT - 1 - y;

    // Kiểm tra click vào button
    for (const auto& btn : g_buttons) {
        if (x >= btn.x && x <= btn.x + btn.w && glY >= btn.y && glY <= btn.y + btn.h) {
            keyboard(btn.key, x, y);
            return;
        }
    }

    // Kiểm tra click vào dải 16 ô màu trên HUD
    const float top = (float)WINDOW_HEIGHT;
    int swatchStartX = 280;
    int swatchY = top - 38;
    int swatchSize = 20;
    int swatchGap = 4;
    for (int i = 0; i < NUM_COLORS; ++i) {
        int sx = swatchStartX + i * (swatchSize + swatchGap);
        if (x >= sx && x <= sx + swatchSize && glY >= swatchY && glY <= swatchY + swatchSize) {
            g_paletteIdx = i;
            g_status = "Selected Color #" + std::to_string(i + 1);
            glutPostRedisplay();
            return;
        }
    }

    if (!isInsideCanvas(x, glY)) return;

    if (g_mode == Mode::DRAW) {
        addVertex({x, glY});
        g_status = "Vertex " + std::to_string(g_vertices.size()) + " added. ENTER to close.";
    } else {
        Color c = getPixel(x, glY);
        if (colorsEqual(c, BOUNDARY_COLOR)) {
            g_status = "Seed is on the boundary. Pick a point inside.";
        } else if (colorsEqual(c, PALETTE[g_paletteIdx])) {
            g_status = "That area is already filled with the selected color.";
        } else {
            g_seed = {x, glY};
            g_hasSeed = true;
            boundaryFill(x, glY, PALETTE[g_paletteIdx], BOUNDARY_COLOR);
            if (g_recursiveCapped) {
                g_status = "Recursive hit depth cap (stack overflow protection). Use Stack/Queue for huge regions!";
            } else {
                g_status = g_closed ? "Done. Region filled." : "Boundary is open: the fill LEAKED out.";
            }
        }
    }
    glutPostRedisplay();
}

void motion(int x, int y) {
    g_mouseX = x;
    g_mouseY = WINDOW_HEIGHT - 1 - y;
    g_mouseValid = isInsideCanvas(g_mouseX, g_mouseY);
    glutPostRedisplay(); // Cập nhật nút hover và dây thun
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case 27:  std::exit(0);
        case 13:  if (g_mode == Mode::DRAW) closePolygon(); break;
        case 'o': case 'O': if (g_mode == Mode::DRAW) finishOpen(); break;
        case 'd': case 'D': g_mode = Mode::DRAW; g_status = "Switched to DRAW mode."; break;
        case 'l': case 'L': 
            if (g_mode == Mode::DRAW) {
                if (g_vertices.size() >= 3) closePolygon();
                else finishOpen();
            }
            g_mode = Mode::FILL; 
            break;
        case 'a': case 'A': g_animate = !g_animate; break;
        case 'c': case 'C': fullClear(); break;
        case 'n': case 'N': startNewShape(); break;
        case 'f': case 'F': g_paletteIdx = (g_paletteIdx + 1) % NUM_COLORS; break;
        case 'b': case 'B':
            g_algo = static_cast<FillAlgo>((static_cast<int>(g_algo) + 1) % 3);
            g_status = (g_algo == FillAlgo::STACK_DFS) ? "Switched to DFS (Stack)."
                     : (g_algo == FillAlgo::QUEUE_BFS) ? "Switched to BFS (Queue)."
                                                       : "Switched to Recursive Boundary Fill.";
            break;
        case '1': loadPreset(1); break;
        case '2': loadPreset(2); break;
        case '3': loadPreset(3); break;
        case '4': loadPreset(4); break;
        case '5': loadPreset(5); break;
        case '6': loadPreset(6); break;
        case '7': loadPreset(7); break;
        case '8': loadPreset(8); break;
        case '9': loadPreset(9); break;
        case 's': case 'S': buildShatteredSword(WINDOW_WIDTH / 2, CANVAS_Y_MIN + (CANVAS_Y_MAX - CANVAS_Y_MIN) / 2); break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Boundary Fill Studio - OpenGL");
    glutSetCursor(GLUT_CURSOR_CROSSHAIR);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    fullClear();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(motion);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}