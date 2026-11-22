// =============================================================================
// TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN (VKU)
// KHOA KY THUAT MAY TINH VA DIEN TU - BO MON DO HOA MAY TINH
// =============================================================================
// BAI TAP THUC HANH TAI LOP (NGAY 16/11):
// DE BAI:
//   Trong khong gian 3D, cho hinh chop SABC, ve va quan sat hinh chop nay.
// YEU CAU:
//   1. Khu khuat hinh chop bang:
//      - Thuat toan Backface (Backface Culling)
//      - Thuat toan Deep-buffer (Depth-buffer / Z-buffer)
//   2. To bong hinh chop bang phuong phap to bong Gouraud (Gouraud Shading)
//   3. Ho tro xuat 3 hinh nop bai (Backface, Deep-buffer, Gouraud) vao Zalo.
//
// BIEN DICH VA CHAY:
//   cmake -B build
//   cmake --build build --target Lab_TH_1611_main
//   build\Lab_TH_1611_main.exe          (Chay giao dien do hoa truc quan)
//   build\Lab_TH_1611_main.exe --test   (Chay bo kiem thu tu dong Self-Tests)
// =============================================================================

#include <GL/freeglut.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static const double PI = 3.14159265358979323846;

// =============================================================================
// 1. TOAN HOC 3D & HINH HOC (3D VECTOR & MATRIX MATH)
// =============================================================================

struct Vec3 {
    double x = 0, y = 0, z = 0;

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(double s) const { return { x * s, y * s, z * s }; }
    Vec3 operator/(double s) const { return { x / s, y / s, z / s }; }
};

