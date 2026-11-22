#define GL_SILENCE_DEPRECATION
#include <GL/glut.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
// ĐỊNH NGHĨA CHẾ ĐỘ VÀ CẤU TRÚC DỮ LIỆU
// ============================================================================
enum ToolMode {
    MODE_CIRCLE = 1,    // Vẽ đường tròn (Bresenham / Midpoint)
    MODE_ELLIPSE = 2,   // Vẽ Ellipse (Midpoint)
    MODE_PARABOLA = 3   // Vẽ Parabol (Bresenham)
};

enum CircleAlgo {
    ALGO_CIRCLE_BRESENHAM = 0,
    ALGO_CIRCLE_MIDPOINT = 1
};

struct Point2D {
    int x, y;
};

struct Shape {
    ToolMode type;
    int xc, yc;         // Tâm (Circle/Ellipse) hoặc Đỉnh (Parabola)
    int p1, p2;         // Circle: p1 = R, p2 = 0
                        // Ellipse: p1 = rx (a), p2 = ry (b)
                        // Parabola: p1 = p (tiêu cự), p2 = dir (1: mở lên, -1: mở xuống)
    CircleAlgo circleAlgo;
    float r, g, b;      // Màu sắc đặc trưng
    std::string title;
    std::string formula;
    std::string recurrence;
    std::string details;
    int pointCount;
    std::vector<Point2D> points; // Danh sách điểm tính toán từ thuật toán
};

struct UIButton {
    int id;
    int x, y, w, h;
    std::string label;
    std::string shortcut;
    float r, g, b;
    bool isActive;
};

// ============================================================================
// BIẾN TOÀN CỤC TRẠNG THÁI CHƯƠNG TRÌNH
// ============================================================================
std::vector<Shape> shapes;

ToolMode currentMode = MODE_CIRCLE;
CircleAlgo currentCircleAlgo = ALGO_CIRCLE_BRESENHAM;
int currentPreset = 4; // 0: Custom drawing, 1: Circle, 2: Ellipse, 3: Parabola, 4: All 3

bool isDrawing = false;
int startX = 0, startY = 0;
int currentX = 0, currentY = 0;

int windowWidth = 1200;
int windowHeight = 740;

bool showGrid = true;
bool showGuides = true;
bool smoothMode = true; // true: Nét vẽ mượt (Line); false: Điểm ảnh rời rạc (Pixel Dots)
float pointSize = 3.5f; // Kích thước điểm pixel khi vẽ ở chế độ Pixel

int hoveredButtonId = -1;
std::vector<UIButton> buttons;

// Khai báo nguyên mẫu hàm
void loadPresetMode(int preset);
std::vector<Point2D> computeCircleBresenham(int xc, int yc, int R);
std::vector<Point2D> computeCircleMidpoint(int xc, int yc, int R);
std::vector<Point2D> computeEllipseMidpoint(int xc, int yc, int rx, int ry);
std::vector<Point2D> computeParabolaBresenham(int xv, int yv, int p, int direction, int maxY);

// ============================================================================
// TIỆN ÍCH HIỂN THỊ ĐỒ HỌA & VĂN BẢN
// ============================================================================
void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_12) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

// Vẽ hình chữ nhật phẳng hoặc bo góc nhẹ có viền
void drawRect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f, bool filled = true) {
    if (filled) {
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + w, y);
            glVertex2f(x + w, y + h);
            glVertex2f(x, y + h);
        glEnd();
    } else {
        glColor4f(r, g, b, a);
        glLineWidth(1.2f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(x, y);
            glVertex2f(x + w, y);
            glVertex2f(x + w, y + h);
            glVertex2f(x, y + h);
        glEnd();
    }
}

// Điểm đánh dấu tâm / đỉnh phát sáng kỹ thuật (CAD Glow marker)
void drawGlowMarker(int x, int y, float r, float g, float b, const std::string& label = "") {
    // Vầng hào quang ngoài
    glColor4f(r, g, b, 0.28f);
    glPointSize(12.0f);
    glBegin(GL_POINTS);
        glVertex2i(x, y);
    glEnd();

    // Điểm sáng trắng trung tâm
    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(4.0f);
    glBegin(GL_POINTS);
        glVertex2i(x, y);
    glEnd();

    // Vòng chỉ định CAD
    glColor4f(r, g, b, 0.85f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 16; ++i) {
            float angle = 2.0f * 3.14159265f * i / 16.0f;
            glVertex2f(x + 6.0f * std::cos(angle), y + 6.0f * std::sin(angle));
        }
    glEnd();

    if (!label.empty()) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(x + 9, y + 4, label, GLUT_BITMAP_HELVETICA_10);
    }
}

