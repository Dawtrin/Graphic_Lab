#define GL_SILENCE_DEPRECATION
#include <GL/glut.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
// HẰNG SỐ VÀ CẤU TRÚC DỮ LIỆU
// ============================================================================
// Định nghĩa mã vùng 4-bit OutCode (Chuẩn: Top-Bottom-Right-Left)
const int INSIDE = 0; // 0000
const int LEFT   = 1; // 0001
const int RIGHT  = 2; // 0010
const int BOTTOM = 4; // 0100
const int TOP    = 8; // 1000

enum ClippingAlgo {
    ALGO_COHEN_SUTHERLAND = 0,
    ALGO_MIDPOINT_SUBDIVISION = 1
};

enum DisplayMode {
    DISP_ALL = 0,       // Hiện cả đoạn gốc mờ và đoạn sau xén đậm
    DISP_CLIPPED_ONLY,  // Chỉ hiện đoạn sau khi xén
    DISP_OUTCODE_GRID   // Hiện các vùng mã OutCode 9 ô
};

struct Point2D {
    double x, y;
};

struct ClipWindow {
    double xmin, xmax;
    double ymin, ymax;
};

struct ClippedLine {
    Point2D origA, origB;       // Đoạn thẳng ban đầu người dùng vẽ
    Point2D clippedA, clippedB; // Đoạn thẳng sau khi xén
    bool accepted;              // Có phần nào nằm trong cửa sổ không
    ClippingAlgo algo;          // Thuật toán sử dụng
    int codeA, codeB;           // Mã OutCode ban đầu của 2 đầu mút
    int steps;                  // Số bước lặp / phép chia nhị phân
    std::vector<Point2D> midpoints; // Danh sách các điểm trung điểm (đối với chia nhị phân)
    float r, g, b;              // Màu sắc hiển thị
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
int windowWidth = 1240;
int windowHeight = 780;

// Cửa sổ xén mặc định (nằm ở trung tâm khu vực bảng vẽ)
ClipWindow clipWin = { 360.0, 880.0, 160.0, 600.0 };

ClippingAlgo currentAlgo = ALGO_COHEN_SUTHERLAND;
DisplayMode currentDisp = DISP_ALL;
bool showGrid = false;
bool showOutCodes = false;
bool showCoordinates = false;

// Trạng thái tương tác chuột
bool isDrawing = false;
Point2D mouseStart = { 0, 0 };
Point2D mouseCurrent = { 0, 0 };

// Danh sách các đoạn thẳng đã vẽ
std::vector<ClippedLine> lines;
std::vector<UIButton> buttons;
int hoveredButtonId = -1;

// Khai báo trước các hàm
void loadPresetLines();
int computeOutCode(double x, double y, const ClipWindow& win);
bool clipCohenSutherland(Point2D A, Point2D B, const ClipWindow& win, 
                         Point2D& outA, Point2D& outB, int& steps);
bool clipMidpointSubdivision(Point2D A, Point2D B, const ClipWindow& win, 
                             Point2D& outA, Point2D& outB, int& steps, 
                             std::vector<Point2D>& sampledMidpoints);

// ============================================================================
// TIỆN ÍCH HIỂN THỊ ĐỒ HỌA & TEXT
// ============================================================================
void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_12) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

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

// Chuyển mã OutCode 4-bit thành chuỗi nhị phân "TBRL"
std::string outCodeToString(int code) {
    std::string s = "";
    s += (code & TOP)    ? "1" : "0";
    s += (code & BOTTOM) ? "1" : "0";
    s += (code & RIGHT)  ? "1" : "0";
    s += (code & LEFT)   ? "1" : "0";
    return s;
}

// Điểm đánh dấu đầu mút rõ nét trên nền trắng
void drawPointMarker(double x, double y, float r, float g, float b, const std::string& label = "") {
    // Vòng tròn định vị
    glColor4f(r, g, b, 0.65f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 16; ++i) {
            float angle = 2.0f * 3.14159265f * i / 16.0f;
            glVertex2d(x + 5.0 * std::cos(angle), y + 5.0 * std::sin(angle));
        }
    glEnd();

    // Điểm tròn trung tâm đặc
    glColor3f(r, g, b);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
        glVertex2d(x, y);
    glEnd();

    if (!label.empty()) {
        glColor3f(0.1f, 0.1f, 0.15f); // Chữ tối rõ nét trên nền trắng
        drawText((float)(x + 8), (float)(y + 4), label, GLUT_BITMAP_HELVETICA_10);
    }
}