static double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static double length(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

static Vec3 normalize(const Vec3& v) {
    double l = length(v);
    if (l < 1e-9) return { 0, 1, 0 };
    return v / l;
}

// =============================================================================
// 2. MO HINH HINH CHOP SABC (PYRAMID SABC DATA STRUCTURE)
// =============================================================================

struct FaceInfo {
    std::string name;           // "SAB", "SBC", "SCA", "ABC"
    std::vector<int> vertIdx;   // 3 chi so dinh theo thu tu CCW tu ngoai nhin vao
    Vec3 normal;                // Vector phap tuyen ngoai
    Vec3 centroid;              // Trong tam mat
    unsigned colorHex;          // Mau mat dac trung
    bool isFrontFace;           // Ket qua thuat toan Backface
    double dotNV;               // Tich vo huong N . V
};

struct PyramidSABC {
    // 4 dinh: S (Apex), A, B, C (Base vertices)
    std::vector<Vec3> vertices;
    std::vector<std::string> vertexNames;
    std::vector<FaceInfo> faces;
    std::vector<Vec3> vertexNormals;     // Dung cho Gouraud Shading (Gop ca day)
    std::vector<Vec3> sideVertexNormals; // Phap tuyen than chop (khong bi mat day keo xuong)
    Vec3 baseNormal;                     // Phap tuyen rieng mat day

    void initDefault() {
        vertices.clear();
        vertexNames = { "S", "A", "B", "C" };
        
        // Dinh S tren cao, 3 dinh A, B, C tao thanh tam giac deu tren mat phang y = -1.0
        vertices.push_back({  0.0,  2.2,  0.0 }); // 0: S
        vertices.push_back({ -1.8, -1.0,  1.2 }); // 1: A
        vertices.push_back({  1.8, -1.0,  1.2 }); // 2: B
        vertices.push_back({  0.0, -1.0, -1.8 }); // 3: C

        faces.clear();
        // Thu tu CCW (Counter-Clockwise) khi nhin tu ngoai vao de phap tuyen huong ra ngoai:
        faces.push_back({ "Mat ben SAB", { 0, 1, 2 }, {}, {}, 0x00D2D3, true, 0.0 });
        faces.push_back({ "Mat ben SBC", { 0, 2, 3 }, {}, {}, 0xFFA62B, true, 0.0 });
        faces.push_back({ "Mat ben SCA", { 0, 3, 1 }, {}, {}, 0x9D4EDD, true, 0.0 });
        faces.push_back({ "Mat day ABC", { 1, 3, 2 }, {}, {}, 0x2ED573, true, 0.0 });

        updateGeometry();
    }

    void updateGeometry() {
        // 1. Tinh vector phap tuyen va trong tam cho tung mat
        for (auto& f : faces) {
            Vec3 v0 = vertices[f.vertIdx[0]];
            Vec3 v1 = vertices[f.vertIdx[1]];
            Vec3 v2 = vertices[f.vertIdx[2]];
            Vec3 e1 = v1 - v0;
            Vec3 e2 = v2 - v0;
            f.normal = normalize(cross(e1, e2));
            f.centroid = (v0 + v1 + v2) * (1.0 / 3.0);
        }

        baseNormal = faces[3].normal;

        // 2A. Phap tuyen than chop (Side-only vertex normals - KHONG bi mat day keo toi)
        sideVertexNormals.assign(vertices.size(), { 0, 0, 0 });
        for (size_t fIdx = 0; fIdx < 3; fIdx++) { // Chi duyet 3 mat ben 0, 1, 2
            const auto& f = faces[fIdx];
            for (int idx : f.vertIdx) {
                sideVertexNormals[idx] = sideVertexNormals[idx] + f.normal;
            }
        }
        for (auto& vn : sideVertexNormals) {
            vn = normalize(vn);
        }

        // 2B. Phap tuyen gop chung toan bo (All-shared)
        vertexNormals.assign(vertices.size(), { 0, 0, 0 });
        for (const auto& f : faces) {
            for (int idx : f.vertIdx) {
                vertexNormals[idx] = vertexNormals[idx] + f.normal;
            }
        }
        for (auto& vn : vertexNormals) {
            vn = normalize(vn);
        }
    }

    // Tinh toan thuat toan Backface Culling doi voi Camera Eye
    void evaluateBackface(const Vec3& eyePos) {
        for (auto& f : faces) {
            Vec3 viewVec = normalize(eyePos - f.centroid);
            f.dotNV = dot(f.normal, viewVec);
            f.isFrontFace = (f.dotNV > 0.0001);
        }
    }
};

// =============================================================================
// 3. CAU TRUC MAU SAC VA GIAO DIEN (COLOR & UI STYLES)
// =============================================================================

struct ColorRGBA {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

static ColorRGBA hexCol(unsigned hex, float alpha = 1.0f) {
    float r = ((hex >> 16) & 0xFF) / 255.0f;
    float g = ((hex >> 8) & 0xFF) / 255.0f;
    float b = (hex & 0xFF) / 255.0f;
    return { r, g, b, alpha };
}

static void setCol(const ColorRGBA& c) {
    glColor4f(c.r, c.g, c.b, c.a);
}

static const unsigned CARD_BG_COLOR   = 0x161C24;
static const unsigned PANEL_BG_COLOR  = 0x11161D;
static const unsigned BORDER_LINE     = 0x2A3544;
static const unsigned TEXT_MAIN       = 0xF0F4F8;
static const unsigned TEXT_MUTED      = 0x8898AA;
static const unsigned ACCENT_CYAN     = 0x00E5BE;
static const unsigned ACCENT_ORANGE   = 0xFF8A3D;
static const unsigned ACCENT_YELLOW   = 0xFFD200;
static const unsigned ACCENT_GREEN    = 0x2ED573;
static const unsigned ACCENT_RED      = 0xFF4757;
static const unsigned ACCENT_PURPLE   = 0xA29BFE;

static void* const FONT_LG = GLUT_BITMAP_HELVETICA_18;
static void* const FONT_MD = GLUT_BITMAP_HELVETICA_12;
static void* const FONT_SM = GLUT_BITMAP_HELVETICA_10;

// =============================================================================
// 4. CAC CHE DO HOAT DONG THEO DUNG DE BAI
// =============================================================================

enum LabMode {
    MODE_BACKFACE = 0,    // 1. Thuat toan Backface
    MODE_DEEP_BUFFER = 1, // 1. Thuat toan Deep-buffer (Depth-buffer)
    MODE_GOURAUD = 2,     // 2. To bong Gouraud (Gouraud Shading)
    MODE_COMPARE = 3      // So sanh tong hop ca 3 phuong phap
};

enum ShadingType {
    SHADE_GOURAUD = 0,
    SHADE_FLAT = 1
};

struct UIControlBtn {
    double x, y, w, h;
    std::string text;
    std::function<void()> onClick;
    bool active;
};

struct UIControlSlider {
    double x, y, w, h;
    std::string label;
    double minVal, maxVal;
    double* pValue;
    std::string unit;
    int id;
};

struct AppState {
    LabMode activeMode = MODE_BACKFACE;

    // Du lieu hinh chop SABC
    PyramidSABC pyramid;
    double apexHeight = 2.2;
    double baseRadius = 2.0;

    // He quan sat & Camera Orbit
    double thetaDeg = 45.0; // Azimuth (goc phuong vi)
    double phiDeg   = 28.0; // Elevation (goc ngẩng)
    double eyeRadius = 8.5; // Khoang cach nhin
    Vec3 target = { 0.0, 0.2, 0.0 };
    bool autoOrbit = false;
    double orbitSpeed = 0.5;

    // Tuy chon Backface
    bool enableBackfaceCulling = true; // Bat/Tat de so sanh
    bool showNormals = true;           // Ve vector phap tuyen
    bool showCentroids = true;         // Hien thi trong tam
    bool showVertexLabels = true;      // Hien thi nhan S, A, B, C

    // Tuy chon Deep-buffer (Depth-buffer)
    bool enableDepthBuffer = true;     // Bat/Tat Depth Test
    bool showDepthMapOverlay = true;   // Hien thi khung phu Depth Map thang xám
    bool simulateReverseDraw = false;  // Dao nguoc thu tu ve de thay loi khi tat Depth Buffer

    // Tuy chon To bong Gouraud
    ShadingType shadingMode = SHADE_GOURAUD; // Gouraud vs Flat
    bool showLightSource = true;       // Hien thi nguon sang 3D
    bool showVertexNormals = true;     // Hien thi phap tuyen dinh
    bool useSmoothSides = true;        // Phap tuyen than chop rieng (khong bi mat day keo toi)
    bool uniformColorMode = true;      // Vat lieu dong nhat de thay ro dai sang bong
    bool showGouraudEdges = false;     // Khung vien Wireframe khi to bong
    double gouraudSubdivDouble = 4.0;  // Do chia luoi tam giac de hien thi ro specular
    Vec3 lightPos = { 4.5, 6.0, 4.5 };
    double lightAmb = 0.25;
    double lightDiff = 0.85;
    double lightSpec = 0.70;
    double shininess = 50.0;

    // Mau sac chóp
    double pyramidColor[3] = { 0.15, 0.80, 0.70 };

    // Thong bao chup anh (Screenshot Notification)
    std::string toastMessage = "";
    int toastTimer = 0;

    // Giao dien & Chuot
    int windowW = 1420, windowH = 900;
    double mouseX = 0, mouseY = 0;
    double lastMouseX = 0, lastMouseY = 0;
    bool isLeftDragging = false;
    bool isRightDragging = false;
    int activeSliderId = -1;

    std::vector<UIControlBtn> buttons;
    std::vector<UIControlSlider> sliders;
} g;

static const double HEADER_H = 65.0;
static double sidePanelW() { return 460.0; }
static double viewportW()  { return g.windowW - sidePanelW(); }
static double viewportH()  { return g.windowH - HEADER_H; }

// =============================================================================
// 5. HAM TINH VI TRI CAMERA & TIEN ICH 2D/3D
// =============================================================================

static Vec3 computeEyePosition(const Vec3& tgt, double thetaDeg, double phiDeg, double radius) {
    double radTheta = thetaDeg * PI / 180.0;
    double radPhi   = phiDeg   * PI / 180.0;
    double x = tgt.x + radius * std::cos(radPhi) * std::sin(radTheta);
    double y = tgt.y + radius * std::sin(radPhi);
    double z = tgt.z + radius * std::cos(radPhi) * std::cos(radTheta);
    return { x, y, z };
}

static void fillRect2D(double x, double y, double w, double h, ColorRGBA c) {
    setCol(c);
    glBegin(GL_QUADS);
    glVertex2d(x, y);
    glVertex2d(x + w, y);
    glVertex2d(x + w, y + h);
    glVertex2d(x, y + h);
    glEnd();
}

static void strokeRect2D(double x, double y, double w, double h, ColorRGBA c, double lw = 1.0) {
    setCol(c);
    glLineWidth((float)lw);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x, y);
    glVertex2d(x + w, y);
    glVertex2d(x + w, y + h);
    glVertex2d(x, y + h);
    glEnd();
}

static void line2D(double x1, double y1, double x2, double y2, ColorRGBA c, double lw = 1.0) {
    setCol(c);
    glLineWidth((float)lw);
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
}

static void circle2D(double cx, double cy, double r, ColorRGBA c, int seg = 24) {
    setCol(c);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(cx, cy);
    for (int i = 0; i <= seg; i++) {
        double a = i * 2.0 * PI / seg;
        glVertex2d(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    glEnd();
}

static double textWidth(const std::string& str, void* font) {
    double w = 0;
    for (char ch : str) w += glutBitmapWidth(font, ch);
    return w;
}

static void drawText(double x, double y, const std::string& str, ColorRGBA c, void* font = FONT_MD) {
    setCol(c);
    glRasterPos2d(x, y);
    for (char ch : str) glutBitmapCharacter(font, ch);
}

static std::string fmt(double v, int prec = 2) {
    if (std::fabs(v) < 1e-7) v = 0.0;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    if (s == "-0") s = "0";
    return s;
}

static void drawString3D(double x, double y, double z, const std::string& s, ColorRGBA c, void* font = FONT_MD) {
    setCol(c);
    glRasterPos3d(x, y, z);
    for (char ch : s) glutBitmapCharacter(font, ch);
}

static void drawButton(double x, double y, double w, double h, const std::string& text, bool active, std::function<void()> onClick, bool enabled = true) {
    bool hovered = enabled && g.mouseX >= x && g.mouseX <= x + w && g.mouseY >= y && g.mouseY <= y + h;
    ColorRGBA bg = active ? hexCol(0x1B5E4B) : hovered ? hexCol(0x2D3748) : hexCol(CARD_BG_COLOR);
    ColorRGBA bd = active ? hexCol(ACCENT_CYAN) : hexCol(BORDER_LINE);
    ColorRGBA tx = active ? hexCol(0xFFFFFF) : enabled ? hexCol(TEXT_MAIN) : hexCol(TEXT_MUTED);

    fillRect2D(x, y, w, h, bg);
    strokeRect2D(x, y, w, h, bd, active ? 1.8 : 1.0);
    double tw = textWidth(text, FONT_MD);
    drawText(x + (w - tw) * 0.5, y + h * 0.5 + 4.0, text, tx, FONT_MD);

    if (enabled) {
        g.buttons.push_back({ x, y, w, h, text, onClick, active });
    }
}

static void drawSlider(double x, double y, double w, double h, int id, const std::string& label, double minV, double maxV, double* pVal, const std::string& unit) {
    double val = *pVal;
    drawText(x, y + 14, label, hexCol(TEXT_MAIN), FONT_MD);

    double t0 = x + 130.0;
    double t1 = x + w - 75.0;
    double cy = y + h * 0.5;

    line2D(t0, cy, t1, cy, hexCol(0x3B4657), 3.0);

    double t = (val - minV) / (maxV - minV);
    t = std::max(0.0, std::min(1.0, t));
    double hx = t0 + t * (t1 - t0);

    circle2D(hx, cy, 6.0, hexCol(ACCENT_CYAN));
    circle2D(hx, cy, 3.0, hexCol(0xFFFFFF));

    std::string valStr = fmt(val) + " " + unit;
    drawText(t1 + 8, y + 14, valStr, hexCol(ACCENT_YELLOW), FONT_MD);

    g.sliders.push_back({ t0 - 8, y, (t1 - t0) + 16, h, label, minV, maxV, pVal, unit, id });
}

// =============================================================================
// 6. XUAT FILE ANH SCREENSHOT BMP (PHUC VU NOP BAI ZALO)
// =============================================================================

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t fileType{ 0x4D42 }; // "BM"
    uint32_t fileSize{ 0 };
    uint16_t reserved1{ 0 };
    uint16_t reserved2{ 0 };
    uint32_t offsetData{ 54 };

    uint32_t size{ 40 };
    int32_t  width{ 0 };
    int32_t  height{ 0 };
    uint16_t planes{ 1 };
    uint16_t bitCount{ 24 };
    uint32_t compression{ 0 };
    uint32_t sizeImage{ 0 };
    int32_t  xPixelsPerMeter{ 2835 };
    int32_t  yPixelsPerMeter{ 2835 };
    uint32_t colorsUsed{ 0 };
    uint32_t colorsImportant{ 0 };
};
#pragma pack(pop)

static bool saveScreenshotBMP(const std::string& filename, int x, int y, int width, int height) {
    int rowPadded = (width * 3 + 3) & (~3);
    uint32_t imageSize = rowPadded * height;

    BMPHeader header;
    header.width = width;
    header.height = height;
    header.sizeImage = imageSize;
    header.fileSize = 54 + imageSize;

    std::vector<uint8_t> pixels(imageSize, 0);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(x, y, width, height, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels.data());

    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "[LOI] Khong the mo file de ghi: " << filename << std::endl;
        return false;
    }
    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
    ofs.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
    ofs.close();

    std::cout << "[THANH CONG] Da luu hinh anh: " << filename << " (" << width << "x" << height << ")" << std::endl;
    return true;
}

static void captureCurrentModeImage() {
    std::string fname;
    if (g.activeMode == MODE_BACKFACE) {
        fname = "Hinh1_KhuKhuat_Backface.bmp";
    } else if (g.activeMode == MODE_DEEP_BUFFER) {
        fname = "Hinh2_KhuKhuat_DeepBuffer.bmp";
    } else if (g.activeMode == MODE_GOURAUD) {
        fname = "Hinh3_ToBong_Gouraud.bmp";
    } else {
        fname = "Hinh_TongHop_1611.bmp";
    }
    saveScreenshotBMP(fname, 0, 0, (int)viewportW(), (int)viewportH());
    g.toastMessage = "Da luu anh: " + fname + " (San sang nop vao Zalo!)";
    g.toastTimer = 180; // Hien thi trong ~3 giay
}

static void captureAllThreeImages() {
    LabMode oldMode = g.activeMode;

    // 1. Backface
    g.activeMode = MODE_BACKFACE;
    glutPostRedisplay();
    saveScreenshotBMP("Hinh1_KhuKhuat_Backface.bmp", 0, 0, (int)viewportW(), (int)viewportH());

    // 2. Deep-buffer
    g.activeMode = MODE_DEEP_BUFFER;
    glutPostRedisplay();
    saveScreenshotBMP("Hinh2_KhuKhuat_DeepBuffer.bmp", 0, 0, (int)viewportW(), (int)viewportH());

    // 3. Gouraud
    g.activeMode = MODE_GOURAUD;
    glutPostRedisplay();
    saveScreenshotBMP("Hinh3_ToBong_Gouraud.bmp", 0, 0, (int)viewportW(), (int)viewportH());

    g.activeMode = oldMode;
    g.toastMessage = "Da tu dong xuat thanh cong ca 3 hinh (1, 2, 3) vao thu muc!";
    g.toastTimer = 240;
}

// =============================================================================
// 7. VE CAC THANH PHAN KHONG GIAN 3D (AXES, GRID, LABELS, LIGHT)
// =============================================================================

static void drawCoordinateAxes3D() {
    double L = 3.8;
    glLineWidth(2.2f);

    // Ox - Do
    setCol(hexCol(0xFF4757));
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0); glVertex3d(L, 0, 0);
    glEnd();
    drawString3D(L + 0.15, 0, 0, "X", hexCol(0xFF4757), FONT_MD);

    // Oy - Xanh la
    setCol(hexCol(0x2ED573));
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0); glVertex3d(0, L, 0);
    glEnd();
    drawString3D(0, L + 0.15, 0, "Y", hexCol(0x2ED573), FONT_MD);

    // Oz - Xanh duong
    setCol(hexCol(0x3A86FF));
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0); glVertex3d(0, 0, L);
    glEnd();
    drawString3D(0, 0, L + 0.15, "Z", hexCol(0x3A86FF), FONT_MD);
}