// ============================================================================
// 1. THUẬT TOÁN BRESENHAM VẼ ĐƯỜNG TRÒN
// ============================================================================
// Tính đối xứng 8 phần (8-way symmetry):
// Phương trình: (x - xc)^2 + (y - yc)^2 = R^2
// Khởi tạo: x = 0, y = R, d0 = 3 - 2*R
// Nếu d < 0: d = d + 4*x + 6
// Nếu d >= 0: y--, d = d + 4*(x - y) + 10
// x++
// Sắp xếp các điểm theo thứ tự vòng tròn ngược chiều kim đồng hồ để tạo đường khép kín mượt mà
std::vector<Point2D> computeCircleBresenham(int xc, int yc, int R) {
    std::vector<Point2D> oct1, oct2, oct3, oct4, oct5, oct6, oct7, oct8;
    if (R <= 0) return oct1;

    int x = 0;
    int y = R;
    int d = 3 - 2 * R; // Thông số quyết định d0 của Bresenham

    while (x <= y) {
        // Thu thập điểm ở 8 phần đối xứng
        oct1.push_back({xc + x, yc + y}); // 90° -> 45°
        oct2.push_back({xc + y, yc + x}); // 0° -> 45°
        oct3.push_back({xc + y, yc - x}); // 360° -> 315°
        oct4.push_back({xc + x, yc - y}); // 270° -> 315°
        oct5.push_back({xc - x, yc - y}); // 270° -> 225°
        oct6.push_back({xc - y, yc - x}); // 180° -> 225°
        oct7.push_back({xc - y, yc + x}); // 180° -> 135°
        oct8.push_back({xc - x, yc + y}); // 90° -> 135°

        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }

    // Ghép 8 cung theo thứ tự vòng tròn liên tục từ 0° -> 360° (Ngược chiều kim đồng hồ)
    std::vector<Point2D> fullCircle;
    for (size_t i = 0; i < oct2.size(); ++i) fullCircle.push_back(oct2[i]); // forward: 0 -> 45
    for (size_t i = 0; i < oct1.size(); ++i) fullCircle.push_back(oct1[oct1.size() - 1 - i]); // reversed: 45 -> 90
    for (size_t i = 0; i < oct8.size(); ++i) fullCircle.push_back(oct8[i]); // forward: 90 -> 135
    for (size_t i = 0; i < oct7.size(); ++i) fullCircle.push_back(oct7[oct7.size() - 1 - i]); // reversed: 135 -> 180
    for (size_t i = 0; i < oct6.size(); ++i) fullCircle.push_back(oct6[i]); // forward: 180 -> 225
    for (size_t i = 0; i < oct5.size(); ++i) fullCircle.push_back(oct5[oct5.size() - 1 - i]); // reversed: 225 -> 270
    for (size_t i = 0; i < oct4.size(); ++i) fullCircle.push_back(oct4[i]); // forward: 270 -> 315
    for (size_t i = 0; i < oct3.size(); ++i) fullCircle.push_back(oct3[oct3.size() - 1 - i]); // reversed: 315 -> 360

    return fullCircle;
}