// ============================================================================
// 1. TÍNH TOÁN MÃ VÙNG OUTCODE 4-BIT (TBRL)
// ============================================================================
int computeOutCode(double x, double y, const ClipWindow& win) {
    int code = INSIDE;
    if (x < win.xmin)      code |= LEFT;
    else if (x > win.xmax) code |= RIGHT;
    
    if (y < win.ymin)      code |= BOTTOM;
    else if (y > win.ymax) code |= TOP;
    
    return code;
}

// ============================================================================
// 2. THUẬT TOÁN XÉN ĐOẠN THẲNG COHEN - SUTHERLAND
// ============================================================================
// Nguyên lý:
// - Gán mã vùng 4 bit cho 2 đầu mút A, B.
// - Kiểm tra:
//   + codeA | codeB == 0: Đoạn thẳng nằm trọn bên trong cửa sổ -> Giữ nguyên.
//   + codeA & codeB != 0: Hai điểm cùng nằm về một phía bên ngoài -> Bỏ toàn bộ.
//   + Khác 0: Đoạn thẳng cắt biên. Chọn điểm ở ngoài, tính giao điểm với cạnh
//     tương ứng của cửa sổ xén theo phương trình đường thẳng, sau đó lặp lại.
bool clipCohenSutherland(Point2D A, Point2D B, const ClipWindow& win, 
                         Point2D& outA, Point2D& outB, int& steps) {
    double x0 = A.x, y0 = A.y;
    double x1 = B.x, y1 = B.y;
    int code0 = computeOutCode(x0, y0, win);
    int code1 = computeOutCode(x1, y1, win);
    bool accept = false;
    steps = 0;

    while (true) {
        steps++;
        if ((code0 | code1) == 0) {
            // Cả 2 điểm nằm trong cửa sổ
            accept = true;
            break;
        } else if (code0 & code1) {
            // Cả 2 điểm cùng nằm một phía bên ngoài
            accept = false;
            break;
        } else {
            // Có ít nhất một điểm ở ngoài -> Tìm giao điểm
            int codeOut = code0 ? code0 : code1;
            double x = 0, y = 0;

            if (codeOut & TOP) {
                // Điểm nằm phía trên cửa sổ: y = ymax
                x = x0 + (x1 - x0) * (win.ymax - y0) / (y1 - y0);
                y = win.ymax;
            } else if (codeOut & BOTTOM) {
                // Điểm nằm phía dưới cửa sổ: y = ymin
                x = x0 + (x1 - x0) * (win.ymin - y0) / (y1 - y0);
                y = win.ymin;
            } else if (codeOut & RIGHT) {
                // Điểm nằm bên phải cửa sổ: x = xmax
                y = y0 + (y1 - y0) * (win.xmax - x0) / (x1 - x0);
                x = win.xmax;
            } else if (codeOut & LEFT) {
                // Điểm nằm bên trái cửa sổ: x = xmin
                y = y0 + (y1 - y0) * (win.xmin - x0) / (x1 - x0);
                x = win.xmin;
            }

            // Cập nhật điểm ngoài thành giao điểm mới và tính lại OutCode
            if (codeOut == code0) {
                x0 = x;
                y0 = y;
                code0 = computeOutCode(x0, y0, win);
            } else {
                x1 = x;
                y1 = y;
                code1 = computeOutCode(x1, y1, win);
            }
        }
    }

    outA = { x0, y0 };
    outB = { x1, y1 };
    return accept;
}