static void drawGroundGrid3D() {
    double r = 4.0;
    double step = 0.5;
    double y = -1.0;

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (double i = -r; i <= r + 1e-4; i += step) {
        if (std::fabs(i) < 1e-4) {
            setCol(hexCol(0x4A5568, 0.7f));
        } else {
            setCol(hexCol(0x242D3D, 0.5f));
        }
        glVertex3d(i, y, -r); glVertex3d(i, y, r);
        glVertex3d(-r, y, i); glVertex3d(r, y, i);
    }
    glEnd();
}

static void drawVertexLabels3D() {
    if (!g.showVertexLabels) return;
    for (size_t i = 0; i < g.pyramid.vertices.size(); i++) {
        const auto& v = g.pyramid.vertices[i];
        const auto& name = g.pyramid.vertexNames[i];

        // Ve diem tron tai dinh
        glPushMatrix();
        glTranslated(v.x, v.y, v.z);
        if (i == 0) {
            setCol(hexCol(ACCENT_YELLOW)); // Dinh S mau vang
            glutSolidSphere(0.08, 16, 16);
        } else {
            setCol(hexCol(ACCENT_CYAN));   // Dinh day mau cyan
            glutSolidSphere(0.065, 16, 16);
        }
        glPopMatrix();

        // Nhan chu
        char lbl[64];
        snprintf(lbl, sizeof lbl, "%s (%.1f, %.1f, %.1f)", name.c_str(), v.x, v.y, v.z);
        drawString3D(v.x + 0.12, v.y + 0.12, v.z + 0.12, lbl, hexCol(0xFFFFFF), FONT_MD);
    }
}

// Ve vector phap tuyen ngoai N va vector huong nhin V cho tung mat
static void drawFaceNormals3D(const Vec3& eyePos) {
    if (!g.showNormals) return;

    for (const auto& f : g.pyramid.faces) {
        // Trong tam mat
        if (g.showCentroids) {
            glPushMatrix();
            glTranslated(f.centroid.x, f.centroid.y, f.centroid.z);
            setCol(f.isFrontFace ? hexCol(ACCENT_GREEN) : hexCol(ACCENT_RED));
            glutSolidSphere(0.05, 12, 12);
            glPopMatrix();
        }

        // Vector phap tuyen N (Do dai = 1.2)
        Vec3 nEnd = f.centroid + f.normal * 1.2;
        glLineWidth(2.5f);
        setCol(f.isFrontFace ? hexCol(ACCENT_GREEN) : hexCol(ACCENT_RED));
        glBegin(GL_LINES);
        glVertex3d(f.centroid.x, f.centroid.y, f.centroid.z);
        glVertex3d(nEnd.x, nEnd.y, nEnd.z);
        glEnd();

        // Ve chóp mui ten phap tuyen
        glPushMatrix();
        glTranslated(nEnd.x, nEnd.y, nEnd.z);
        glutSolidCone(0.05, 0.12, 10, 2);
        glPopMatrix();

        // Nhan N va gia tri dot
        char nLbl[64];
        snprintf(nLbl, sizeof nLbl, "N_%s (dot=%.2f: %s)", f.name.substr(8).c_str(), f.dotNV, f.isFrontFace ? "HIEN" : "KHUAT");
        drawString3D(nEnd.x + 0.08, nEnd.y + 0.08, nEnd.z + 0.08, nLbl,
                     f.isFrontFace ? hexCol(ACCENT_GREEN) : hexCol(ACCENT_RED), FONT_SM);
    }
}

// Ve vector phap tuyen tai moi dinh dung cho Gouraud
static void drawVertexNormals3D() {
    if (!g.showVertexNormals) return;

    const auto& vNorms = g.useSmoothSides ? g.pyramid.sideVertexNormals : g.pyramid.vertexNormals;

    glLineWidth(2.2f);
    for (size_t i = 0; i < g.pyramid.vertices.size(); i++) {
        const auto& v = g.pyramid.vertices[i];
        const auto& n = vNorms[i];
        Vec3 end = v + n * 0.9;

        setCol(hexCol(ACCENT_YELLOW));
        glBegin(GL_LINES);
        glVertex3d(v.x, v.y, v.z);
        glVertex3d(end.x, end.y, end.z);
        glEnd();

        glPushMatrix();
        glTranslated(end.x, end.y, end.z);
        glutSolidSphere(0.045, 12, 12);
        glPopMatrix();

        std::string lbl = "Nv_" + g.pyramid.vertexNames[i];
        drawString3D(end.x + 0.06, end.y + 0.06, end.z + 0.06, lbl, hexCol(ACCENT_YELLOW), FONT_SM);
    }
}