// Thuật toán Midpoint vẽ đường tròn (lựa chọn nâng cao để so sánh)
std::vector<Point2D> computeCircleMidpoint(int xc, int yc, int R) {
    std::vector<Point2D> oct1, oct2, oct3, oct4, oct5, oct6, oct7, oct8;
    if (R <= 0) return oct1;

    int x = 0;
    int y = R;
    int p = 1 - R; // Thông số quyết định Midpoint p0

    while (x <= y) {
        oct1.push_back({xc + x, yc + y});
        oct2.push_back({xc + y, yc + x});
        oct3.push_back({xc + y, yc - x});
        oct4.push_back({xc + x, yc - y});
        oct5.push_back({xc - x, yc - y});
        oct6.push_back({xc - y, yc - x});
        oct7.push_back({xc - y, yc + x});
        oct8.push_back({xc - x, yc + y});

        if (p < 0) {
            p += 2 * x + 3;
        } else {
            p += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }

    std::vector<Point2D> fullCircle;
    for (size_t i = 0; i < oct2.size(); ++i) fullCircle.push_back(oct2[i]);
    for (size_t i = 0; i < oct1.size(); ++i) fullCircle.push_back(oct1[oct1.size() - 1 - i]);
    for (size_t i = 0; i < oct8.size(); ++i) fullCircle.push_back(oct8[i]);
    for (size_t i = 0; i < oct7.size(); ++i) fullCircle.push_back(oct7[oct7.size() - 1 - i]);
    for (size_t i = 0; i < oct6.size(); ++i) fullCircle.push_back(oct6[i]);
    for (size_t i = 0; i < oct5.size(); ++i) fullCircle.push_back(oct5[oct5.size() - 1 - i]);
    for (size_t i = 0; i < oct4.size(); ++i) fullCircle.push_back(oct4[i]);
    for (size_t i = 0; i < oct3.size(); ++i) fullCircle.push_back(oct3[oct3.size() - 1 - i]);

    return fullCircle;
}

// ============================================================================
// 2. THUẬT TOÁN MIDPOINT VẼ ELLIPSE
// ============================================================================
// Phương trình: (x - xc)^2 / rx^2 + (y - yc)^2 / ry^2 = 1
// Tính đối xứng 4 phần (4-way symmetry): (xc +- x, yc +- y)
// Chia góc phần tư thứ nhất thành 2 vùng:
// - Vùng 1: |m| < 1 (2*ry^2*x < 2*rx^2*y) -> bước theo trục x
// - Vùng 2: |m| >= 1 (2*ry^2*x >= 2*rx^2*y) -> bước theo trục y
std::vector<Point2D> computeEllipseMidpoint(int xc, int yc, int rx, int ry) {
    std::vector<Point2D> q1; // Điểm góc phần tư 1 từ (0, ry) đến (rx, 0)
    if (rx <= 0 || ry <= 0) return q1;

    long long rx2 = (long long)rx * rx;
    long long ry2 = (long long)ry * ry;
    long long twoRx2 = 2 * rx2;
    long long twoRy2 = 2 * ry2;

    int x = 0;
    int y = ry;

    // --- VÙNG 1: Tiếp tuyến có |m| < 1 (bước theo trục x) ---
    // p1_0 = ry^2 - rx^2 * ry + 1/4 * rx^2 (làm tròn số nguyên chuẩn: (rx2 + 2) / 4)
    double p1 = (double)ry2 - (double)rx2 * ry + 0.25 * rx2;
    long long dx = 0;
    long long dy = twoRx2 * y;

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

    // --- VÙNG 2: Tiếp tuyến có |m| >= 1 (bước theo trục y) ---
    // p2_0 = ry^2 * (x + 0.5)^2 + rx^2 * (y - 1)^2 - rx^2 * ry^2
    double p2 = (double)ry2 * (x + 0.5) * (x + 0.5) + (double)rx2 * (y - 1) * (y - 1) - (double)rx2 * ry2;
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

    // Ghép 4 góc phần tư đối xứng theo chiều kim đồng hồ liên tục:
    std::vector<Point2D> fullEllipse;
    // Góc phần tư 1 (trên phải): từ (0, ry) đến (rx, 0)
    for (size_t i = 0; i < q1.size(); ++i) fullEllipse.push_back({xc + q1[i].x, yc + q1[i].y});
    // Góc phần tư 4 (dưới phải): từ (rx, 0) đến (0, -ry)
    for (size_t i = q1.size(); i > 0; --i) fullEllipse.push_back({xc + q1[i - 1].x, yc - q1[i - 1].y});
    // Góc phần tư 3 (dưới trái): từ (0, -ry) đến (-rx, 0)
    for (size_t i = 0; i < q1.size(); ++i) fullEllipse.push_back({xc - q1[i].x, yc - q1[i].y});
    // Góc phần tư 2 (trên trái): từ (-rx, 0) đến (0, ry)
    for (size_t i = q1.size(); i > 0; --i) fullEllipse.push_back({xc - q1[i - 1].x, yc + q1[i - 1].y});

    return fullEllipse;
}

// ============================================================================
// 3. THUẬT TOÁN BRESENHAM VẼ PARABOL: (x - xv)^2 = 4p*(y - yv)
// ============================================================================
// Tính đối xứng 2 bên qua trục đối xứng đứng: (xv +- x, yv + direction * y)
// Đạo hàm: dy/dx = x / (2p)
// - Vùng 1: dy/dx <= 1 <=> x <= 2p (bước theo trục x, x = x + 1)
//   Điểm giữa (x + 1, y + 0.5), d1_0 = 1 - 2p
//   Nếu d1 < 0: d1 = d1 + 2x + 3
//   Nếu d1 >= 0: y = y + 1, d1 = d1 + 2x + 3 - 4p
//   (Lưu ý: cập nhật d1 với giá trị x hiện tại TRƯỚC KHI tăng x)
// - Vùng 2: dy/dx > 1 <=> x > 2p (bước theo trục y, y = y + 1)
//   Điểm giữa (x + 0.5, y + 1), d2_0 = (x + 0.5)^2 - 4p(y + 1)
//   Nếu d2 > 0: d2 = d2 - 4p
//   Nếu d2 <= 0: d2 = d2 + 2x + 2 - 4p, x = x + 1
//   y = y + 1
std::vector<Point2D> computeParabolaBresenham(int xv, int yv, int p, int direction = 1, int maxY = 340) {
    std::vector<Point2D> half;
    if (p <= 0) p = 1;

    int x = 0;
    int y = 0;

    // --- VÙNG 1: Độ dốc <= 1 <=> x <= 2p (bước theo trục x) ---
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

    // --- VÙNG 2: Độ dốc > 1 <=> x > 2p (bước theo trục y) ---
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

    // Ghép nhánh trái (x âm giảm về 0) và nhánh phải (x từ 0 tăng lên)
    std::vector<Point2D> fullParabola;
    for (size_t i = half.size(); i > 0; --i) {
        fullParabola.push_back({xv - half[i - 1].x, yv + direction * half[i - 1].y});
    }
    // Tránh trùng lặp điểm đỉnh (0, 0)
    for (size_t i = 1; i < half.size(); ++i) {
        fullParabola.push_back({xv + half[i].x, yv + direction * half[i].y});
    }

    return fullParabola;
}

// ============================================================================
// HÀM VẼ TỔNG HỢP CÁC ĐỐI TƯỢNG HÌNH HỌC (RENDER SHAPE)
// ============================================================================
void renderShape(const Shape& s) {
    if (s.points.empty()) return;

    glColor3f(s.r, s.g, s.b);
    if (smoothMode) {
        // Chế độ nét cong mượt mà (chống răng cưa, đẹp mắt cho báo cáo)
        glLineWidth(2.2f);
        if (s.type == MODE_CIRCLE || s.type == MODE_ELLIPSE) {
            glBegin(GL_LINE_LOOP);
        } else {
            glBegin(GL_LINE_STRIP);
        }
        for (const auto& pt : s.points) {
            glVertex2i(pt.x, pt.y);
        }
        glEnd();
    } else {
        // Chế độ điểm ảnh Bresenham/Midpoint rời rạc (chuẩn raster graphics)
        glPointSize(pointSize);
        glBegin(GL_POINTS);
        for (const auto& pt : s.points) {
            glVertex2i(pt.x, pt.y);
        }
        glEnd();
    }
}

// ============================================================================
// CHẾ ĐỘ HIỂN THỊ HÌNH MẪU CĂN GIỮA (PRESET MODES)
// ============================================================================
void loadPresetMode(int preset) {
    shapes.clear();
    currentPreset = preset;

    int centerY = (windowHeight - 80 - 40) / 2 + 40 + 25; // Căn giữa vùng làm việc
    int centerX = windowWidth / 2;
    int colWidth = windowWidth / 3;

    if (preset == 1 || preset == 4) {
        Shape c;
        c.type = MODE_CIRCLE;
        c.xc = (preset == 1) ? centerX : colWidth / 2;
        c.yc = centerY;
        c.p1 = (preset == 1) ? 200 : 120; // R
        c.p2 = 0;
        c.circleAlgo = ALGO_CIRCLE_BRESENHAM;
        c.r = 0.0f; c.g = 0.96f; c.b = 0.85f;
        c.title = "1. DUONG TRON (BRESENHAM)";
        c.formula = "(x - xc)^2 + (y - yc)^2 = R^2";
        c.recurrence = "d0 = 3 - 2R  |  d < 0: +4x+6  |  d >= 0: +4(x-y)+10";
        c.details = "Tam I(" + std::to_string(c.xc) + ", " + std::to_string(c.yc) + "), Ban kinh R = " + std::to_string(c.p1) + " px (8-way symmetry)";
        c.points = computeCircleBresenham(c.xc, c.yc, c.p1);
        c.pointCount = (int)c.points.size();
        shapes.push_back(c);
        if (preset == 1) currentMode = MODE_CIRCLE;
    }

    if (preset == 2 || preset == 4) {
        Shape e;
        e.type = MODE_ELLIPSE;
        e.xc = (preset == 2) ? centerX : colWidth + colWidth / 2;
        e.yc = centerY;
        e.p1 = (preset == 2) ? 250 : 150; // a
        e.p2 = (preset == 2) ? 150 : 95;  // b
        e.circleAlgo = ALGO_CIRCLE_BRESENHAM; // Not used for ellipse but kept for struct
        e.r = 1.0f; e.g = 0.82f; e.b = 0.20f;
        e.title = "2. ELLIPSE (MIDPOINT)";
        e.formula = "(x - xc)^2/a^2 + (y - yc)^2/b^2 = 1";
        e.recurrence = "Vung 1 (|m|<1, step x)  |  Vung 2 (|m|>=1, step y)";
        e.details = "Tam I(" + std::to_string(e.xc) + ", " + std::to_string(e.yc) + "), a = " + std::to_string(e.p1) + ", b = " + std::to_string(e.p2) + " (4-way)";
        e.points = computeEllipseMidpoint(e.xc, e.yc, e.p1, e.p2);
        e.pointCount = (int)e.points.size();
        shapes.push_back(e);
        if (preset == 2) currentMode = MODE_ELLIPSE;
    }

    if (preset == 3 || preset == 4) {
        Shape p;
        p.type = MODE_PARABOLA;
        p.xc = (preset == 3) ? centerX : 2 * colWidth + colWidth / 2;
        p.yc = (preset == 3) ? centerY - 250 : centerY - 150;
        p.p1 = (preset == 3) ? 50 : 32; // p
        p.p2 = 1; // Mở lên
        p.circleAlgo = ALGO_CIRCLE_BRESENHAM;
        p.r = 1.0f; p.g = 0.24f; p.b = 0.52f;
        p.title = "3. PARABOL (BRESENHAM)";
        p.formula = "(x - xv)^2 = 4p*(y - yv)";
        p.recurrence = "V1: dy/dx<=1 (x<=2p)  |  V2: dy/dx>1 (x>2p)";
        p.details = "Dinh V(" + std::to_string(p.xc) + ", " + std::to_string(p.yc) + "), p = " + std::to_string(p.p1) + ", F(" + std::to_string(p.xc) + ", " + std::to_string(p.yc + p.p1) + ")";
        p.points = computeParabolaBresenham(p.xc, p.yc, p.p1, p.p2, (preset == 3) ? 500 : 310);
        p.pointCount = (int)p.points.size();
        shapes.push_back(p);
        if (preset == 3) currentMode = MODE_PARABOLA;
    }

    std::cout << "[CHUYEN CHE DO] Da nap Preset: " << preset << "\n";
}

// ============================================================================
// THIẾT KẾ GIAO DIỆN THANH CÔNG CỤ (MODERN GLASSMORPHISM TOOLBAR)
// ============================================================================
void initButtons() {
    buttons.clear();
    int yTop = windowHeight - 48;
    int h = 34;

    // Nhóm chế độ hiển thị & công cụ
    buttons.push_back({1, 15,  yTop, 125, h, "1. H.Tron",    "[1]", 0.0f, 0.96f, 0.85f, (currentPreset == 1 || (currentPreset == 0 && currentMode == MODE_CIRCLE))});
    buttons.push_back({2, 146, yTop, 120, h, "2. Ellipse",   "[2]", 1.0f, 0.82f, 0.20f, (currentPreset == 2 || (currentPreset == 0 && currentMode == MODE_ELLIPSE))});
    buttons.push_back({3, 272, yTop, 120, h, "3. Parabol",   "[3]", 1.0f, 0.24f, 0.52f, (currentPreset == 3 || (currentPreset == 0 && currentMode == MODE_PARABOLA))});
    
    // Nút hiển thị 3 hình
    buttons.push_back({4, 405, yTop, 130, h, "4. Ca 3 Hinh", "[4]", 0.2f, 0.8f, 1.0f, (currentPreset == 4)});

    // Nhóm tiện ích (Tinh gọn)
    buttons.push_back({5, 545, yTop, 100, h, "Xoa het",     "[C]", 0.95f, 0.35f, 0.35f, false});
    buttons.push_back({6, 655, yTop, 90,  h, "Luoi",        "[G]", 0.55f, 0.65f, 0.80f, showGrid});
    buttons.push_back({7, 755, yTop, 105, h, "Thong so",    "[I]", 0.40f, 0.80f, 0.95f, showGuides});
}

void renderUI() {
    // 1. Thanh tiêu đề và công cụ trên (Top Header & Toolbar)
    int barHeight = 75;
    int barY = windowHeight - barHeight;

    // Nền thanh công cụ Dark Obsidian Glass
    drawRect(0, barY, windowWidth, barHeight, 0.06f, 0.08f, 0.12f, 0.97f, true);

    // Dòng thông tin trường và môn học góc trên cùng
    glColor3f(0.50f, 0.58f, 0.70f);
    drawText(16, windowHeight - 16, "VKU - KHOA KY THUAT MAY TINH & DIEN TU  |  DO HOA MAY TINH - LAB 2 (14/9)", GLUT_BITMAP_HELVETICA_10);

    glColor3f(0.20f, 0.85f, 0.65f);
    std::string sub = "Thuat toan: Duong tron (Bresenham/Midpoint), Ellipse (Midpoint), Parabol (Bresenham)";
    drawText(windowWidth - 580, windowHeight - 16, sub, GLUT_BITMAP_HELVETICA_10);

    // Đường kẻ phân cách thanh công cụ
    glColor3f(0.18f, 0.22f, 0.32f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
        glVertex2i(0, barY);
        glVertex2i(windowWidth, barY);
    glEnd();

    // 2. Vẽ danh sách nút bấm
    initButtons();
    for (const auto& btn : buttons) {
        bool isHover = (hoveredButtonId == btn.id);

        if (btn.isActive) {
            // Nút đang kích hoạt (Active)
            drawRect(btn.x, btn.y, btn.w, btn.h, btn.r * 0.22f, btn.g * 0.22f, btn.b * 0.22f, 0.95f, true);
            drawRect(btn.x, btn.y, btn.w, btn.h, btn.r, btn.g, btn.b, 1.0f, false);
            glColor3f(btn.r, btn.g, btn.b);
        } else if (isHover) {
            // Nút đang rê chuột (Hover)
            drawRect(btn.x, btn.y, btn.w, btn.h, 0.15f, 0.19f, 0.28f, 0.92f, true);
            drawRect(btn.x, btn.y, btn.w, btn.h, btn.r * 0.7f, btn.g * 0.7f, btn.b * 0.7f, 0.85f, false);
            glColor3f(btn.r, btn.g, btn.b);
        } else {
            // Nút bình thường
            drawRect(btn.x, btn.y, btn.w, btn.h, 0.10f, 0.12f, 0.18f, 0.85f, true);
            drawRect(btn.x, btn.y, btn.w, btn.h, 0.20f, 0.25f, 0.35f, 0.60f, false);
            glColor3f(0.80f, 0.85f, 0.92f);
        }

        // Tên nút
        drawText(btn.x + 10, btn.y + 12, btn.label, GLUT_BITMAP_HELVETICA_12);

        // Phím tắt góc phải nút
        if (btn.w >= 90) {
            glColor4f(0.50f, 0.55f, 0.65f, 0.8f);
            drawText(btn.x + btn.w - 26, btn.y + 12, btn.shortcut, GLUT_BITMAP_HELVETICA_10);
        }
    }

    // 3. Thanh trạng thái dưới đáy (Bottom Status Bar)
    int botHeight = 36;
    drawRect(0, 0, windowWidth, botHeight, 0.05f, 0.07f, 0.10f, 0.98f, true);

    glColor3f(0.18f, 0.22f, 0.32f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
        glVertex2i(0, botHeight);
        glVertex2i(windowWidth, botHeight);
    glEnd();

    // Hướng dẫn tương tác thời gian thực
    if (!isDrawing) {
        glColor3f(0.2f, 0.95f, 0.65f);
        drawText(16, 12, "[HUONG DAN] Chon hinh [1, 2, 3, 4] de xem cac mau chuan  |  Hoac click chuot vao bang ve de ve hinh moi", GLUT_BITMAP_HELVETICA_12);
    } else {
        glColor3f(1.0f, 0.85f, 0.25f);
        drawText(16, 12, "[DANG VE] Re chuot xem kich thuoc  ->  Click chuot trai lan 2 de CHOT HINH  (Click chuot phai: Huy bo)", GLUT_BITMAP_HELVETICA_12);
    }

    // Tọa độ chuột
    glColor3f(0.65f, 0.72f, 0.85f);
    std::string statusRight = "Toa do: (" + std::to_string(currentX) + ", " + std::to_string(currentY) + ")";
    drawText(windowWidth - 180, 12, statusRight, GLUT_BITMAP_HELVETICA_10);
}

// ============================================================================
// ĐƯỜNG GIÓNG KỸ THUẬT VÀ THẺ THÔNG SỐ (CAD GUIDELINES & INFO CARDS)
// ============================================================================
void drawCardForShape(const Shape& s) {
    if (!showGuides) return;

    // 1. Điểm đánh dấu tâm / đỉnh
    drawGlowMarker(s.xc, s.yc, s.r, s.g, s.b, (s.type == MODE_PARABOLA ? "V" : "I"));

    // 2. Đường gióng kích thước chuẩn kỹ thuật
    glColor4f(s.r, s.g, s.b, 0.40f);
    glLineWidth(1.0f);

    if (s.type == MODE_CIRCLE) {
        // Đường bán kính R
        glBegin(GL_LINES);
            glVertex2i(s.xc, s.yc);
            glVertex2i(s.xc + s.p1, s.yc);
        glEnd();
        drawGlowMarker(s.xc + s.p1, s.yc, s.r, s.g, s.b, "R");
    } 
    else if (s.type == MODE_ELLIPSE) {
        // Bán trục lớn a và bán trục nhỏ b
        glBegin(GL_LINES);
            glVertex2i(s.xc - s.p1, s.yc);
            glVertex2i(s.xc + s.p1, s.yc);
            glVertex2i(s.xc, s.yc - s.p2);
            glVertex2i(s.xc, s.yc + s.p2);
        glEnd();
        drawGlowMarker(s.xc + s.p1, s.yc, s.r, s.g, s.b, "a");
        drawGlowMarker(s.xc, s.yc + s.p2, s.r, s.g, s.b, "b");
    } 
    else if (s.type == MODE_PARABOLA) {
        // Trục đối xứng và Tiêu điểm Focus F
        int focusY = s.yc + s.p2 * s.p1;
        glBegin(GL_LINES);
            glVertex2i(s.xc, s.yc - 10);
            glVertex2i(s.xc, s.yc + s.p2 * 320);
        glEnd();
        drawGlowMarker(s.xc, focusY, 1.0f, 0.85f, 0.20f, "F");
    }

    // 3. Khung thẻ thông số (Info Card) hiển thị chi tiết toán học và thuật toán
    int cardW = 340;
    int cardH = 88;
    int cardX = std::max(10, std::min(s.xc - cardW / 2, windowWidth - cardW - 10));
    // Giữ thẻ thông số nằm gọn gàng bên dưới hình hoặc ở chân trang
    int cardY = (s.type == MODE_PARABOLA) ? 50 : std::max(50, s.yc - (s.type == MODE_CIRCLE ? s.p1 : s.p2) - cardH - 15);
    cardY = std::min(cardY, std::max(50, windowHeight - 85 - cardH));

    // Nền thẻ kính mờ Glassmorphism
    drawRect(cardX, cardY, cardW, cardH, 0.07f, 0.09f, 0.14f, 0.92f, true);
    drawRect(cardX, cardY, cardW, cardH, s.r * 0.7f, s.g * 0.7f, s.b * 0.7f, 0.85f, false);

    // Tiêu đề thẻ (Tên thuật toán)
    glColor3f(s.r, s.g, s.b);
    drawText(cardX + 12, cardY + cardH - 18, s.title, GLUT_BITMAP_HELVETICA_12);

    // Công thức toán học tổng quát
    glColor3f(0.85f, 0.88f, 0.95f);
    drawText(cardX + 12, cardY + cardH - 36, "PT: " + s.formula, GLUT_BITMAP_HELVETICA_10);

    // Công thức đệ quy biến quyết định
    glColor3f(0.65f, 0.75f, 0.88f);
    drawText(cardX + 12, cardY + cardH - 54, s.recurrence, GLUT_BITMAP_HELVETICA_10);

    // Chi tiết tham số thực tế & số lượng điểm sinh ra
    glColor3f(0.3f, 0.95f, 0.65f);
    std::string statStr = s.details + " | " + std::to_string(s.pointCount) + " pts";
    drawText(cardX + 12, cardY + cardH - 72, statStr, GLUT_BITMAP_HELVETICA_10);
}

// ============================================================================
// LƯỚI TỌA ĐỘ VÀ PHÂN CHIA 3 CỘT KỸ THUẬT
// ============================================================================
void drawGridAndLayout() {
    if (!showGrid) return;

    int topBound = windowHeight - 75;
    int botBound = 36;

    // 1. Lưới phụ cách mỗi 25px
    glColor4f(0.12f, 0.15f, 0.22f, 0.35f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int x = 0; x < windowWidth; x += 25) {
        glVertex2i(x, botBound);
        glVertex2i(x, topBound);
    }
    for (int y = botBound; y < topBound; y += 25) {
        glVertex2i(0, y);
        glVertex2i(windowWidth, y);
    }
    glEnd();

    // 2. Lưới chính cách mỗi 100px
    glColor4f(0.16f, 0.20f, 0.30f, 0.60f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    for (int x = 0; x < windowWidth; x += 100) {
        glVertex2i(x, botBound);
        glVertex2i(x, topBound);
    }
    for (int y = botBound; y < topBound; y += 100) {
        glVertex2i(0, y);
        glVertex2i(windowWidth, y);
    }
    glEnd();

    // 3. Đường phân chia 3 cột CAD thanh lịch (khi nạp bộ 3 hình mẫu)
    int colWidth = windowWidth / 3;
    glColor4f(0.25f, 0.32f, 0.45f, 0.50f);
    glLineWidth(1.5f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0xAAAA); // Nét đứt kỹ thuật
    glBegin(GL_LINES);
        glVertex2i(colWidth, botBound);
        glVertex2i(colWidth, topBound);

        glVertex2i(2 * colWidth, botBound);
        glVertex2i(2 * colWidth, topBound);
    glEnd();
    glDisable(GL_LINE_STIPPLE);
}

// ============================================================================
// HÀM VẼ CHÍNH CỦA OPENGL (DISPLAY CALLBACK)
// ============================================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 1. Lưới tọa độ và khung phân chia khu vực
    drawGridAndLayout();

    // 2. Vẽ tất cả các hình đã được chốt trên bảng vẽ
    for (const auto& s : shapes) {
        renderShape(s);
        drawCardForShape(s);
    }

    // 3. Xem trước động thời gian thực khi đang kéo chuột (Live Preview)
    if (isDrawing) {
        drawGlowMarker(startX, startY, 1.0f, 1.0f, 1.0f, "Tam/Dinh");

        // Đường nối dóng từ gốc đến con trỏ chuột
        glColor4f(1.0f, 1.0f, 1.0f, 0.35f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
            glVertex2i(startX, startY);
            glVertex2i(currentX, currentY);
        glEnd();

        if (currentMode == MODE_CIRCLE) {
            int dx = currentX - startX;
            int dy = currentY - startY;
            int R = (int)std::round(std::sqrt((double)dx * dx + (double)dy * dy));
            if (R > 0) {
                Shape preview;
                preview.type = MODE_CIRCLE;
                preview.r = 0.0f; preview.g = 0.96f; preview.b = 0.85f;
                if (currentCircleAlgo == ALGO_CIRCLE_BRESENHAM) {
                    preview.points = computeCircleBresenham(startX, startY, R);
                } else {
                    preview.points = computeCircleMidpoint(startX, startY, R);
                }
                renderShape(preview);

                // Huy hiệu kích thước nổi bên cạnh con trỏ
                glColor3f(0.0f, 0.96f, 0.85f);
                drawText(currentX + 14, currentY + 8, "R = " + std::to_string(R) + " px", GLUT_BITMAP_HELVETICA_12);
            }
        } 
        else if (currentMode == MODE_ELLIPSE) {
            int rx = std::abs(currentX - startX);
            int ry = std::abs(currentY - startY);
            if (rx > 0 && ry > 0) {
                Shape preview;
                preview.type = MODE_ELLIPSE;
                preview.r = 1.0f; preview.g = 0.82f; preview.b = 0.20f;
                preview.points = computeEllipseMidpoint(startX, startY, rx, ry);
                renderShape(preview);

                glColor3f(1.0f, 0.82f, 0.20f);
                drawText(currentX + 14, currentY + 8, "a = " + std::to_string(rx) + ", b = " + std::to_string(ry) + " px", GLUT_BITMAP_HELVETICA_12);
            }
        } 
        else if (currentMode == MODE_PARABOLA) {
            int dy = currentY - startY;
            int dx = std::abs(currentX - startX);
            int direction = (dy >= 0) ? 1 : -1;
            int pVal = (dx * dx) / (4 * std::max(1, std::abs(dy)));
            if (pVal <= 0) pVal = 20;
            if (pVal > 180) pVal = 180;

            Shape preview;
            preview.type = MODE_PARABOLA;
            preview.r = 1.0f; preview.g = 0.24f; preview.b = 0.52f;
            preview.points = computeParabolaBresenham(startX, startY, pVal, direction, 320);
            renderShape(preview);

            glColor3f(1.0f, 0.24f, 0.52f);
            drawText(currentX + 14, currentY + 8, "p = " + std::to_string(pVal) + " (" + (direction > 0 ? "Mo len" : "Mo xuong") + ")", GLUT_BITMAP_HELVETICA_12);
        }
    }

    // 4. Thanh giao diện công cụ và trạng thái
    renderUI();

    glFlush();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    bool isResized = (w != windowWidth || h != windowHeight);
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);

    // Tự động canh giữa lại các preset khi thay đổi kích thước cửa sổ
    if (isResized && currentPreset > 0) {
        loadPresetMode(currentPreset);
    }
}

// ============================================================================
// XỬ LÝ SỰ KIỆN TƯƠNG TÁC NÚT BẤM (BUTTON CLICK HANDLER)
// ============================================================================
void handleButtonClick(int btnId) {
    switch (btnId) {
        case 1:
            currentMode = MODE_CIRCLE;
            if (currentPreset > 0) loadPresetMode(1);
            isDrawing = false;
            break;
        case 2:
            currentMode = MODE_ELLIPSE;
            if (currentPreset > 0) loadPresetMode(2);
            isDrawing = false;
            break;
        case 3:
            currentMode = MODE_PARABOLA;
            if (currentPreset > 0) loadPresetMode(3);
            isDrawing = false;
            break;
        case 4:
            loadPresetMode(4);
            isDrawing = false;
            break;
        case 5: // Xóa hết
            shapes.clear();
            currentPreset = 0; // Trở về custom mode
            isDrawing = false;
            std::cout << "[XOA HET] Da xoa toan bo bang ve. San sang de ve!\n";
            break;
        case 6: // Lưới
            showGrid = !showGrid;
            std::cout << "[LUOI] " << (showGrid ? "Bat" : "Tat") << "\n";
            break;
        case 7: // Thông số
            showGuides = !showGuides;
            std::cout << "[CHI SO] " << (showGuides ? "Bat" : "Tat") << "\n";
            break;
    }
}

// ============================================================================
// XỬ LÝ SỰ KIỆN CHUỘT (MOUSE CLICKS & MOTIONS)
// ============================================================================
void mouseClick(int button, int state, int x, int y) {
    int worldX = x;
    int worldY = windowHeight - y;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // 1. Kiểm tra click vào nút bấm trên thanh Top Bar
        if (worldY >= windowHeight - 75) {
            for (const auto& btn : buttons) {
                if (worldX >= btn.x && worldX <= btn.x + btn.w &&
                    worldY >= btn.y && worldY <= btn.y + btn.h) {
                    handleButtonClick(btn.id);
                    glutPostRedisplay();
                    return;
                }
            }
            return;
        }

        // Bỏ qua click vào thanh trạng thái dưới cùng
        if (worldY <= 36) return;

        // 2. Click vẽ trên bảng vẽ
        if (!isDrawing) {
            // Click lần 1: Chọn Tâm hoặc Đỉnh
            isDrawing = true;
            startX = worldX;
            startY = worldY;
            currentX = worldX;
            currentY = worldY;
        } else {
            // Click lần 2: Chốt hình vào danh sách
            isDrawing = false;
            Shape s;
            s.type = currentMode;
            s.xc = startX;
            s.yc = startY;
            s.circleAlgo = currentCircleAlgo;

            if (currentMode == MODE_CIRCLE) {
                int dx = worldX - startX;
                int dy = worldY - startY;
                int R = (int)std::round(std::sqrt((double)dx * dx + (double)dy * dy));
                if (R < 8) R = 8;
                s.p1 = R;
                s.p2 = 0;
                s.r = 0.0f; s.g = 0.96f; s.b = 0.85f;
                if (currentCircleAlgo == ALGO_CIRCLE_BRESENHAM) {
                    s.title = "DUONG TRON (BRESENHAM)";
                    s.formula = "(x - xc)^2 + (y - yc)^2 = R^2";
                    s.recurrence = "d0 = 3 - 2R  |  d < 0: +4x+6  |  d >= 0: +4(x-y)+10";
                    s.points = computeCircleBresenham(s.xc, s.yc, s.p1);
                } else {
                    s.title = "DUONG TRON (MIDPOINT)";
                    s.formula = "(x - xc)^2 + (y - yc)^2 = R^2";
                    s.recurrence = "p0 = 1 - R  |  p < 0: +2x+3  |  p >= 0: +2(x-y)+5";
                    s.points = computeCircleMidpoint(s.xc, s.yc, s.p1);
                }
                s.details = "Tam I(" + std::to_string(s.xc) + ", " + std::to_string(s.yc) + "), R = " + std::to_string(s.p1) + " px (8-way)";
                s.pointCount = (int)s.points.size();
            } 
            else if (currentMode == MODE_ELLIPSE) {
                s.p1 = std::max(12, std::abs(worldX - startX));
                s.p2 = std::max(12, std::abs(worldY - startY));
                s.r = 1.0f; s.g = 0.82f; s.b = 0.20f;
                s.title = "ELLIPSE (MIDPOINT)";
                s.formula = "(x - xc)^2/a^2 + (y - yc)^2/b^2 = 1";
                s.recurrence = "Vung 1 (|m|<1, step x)  |  Vung 2 (|m|>=1, step y)";
                s.details = "Tam I(" + std::to_string(s.xc) + ", " + std::to_string(s.yc) + "), a = " + std::to_string(s.p1) + ", b = " + std::to_string(s.p2);
                s.points = computeEllipseMidpoint(s.xc, s.yc, s.p1, s.p2);
                s.pointCount = (int)s.points.size();
            } 
            else if (currentMode == MODE_PARABOLA) {
                int dy = worldY - startY;
                int dx = std::abs(worldX - startX);
                s.p2 = (dy >= 0) ? 1 : -1;
                int pVal = (dx * dx) / (4 * std::max(1, std::abs(dy)));
                if (pVal <= 0) pVal = 20;
                if (pVal > 180) pVal = 180;
                s.p1 = pVal;
                s.r = 1.0f; s.g = 0.24f; s.b = 0.52f;
                s.title = "PARABOL (BRESENHAM)";
                s.formula = "(x - xv)^2 = 4p*(y - yv)";
                s.recurrence = "V1: dy/dx<=1 (x<=2p)  |  V2: dy/dx>1 (x>2p)";
                s.details = "Dinh V(" + std::to_string(s.xc) + ", " + std::to_string(s.yc) + "), p = " + std::to_string(s.p1) + ", F(" + std::to_string(s.xc) + ", " + std::to_string(s.yc + s.p2 * s.p1) + ")";
                s.points = computeParabolaBresenham(s.xc, s.yc, s.p1, s.p2, 320);
                s.pointCount = (int)s.points.size();
            }

            shapes.push_back(s);
            currentPreset = 0; // Hủy preset khi người dùng bắt đầu vẽ thủ công
            std::cout << "[DA VE] " << s.title << " tai (" << s.xc << ", " << s.yc << ") voi " << s.pointCount << " diem anh.\n";
        }
        glutPostRedisplay();
    } 
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        // Chuột phải hủy bỏ trạng thái vẽ dở
        if (isDrawing) {
            isDrawing = false;
            std::cout << "[HUY] Da huy thao tac ve hien tai.\n";
            glutPostRedisplay();
        }
    }
}

void mouseMotion(int x, int y) {
    currentX = x;
    currentY = windowHeight - y;
    if (isDrawing) {
        glutPostRedisplay();
    }
}

void mousePassiveMotion(int x, int y) {
    currentX = x;
    currentY = windowHeight - y;

    int prevHover = hoveredButtonId;
    hoveredButtonId = -1;
    if (currentY >= windowHeight - 75) {
        for (const auto& btn : buttons) {
            if (currentX >= btn.x && currentX <= btn.x + btn.w &&
                currentY >= btn.y && currentY <= btn.y + btn.h) {
                hoveredButtonId = btn.id;
                break;
            }
        }
    }

    if (prevHover != hoveredButtonId || isDrawing) {
        glutPostRedisplay();
    }
}

// ============================================================================
// XỬ LÝ BÀN PHÍM TOÀN DIỆN (CHỐNG LỖI UNIKEY VÀ HỖ TRỢ PHÍM CHỨC NĂNG)
// ============================================================================
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case '1':
            handleButtonClick(1);
            break;
        case '2':
            handleButtonClick(2);
            break;
        case '3':
            handleButtonClick(3);
            break;
        case '4':
            handleButtonClick(4);
            break;
        case 'c':
        case 'C':
            handleButtonClick(5); // Xóa hết
            break;
        case 'g':
        case 'G':
            handleButtonClick(6); // Bật/Tắt Lưới
            break;
        case 'i':
        case 'I':
            handleButtonClick(7); // Thẻ chỉ số
            break;
        case '+':
        case '=':
            pointSize = std::min(10.0f, pointSize + 0.5f);
            std::cout << "[PIXEL SIZE] Kich thuoc diem anh: " << pointSize << "\n";
            break;
        case '-':
        case '_':
            pointSize = std::max(1.5f, pointSize - 0.5f);
            std::cout << "[PIXEL SIZE] Kich thuoc diem anh: " << pointSize << "\n";
            break;
        case 27: // Phím ESC
            std::cout << "[THOAT] Cam on ban da su dung chuong trinh Lab 2!\n";
            exit(0);
            break;
    }
    glutPostRedisplay();
}