// ============================================================================
// 3. THUẬT TOÁN CHIA NHỊ PHÂN (MIDPOINT SUBDIVISION LINE CLIPPING)
// ============================================================================
// Nguyên lý chuẩn giáo trình Đồ họa máy tính (Hearn & Baker / Foley / Newman & Sproull):
// - Sử dụng mã OutCode để kiểm tra nhanh:
//   + (code1 | code2) == 0: Đoạn thẳng nằm hoàn toàn bên trong cửa sổ -> Chấp nhận.
//   + (code1 & code2) != 0: Hai điểm cùng nằm về một phía bên ngoài -> Loại bỏ.
// - Nếu đoạn thẳng cắt biên cửa sổ:
//   + Không dùng phép nhân/chia số thực giải phương trình giao điểm như Cohen-Sutherland,
//     mà chia đôi đoạn thẳng tại trung điểm: M = ((p1.x + p2.x)/2, (p1.y + p2.y)/2).
//   + Đệ quy xét 2 đoạn con [p1, M] và [M, p2].
//   + Phép chia đôi này trên phần cứng tương đương 1 phép cộng và 1 phép dịch bit phải (>> 1),
//     tối ưu vượt trội cho các hệ thống phần cứng không có FPU.
//   + Quá trình đệ quy dừng lại khi đoạn con được chấp nhận, loại bỏ hoặc độ dài < 0.1 pixel.
void subdivideRecursive(Point2D p1, Point2D p2, int code1, int code2, const ClipWindow& win, 
                        std::vector<std::pair<Point2D, Point2D>>& acceptedList, int depth,
                        int& steps, std::vector<Point2D>& midpoints) {
    steps++;

    // 1. Chấp nhận toàn bộ
    if ((code1 | code2) == 0) {
        acceptedList.push_back({ p1, p2 });
        return;
    }

    // 2. Loại bỏ toàn bộ cùng phía
    if (code1 & code2) {
        return;
    }

    // 3. Ngưỡng dừng độ phân giải pixel (dưới 0.1 px) hoặc độ sâu tối đa
    if (depth > 22 || std::hypot(p1.x - p2.x, p1.y - p2.y) < 0.1) {
        Point2D mid = { (p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5 };
        if (computeOutCode(mid.x, mid.y, win) == INSIDE) {
            acceptedList.push_back({ mid, mid });
        }
        return;
    }

    // 4. Chia đôi tại trung điểm
    Point2D mid = { (p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5 };
    if (midpoints.size() < 300) {
        midpoints.push_back(mid);
    }
    int codeMid = computeOutCode(mid.x, mid.y, win);

    // Đệ quy theo thứ tự từ p1 đến mid rồi từ mid đến p2 (duyệt in-order theo chiều đoạn thẳng)
    subdivideRecursive(p1, mid, code1, codeMid, win, acceptedList, depth + 1, steps, midpoints);
    subdivideRecursive(mid, p2, codeMid, code2, win, acceptedList, depth + 1, steps, midpoints);
}

bool clipMidpointSubdivision(Point2D A, Point2D B, const ClipWindow& win, 
                             Point2D& outA, Point2D& outB, int& steps, 
                             std::vector<Point2D>& sampledMidpoints) {
    int codeA = computeOutCode(A.x, A.y, win);
    int codeB = computeOutCode(B.x, B.y, win);
    steps = 0;
    sampledMidpoints.clear();

    // 1. Nằm hoàn toàn bên trong
    if ((codeA | codeB) == 0) {
        outA = A;
        outB = B;
        return true;
    }

    // 2. Cùng nằm một phía bên ngoài
    if (codeA & codeB) {
        outA = A;
        outB = B;
        return false;
    }

    // 3. Giao cắt biên: Đệ quy chia đôi tại trung điểm
    std::vector<std::pair<Point2D, Point2D>> acceptedList;
    subdivideRecursive(A, B, codeA, codeB, win, acceptedList, 0, steps, sampledMidpoints);

    if (acceptedList.empty()) {
        outA = A;
        outB = B;
        return false;
    }

    // Ghép điểm đầu của phân đoạn đầu tiên và điểm cuối của phân đoạn cuối cùng
    outA = acceptedList.front().first;
    outB = acceptedList.back().second;
    return true;
}

// ============================================================================
// HÀM TẠO ĐOẠN THẲNG MỚI ĐÃ ĐƯỢC XÉN
// ============================================================================
ClippedLine processLine(Point2D A, Point2D B, ClippingAlgo algo, float r, float g, float b) {
    ClippedLine cl;
    cl.origA = A;
    cl.origB = B;
    cl.algo = algo;
    cl.codeA = computeOutCode(A.x, A.y, clipWin);
    cl.codeB = computeOutCode(B.x, B.y, clipWin);
    cl.r = r;
    cl.g = g;
    cl.b = b;

    if (algo == ALGO_COHEN_SUTHERLAND) {
        cl.accepted = clipCohenSutherland(A, B, clipWin, cl.clippedA, cl.clippedB, cl.steps);
    } else {
        cl.accepted = clipMidpointSubdivision(A, B, clipWin, cl.clippedA, cl.clippedB, cl.steps, cl.midpoints);
    }

    return cl;
}

// ============================================================================
// DỮ LIỆU MẪU ĐIỂN HÌNH (PRESET DEMO)
// ============================================================================
// ============================================================================
// DỮ LIỆU MẪU ĐIỂN HÌNH (PRESET DEMO)
// ============================================================================
void loadPresetLines() {
    lines.clear();
    // 1. Đoạn hoàn toàn bên trong (Accept) - Xanh lá cây đậm
    lines.push_back(processLine({ 420, 240 }, { 750, 480 }, currentAlgo, 0.05f, 0.60f, 0.20f));

    // 2. Đoạn hoàn toàn bên ngoài (Reject cùng phía bên trái) - Đỏ đậm
    lines.push_back(processLine({ 180, 200 }, { 280, 520 }, currentAlgo, 0.85f, 0.20f, 0.20f));

    // 3. Đoạn cắt xuyên qua 2 biên (Trái sang Phải) - Xanh dương đậm
    lines.push_back(processLine({ 260, 320 }, { 960, 420 }, currentAlgo, 0.10f, 0.40f, 0.85f));

    // 4. Đoạn cắt xuyên từ Dưới lên Trên - Cam đậm
    lines.push_back(processLine({ 500, 100 }, { 680, 660 }, currentAlgo, 0.90f, 0.45f, 0.05f));

    // 5. Đoạn cắt qua góc (Trên sang Phải) - Tím
    lines.push_back(processLine({ 800, 650 }, { 940, 480 }, currentAlgo, 0.65f, 0.15f, 0.75f));

    // 6. Đoạn nằm chéo bên ngoài không cùng mã (Cắt ngoài cửa sổ) - Xám tím
    lines.push_back(processLine({ 280, 640 }, { 920, 120 }, currentAlgo, 0.45f, 0.35f, 0.65f));
}

// ============================================================================
// VẼ CỬA SỔ XÉN VÀ LƯỚI TỌA ĐỘ
// ============================================================================
void drawGridAndClipWindow() {
    // 1. Lưới nền kỹ thuật nhẹ trên nền trắng (tùy chọn bật/tắt bằng phím G)
    if (showGrid) {
        glColor4f(0.88f, 0.90f, 0.93f, 0.8f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        for (int x = 0; x < windowWidth; x += 40) {
            glVertex2i(x, 0);
            glVertex2i(x, windowHeight);
        }
        for (int y = 0; y < windowHeight; y += 40) {
            glVertex2i(0, y);
            glVertex2i(windowWidth, y);
        }
        glEnd();
    }

    // 2. Khung cửa sổ xén đơn giản, rõ nét (Không có nhãn, Pmax, Pmin)
    glColor3f(0.12f, 0.22f, 0.50f); // Đường viền màu xanh navy đậm sắc nét
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2d(clipWin.xmin, clipWin.ymin);
        glVertex2d(clipWin.xmax, clipWin.ymin);
        glVertex2d(clipWin.xmax, clipWin.ymax);
        glVertex2d(clipWin.xmin, clipWin.ymax);
    glEnd();
}

// Hàm vẽ một đoạn đường thẳng với màu sắc và độ dày chỉ định
void drawLinePart(Point2D p1, Point2D p2, float r, float g, float b, float width) {
    if (std::hypot(p1.x - p2.x, p1.y - p2.y) < 0.5) return;
    glColor3f(r, g, b);
    glLineWidth(width);
    glBegin(GL_LINES);
        glVertex2d(p1.x, p1.y);
        glVertex2d(p2.x, p2.y);
    glEnd();
}

// ============================================================================
// RENDER CÁC ĐOẠN THẲNG TRƯỚC VÀ SAU KHI XÉN
// ============================================================================
void renderClippedLine(const ClippedLine& cl, int index) {
    std::string idStr = std::to_string(index + 1);

    if (cl.accepted) {
        // 1. Phần ngoài cửa sổ tại đầu mút A (Bị xén bỏ) -> Vẽ màu ĐỎ khi ở chế độ DISP_ALL
        if (currentDisp == DISP_ALL && std::hypot(cl.origA.x - cl.clippedA.x, cl.origA.y - cl.clippedA.y) > 0.5) {
            drawLinePart(cl.origA, cl.clippedA, 0.90f, 0.20f, 0.15f, 2.0f);
            drawPointMarker(cl.clippedA.x, cl.clippedA.y, 0.95f, 0.50f, 0.05f, "I1");
        }

        // 2. Phần trong cửa sổ (Được giữ lại) -> Vẽ màu XANH LÁ ĐẬM nét dày
        drawLinePart(cl.clippedA, cl.clippedB, 0.05f, 0.65f, 0.20f, 3.5f);

        // 3. Phần ngoài cửa sổ tại đầu mút B (Bị xén bỏ) -> Vẽ màu ĐỎ khi ở chế độ DISP_ALL
        if (currentDisp == DISP_ALL && std::hypot(cl.origB.x - cl.clippedB.x, cl.origB.y - cl.clippedB.y) > 0.5) {
            drawLinePart(cl.clippedB, cl.origB, 0.90f, 0.20f, 0.15f, 2.0f);
            drawPointMarker(cl.clippedB.x, cl.clippedB.y, 0.95f, 0.50f, 0.05f, "I2");
        }
    } else if (currentDisp == DISP_ALL) {
        // Nằm hoàn toàn bên ngoài cửa sổ -> Vẽ toàn bộ từ A đến B bằng màu ĐỎ (Bị xén bỏ hoàn toàn)
        drawLinePart(cl.origA, cl.origB, 0.90f, 0.20f, 0.15f, 2.0f);
    }

    // Luôn vẽ 2 điểm mút A và B (khi DISP_ALL) hoặc 2 điểm đầu mút sau xén (khi DISP_CLIPPED_ONLY)
    if (currentDisp == DISP_ALL) {
        drawPointMarker(cl.origA.x, cl.origA.y, 0.12f, 0.40f, 0.85f, "A" + idStr);
        drawPointMarker(cl.origB.x, cl.origB.y, 0.12f, 0.40f, 0.85f, "B" + idStr);
    } else if (cl.accepted) {
        drawPointMarker(cl.clippedA.x, cl.clippedA.y, 0.95f, 0.50f, 0.05f, "I1");
        drawPointMarker(cl.clippedB.x, cl.clippedB.y, 0.95f, 0.50f, 0.05f, "I2");
    }
}

// ============================================================================
// VẼ GIAO DIỆN THANH CÔNG CỤ (TOP BAR) & STATUS BAR PHONG CÁCH SÁNG (LIGHT THEME)
// ============================================================================
void initButtons() {
    buttons.clear();
    int x = 20;
    int y = windowHeight - 52;
    int h = 34;

    buttons.push_back({ 1, x, y, 185, h, "1. Cohen-Sutherland", "Phim 1", 0.12f, 0.45f, 0.90f, currentAlgo == ALGO_COHEN_SUTHERLAND });
    x += 195;
    buttons.push_back({ 2, x, y, 195, h, "2. Chia nhi phan (Midpoint)", "Phim 2", 0.85f, 0.45f, 0.10f, currentAlgo == ALGO_MIDPOINT_SUBDIVISION });
    x += 205;
    buttons.push_back({ 3, x, y, 130, h, "Doi che do xem", "Phim V", 0.15f, 0.65f, 0.40f, false });
    x += 140;
    buttons.push_back({ 4, x, y, 110, h, "Du lieu mau", "Phim P", 0.60f, 0.25f, 0.80f, false });
    x += 120;
    buttons.push_back({ 5, x, y, 90, h, "Xoa het", "Phim C", 0.85f, 0.20f, 0.20f, false });
    x += 100;
    buttons.push_back({ 6, x, y, 100, h, "Bat/Tat Luoi", "Phim G", 0.40f, 0.45f, 0.55f, showGrid });
}

void renderUI() {
    // 1. Thanh tiêu đề phía trên (Top Header Bar) - Nền sáng
    drawRect(0, (float)(windowHeight - 64), (float)windowWidth, 64, 0.95f, 0.96f, 0.97f, 1.0f, true);
    drawRect(0, (float)(windowHeight - 64), (float)windowWidth, 64, 0.82f, 0.84f, 0.87f, 1.0f, false);

    // Tiêu đề trường & môn học
    glColor3f(0.10f, 0.15f, 0.25f);
    drawText(20, (float)(windowHeight - 18), "VKU - DO HOA MAY TINH | LAB 3: THUAT TOAN XEN HINH (LINE CLIPPING)", GLUT_BITMAP_HELVETICA_12);

    // Vẽ các nút bấm chức năng
    initButtons();
    for (const auto& btn : buttons) {
        bool isHovered = (btn.id == hoveredButtonId);
        if (btn.isActive) {
            drawRect((float)btn.x, (float)btn.y, (float)btn.w, (float)btn.h, btn.r, btn.g, btn.b, 0.90f, true);
            drawRect((float)btn.x, (float)btn.y, (float)btn.w, (float)btn.h, btn.r * 0.8f, btn.g * 0.8f, btn.b * 0.8f, 1.0f, false);
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            float bgAlpha = isHovered ? 0.95f : 0.85f;
            drawRect((float)btn.x, (float)btn.y, (float)btn.w, (float)btn.h, 0.90f, 0.92f, 0.94f, bgAlpha, true);
            drawRect((float)btn.x, (float)btn.y, (float)btn.w, (float)btn.h, 0.78f, 0.80f, 0.84f, 1.0f, false);
            glColor3f(0.15f, 0.20f, 0.28f);
        }
        drawText((float)(btn.x + 10), (float)(btn.y + 12), btn.label, GLUT_BITMAP_HELVETICA_10);
    }

    // 2. Thanh trạng thái đáy màn hình (Bottom Status Bar) - Nền sáng
    drawRect(0, 0, (float)windowWidth, 32, 0.95f, 0.96f, 0.97f, 1.0f, true);
    drawRect(0, 0, (float)windowWidth, 32, 0.82f, 0.84f, 0.87f, 1.0f, false);

    glColor3f(0.15f, 0.20f, 0.30f);
    std::string mouseStatus = isDrawing ? 
        "DANG VE DOAN THANG: A(" + std::to_string((int)mouseStart.x) + ", " + std::to_string((int)mouseStart.y) + ") -> B(" + std::to_string((int)mouseCurrent.x) + ", " + std::to_string((int)mouseCurrent.y) + ") [Click de chot B]" :
        "SAN SANG: Click chuot trai de bat dau ve doan thang AB (Click 1: Diem A, Click 2: Diem B)";
    drawText(20, 11, mouseStatus, GLUT_BITMAP_HELVETICA_10);

    // Chú thích màu sắc rõ ràng cho người dùng
    glColor3f(0.0f, 0.55f, 0.15f);
    drawText((float)(windowWidth - 560), 11, "[Xanh: Giu lai]", GLUT_BITMAP_HELVETICA_10);
    glColor3f(0.85f, 0.15f, 0.15f);
    drawText((float)(windowWidth - 460), 11, "[Do: Bi xen]", GLUT_BITMAP_HELVETICA_10);
    glColor3f(0.3f, 0.35f, 0.45f);
    drawText((float)(windowWidth - 370), 11, "| Phim: [1] Cohen-Sutherland  [2] Chia nhi phan  [P] Demo  [C] Xoa", GLUT_BITMAP_HELVETICA_10);
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

    // 1. Cửa sổ xén (khung chữ nhật đơn giản trên nền trắng)
    drawGridAndClipWindow();

    // 2. Vẽ tất cả các đoạn thẳng đã chốt
    for (size_t i = 0; i < lines.size(); ++i) {
        renderClippedLine(lines[i], (int)i);
    }

    // 3. Xem trước động thời gian thực (Live Preview) khi đang kéo chuột
    if (isDrawing) {
        // Tính thử xén ngay lập tức trong thời gian thực
        ClippedLine preview = processLine(mouseStart, mouseCurrent, currentAlgo, 0.05f, 0.65f, 0.20f);

        if (preview.accepted) {
            // Phần ngoài A -> I1 (Bị xén bỏ ngoài khung -> Màu Đỏ)
            if (std::hypot(mouseStart.x - preview.clippedA.x, mouseStart.y - preview.clippedA.y) > 0.5) {
                drawLinePart(mouseStart, preview.clippedA, 0.90f, 0.20f, 0.15f, 2.0f);
                drawPointMarker(preview.clippedA.x, preview.clippedA.y, 0.95f, 0.50f, 0.05f, "I1");
            }

            // Phần trong I1 -> I2 (Giữ lại trong khung -> Màu Xanh lá đậm)
            drawLinePart(preview.clippedA, preview.clippedB, 0.05f, 0.65f, 0.20f, 3.5f);

            // Phần ngoài I2 -> B (Bị xén bỏ ngoài khung -> Màu Đỏ)
            if (std::hypot(mouseCurrent.x - preview.clippedB.x, mouseCurrent.y - preview.clippedB.y) > 0.5) {
                drawLinePart(preview.clippedB, mouseCurrent, 0.90f, 0.20f, 0.15f, 2.0f);
                drawPointMarker(preview.clippedB.x, preview.clippedB.y, 0.95f, 0.50f, 0.05f, "I2");
            }
        } else {
            // Toàn bộ đoạn nằm ngoài cửa sổ -> Vẽ màu Đỏ từ A đến B
            drawLinePart(mouseStart, mouseCurrent, 0.90f, 0.20f, 0.15f, 2.0f);
        }

        // Vẽ 2 điểm A và B đang kéo tại đúng vị trí chuột
        drawPointMarker(mouseStart.x, mouseStart.y, 0.12f, 0.40f, 0.85f, 
                        "A (" + std::to_string((int)mouseStart.x) + "," + std::to_string((int)mouseStart.y) + ")");
        drawPointMarker(mouseCurrent.x, mouseCurrent.y, 0.85f, 0.35f, 0.10f, 
                        "B (" + std::to_string((int)mouseCurrent.x) + "," + std::to_string((int)mouseCurrent.y) + ")");
    }

    // 4. Thanh giao diện UI phía trên và thanh trạng thái phía dưới
    renderUI();

    glFlush();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    
    // Tự động canh chỉnh lại vị trí cửa sổ xén theo tỷ lệ màn hình
    clipWin.xmin = windowWidth * 0.28;
    clipWin.xmax = windowWidth * 0.76;
    clipWin.ymin = windowHeight * 0.22;
    clipWin.ymax = windowHeight * 0.78;
    
    // Tính toán lại tất cả các đoạn thẳng theo kích thước mới
    for (auto& l : lines) {
        l = processLine(l.origA, l.origB, l.algo, l.r, l.g, l.b);
    }
}

// ============================================================================
// XỬ LÝ SỰ KIỆN NÚT BẤM (BUTTON CLICK HANDLER)
// ============================================================================
void handleButtonClick(int btnId) {
    switch (btnId) {
        case 1:
            currentAlgo = ALGO_COHEN_SUTHERLAND;
            for (auto& l : lines) l = processLine(l.origA, l.origB, currentAlgo, l.r, l.g, l.b);
            std::cout << "[THUAT TOAN] Chuyen sang: Cohen-Sutherland\n";
            break;
        case 2:
            currentAlgo = ALGO_MIDPOINT_SUBDIVISION;
            for (auto& l : lines) l = processLine(l.origA, l.origB, currentAlgo, l.r, l.g, l.b);
            std::cout << "[THUAT TOAN] Chuyen sang: Chia nhi phan (Midpoint Subdivision)\n";
            break;
        case 3:
            currentDisp = (DisplayMode)((currentDisp + 1) % 2);
            break;
        case 4:
            loadPresetLines();
            std::cout << "[DEMO] Da nap cac doan thang mau test moi truong hop cat xen!\n";
            break;
        case 5:
            lines.clear();
            isDrawing = false;
            std::cout << "[XOA] Da xoa toan bo doan thang!\n";
            break;
        case 6:
            showGrid = !showGrid;
            break;
    }
}

// ============================================================================
// XỬ LÝ SỰ KIỆN CHUỘT (MOUSE CLICKS & MOTION)
// ============================================================================
void mouseClick(int button, int state, int x, int y) {
    int worldX = x;
    int worldY = windowHeight - y;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // 1. Kiểm tra bấm nút trên Top Bar
        if (worldY >= windowHeight - 64) {
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

        // Bỏ qua bấm vào vùng status bar dưới cùng
        if (worldY <= 32) return;

        // 2. Vẽ đoạn thẳng AB (Cho phép click vẽ tự do toàn màn hình)
        if (!isDrawing) {
            // Click lần 1: Đặt điểm đầu A
            isDrawing = true;
            mouseStart = { (double)worldX, (double)worldY };
            mouseCurrent = mouseStart;
            std::cout << "[CLICK 1] Chon diem A(" << (int)mouseStart.x << ", " << (int)mouseStart.y << ")\n";
        } else {
            // Click lần 2: Đặt điểm cuối B và hoàn tất đoạn thẳng
            isDrawing = false;
            Point2D endPt = { (double)worldX, (double)worldY };
            
            // Bảng màu đẹp, tương phản rõ trên nền trắng
            float palette[6][3] = {
                { 0.05f, 0.60f, 0.20f }, // Xanh lá đậm
                { 0.10f, 0.40f, 0.85f }, // Xanh dương
                { 0.85f, 0.20f, 0.15f }, // Đỏ
                { 0.90f, 0.45f, 0.05f }, // Cam
                { 0.65f, 0.15f, 0.75f }, // Tím
                { 0.10f, 0.55f, 0.60f }  // Xanh mòng két
            };
            int colorIdx = (int)lines.size() % 6;

            ClippedLine cl = processLine(mouseStart, endPt, currentAlgo, 
                                         palette[colorIdx][0], palette[colorIdx][1], palette[colorIdx][2]);
            lines.push_back(cl);

            std::cout << "[CLICK 2] Chot diem B(" << (int)endPt.x << ", " << (int)endPt.y << ") -> Xen bang "
                      << (currentAlgo == ALGO_COHEN_SUTHERLAND ? "Cohen-Sutherland" : "Chia nhi phan")
                      << " -> " << (cl.accepted ? "CHAP NHAN" : "LOAI BO") << "\n";
        }
        glutPostRedisplay();
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        // Click chuột phải để hủy thao tác vẽ dở dang
        if (isDrawing) {
            isDrawing = false;
            std::cout << "[HUY] Da huy ve doan thang hien tai.\n";
            glutPostRedisplay();
        }
    }
}

void mousePassiveMotion(int x, int y) {
    int worldX = x;
    int worldY = windowHeight - y;

    // Hover button
    int oldHovered = hoveredButtonId;
    hoveredButtonId = -1;
    if (worldY >= windowHeight - 64) {
        for (const auto& btn : buttons) {
            if (worldX >= btn.x && worldX <= btn.x + btn.w &&
                worldY >= btn.y && worldY <= btn.y + btn.h) {
                hoveredButtonId = btn.id;
                break;
            }
        }
    }

    if (isDrawing) {
        mouseCurrent = { (double)worldX, (double)worldY };
        glutPostRedisplay();
    } else if (oldHovered != hoveredButtonId) {
        glutPostRedisplay();
    }
}

void mouseMotion(int x, int y) {
    mousePassiveMotion(x, y);
}

// ============================================================================
// XỬ LÝ BÀN PHÍM (HOTKEYS)
// ============================================================================
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case '1':
            handleButtonClick(1);
            break;
        case '2':
            handleButtonClick(2);
            break;
        case 'v': case 'V':
            handleButtonClick(3);
            break;
        case 'p': case 'P':
            handleButtonClick(4);
            break;
        case 'c': case 'C':
            handleButtonClick(5);
            break;
        case 'g': case 'G':
            handleButtonClick(6);
            break;
        case 27: // Phím ESC: Hủy vẽ hoặc thoát
            if (isDrawing) {
                isDrawing = false;
            } else {
                exit(0);
            }
            break;
    }
    glutPostRedisplay();
}

// ============================================================================
// HÀM KHỞI TẠO ĐỒ HỌA
// ============================================================================
void initGL() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // NỀN TRẮNG TINH KHIẾT
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    showGrid = false;
    showOutCodes = false;

    // Tự động tải bộ dữ liệu mẫu ban đầu
    loadPresetLines();
}