// Ve nguon sang 3D
static void drawLightSource3D() {
    if (!g.showLightSource) return;

    glPushMatrix();
    glTranslated(g.lightPos.x, g.lightPos.y, g.lightPos.z);
    
    // Nguon sang phat quang mau vang ruc ro
    setCol(hexCol(0xFFD200));
    glutSolidSphere(0.18, 16, 16);

    // Tia sang huong ve tam hinh chop
    glPopMatrix();

    glLineWidth(1.2f);
    setCol(hexCol(0xFFD200, 0.4f));
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0xAAAA);
    glBegin(GL_LINES);
    glVertex3d(g.lightPos.x, g.lightPos.y, g.lightPos.z);
    glVertex3d(0.0, 0.5, 0.0);
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    drawString3D(g.lightPos.x + 0.22, g.lightPos.y + 0.22, g.lightPos.z, "Nguon sang Light0", hexCol(0xFFD200), FONT_MD);
}

// =============================================================================
// 8. RENDER HINH CHOP SABC THEO TUNG CHE DO YEU CAU
// =============================================================================

// --- 8.1. CHE DO 1: KHU KHUAT BACKFACE (BACKFACE CULLING) ---
static void renderPyramidBackface(const Vec3& eyePos) {
    glDisable(GL_LIGHTING);

    // Tat Depth Buffer de chung minh rieng thuat toan Backface da du de khu khuat vat the loi!
    glDisable(GL_DEPTH_TEST);

    if (g.enableBackfaceCulling) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }

    // Ve cac mat tam giac voi mau rieng biet de de quan sat mat truoc/mat sau
    for (size_t i = 0; i < g.pyramid.faces.size(); i++) {
        const auto& f = g.pyramid.faces[i];
        
        // Neu khong bat Backface culling ma muon the hien ro mat sau:
        ColorRGBA faceColor = hexCol(f.colorHex, 0.88f);
        if (!g.enableBackfaceCulling && !f.isFrontFace) {
            // Khi tat culling, to mau xam/mo cho mat sau de de phan biet
            faceColor = hexCol(0x555555, 0.70f);
        }

        setCol(faceColor);
        glBegin(GL_TRIANGLES);
        for (int idx : f.vertIdx) {
            const auto& v = g.pyramid.vertices[idx];
            glVertex3d(v.x, v.y, v.z);
        }
        glEnd();
    }

    // Ve khung day (Wireframe Edges)
    glLineWidth(2.0f);
    setCol(hexCol(0x000000, 0.85f));
    for (const auto& f : g.pyramid.faces) {
        if (g.enableBackfaceCulling && !f.isFrontFace) continue; // Khong ve vien cua mat bi cull
        glBegin(GL_LINE_LOOP);
        for (int idx : f.vertIdx) {
            const auto& v = g.pyramid.vertices[idx];
            glVertex3d(v.x, v.y, v.z);
        }
        glEnd();
    }

    glDisable(GL_CULL_FACE);
    drawFaceNormals3D(eyePos);
}

// --- 8.2. CHE DO 2: KHU KHUAT DEEP-BUFFER (DEPTH-BUFFER / Z-BUFFER) ---
static void renderPyramidDeepBuffer(const Vec3& eyePos) {
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE); // Tat Backface Culling de Deep-buffer dam nhiem toan bo viec khu khuat

    if (g.enableDepthBuffer) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    // Thu tu ve cac mat: neu simulateReverseDraw bat, ta ve mat truoc roi den mat sau
    // Khi do neu khong co Depth Buffer, mat sau se de len mat truoc gay loi nghiem trong!
    std::vector<size_t> drawOrder = { 0, 1, 2, 3 };
    if (g.simulateReverseDraw) {
        // Sap xep mat gan camera ve TRUOC, mat xa camera ve SAU
        // (Neu khong co Z-buffer, mat xa se ve de len mat gan => Hien tuong sai thu tu)
        std::sort(drawOrder.begin(), drawOrder.end(), [&](size_t a, size_t b) {
            double distA = length(eyePos - g.pyramid.faces[a].centroid);
            double distB = length(eyePos - g.pyramid.faces[b].centroid);
            return distA < distB; // Gan ve truoc, xa ve sau!
        });
    }

    for (size_t idx : drawOrder) {
        const auto& f = g.pyramid.faces[idx];
        setCol(hexCol(f.colorHex, 0.92f));

        glBegin(GL_TRIANGLES);
        for (int vIdx : f.vertIdx) {
            const auto& v = g.pyramid.vertices[vIdx];
            glVertex3d(v.x, v.y, v.z);
        }
        glEnd();

        // Khung vien
        glLineWidth(2.0f);
        setCol(hexCol(0x000000, 0.9f));
        glBegin(GL_LINE_LOOP);
        for (int vIdx : f.vertIdx) {
            const auto& v = g.pyramid.vertices[vIdx];
            glVertex3d(v.x, v.y, v.z);
        }
        glEnd();
    }
}

// --- 8.3. CHE DO 2 PHU: VE OVERLAY DEPTH-MAP (Z-BUFFER VISUALIZATION) ---
static void renderDepthMapPictureInPicture() {
    if (!g.showDepthMapOverlay || g.activeMode != MODE_DEEP_BUFFER) return;

    double boxW = 200.0, boxH = 150.0;
    double boxX = 20.0, boxY = HEADER_H + 20.0;

    // Khung vien hop Z-Buffer
    fillRect2D(boxX, boxY, boxW, boxH, hexCol(0x0B0F14, 0.90f));
    strokeRect2D(boxX, boxY, boxW, boxH, hexCol(ACCENT_CYAN), 1.5);

    drawText(boxX + 10, boxY + 18, "Z-BUFFER / DEPTH MAP (GRAYSCALE):", hexCol(ACCENT_CYAN), FONT_SM);
    drawText(boxX + 10, boxY + 32, "Trang = Gan camera | Den = Xa camera", hexCol(TEXT_MUTED), FONT_SM);

    // Tinh khoang cach tu Camera den tung dinh
    Vec3 eye = computeEyePosition(g.target, g.thetaDeg, g.phiDeg, g.eyeRadius);
    double dMin = 1e9, dMax = -1e9;
    std::vector<double> dists;
    for (const auto& v : g.pyramid.vertices) {
        double d = length(eye - v);
        dists.push_back(d);
        if (d < dMin) dMin = d;
        if (d > dMax) dMax = d;
    }
    if (dMax - dMin < 1e-4) dMax = dMin + 1.0;

    double rowY = boxY + 52.0;
    for (size_t i = 0; i < g.pyramid.vertices.size(); i++) {
        double normZ = (dists[i] - dMin) / (dMax - dMin); // 0 (gan nhat) .. 1 (xa nhat)
        float gray = (float)(1.0 - normZ * 0.75); // Gan = Sang, Xa = Toi

        // O vuong the hien gia tri xam
        fillRect2D(boxX + 12, rowY - 10, 16, 12, { gray, gray, gray, 1.0f });
        strokeRect2D(boxX + 12, rowY - 10, 16, 12, hexCol(BORDER_LINE), 1.0);

        char zBufStr[64];
        snprintf(zBufStr, sizeof zBufStr, "Dinh %s: d=%.2f m  (Z_norm=%.2f)", 
                 g.pyramid.vertexNames[i].c_str(), dists[i], normZ);
        drawText(boxX + 36, rowY, zBufStr, hexCol(TEXT_MAIN), FONT_SM);
        rowY += 22.0;
    }
}

// Ve 1 tam giac duoc chia nho thanh luoi con de noi suy Gouraud Shading cuc ky min mang
static void drawSubdividedTriangleGouraud(const Vec3& v0, const Vec3& v1, const Vec3& v2,
                                         const Vec3& n0, const Vec3& n1, const Vec3& n2,
                                         int subdivisions) {
    if (subdivisions <= 1) {
        glBegin(GL_TRIANGLES);
        glNormal3d(n0.x, n0.y, n0.z); glVertex3d(v0.x, v0.y, v0.z);
        glNormal3d(n1.x, n1.y, n1.z); glVertex3d(v1.x, v1.y, v1.z);
        glNormal3d(n2.x, n2.y, n2.z); glVertex3d(v2.x, v2.y, v2.z);
        glEnd();
        return;
    }

    int N = subdivisions;
    for (int i = 0; i < N; i++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= N - i; j++) {
            // Diem tren hang i
            double a1 = (double)j / N;
            double b1 = (double)i / N;
            double c1 = 1.0 - a1 - b1;
            Vec3 pos1 = v0 * c1 + v1 * a1 + v2 * b1;
            Vec3 nrm1 = normalize(n0 * c1 + n1 * a1 + n2 * b1);

            glNormal3d(nrm1.x, nrm1.y, nrm1.z);
            glVertex3d(pos1.x, pos1.y, pos1.z);

            if (j < N - i) {
                // Diem tren hang i + 1
                double a2 = (double)j / N;
                double b2 = (double)(i + 1) / N;
                double c2 = 1.0 - a2 - b2;
                Vec3 pos2 = v0 * c2 + v1 * a2 + v2 * b2;
                Vec3 nrm2 = normalize(n0 * c2 + n1 * a2 + n2 * b2);

                glNormal3d(nrm2.x, nrm2.y, nrm2.z);
                glVertex3d(pos2.x, pos2.y, pos2.z);
            }
        }
        glEnd();
    }
}