// Các phím chức năng F1 -> F8 không bao giờ bị bộ gõ tiếng Việt Unikey/EVKey nuốt
void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_F1:
            handleButtonClick(1);
            break;
        case GLUT_KEY_F2:
            handleButtonClick(2);
            break;
        case GLUT_KEY_F3:
            handleButtonClick(3);
            break;
        case GLUT_KEY_F4:
            handleButtonClick(4);
            break;
        case GLUT_KEY_F5:
            handleButtonClick(5); // F5 = Xóa toàn bộ
            break;
        case GLUT_KEY_F6:
            handleButtonClick(6); // F6 = Đổi hiển thị lưới
            break;
        case GLUT_KEY_F7:
            handleButtonClick(7); // F7 = Đổi hiển thị thông số
            break;
    }
    glutPostRedisplay();
}

// ============================================================================
// KHỞI TẠO MÔI TRƯỜNG OPENGL
// ============================================================================
void init() {
    // Màu nền Obsidian Dark thanh lịch (#0a0d14)
    glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

// ============================================================================
// HÀM MAIN
// ============================================================================
int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "===================================================================================\n";
    std::cout << "          TRUONG DAI HOC CNTT & TRUYEN THONG VIET - HAN (VKU)                      \n";
    std::cout << "          DO HOA MAY TINH - LAB 2: CAC THUAT TOAN VE DUONG CONG CO SO              \n";
    std::cout << "===================================================================================\n\n";
    std::cout << "1. DANH SACH THUAT TOAN CAI DAT (CHUYEN NGHIEP):\n";
    std::cout << "   [1] Duong tron: Thuat toan BRESENHAM (8-way symmetry)\n";
    std::cout << "   [2] Ellipse   : Thuat toan MIDPOINT (4-way symmetry)\n";
    std::cout << "   [3] Parabol   : Thuat toan BRESENHAM (2-way symmetry)\n\n";
    std::cout << "2. TUONG TAC CHUOT & GIAO DIEN:\n";
    std::cout << "   - Ban co the BẤM PHÍM TẮT hoac CLICK TRUC TIEP tren thanh cong cu:\n";
    std::cout << "     * [1] H.Tron     : Xem 1 hinh tron chuan muc o giua\n";
    std::cout << "     * [2] Ellipse    : Xem 1 hinh ellipse chuan muc o giua\n";
    std::cout << "     * [3] Parabol    : Xem 1 hinh parabol chuan muc o giua\n";
    std::cout << "     * [4] Ca 3 Hinh  : Hien thi dong thoi 3 hinh dan deu 3 cot\n";
    std::cout << "     * [C] Xoa het    : Xoa sach bang ve de tu do ve tay\n";
    std::cout << "     * [G] Luoi       : Bat/Tat luoi toa do\n";
    std::cout << "     * [I] Thong so   : Bat/Tat the thong so\n";
    std::cout << "   - Thao tac ve chuot tren bang ve:\n";
    std::cout << "     * Click lan 1: Chon TAM hoac DINH\n";
    std::cout << "     * Re chuot: Xem truoc hinh anh va kich thuoc dong thoi gian thuc\n";
    std::cout << "     * Click lan 2: CHOT HINH vao bang ve\n";
    std::cout << "     * Click chuot phai: HUY BO thao tac ve\n\n";
    std::cout << "3. PHIM TAT HO TRO:\n";
    std::cout << "   * Phim ky tu: 1, 2, 3, 4, C, G, I, ESC\n";
    std::cout << "   * Phim chuc nang (chong loi Unikey): F1 -> F7\n";
    std::cout << "===================================================================================\n" << std::endl;

    // Tự động nạp sẵn 3 hình dàn đều khi khởi động
    loadPresetMode(4);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("VKU - Lab 2: Ve Duong Tron (Bresenham), Ellipse (Midpoint), Parabol (Bresenham)");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}