// ============================================================================
// HÀM MAIN
// ============================================================================
int main(int argc, char** argv) {
    #ifdef _WIN32
    SetConsoleOutputCP(65001); // UTF-8 console output
    #endif

    std::cout << "========================================================\n";
    std::cout << "   VKU - LAB 3: THUAT TOAN XEN HINH (LINE CLIPPING)     \n";
    std::cout << "   1. Cohen - Sutherland                                \n";
    std::cout << "   2. Chia nhi phan (Midpoint Subdivision)              \n";
    std::cout << "========================================================\n";
    std::cout << "[HUONG DAN SU DUNG]\n";
    std::cout << "- Click chuot trai lan 1: Chon diem A can xen\n";
    std::cout << "- Di chuyen chuot: Xem truoc xen thoi gian thuc\n";
    std::cout << "- Click chuot trai lan 2: Chot diem B va luu doan thang\n";
    std::cout << "- Nhan phim '1': Thuat toan Cohen-Sutherland\n";
    std::cout << "- Nhan phim '2': Thuat toan Chia nhi phan (Midpoint)\n";
    std::cout << "- Nhan phim 'P': Nap du lieu mau cac truong hop cat xen\n";
    std::cout << "- Nhan phim 'C': Xoa toan bo doan thang\n";
    std::cout << "- Nhan phim 'ESC': Huy thao tac ve / Thoat\n";
    std::cout << "========================================================\n\n";

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(80, 40);
    glutCreateWindow("VKU - Lab 3: Thuat toan xen hinh (Cohen-Sutherland & Chia nhi phan)");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