// --- 8.4. CHE DO 3: TO BONG GOURAUD (GOURAUD SHADING) ---
static void renderPyramidGouraud(const Vec3& eyePos) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Chieu sang nguon sang
    GLfloat lPos[]  = { (float)g.lightPos.x, (float)g.lightPos.y, (float)g.lightPos.z, 1.0f };
    GLfloat lAmb[]  = { (float)g.lightAmb, (float)g.lightAmb, (float)g.lightAmb, 1.0f };
    GLfloat lDiff[] = { (float)g.lightDiff, (float)g.lightDiff, (float)g.lightDiff, 1.0f };
    GLfloat lSpec[] = { (float)g.lightSpec, (float)g.lightSpec, (float)g.lightSpec, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lSpec);

    // Vat lieu chóp
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat matSpec[] = { (float)g.lightSpec, (float)g.lightSpec, (float)g.lightSpec, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float)g.shininess);

    if (g.shadingMode == SHADE_GOURAUD) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    const auto& vNorms = g.useSmoothSides ? g.pyramid.sideVertexNormals : g.pyramid.vertexNormals;
    int subdiv = (g.shadingMode == SHADE_GOURAUD) ? (int)std::round(g.gouraudSubdivDouble) : 1;

    // 1. Ve 3 mat ben (SAB, SBC, SCA)
    for (size_t fIdx = 0; fIdx < 3; fIdx++) {
        const auto& f = g.pyramid.faces[fIdx];
        ColorRGBA col = g.uniformColorMode ? hexCol(0x00E5BE) : hexCol(f.colorHex);
        setCol(col);

        int i0 = f.vertIdx[0];
        int i1 = f.vertIdx[1];
        int i2 = f.vertIdx[2];
        const auto& v0 = g.pyramid.vertices[i0];
        const auto& v1 = g.pyramid.vertices[i1];
        const auto& v2 = g.pyramid.vertices[i2];

        if (g.shadingMode == SHADE_FLAT) {
            glNormal3d(f.normal.x, f.normal.y, f.normal.z);
            glBegin(GL_TRIANGLES);
            glVertex3d(v0.x, v0.y, v0.z);
            glVertex3d(v1.x, v1.y, v1.z);
            glVertex3d(v2.x, v2.y, v2.z);
            glEnd();
        } else {
            const auto& n0 = vNorms[i0];
            const auto& n1 = vNorms[i1];
            const auto& n2 = vNorms[i2];
            drawSubdividedTriangleGouraud(v0, v1, v2, n0, n1, n2, subdiv);
        }
    }

    // 2. Ve mat day (Face 3: ABC)
    {
        const auto& f = g.pyramid.faces[3];
        ColorRGBA col = g.uniformColorMode ? hexCol(0x00A896) : hexCol(f.colorHex);
        setCol(col);

        int i0 = f.vertIdx[0];
        int i1 = f.vertIdx[1];
        int i2 = f.vertIdx[2];
        const auto& v0 = g.pyramid.vertices[i0];
        const auto& v1 = g.pyramid.vertices[i1];
        const auto& v2 = g.pyramid.vertices[i2];

        Vec3 n0 = g.useSmoothSides ? f.normal : vNorms[i0];
        Vec3 n1 = g.useSmoothSides ? f.normal : vNorms[i1];
        Vec3 n2 = g.useSmoothSides ? f.normal : vNorms[i2];

        if (g.shadingMode == SHADE_FLAT) {
            glNormal3d(f.normal.x, f.normal.y, f.normal.z);
            glBegin(GL_TRIANGLES);
            glVertex3d(v0.x, v0.y, v0.z);
            glVertex3d(v1.x, v1.y, v1.z);
            glVertex3d(v2.x, v2.y, v2.z);
            glEnd();
        } else {
            drawSubdividedTriangleGouraud(v0, v1, v2, n0, n1, n2, g.useSmoothSides ? 1 : subdiv);
        }
    }

    glDisable(GL_LIGHTING);

    // Khung vien Wireframe (neu duoc bat)
    if (g.showGouraudEdges) {
        glLineWidth(1.8f);
        setCol(hexCol(0x000000, 0.7f));
        for (const auto& f : g.pyramid.faces) {
            glBegin(GL_LINE_LOOP);
            for (int idx : f.vertIdx) {
                const auto& v = g.pyramid.vertices[idx];
                glVertex3d(v.x, v.y, v.z);
            }
            glEnd();
        }
    }

    drawLightSource3D();
    drawVertexNormals3D();
}

// --- 8.5. CHE DO 4: TONG HOP & SO SANH (OVERVIEW / COMPARE) ---
static void renderPyramidCompare(const Vec3& eyePos) {
    // Mac dinh o goc nhin tong the su dung Gouraud shading va Depth test
    renderPyramidGouraud(eyePos);
}

// =============================================================================
// 9. GIAO DIEN NGUOI DUNG (SIDEBAR & HEADER UI RENDERING)
// =============================================================================

static void renderHeader() {
    fillRect2D(0, 0, g.windowW, HEADER_H, hexCol(0x0E131A));
    line2D(0, HEADER_H, g.windowW, HEADER_H, hexCol(BORDER_LINE), 1.5);

    drawText(20, 24, "VKU GRAPHICS - LAB TH 16/11: HINH CHOP SABC (3D)", hexCol(ACCENT_CYAN), FONT_LG);
    drawText(20, 46, "1. Khu khuat Backface & Deep-buffer | 2. To bong Gouraud | Xuat 3 hinh nop Zalo", hexCol(TEXT_MUTED), FONT_SM);

    double btnW = 160.0, btnH = 36.0, btnY = 14.0;
    double tX = g.windowW - sidePanelW() - 530.0;

    // 3 Tab chinh dap ung chuan xac yeu cau de bai
    drawButton(tX, btnY, btnW, btnH, "1. KHU KHUAT BACKFACE", g.activeMode == MODE_BACKFACE, [] {
        g.activeMode = MODE_BACKFACE;
    });
    drawButton(tX + btnW + 10.0, btnY, btnW, btnH, "2. DEEP-BUFFER (Z-BUF)", g.activeMode == MODE_DEEP_BUFFER, [] {
        g.activeMode = MODE_DEEP_BUFFER;
    });
    drawButton(tX + (btnW + 10.0) * 2, btnY, btnW, btnH, "3. TO BONG GOURAUD", g.activeMode == MODE_GOURAUD, [] {
        g.activeMode = MODE_GOURAUD;
    });
}

static void renderSidebarUI() {
    double pX = viewportW();
    double pY = HEADER_H;
    double pW = sidePanelW();
    double pH = viewportH();

    fillRect2D(pX, pY, pW, pH, hexCol(PANEL_BG_COLOR));
    line2D(pX, pY, pX, pY + pH, hexCol(BORDER_LINE), 1.5);

    double curY = pY + 12.0;
    double cardW = pW - 28.0;
    double padX = pX + 14.0;

    // --- CARD 1: XUAT ANH NOP BAI ZALO (QUICK EXPORT FOR SUBMISSION) ---
    fillRect2D(padX, curY, cardW, 96.0, hexCol(CARD_BG_COLOR));
    strokeRect2D(padX, curY, cardW, 96.0, hexCol(ACCENT_ORANGE), 1.2);

    drawText(padX + 12, curY + 18, "NOP BAI GROUP ZALO (CHECK COT 16/11):", hexCol(ACCENT_ORANGE), FONT_MD);
    
    double bHalfW = (cardW - 32.0) / 2.0;
    drawButton(padX + 10, curY + 28, bHalfW, 30.0, "📸 Chup anh hien tai", false, [] {
        captureCurrentModeImage();
    });
    drawButton(padX + 22 + bHalfW, curY + 28, bHalfW, 30.0, "⭐ CHUP CA 3 HINH", false, [] {
        captureAllThreeImages();
    });

    drawText(padX + 12, curY + 80, "Luu tu dong 3 file .BMP chat luong cao vao thu muc!", hexCol(TEXT_MUTED), FONT_SM);
    curY += 108.0;

    // --- CARD 2: CHI TIET THEO TUNG CHE DO YEU CAU ---
    if (g.activeMode == MODE_BACKFACE) {
        fillRect2D(padX, curY, cardW, 250.0, hexCol(CARD_BG_COLOR));
        strokeRect2D(padX, curY, cardW, 250.0, hexCol(BORDER_LINE), 1.0);

        drawText(padX + 12, curY + 18, "YEU CAU 1A: THUAT TOAN KHU KHUAT BACKFACE", hexCol(ACCENT_CYAN), FONT_MD);
        drawText(padX + 12, curY + 34, "Nguyen ly: Kiem tra tich vo huong dot(N, V)", hexCol(TEXT_MUTED), FONT_SM);
        drawText(padX + 12, curY + 48, "dot(N, V) > 0: Mat truoc (Hien) | <= 0: Mat sau (Khuat)", hexCol(TEXT_MUTED), FONT_SM);

        drawButton(padX + 10, curY + 58, cardW - 20, 28.0, 
                   g.enableBackfaceCulling ? "Backface Culling: [DANG BAT - KHU KHUAT]" : "Backface Culling: [DANG TAT - XUYEN THAU]", 
                   g.enableBackfaceCulling, [] {
            g.enableBackfaceCulling = !g.enableBackfaceCulling;
        });

        drawButton(padX + 10, curY + 92, (cardW - 26) / 2.0, 26.0, 
                   g.showNormals ? "Phap tuyen N: [BAT]" : "Phap tuyen N: [TAT]", g.showNormals, [] {
            g.showNormals = !g.showNormals;
        });
        drawButton(padX + 16 + (cardW - 26) / 2.0, curY + 92, (cardW - 26) / 2.0, 26.0, 
                   g.showCentroids ? "Trong tam: [BAT]" : "Trong tam: [TAT]", g.showCentroids, [] {
            g.showCentroids = !g.showCentroids;
        });

        // Bang trang thai 4 mat
        drawText(padX + 12, curY + 134, "BANG TRANG THAI CAC MAT CUA HINH CHOP SABC:", hexCol(ACCENT_YELLOW), FONT_SM);
        double tableY = curY + 150.0;
        for (const auto& f : g.pyramid.faces) {
            ColorRGBA cStatus = f.isFrontFace ? hexCol(ACCENT_GREEN) : hexCol(ACCENT_RED);
            char rowBuf[96];
            snprintf(rowBuf, sizeof rowBuf, "- %-12s: dot= %+.2f  =>  [%s]", 
                     f.name.c_str(), f.dotNV, f.isFrontFace ? "MAT TRUOC (HIEN)" : "MAT SAU (CULLED)");
            drawText(padX + 12, tableY, rowBuf, cStatus, FONT_SM);
            tableY += 20.0;
        }

        curY += 262.0;

    } else if (g.activeMode == MODE_DEEP_BUFFER) {
        fillRect2D(padX, curY, cardW, 250.0, hexCol(CARD_BG_COLOR));
        strokeRect2D(padX, curY, cardW, 250.0, hexCol(BORDER_LINE), 1.0);

        drawText(padX + 12, curY + 18, "YEU CAU 1B: THUAT TOAN DEEP-BUFFER (Z-BUFFER)", hexCol(ACCENT_CYAN), FONT_MD);
        drawText(padX + 12, curY + 34, "Nguyen ly: Luu tru gia tri do sau Z cho tung pixel", hexCol(TEXT_MUTED), FONT_SM);
        drawText(padX + 12, curY + 48, "Neu Z_pixel < Z_buffer: Cap nhat mau va do sau Z", hexCol(TEXT_MUTED), FONT_SM);

        drawButton(padX + 10, curY + 58, cardW - 20, 28.0, 
                   g.enableDepthBuffer ? "Depth Buffer (Z-Buffer): [DANG BAT]" : "Depth Buffer (Z-Buffer): [DANG TAT]", 
                   g.enableDepthBuffer, [] {
            g.enableDepthBuffer = !g.enableDepthBuffer;
        });

        drawButton(padX + 10, curY + 92, (cardW - 26) / 2.0, 26.0, 
                   g.showDepthMapOverlay ? "Depth Map Mini: [BAT]" : "Depth Map Mini: [TAT]", g.showDepthMapOverlay, [] {
            g.showDepthMapOverlay = !g.showDepthMapOverlay;
        });
        drawButton(padX + 16 + (cardW - 26) / 2.0, curY + 92, (cardW - 26) / 2.0, 26.0, 
                   g.simulateReverseDraw ? "Dao thu tu: [BAT]" : "Dao thu tu: [TAT]", g.simulateReverseDraw, [] {
            g.simulateReverseDraw = !g.simulateReverseDraw;
        });

        drawText(padX + 12, curY + 134, "THI NGHIEM SO SANH HIEN TUONG:", hexCol(ACCENT_YELLOW), FONT_SM);
        drawText(padX + 12, curY + 152, "1. Khi BAT Depth Buffer: Khu khuat hoan hao moi thu tu ve", hexCol(TEXT_MAIN), FONT_SM);
        drawText(padX + 12, curY + 170, "2. Khi TAT Depth Buffer + BAT Dao thu tu: Cac mat o xa", hexCol(ACCENT_RED), FONT_SM);
        drawText(padX + 12, curY + 188, "   se bi ve de len mat o gan (Sai quy luat hien thi 3D)", hexCol(ACCENT_RED), FONT_SM);
        drawText(padX + 12, curY + 210, "Khung hinh thu nho ben trai dang chieu Depth Map!", hexCol(ACCENT_CYAN), FONT_SM);

        curY += 262.0;

    } else if (g.activeMode == MODE_GOURAUD) {
        fillRect2D(padX, curY, cardW, 310.0, hexCol(CARD_BG_COLOR));
        strokeRect2D(padX, curY, cardW, 310.0, hexCol(BORDER_LINE), 1.0);

        drawText(padX + 12, curY + 18, "YEU CAU 2: PHUONG PHAP TO BONG GOURAUD", hexCol(ACCENT_CYAN), FONT_MD);
        drawText(padX + 12, curY + 32, "Nguyen ly: Tinh cuong do sang tai cac DINH S, A, B, C", hexCol(TEXT_MUTED), FONT_SM);
        drawText(padX + 12, curY + 44, "sau do noi suy tuyen tinh (Bilinear) tren be mat da giac.", hexCol(TEXT_MUTED), FONT_SM);

        double halfW = (cardW - 26.0) / 2.0;

        // Hang 1: Che do To bong
        drawButton(padX + 10, curY + 54, halfW, 26.0, 
                   "Gouraud (Smooth)", g.shadingMode == SHADE_GOURAUD, [] {
            g.shadingMode = SHADE_GOURAUD;
        });
        drawButton(padX + 16 + halfW, curY + 54, halfW, 26.0, 
                   "Flat Shading", g.shadingMode == SHADE_FLAT, [] {
            g.shadingMode = SHADE_FLAT;
        });

        // Hang 2: Tuy chon Toan hoc Phap tuyen & Mau sac
        drawButton(padX + 10, curY + 84, halfW, 26.0, 
                   g.useSmoothSides ? "PT: Than chuan" : "PT: Gop day", g.useSmoothSides, [] {
            g.useSmoothSides = !g.useSmoothSides;
        });
        drawButton(padX + 16 + halfW, curY + 84, halfW, 26.0, 
                   g.uniformColorMode ? "Mau: Dong nhat" : "Mau: Da sac", g.uniformColorMode, [] {
            g.uniformColorMode = !g.uniformColorMode;
        });

        // Hang 3: Bat/Tat bo tro
        drawButton(padX + 10, curY + 114, halfW, 26.0, 
                   g.showVertexNormals ? "Phap tuyen Nv: [BAT]" : "Phap tuyen Nv: [TAT]", g.showVertexNormals, [] {
            g.showVertexNormals = !g.showVertexNormals;
        });
        drawButton(padX + 16 + halfW, curY + 114, halfW, 26.0, 
                   g.showLightSource ? "Nguon sang: [BAT]" : "Nguon sang: [TAT]", g.showLightSource, [] {
            g.showLightSource = !g.showLightSource;
        });

        // Hang 4: Cac thanh truot tham so
        drawSlider(padX + 10, curY + 144, cardW - 20, 20.0, 9,  "Do min luoi Subdiv:", 1.0, 8.0, &g.gouraudSubdivDouble, "x");
        drawSlider(padX + 10, curY + 168, cardW - 20, 20.0, 10, "Do bong (Shininess):", 5.0, 120.0, &g.shininess, "");
        drawSlider(padX + 10, curY + 192, cardW - 20, 20.0, 11, "Nguon sang X:", -8.0, 8.0, &g.lightPos.x, "");
        drawSlider(padX + 10, curY + 216, cardW - 20, 20.0, 12, "Nguon sang Y:", 1.0, 10.0, &g.lightPos.y, "");
        drawSlider(padX + 10, curY + 240, cardW - 20, 20.0, 13, "Nguon sang Z:", -8.0, 8.0, &g.lightPos.z, "");

        drawButton(padX + 10, curY + 268, cardW - 20, 24.0, 
                   g.showGouraudEdges ? "Vien Wireframe: [DANG BAT]" : "Vien Wireframe: [DANG TAT (Muot ma)]", g.showGouraudEdges, [] {
            g.showGouraudEdges = !g.showGouraudEdges;
        });

        drawText(padX + 12, curY + 302, "Loi khuyen: 'Than chuan' + 'Dong nhat' giup thay ro vet bong sang nhat!", hexCol(ACCENT_YELLOW), FONT_SM);

        curY += 320.0;
    }

    // --- CARD 3: THIET LAP HINH CHOP & HE QUAN SAT ---
    fillRect2D(padX, curY, cardW, 185.0, hexCol(CARD_BG_COLOR));
    strokeRect2D(padX, curY, cardW, 185.0, hexCol(BORDER_LINE), 1.0);

    drawText(padX + 12, curY + 18, "HE QUAN SAT & THONG SO HINH CHOP SABC:", hexCol(ACCENT_YELLOW), FONT_MD);

    drawSlider(padX + 10, curY + 26, cardW - 20, 20.0, 1, "Goc phuong vi Theta:", 0.0, 360.0, &g.thetaDeg, "deg");
    drawSlider(padX + 10, curY + 50, cardW - 20, 20.0, 2, "Goc ngung Phi:", -80.0, 80.0, &g.phiDeg, "deg");
    drawSlider(padX + 10, curY + 74, cardW - 20, 20.0, 3, "Khoang cach Camera:", 3.5, 20.0, &g.eyeRadius, "");
    drawSlider(padX + 10, curY + 98, cardW - 20, 20.0, 4, "Chieu cao dinh S:", 1.0, 4.0, &g.apexHeight, "");

    drawButton(padX + 10, curY + 126, (cardW - 26) / 2.0, 26.0, 
               g.autoOrbit ? "Tu dong xoay: [BAT]" : "Tu dong xoay: [TAT]", g.autoOrbit, [] {
        g.autoOrbit = !g.autoOrbit;
    });

    drawButton(padX + 16 + (cardW - 26) / 2.0, curY + 126, (cardW - 26) / 2.0, 26.0, 
               "Dat lai mac dinh (R)", false, [] {
        g.thetaDeg = 45.0;
        g.phiDeg = 28.0;
        g.eyeRadius = 8.5;
        g.apexHeight = 2.2;
        g.useSmoothSides = true;
        g.uniformColorMode = true;
        g.showGouraudEdges = false;
        g.gouraudSubdivDouble = 4.0;
        g.pyramid.initDefault();
    });

    drawText(padX + 12, curY + 168, "Keo chuot trai: Xoay | Keo chuot phai / Lan chuot: Zoom", hexCol(TEXT_MUTED), FONT_SM);
    curY += 195.0;

    // --- THONG BAO TOAST KHI CHUP ANH ---
    if (g.toastTimer > 0) {
        fillRect2D(padX, pH - 50.0, cardW, 38.0, hexCol(0x1B5E4B));
        strokeRect2D(padX, pH - 50.0, cardW, 38.0, hexCol(ACCENT_GREEN), 1.5);
        drawText(padX + 12, pH - 26.0, g.toastMessage, hexCol(0xFFFFFF), FONT_MD);
    }
}

// =============================================================================
// 10. DISPLAY CALLBACK & EVENT DISPATCHER
// =============================================================================

static void display() {
    g.buttons.clear();
    g.sliders.clear();

    // Cap nhat lai do cao dinh S neu slider thay doi
    g.pyramid.vertices[0].y = g.apexHeight;
    g.pyramid.updateGeometry();

    Vec3 eye = computeEyePosition(g.target, g.thetaDeg, g.phiDeg, g.eyeRadius);
    g.pyramid.evaluateBackface(eye);

    // 1. VE KHONG GIAN 3D TRONG VIEWPORT BEN TRAI
    glViewport(0, 0, (GLsizei)viewportW(), (GLsizei)viewportH());
    glClearColor(0.05f, 0.07f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = viewportW() / viewportH();
    gluPerspective(45.0, aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eye.x, eye.y, eye.z, g.target.x, g.target.y, g.target.z, 0.0, 1.0, 0.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);

    drawCoordinateAxes3D();
    drawGroundGrid3D();
    drawVertexLabels3D();

    // Ve hinh chop theo dung de bai cua che do dang chon
    if (g.activeMode == MODE_BACKFACE) {
        renderPyramidBackface(eye);
    } else if (g.activeMode == MODE_DEEP_BUFFER) {
        renderPyramidDeepBuffer(eye);
    } else if (g.activeMode == MODE_GOURAUD) {
        renderPyramidGouraud(eye);
    } else {
        renderPyramidCompare(eye);
    }

    // 2. VE GIAO DIEN 2D OVERLAY (HUD, HEADER, SIDEBAR)
    glViewport(0, 0, g.windowW, g.windowH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, g.windowW, g.windowH, 0); // Goc tren trai (0, 0)

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    // Render khung Depth Map mini neu o che do Deep-buffer
    renderDepthMapPictureInPicture();

    renderHeader();
    renderSidebarUI();

    glutSwapBuffers();
}

static void reshape(int w, int h) {
    g.windowW = std::max(800, w);
    g.windowH = std::max(600, h);
    glutPostRedisplay();
}

static void timer(int v) {
    if (g.autoOrbit) {
        g.thetaDeg += g.orbitSpeed;
        if (g.thetaDeg >= 360.0) g.thetaDeg -= 360.0;
        glutPostRedisplay();
    }
    if (g.toastTimer > 0) {
        g.toastTimer--;
        if (g.toastTimer == 0) glutPostRedisplay();
    }
    glutTimerFunc(16, timer, 0);
}

// =============================================================================
// 11. XU LY SU KIEN CHUOT & BAN PHIM (INTERACTIONS)
// =============================================================================

static void mouse(int button, int state, int x, int y) {
    g.mouseX = x;
    g.mouseY = y;

    if (state == GLUT_DOWN) {
        // Kiem tra click nut bam
        for (const auto& btn : g.buttons) {
            if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
                if (btn.onClick) btn.onClick();
                glutPostRedisplay();
                return;
            }
        }

        // Kiem tra click slider
        for (const auto& sl : g.sliders) {
            if (x >= sl.x && x <= sl.x + sl.w && y >= sl.y - 4 && y <= sl.y + sl.h + 4) {
                g.activeSliderId = sl.id;
                double t = (x - sl.x) / sl.w;
                t = std::max(0.0, std::min(1.0, t));
                *(sl.pValue) = sl.minVal + t * (sl.maxVal - sl.minVal);
                glutPostRedisplay();
                return;
            }
        }

        // Keo chuot quan sat 3D
        if (x < viewportW() && y > HEADER_H) {
            if (button == GLUT_LEFT_BUTTON) {
                g.isLeftDragging = true;
                g.lastMouseX = x;
                g.lastMouseY = y;
            } else if (button == GLUT_RIGHT_BUTTON) {
                g.isRightDragging = true;
                g.lastMouseX = x;
                g.lastMouseY = y;
            }
        }

        // Cuon chuot Zoom in / out
        if (button == 3) {
            g.eyeRadius = std::max(3.0, g.eyeRadius - 0.5);
            glutPostRedisplay();
        } else if (button == 4) {
            g.eyeRadius = std::min(25.0, g.eyeRadius + 0.5);
            glutPostRedisplay();
        }
    } else if (state == GLUT_UP) {
        g.isLeftDragging = false;
        g.isRightDragging = false;
        g.activeSliderId = -1;
    }
}

static void motion(int x, int y) {
    g.mouseX = x;
    g.mouseY = y;

    // Keo Slider
    if (g.activeSliderId != -1) {
        for (const auto& sl : g.sliders) {
            if (sl.id == g.activeSliderId) {
                double t = (x - sl.x) / sl.w;
                t = std::max(0.0, std::min(1.0, t));
                *(sl.pValue) = sl.minVal + t * (sl.maxVal - sl.minVal);
                glutPostRedisplay();
                return;
            }
        }
    }

    // Xoay Camera Orbit (Chuot trai)
    if (g.isLeftDragging) {
        double dx = x - g.lastMouseX;
        double dy = y - g.lastMouseY;
        g.thetaDeg += dx * 0.45;
        g.phiDeg   += dy * 0.45;
        if (g.phiDeg > 85.0)  g.phiDeg = 85.0;
        if (g.phiDeg < -85.0) g.phiDeg = -85.0;
        while (g.thetaDeg >= 360.0) g.thetaDeg -= 360.0;
        while (g.thetaDeg < 0.0)    g.thetaDeg += 360.0;
        g.lastMouseX = x;
        g.lastMouseY = y;
        glutPostRedisplay();
    }

    // Zoom Camera (Chuot phai)
    if (g.isRightDragging) {
        double dy = y - g.lastMouseY;
        g.eyeRadius += dy * 0.05;
        if (g.eyeRadius < 3.0)  g.eyeRadius = 3.0;
        if (g.eyeRadius > 25.0) g.eyeRadius = 25.0;
        g.lastMouseY = y;
        glutPostRedisplay();
    }
}

static void passiveMotion(int x, int y) {
    g.mouseX = x;
    g.mouseY = y;
    glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC
            exit(0);
            break;
        case '1':
            g.activeMode = MODE_BACKFACE;
            break;
        case '2':
            g.activeMode = MODE_DEEP_BUFFER;
            break;
        case '3':
            g.activeMode = MODE_GOURAUD;
            break;
        case ' ':
            g.autoOrbit = !g.autoOrbit;
            break;
        case 'b': case 'B':
            g.enableBackfaceCulling = !g.enableBackfaceCulling;
            break;
        case 'd': case 'D':
            g.enableDepthBuffer = !g.enableDepthBuffer;
            break;
        case 'g': case 'G':
            g.shadingMode = (g.shadingMode == SHADE_GOURAUD) ? SHADE_FLAT : SHADE_GOURAUD;
            break;
        case 'n': case 'N':
            g.showNormals = !g.showNormals;
            g.showVertexNormals = !g.showVertexNormals;
            break;
        case 'p': case 'P':
            captureCurrentModeImage();
            break;
        case 'r': case 'R':
            g.thetaDeg = 45.0;
            g.phiDeg = 28.0;
            g.eyeRadius = 8.5;
            g.apexHeight = 2.2;
            g.pyramid.initDefault();
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

static void special(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            g.thetaDeg -= 5.0;
            break;
        case GLUT_KEY_RIGHT:
            g.thetaDeg += 5.0;
            break;
        case GLUT_KEY_UP:
            g.phiDeg = std::min(85.0, g.phiDeg + 5.0);
            break;
        case GLUT_KEY_DOWN:
            g.phiDeg = std::max(-85.0, g.phiDeg - 5.0);
            break;
        case GLUT_KEY_F12:
            captureCurrentModeImage();
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

// =============================================================================
// 12. KIEM THU TU DONG TOAN HOC (SELF-TESTS SUITE --test)
// =============================================================================

static int runSelfTests() {
    int total = 0, fail = 0;
    auto chk = [&](bool condition, const char* msg) {
        total++;
        if (condition) {
            std::cout << "  [PASS] " << msg << std::endl;
        } else {
            fail++;
            std::cout << "  [FAIL] " << msg << std::endl;
        }
    };

    std::cout << "========================================================\n";
    std::cout << "  CHAY BO KIEM THU TU DONG (VKU GRAPHICS LAB 16/11)\n";
    std::cout << "========================================================\n\n";

    // 1. Khoi tao hinh chop SABC
    PyramidSABC pyr;
    pyr.initDefault();
    chk(pyr.vertices.size() == 4, "Khoi tao du 4 dinh S, A, B, C");
    chk(pyr.faces.size() == 4, "Khoi tao du 4 mat (SAB, SBC, SCA, ABC)");
    chk(pyr.vertices[0].y == 2.2, "Toa do dinh S tren cao (y = 2.2)");

    // 2. Kiem tra tinh toan phap tuyen ngoai
    bool allNormalsValid = true;
    for (const auto& f : pyr.faces) {
        double l = length(f.normal);
        if (std::fabs(l - 1.0) > 1e-4) allNormalsValid = false;
    }
    chk(allNormalsValid, "Cac vector phap tuyen mat deu duoc chuan hoa (do dai = 1)");

    // Mat day ABC phai huong xuong duoi (Ny < 0)
    chk(pyr.faces[3].normal.y < -0.9, "Phap tuyen mat day ABC huong xuong duoi (Ny ~ -1)");

    // 3. Kiem tra thuat toan Backface Culling
    Vec3 eyeFront = { 0.0, 0.5, 8.0 }; // Camera nhin tu phia truoc (+Z)
    pyr.evaluateBackface(eyeFront);
    // Mat SAB nam o mat truoc (+Z) phai duoc nhin thay (dot > 0)
    chk(pyr.faces[0].isFrontFace == true, "Mat SAB quay ve phia camera => La Front Face");
    chk(pyr.faces[0].dotNV > 0.0, "dot(N_SAB, V) > 0");

    // 4. Kiem tra tinh toan phap tuyen dinh cho Gouraud Shading
    chk(pyr.vertexNormals.size() == 4, "Tinh du 4 vector phap tuyen tai 4 dinh S, A, B, C");
    chk(pyr.vertexNormals[0].y > 0.8, "Phap tuyen tai dinh S huong len tren dinh");

    // 5. Kiem tra ghi file BMP
    bool bmpOk = saveScreenshotBMP("test_export.bmp", 0, 0, 10, 10);
    chk(bmpOk, "Ham saveScreenshotBMP ghi file thanh cong");

    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "Ket qua: " << (total - fail) << "/" << total << " tests PASSED.\n";
    std::cout << "========================================================\n";
    return fail;
}

// =============================================================================
// 13. HAM MAIN (ENTRY POINT)
// =============================================================================

int main(int argc, char** argv) {
    if (argc > 1 && (std::string(argv[1]) == "--test" || std::string(argv[1]) == "-t")) {
        return runSelfTests();
    }

    g.pyramid.initDefault();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(g.windowW, g.windowH);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("VKU - BAI TAP THUC HANH DO HOA 16/11: HINH CHOP SABC");

    // In thong tin huong dan ra Console
    std::cout << "=================================================================\n";
    std::cout << "  TRUONG DH CNTT VA TRUYEN THONG VIET - HAN (VKU)\n";
    std::cout << "  BAI TAP THUC HANH TAI LOP: HINH CHOP SABC (16/11)\n";
    std::cout << "=================================================================\n";
    std::cout << "  - Yeu cau 1: Khu khuat Backface & Deep-buffer (Depth-buffer)\n";
    std::cout << "  - Yeu cau 2: To bong Gouraud (Gouraud Shading)\n";
    std::cout << "  - Yeu cau 3: Xuat 3 hinh anh nop vao group Zalo\n\n";
    std::cout << "  PHIM TAT DIEU KHIEN:\n";
    std::cout << "    [1]         : Chuyen che do Khử khuất Backface\n";
    std::cout << "    [2]         : Chuyen che do Khử khuất Deep-buffer\n";
    std::cout << "    [3]         : Chuyen che do Tô bóng Gouraud\n";
    std::cout << "    [Space]     : Bật/Tắt tự động xoay camera\n";
    std::cout << "    [B]         : Bật/Tắt Backface Culling\n";
    std::cout << "    [D]         : Bật/Tắt Depth Buffer (Z-Buffer)\n";
    std::cout << "    [G]         : Chuyển đổi giữa Gouraud Shading và Flat Shading\n";
    std::cout << "    [N]         : Bật/Tắt hiển thị vector pháp tuyến\n";
    std::cout << "    [P] / [F12] : Chụp ảnh màn hình hiện tại (.BMP)\n";
    std::cout << "    [R]         : Đặt lại góc nhìn và thông số ban đầu\n";
    std::cout << "    [ESC]       : Thoát chương trình\n";
    std::cout << "=================================================================\n";

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passiveMotion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
