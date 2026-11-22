// =============================================================================
// VKU - KHOA CONG NGHE THONG TIN VA TRUYEN THONG
// LAB 3D: BIEU DIEN HINH CHOP ABCD VA CAC PHEP BIEN DOI 3D
// Ngay: 26/10
//
// Yeu cau de bai:
// 1. Bieu dien hinh chop ABCD theo Mo hinh mat da giac / Khung ket noi (Wireframe)
// 2. Thuc hien phep chieu Song song / Phoi canh (chon mat phang: XOY, YOZ, ZOX)
// 3. Cac phep bien doi 3D tren hinh chop ABCD:
//    - Phep tinh tien (Translation)
//    - Phep Ti le voi tam ti le la 1 dinh cua doi tuong (Scaling with pivot vertex)
//    - Phep Quay quanh cac truc Ox, Oy, Oz de quan sat vat the (Rotation around Ox, Oy, Oz)
// 4. Tuong tac chuot / ban phim:
//    - Phim (T, S, R) de chuyen doi cac phep bien doi: Tinh tien, Ti le, Quay
//    - Phim mui ten (Up, Down, Left, Right) de tang/giam goc quay / he so
//    - Click chuot (Left, Right) de tang/giam he so ti le; click chuot de xac dinh vi tri tinh tien
//
// Chuan toan hoc (VKU Computer Graphics):
//    Vector hang P = [x y z 1], P' = P x M_4x4
//
// Bien dich:
//    cmake --build build --target Lab_3D_main
// Chay:
//    main.exe         (Giao dien do hoa 3D)
//    main.exe --test  (Chay bo kiem thu tu dong Self-Tests)
// =============================================================================

#include <GL/freeglut.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

static const double PI = 3.14159265358979323846;

// ============================ 1. TOAN HOC 3D (MATH) ============================
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
    if (l < 1e-9) return { 0, 0, 0 };
    return v / l;
}

// Ma tran 4x4 (Row-vector convention: P = [x y z 1], P' = P x M)
struct Mat4 {
    double m[4][4];
};

static Mat4 ident4() {
    Mat4 r{};
    for (int i = 0; i < 4; i++) r.m[i][i] = 1.0;
    return r;
}

static Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

static Vec3 operator*(const Vec3& p, const Mat4& M) {
    double x = p.x * M.m[0][0] + p.y * M.m[1][0] + p.z * M.m[2][0] + M.m[3][0];
    double y = p.x * M.m[0][1] + p.y * M.m[1][1] + p.z * M.m[2][1] + M.m[3][1];
    double z = p.x * M.m[0][2] + p.y * M.m[1][2] + p.z * M.m[2][2] + M.m[3][2];
    double w = p.x * M.m[0][3] + p.y * M.m[1][3] + p.z * M.m[2][3] + M.m[3][3];
    if (std::fabs(w) > 1e-9 && std::fabs(w - 1.0) > 1e-7) {
        return { x / w, y / w, z / w };
    }
    return { x, y, z };
}

// 1. Ma tran Tinh tien 3D
static Mat4 translation4(double tx, double ty, double tz) {
    Mat4 r = ident4();
    r.m[3][0] = tx;
    r.m[3][1] = ty;
    r.m[3][2] = tz;
    return r;
}
static Vec3 translateDirect(const Vec3& p, double tx, double ty, double tz) {
    return { p.x + tx, p.y + ty, p.z + tz };
}

// 2. Ma tran Ti le 3D quanh goc toa do
static Mat4 scaling4(double sx, double sy, double sz) {
    Mat4 r = ident4();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    r.m[2][2] = sz;
    return r;
}

// Ma tran Ti le 3D voi tam ti le la 1 dinh P0(x0, y0, z0)
// M = T(-P0) * S(sx, sy, sz) * T(P0)
static Mat4 scalePivotMat4(const Vec3& p0, double sx, double sy, double sz) {
    return translation4(-p0.x, -p0.y, -p0.z) * scaling4(sx, sy, sz) * translation4(p0.x, p0.y, p0.z);
}
static Vec3 scalePivotDirect(const Vec3& p, const Vec3& p0, double sx, double sy, double sz) {
    return {
        p0.x + (p.x - p0.x) * sx,
        p0.y + (p.y - p0.y) * sy,
        p0.z + (p.z - p0.z) * sz
    };
}

// 3. Ma tran Quay 3D quanh cac truc toa do Ox, Oy, Oz
static Mat4 rotX4(double rad) {
    Mat4 r = ident4();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[1][1] = c;  r.m[1][2] = s;
    r.m[2][1] = -s; r.m[2][2] = c;
    return r;
}

static Mat4 rotY4(double rad) {
    Mat4 r = ident4();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][2] = -s;
    r.m[2][0] = s;  r.m[2][2] = c;
    return r;
}

static Mat4 rotZ4(double rad) {
    Mat4 r = ident4();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][1] = s;
    r.m[1][0] = -s; r.m[1][1] = c;
    return r;
}

// Quay tong hop quanh Ox, Oy, Oz (theo thu tu Rz * Ry * Rx)
static Mat4 rotationEuler4(double degX, double degY, double degZ) {
    return rotZ4(degZ * PI / 180.0) * rotY4(degY * PI / 180.0) * rotX4(degX * PI / 180.0);
}

static Vec3 rotateDirectOx(const Vec3& p, double rad) {
    double c = std::cos(rad), s = std::sin(rad);
    return { p.x, p.y * c - p.z * s, p.y * s + p.z * c };
}
static Vec3 rotateDirectOy(const Vec3& p, double rad) {
    double c = std::cos(rad), s = std::sin(rad);
    return { p.x * c + p.z * s, p.y, -p.x * s + p.z * c };
}
static Vec3 rotateDirectOz(const Vec3& p, double rad) {
    double c = std::cos(rad), s = std::sin(rad);
    return { p.x * c - p.y * s, p.x * s + p.y * c, p.z };
}

// 4. Phep chieu Song song (Orthographic) len cac mat phang: XOY, YOZ, ZOX
enum ProjPlane {
    PLANE_XOY = 0, // z = 0
    PLANE_YOZ = 1, // x = 0
    PLANE_ZOX = 2, // y = 0
    PLANE_3D   = 3  // Quan sat toan canh 3D
};

static Mat4 parallelProjMat(ProjPlane pl) {
    Mat4 r = ident4();
    if (pl == PLANE_XOY) r.m[2][2] = 0.0;      // z -> 0
    else if (pl == PLANE_YOZ) r.m[0][0] = 0.0; // x -> 0
    else if (pl == PLANE_ZOX) r.m[1][1] = 0.0; // y -> 0
    return r;
}

// 5. Phep chieu Phoi canh (Perspective) voi tam chieu C cach mat phang khoang d
static Vec3 projectPerspective(const Vec3& p, ProjPlane pl, double d = 6.0) {
    Vec3 r = p;
    if (pl == PLANE_XOY) {
        double denom = (d - p.z) / d;
        if (std::fabs(denom) < 1e-4) denom = 1e-4;
        r.x = p.x / denom;
        r.y = p.y / denom;
        r.z = 0.0;
    } else if (pl == PLANE_YOZ) {
        double denom = (d - p.x) / d;
        if (std::fabs(denom) < 1e-4) denom = 1e-4;
        r.x = 0.0;
        r.y = p.y / denom;
        r.z = p.z / denom;
    } else if (pl == PLANE_ZOX) {
        double denom = (d - p.y) / d;
        if (std::fabs(denom) < 1e-4) denom = 1e-4;
        r.x = p.x / denom;
        r.y = 0.0;
        r.z = p.z / denom;
    }
    return r;
}

static Vec3 projectParallel(const Vec3& p, ProjPlane pl) {
    Vec3 r = p;
    if (pl == PLANE_XOY) r.z = 0;
    else if (pl == PLANE_YOZ) r.x = 0;
    else if (pl == PLANE_ZOX) r.y = 0;
    return r;
}

// The tich hinh chop tu dien ABCD: V = 1/6 * |(AB x AC) . AD|
static double tetrahedronVolume(const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D) {
    Vec3 ab = B - A;
    Vec3 ac = C - A;
    Vec3 ad = D - A;
    return std::fabs(dot(cross(ab, ac), ad)) / 6.0;
}

// ============================ 2. SELF-TESTS ==================================
static int runTests() {
    int fail = 0, total = 0;
    auto chk = [&](bool c, const char* n) { total++; if (!c) fail++; printf("[%s] %s\n", c ? "PASS" : "FAIL", n); };
    auto eq = [](const Vec3& a, const Vec3& b) {
        return std::fabs(a.x - b.x) < 1e-6 && std::fabs(a.y - b.y) < 1e-6 && std::fabs(a.z - b.z) < 1e-6;
    };

    printf("=================================================================\n");
    printf("  RUNNING LAB 3D SELF-TESTS: PYRAMID ABCD & 3D TRANSFORMATIONS\n");
    printf("=================================================================\n");

    // Dinh hinh chop ABCD mau
    Vec3 A{ 0.0, 2.5, 0.0 };
    Vec3 B{ -2.0, -1.5, 1.5 };
    Vec3 C{ 2.0, -1.5, 1.5 };
    Vec3 D{ 0.0, -1.5, -2.0 };

    // --- A. TINH TIEN 3D (TRANSLATION) ---
    chk(eq(A * translation4(0, 0, 0), A), "T1: Tinh tien don vi (0,0,0) khong doi");
    chk(eq(A * translation4(2, -1, 3), { 2.0, 1.5, 3.0 }), "T2: Tinh tien vector (2, -1, 3)");
    {
        bool ok = true;
        for (double tx : { -2.0, 1.5 }) for (double ty : { -1.0, 2.0 }) for (double tz : { -3.0, 4.0 }) {
            Mat4 M = translation4(tx, ty, tz);
            ok = ok && eq(A * M, translateDirect(A, tx, ty, tz)) &&
                       eq(B * M, translateDirect(B, tx, ty, tz));
        }
        chk(ok, "T3: Ma tran tinh tien hang P * M == Cong thuc truc tiep");
    }
    {
        double v0 = tetrahedronVolume(A, B, C, D);
        Mat4 M = translation4(4, -3, 2);
        double v1 = tetrahedronVolume(A * M, B * M, C * M, D * M);
        chk(std::fabs(v0 - v1) < 1e-9, "T4: Tinh tien bao toan the tich hinh chop ABCD");
    }

    // --- B. TI LE 3D VOI TAM LA 1 DINH (SCALING WITH PIVOT VERTEX) ---
    chk(eq(A * scalePivotMat4(A, 1, 1, 1), A), "S1: Ti le identity (1,1,1)");
    chk(eq(A * scalePivotMat4(A, 2, 2, 2), A), "S2: Tam ti le A bat bien qua phep ti le (S*A = A)");
    chk(eq(B * scalePivotMat4(B, 3, 0.5, 2), B), "S3: Tam ti le B bat bien khi tam ti le la B");
    {
        // Kiem tra cong thuc ma tran va cong thuc truc tiep
        bool ok = true;
        for (double s : { 0.5, 1.5, 2.0 }) {
            Mat4 M = scalePivotMat4(A, s, s, s);
            ok = ok && eq(B * M, scalePivotDirect(B, A, s, s, s)) &&
                       eq(C * M, scalePivotDirect(C, A, s, s, s)) &&
                       eq(D * M, scalePivotDirect(D, A, s, s, s));
        }
        chk(ok, "S4: Phep ti le quanh dinh P0: Matrix == Direct formula");
    }
    {
        double v0 = tetrahedronVolume(A, B, C, D);
        double s = 2.0;
        Mat4 M = scalePivotMat4(A, s, s, s);
        double v1 = tetrahedronVolume(A * M, B * M, C * M, D * M);
        chk(std::fabs(v1 - v0 * (s * s * s)) < 1e-7, "S5: The tich sau phep ti le: V' = V * S^3");
    }

    // --- C. QUAY 3D QUANH TRUC OX, OY, OZ (ROTATION AROUND OX, OY, OZ) ---
    chk(eq(Vec3{ 0, 1, 0 } * rotX4(PI / 2.0), { 0, 0, 1 }), "R1: Quay 90 deg quanh Ox: Oy -> Oz");
    chk(eq(Vec3{ 0, 0, 1 } * rotY4(PI / 2.0), { 1, 0, 0 }), "R2: Quay 90 deg quanh Oy: Oz -> Ox");
    chk(eq(Vec3{ 1, 0, 0 } * rotZ4(PI / 2.0), { 0, 1, 0 }), "R3: Quay 90 deg quanh Oz: Ox -> Oy");
    {
        double v0 = tetrahedronVolume(A, B, C, D);
        Mat4 M = rotationEuler4(30, 45, 60);
        double v1 = tetrahedronVolume(A * M, B * M, C * M, D * M);
        chk(std::fabs(v0 - v1) < 1e-9, "R4: Phep quay bao toan the tich hinh chop ABCD");
    }

    // --- D. PHEP CHIEU SONG SONG & PHOI CANH ---
    {
        Vec3 p{ 3.0, 4.0, 5.0 };
        Vec3 projXOY = projectParallel(p, PLANE_XOY);
        Vec3 projYOZ = projectParallel(p, PLANE_YOZ);
        Vec3 projZOX = projectParallel(p, PLANE_ZOX);
        chk(eq(projXOY, { 3.0, 4.0, 0.0 }), "P1: Chieu song song len XOY triet tieu z (z=0)");
        chk(eq(projYOZ, { 0.0, 4.0, 5.0 }), "P2: Chieu song song len YOZ triet tieu x (x=0)");
        chk(eq(projZOX, { 3.0, 0.0, 5.0 }), "P3: Chieu song song len ZOX triet tieu y (y=0)");
    }
    {
        Vec3 p{ 2.0, 2.0, 0.0 }; // tren mat phang XOY
        Vec3 persp = projectPerspective(p, PLANE_XOY, 6.0);
        chk(eq(persp, { 2.0, 2.0, 0.0 }), "P4: Chieu phoi canh diem tren mat phang chieu bat bien");
    }

    printf("=================================================================\n");
    printf("  TEST RESULT: %d / %d tests passed!\n", total - fail, total);
    printf("=================================================================\n");
    return fail ? 1 : 0;
}

// ============================ 3. APPLICATION STATE ===========================
enum TransMode {
    MODE_TRANSLATE = 0,
    MODE_SCALE     = 1,
    MODE_ROTATE    = 2
};

enum RenderModel {
    MODEL_WIREFRAME = 0, // Khung ket noi
    MODEL_POLYGON   = 1, // Mat da giac
    MODEL_BOTH      = 2  // Ket hop ca Khung + Mat da giac
};

enum ProjectionType {
    PROJ_PARALLEL    = 0, // Chieu song song
    PROJ_PERSPECTIVE = 1  // Chieu phoi canh
};

enum RotAxis {
    AXIS_X = 0,
    AXIS_Y = 1,
    AXIS_Z = 2
};

struct Btn {
    double x, y, w, h;
    std::function<void()> fn;
};

struct Sld {
    double x, y, w, h;
    int id;
    double lo, hi;
};

struct AppState {
    // 4 Dinh goc cua hinh chop ABCD
    std::vector<Vec3> origVertices = {
        {  0.0,  2.2,  0.0 }, // A: Dinh chop (Apex)
        { -1.8, -1.2,  1.4 }, // B: Day trai truoc
        {  1.8, -1.2,  1.4 }, // C: Day phai truoc
        {  0.0, -1.2, -1.8 }  // D: Day sau
    };

    // 4 Mat da giac cua hinh chop (Tam giac)
    std::vector<std::array<int, 3>> faces = {
        { 0, 1, 2 }, // ABC (Mat truoc)
        { 0, 2, 3 }, // ACD (Mat phai)
        { 0, 3, 1 }, // ADB (Mat trai)
        { 1, 3, 2 }  // BDC (Mat day)
    };

    // 6 Canh khung ket noi (Wireframe edges)
    std::vector<std::pair<int, int>> edges = {
        { 0, 1 }, { 0, 2 }, { 0, 3 }, // AB, AC, AD
        { 1, 2 }, { 2, 3 }, { 3, 1 }  // BC, CD, DB
    };

    // Che do bien doi hien tai (T, S, R)
    TransMode mode = MODE_ROTATE;

    // Mo hinh bieu dien (Khung ket noi / Mat da giac / Ca hai)
    RenderModel model = MODEL_BOTH;

    // Phep chieu (Song song / Phoi canh)
    ProjectionType projType = PROJ_PERSPECTIVE;

    // Mat phang chieu phu hop (XOY, YOZ, ZOX, hoac 3D tu do)
    ProjPlane projPlane = PLANE_3D;

    // Dinh duoc chon lam tam ti le (0: A, 1: B, 2: C, 3: D)
    int pivotIdx = 0;

    // Truc quay hien tai dang dieu khien (0: Ox, 1: Oy, 2: Oz)
    RotAxis activeAxis = AXIS_Y;

    // Tham so phep bien doi
    // 1. Tinh tien (Translation)
    double tx = 0.0, ty = 0.0, tz = 0.0;

    // 2. Ti le (Scaling)
    double scaleFactor = 1.0;
    double sx = 1.0, sy = 1.0, sz = 1.0;

    // 3. Quay quan sat vat the (Rotation around Ox, Oy, Oz)
    double rotX = 20.0;
    double rotY = 35.0;
    double rotZ = 0.0;

    // Camera Orbit / Quan sat 3D
    double camPitch = 25.0;
    double camYaw   = -35.0;
    double camDist  = 12.0;
    double camTargetX = 0, camTargetY = 0, camTargetZ = 0;

    // Hien thi
    bool showOriginal = true;
    bool showAxes = true;
    bool showGrid = true;
    bool showProjectionShadow = true;
    bool showNormals = false;
    bool showLabels = true;
    bool autoSpin = false;
    double faceOpacity = 0.82;
    bool showGizmos = true;
    bool showRotationRing = true; // Bat/tat vong tron goc quay quanh truc

    // Kich thuoc cua so
    int W = 1360, H = 840;
    double mx = 0, my = 0, lx = 0, ly = 0;
    int dragSlider = -1;
    bool mouseRotating = false;
    bool mousePanning = false;

    int selParam = 0; // Tham so dang active cho phim mui ten

    std::vector<Btn> btns;
    std::vector<Sld> slds;
} g;

static const double HDR_H = 60.0;
static double panelW() { return 450.0; }
static double vw() { return g.W - panelW(); }
static double vh() { return g.H - HDR_H; }

// Tinh ma tran bien doi hien tai
static Mat4 currentTransformMatrix() {
    Mat4 M = ident4();

    // 1. Ti le quanh dinh tam ti le P0
    Vec3 p0 = g.origVertices[g.pivotIdx];
    Mat4 mScale = scalePivotMat4(p0, g.scaleFactor, g.scaleFactor, g.scaleFactor);

    // 2. Quay quanh cac truc Ox, Oy, Oz
    Mat4 mRot = rotationEuler4(g.rotX, g.rotY, g.rotZ);

    // 3. Tinh tien theo vector (tx, ty, tz)
    Mat4 mTrans = translation4(g.tx, g.ty, g.tz);

    // Ket hop tuy theo che do dang chon, hoac tich luy cac phep
    if (g.mode == MODE_SCALE) {
        M = mScale;
    } else if (g.mode == MODE_ROTATE) {
        M = mRot;
    } else if (g.mode == MODE_TRANSLATE) {
        M = mTrans;
    }

    return M;
}

// Lay toa do cac dinh sau bien doi
static std::vector<Vec3> getTransformedVertices() {
    Mat4 M = currentTransformMatrix();
    std::vector<Vec3> res;
    res.reserve(g.origVertices.size());
    for (const auto& v : g.origVertices) {
        res.push_back(v * M);
    }
    return res;
}

// Reset cac thong so bien doi
static void resetTransform() {
    g.tx = 0.0; g.ty = 0.0; g.tz = 0.0;
    g.scaleFactor = 1.0;
    g.sx = 1.0; g.sy = 1.0; g.sz = 1.0;
    g.rotX = 20.0; g.rotY = 35.0; g.rotZ = 0.0;
    g.pivotIdx = 0; // Dinh A
}

// ============================ 4. RENDERING & UI HELPERS ======================
struct ColorRGBA {
    float r, g, b, a;
};

static ColorRGBA hexCol(unsigned h, float a = 1.0f) {
    return { ((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a };
}

static const unsigned BG_COLOR       = 0x11141A;
static const unsigned PANEL_BG       = 0x191D24;
static const unsigned CARD_BG        = 0x222733;
static const unsigned ACCENT_CYAN    = 0x3DD6B5;
static const unsigned ACCENT_ORANGE  = 0xF79F37;
static const unsigned ACCENT_GOLD    = 0xFFCC00;
static const unsigned TEXT_WHITE     = 0xF3F5F7;
static const unsigned TEXT_MUTED     = 0x8C97A8;
static const unsigned BORDER_COLOR   = 0x333B4A;

static void* F_NORMAL = GLUT_BITMAP_HELVETICA_12;
static void* F_BOLD   = GLUT_BITMAP_HELVETICA_18;

static void setCol(ColorRGBA c) {
    glColor4f(c.r, c.g, c.b, c.a);
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

static void circle2D(double cx, double cy, double r, ColorRGBA c) {
    setCol(c);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(cx, cy);
    for (int i = 0; i <= 24; i++) {
        double a = i * 2.0 * PI / 24.0;
        glVertex2d(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    glEnd();
}

static double textWidth(const std::string& s, void* f = F_NORMAL) {
    return glutBitmapLength(f, (const unsigned char*)s.c_str());
}

static void drawText(double x, double y, const std::string& s, ColorRGBA c, void* f = F_NORMAL) {
    setCol(c);
    glRasterPos2d(x, y);
    for (unsigned char ch : s) glutBitmapCharacter(f, ch);
}

static void renderButton(double x, double y, double w, double h, const std::string& label, bool active, std::function<void()> fn, bool enabled = true) {
    bool hov = enabled && g.mx >= x && g.mx <= x + w && g.my >= y && g.my <= y + h;
    fillRect2D(x, y, w, h, active ? hexCol(0x1F6B5F) : hov ? hexCol(0x2B3648) : hexCol(CARD_BG));
    strokeRect2D(x, y, w, h, active ? hexCol(ACCENT_CYAN) : hexCol(BORDER_COLOR), active ? 1.5 : 1.0);
    drawText(x + (w - textWidth(label, F_NORMAL)) / 2.0, y + h / 2.0 - 4, label, hexCol(active ? TEXT_WHITE : enabled ? TEXT_WHITE : TEXT_MUTED), F_NORMAL);
    if (enabled) g.btns.push_back({ x, y, w, h, fn });
}

// Dinh dang so thuc dep mat
static std::string fmtNum(double val, int prec = 2) {
    if (std::fabs(val) < 1e-9) val = 0.0;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << val;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    if (s == "-0") s = "0";
    return s;
}

static double getParamValue(int id) {
    switch (id) {
        case 0: return g.tx;
        case 1: return g.ty;
        case 2: return g.tz;
        case 3: return g.scaleFactor;
        case 4: return g.rotX;
        case 5: return g.rotY;
        case 6: return g.rotZ;
    }
    return 0.0;
}

static void setParamValue(int id, double v) {
    switch (id) {
        case 0: g.tx = std::round(v * 20.0) / 20.0; break;
        case 1: g.ty = std::round(v * 20.0) / 20.0; break;
        case 2: g.tz = std::round(v * 20.0) / 20.0; break;
        case 3: g.scaleFactor = std::max(0.1, std::round(v * 20.0) / 20.0); break;
        case 4: g.rotX = std::round(v); break;
        case 5: g.rotY = std::round(v); break;
        case 6: g.rotZ = std::round(v); break;
    }
}

static void renderSliderRow(double x, double y, double w, int id, const char* label, double lo, double hi, bool en) {
    const double h = 24.0;
    double val = getParamValue(id);
    bool isSel = (g.selParam == id);

    drawText(x, y + 8, (isSel ? "> " : "  ") + std::string(label), hexCol(isSel ? ACCENT_GOLD : en ? TEXT_WHITE : TEXT_MUTED), F_NORMAL);

    double t0 = x + 75.0, t1 = x + w - 85.0, cy = y + h / 2.0;
    line2D(t0, cy, t1, cy, hexCol(en ? 0x475366 : 0x272E3B), 3.0);

    // Vach zero
    if (lo < 0 && hi > 0) {
        double zx = t0 + (0 - lo) / (hi - lo) * (t1 - t0);
        line2D(zx, cy - 6, zx, cy + 6, hexCol(0x6D7C91), 1.0);
    }

    double hx = t0 + (val - lo) / (hi - lo) * (t1 - t0);
    hx = std::max(t0, std::min(t1, hx));
    circle2D(hx, cy, 6, hexCol(en ? ACCENT_CYAN : 0x475366));

    std::string valStr = fmtNum(val) + (id >= 4 ? " deg" : "");
    drawText(t1 + 10, y + 8, valStr, hexCol(en ? TEXT_WHITE : TEXT_MUTED), F_NORMAL);

    renderButton(x + w - 24, y, 24, h, (id == 3 ? "1" : "0"), false, [id] {
        setParamValue(id, (id == 3 ? 1.0 : 0.0));
    }, en);

    if (en) g.slds.push_back({ t0 - 8, y, t1 - t0 + 16, h, id, lo, hi });
}

static void handleSliderDrag(const Sld& s) {
    double t = (g.mx - (s.x + 8)) / (s.w - 16);
    t = std::max(0.0, std::min(1.0, t));
    setParamValue(s.id, s.lo + t * (s.hi - s.lo));
    g.selParam = s.id;
}

// ============================ 5. 3D RENDERING FUNCTIONS =====================

// Ve chuoi ky tu trong khong gian 3D an toan (khong in null character)
static void drawString3D(double x, double y, double z, const std::string& str, ColorRGBA c, void* font = F_BOLD) {
    setCol(c);
    glRasterPos3d(x, y, z);
    for (char ch : str) glutBitmapCharacter(font, ch);
}

// Ve mui ten truc toa do 3D chuan huong theo truc (Ox: +X, Oy: +Y, Oz: +Z)
static void drawAxisArrow(int axis, double length, ColorRGBA col) {
    setCol(col);
    glLineWidth(2.8f);

    double tipLen = 0.55;
    double tipRad = 0.16;
    double shaftLen = length - tipLen;

    // Doan thang than truc toa do
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    if (axis == 0) glVertex3d(shaftLen, 0, 0);
    else if (axis == 1) glVertex3d(0, shaftLen, 0);
    else glVertex3d(0, 0, shaftLen);
    glEnd();

    // Mui ten hinh non (Cone) xoay chinh xac theo huong truc
    glPushMatrix();
    if (axis == 0) { // Truc Ox (+X): Xoay tu +Z sang +X quanh Oy
        glTranslated(shaftLen, 0, 0);
        glRotated(90.0, 0, 1, 0);
    } else if (axis == 1) { // Truc Oy (+Y): Xoay tu +Z sang +Y quanh Ox
        glTranslated(0, shaftLen, 0);
        glRotated(-90.0, 1, 0, 0);
    } else { // Truc Oz (+Z): Mac dinh glutSolidCone da huong theo +Z
        glTranslated(0, 0, shaftLen);
    }
    glutSolidCone(tipRad, tipLen, 16, 2);
    glPopMatrix();
}

// Ve he truc toa do Oxyz voi mui ten chuan va nhan truc ro rang
static void drawCoordinateAxes() {
    if (!g.showAxes) return;

    double len = 5.2;

    // Ox (Do Tuoi)
    drawAxisArrow(0, len, hexCol(0xFF5252));
    drawString3D(len + 0.35, 0, 0, "Ox (+X)", hexCol(0xFF5252), F_BOLD);

    // Oy (Xanh La Tuoi)
    drawAxisArrow(1, len, hexCol(0x38EF7D));
    drawString3D(0, len + 0.35, 0, "Oy (+Y)", hexCol(0x38EF7D), F_BOLD);

    // Oz (Xanh Duong Sang)
    drawAxisArrow(2, len, hexCol(0x4D96FF));
    drawString3D(0, 0, len + 0.35, "Oz (+Z)", hexCol(0x4D96FF), F_BOLD);
}

// Ve luoi mat phang 3D (San nam ngang va mat phang chieu)
static void drawGridPlane(ProjPlane plane) {
    if (!g.showGrid) return;

    int steps = 7;
    double stepSize = 1.0;
    double maxL = steps * stepSize;
    double groundY = -1.4; // San nam ngang ngay duoi day hinh chop

    // 1. Luoi san nam ngang (Ground floor XOZ) giup cam nhan chieu sau va khong gian
    glLineWidth(1.0f);
    setCol(hexCol(0x242C3A, 0.6f));
    glBegin(GL_LINES);
    for (int i = -steps; i <= steps; i++) {
        double d = i * stepSize;
        glVertex3d(-maxL, groundY, d); glVertex3d(maxL, groundY, d);
        glVertex3d(d, groundY, -maxL); glVertex3d(d, groundY, maxL);
    }
    glEnd();

    // 2. Khi chon mat phang chieu cu the, ve mat phang chieu trong suot voi vien cam
    if (plane != PLANE_3D) {
        setCol(hexCol(0xF79F37, 0.12f));
        glBegin(GL_QUADS);
        if (plane == PLANE_XOY) {
            glVertex3d(-maxL, -maxL, 0); glVertex3d(maxL, -maxL, 0);
            glVertex3d(maxL, maxL, 0); glVertex3d(-maxL, maxL, 0);
        } else if (plane == PLANE_YOZ) {
            glVertex3d(0, -maxL, -maxL); glVertex3d(0, maxL, -maxL);
            glVertex3d(0, maxL, maxL); glVertex3d(0, -maxL, maxL);
        } else if (plane == PLANE_ZOX) {
            glVertex3d(-maxL, 0, -maxL); glVertex3d(maxL, 0, -maxL);
            glVertex3d(maxL, 0, maxL); glVertex3d(-maxL, 0, maxL);
        }
        glEnd();

        // Vien mat phang chieu
        setCol(hexCol(0xF79F37, 0.7f));
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        if (plane == PLANE_XOY) {
            glVertex3d(-maxL, -maxL, 0); glVertex3d(maxL, -maxL, 0);
            glVertex3d(maxL, maxL, 0); glVertex3d(-maxL, maxL, 0);
        } else if (plane == PLANE_YOZ) {
            glVertex3d(0, -maxL, -maxL); glVertex3d(0, maxL, -maxL);
            glVertex3d(0, maxL, maxL); glVertex3d(0, -maxL, maxL);
        } else if (plane == PLANE_ZOX) {
            glVertex3d(-maxL, 0, -maxL); glVertex3d(maxL, 0, -maxL);
            glVertex3d(maxL, 0, maxL); glVertex3d(-maxL, 0, maxL);
        }
        glEnd();
    }
}

// Ve hinh chop ABCD (Tetrahedron) dep mat, ro rang, khong bi toi hay chong cheo
static void drawPyramid(const std::vector<Vec3>& v, bool isOriginal) {
    if (v.size() < 4) return;

    // 4 mau sac ruc ro, phan biet ro 4 mat cua hinh chop
    static const ColorRGBA faceColors[4] = {
        hexCol(0x00E5BE, 0.90f), // Mat ABC (Truoc): Xanh ngoc sang
        hexCol(0xFFA62B, 0.90f), // Mat ACD (Phai): Vang cam ruc ro
        hexCol(0x9D4EDD, 0.90f), // Mat ADB (Trai): Tim hien dai
        hexCol(0x3A86FF, 0.90f)  // Mat BDC (Day): Xanh cobalt sang
    };

    if (isOriginal) {
        if (!g.showOriginal) return;

        // Hinh goc ABCD chi ve KHUNG NET DUT (Wireframe Ghost) de KHONG lam toi/che khuat hinh chinh!
        setCol(hexCol(0x7E8B9B, 0.65f));
        glLineWidth(1.8f);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(2, 0xAAAA);
        glBegin(GL_LINES);
        for (const auto& e : g.edges) {
            glVertex3d(v[e.first].x, v[e.first].y, v[e.first].z);
            glVertex3d(v[e.second].x, v[e.second].y, v[e.second].z);
        }
        glEnd();
        glDisable(GL_LINE_STIPPLE);

        // Nhan cac dinh goc A, B, C, D
        if (g.showLabels) {
            for (size_t i = 0; i < v.size(); i++) {
                char name[8];
                snprintf(name, sizeof name, "%c", (char)('A' + i));
                drawString3D(v[i].x + 0.15, v[i].y + 0.15, v[i].z + 0.15, name, hexCol(0x8C9BAE), F_NORMAL);
            }
        }
        return;
    }

    // 1. Ve mat da giac (Polygon Faces) cho hinh bien doi A'B'C'D'
    if (g.model == MODEL_POLYGON || g.model == MODEL_BOTH) {
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        for (int i = 0; i < 4; i++) {
            const auto& f = g.faces[i];
            Vec3 norm = normalize(cross(v[f[1]] - v[f[0]], v[f[2]] - v[f[0]]));

            ColorRGBA col = faceColors[i];
            col.a = (float)g.faceOpacity;
            setCol(col);

            GLfloat matSpec[] = { 0.35f, 0.35f, 0.35f, 1.0f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24.0f);

            glBegin(GL_TRIANGLES);
            glNormal3d(norm.x, norm.y, norm.z);
            glVertex3d(v[f[0]].x, v[f[0]].y, v[f[0]].z);
            glVertex3d(v[f[1]].x, v[f[1]].y, v[f[1]].z);
            glVertex3d(v[f[2]].x, v[f[2]].y, v[f[2]].z);
            glEnd();
        }
        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_LIGHTING);
    }

    // 2. Ve khung ket noi vien canh (Wireframe Edges) cho hinh bien doi
    if (g.model == MODEL_WIREFRAME || g.model == MODEL_BOTH) {
        setCol(hexCol(0xFFFFFF));
        glLineWidth(2.8f);
        glBegin(GL_LINES);
        for (const auto& e : g.edges) {
            glVertex3d(v[e.first].x, v[e.first].y, v[e.first].z);
            glVertex3d(v[e.second].x, v[e.second].y, v[e.second].z);
        }
        glEnd();
    }

    // 3. Ve cac dinh va nhan dinh A', B', C', D'
    for (size_t i = 0; i < v.size(); i++) {
        bool isPivot = ((int)i == g.pivotIdx && g.mode == MODE_SCALE);
        glPushMatrix();
        glTranslated(v[i].x, v[i].y, v[i].z);
        if (isPivot) {
            // Dinh tam ti le phat sang mau vang ruc ro
            setCol(hexCol(ACCENT_GOLD));
            glutSolidSphere(0.18, 16, 16);
            setCol(hexCol(0xFFFFFF));
            glutWireSphere(0.24, 12, 12);
        } else {
            setCol(hexCol(0x00E5BE));
            glutSolidSphere(0.13, 14, 14);
            setCol(hexCol(0xFFFFFF));
            glutWireSphere(0.14, 8, 8);
        }
        glPopMatrix();

        // Nhan dinh A', B', C', D'
        if (g.showLabels) {
            char name[16];
            snprintf(name, sizeof name, "%c'", (char)('A' + i));
            drawString3D(v[i].x + 0.18, v[i].y + 0.18, v[i].z + 0.18, name, isPivot ? hexCol(ACCENT_GOLD) : hexCol(0xFFFFFF), F_BOLD);
        }
    }
}

// Ve hinh chieu (Projection) cua hinh chop len mat phang chieu duoc chon
static void drawProjectionView(const std::vector<Vec3>& vTrans) {
    if (g.projPlane == PLANE_3D || !g.showProjectionShadow) return;

    std::vector<Vec3> vProj;
    vProj.reserve(vTrans.size());
    for (const auto& pt : vTrans) {
        if (g.projType == PROJ_PARALLEL) {
            vProj.push_back(projectParallel(pt, g.projPlane));
        } else {
            vProj.push_back(projectPerspective(pt, g.projPlane, 7.0));
        }
    }

    // 1. Ve tia chieu tu dinh that den dinh chieu
    setCol(hexCol(0x56708A, 0.7f));
    glLineWidth(1.0f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x5555);
    glBegin(GL_LINES);
    for (size_t i = 0; i < vTrans.size(); i++) {
        glVertex3d(vTrans[i].x, vTrans[i].y, vTrans[i].z);
        glVertex3d(vProj[i].x, vProj[i].y, vProj[i].z);
    }
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    // 2. To mat bong chieu len mat phang
    setCol(hexCol(ACCENT_ORANGE, 0.35f));
    glBegin(GL_TRIANGLES);
    for (const auto& f : g.faces) {
        glVertex3d(vProj[f[0]].x, vProj[f[0]].y, vProj[f[0]].z);
        glVertex3d(vProj[f[1]].x, vProj[f[1]].y, vProj[f[1]].z);
        glVertex3d(vProj[f[2]].x, vProj[f[2]].y, vProj[f[2]].z);
    }
    glEnd();

    // 3. Khung vien hinh chieu
    setCol(hexCol(ACCENT_ORANGE));
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (const auto& e : g.edges) {
        glVertex3d(vProj[e.first].x, vProj[e.first].y, vProj[e.first].z);
        glVertex3d(vProj[e.second].x, vProj[e.second].y, vProj[e.second].z);
    }
    glEnd();

    // 4. Diem chieu
    for (size_t i = 0; i < vProj.size(); i++) {
        glPushMatrix();
        glTranslated(vProj[i].x, vProj[i].y, vProj[i].z);
        setCol(hexCol(ACCENT_ORANGE));
        glutSolidSphere(0.09, 10, 10);
        glPopMatrix();

        if (g.showLabels) {
            char name[16];
            snprintf(name, sizeof name, "%c_p", (char)('A' + i));
            drawString3D(vProj[i].x + 0.12, vProj[i].y + 0.12, vProj[i].z + 0.12, name, hexCol(ACCENT_ORANGE), F_NORMAL);
        }
    }
}

// 1. Gizmo vong tron goc quay quanh truc (Rotation Ring)
static void drawRotationGizmo() {
    if (g.mode != MODE_ROTATE || !g.showRotationRing) return;

    double R = 3.2;
    int segments = 60;
    ColorRGBA col = (g.activeAxis == AXIS_X) ? hexCol(0xFF5252) :
                    (g.activeAxis == AXIS_Y) ? hexCol(0x38EF7D) : hexCol(0x4D96FF);

    glLineWidth(2.2f);
    setCol(col);

    // Ve vong tron quanh truc dang chon
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        double a = i * 2.0 * PI / segments;
        if (g.activeAxis == AXIS_X) glVertex3d(0, R * std::cos(a), R * std::sin(a));
        else if (g.activeAxis == AXIS_Y) glVertex3d(R * std::sin(a), 0, R * std::cos(a));
        else glVertex3d(R * std::cos(a), R * std::sin(a), 0);
    }
    glEnd();

    // Ve tia tu tam den goc quay hien tai
    double curAngle = (g.activeAxis == AXIS_X ? g.rotX : g.activeAxis == AXIS_Y ? g.rotY : g.rotZ) * PI / 180.0;
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    if (g.activeAxis == AXIS_X) glVertex3d(0, R * std::cos(curAngle), R * std::sin(curAngle));
    else if (g.activeAxis == AXIS_Y) glVertex3d(R * std::sin(curAngle), 0, R * std::cos(curAngle));
    else glVertex3d(R * std::cos(curAngle), R * std::sin(curAngle), 0);
    glEnd();

    // Nhan thong so goc quay
    char buf[64];
    snprintf(buf, sizeof buf, "Goc quay: %.1f deg", (g.activeAxis == AXIS_X ? g.rotX : g.activeAxis == AXIS_Y ? g.rotY : g.rotZ));
    if (g.activeAxis == AXIS_X) drawString3D(0.2, R * std::cos(curAngle), R * std::sin(curAngle) + 0.3, buf, col, F_BOLD);
    else if (g.activeAxis == AXIS_Y) drawString3D(R * std::sin(curAngle) + 0.3, 0.2, R * std::cos(curAngle), buf, col, F_BOLD);
    else drawString3D(R * std::cos(curAngle) + 0.3, R * std::sin(curAngle), 0.2, buf, col, F_BOLD);
}

// 2. Gizmo tia ti le tu tam dinh (Scaling Rays)
static void drawScalingGizmo(const std::vector<Vec3>& transV) {
    if (g.mode != MODE_SCALE || !g.showGizmos) return;

    Vec3 p0 = g.origVertices[g.pivotIdx];

    // Ve tia dan no noi tu tam P0 qua cac dinh
    glLineWidth(1.8f);
    setCol(hexCol(0xFFD700, 0.75f));
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0x5555);
    glBegin(GL_LINES);
    for (size_t i = 0; i < transV.size(); i++) {
        if ((int)i == g.pivotIdx) continue;
        glVertex3d(p0.x, p0.y, p0.z);
        glVertex3d(transV[i].x, transV[i].y, transV[i].z);
    }
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    // Nhan tam ti le P0
    char buf[64];
    snprintf(buf, sizeof buf, "Tam ti le %c (Co dinh)", (char)('A' + g.pivotIdx));
    drawString3D(p0.x + 0.2, p0.y + 0.3, p0.z + 0.2, buf, hexCol(0xFFD700), F_BOLD);
}

// 3. Gizmo vector tinh tien (Translation Vector)
static void drawTranslationGizmo(const std::vector<Vec3>& transV) {
    if (g.mode != MODE_TRANSLATE || !g.showGizmos) return;

    if (std::fabs(g.tx) < 1e-4 && std::fabs(g.ty) < 1e-4 && std::fabs(g.tz) < 1e-4) return;

    // Tinh trong tam goc va trong tam moi cua hinh chop
    Vec3 cOrig{ 0, 0, 0 };
    for (const auto& v : g.origVertices) cOrig = cOrig + v;
    cOrig = cOrig / 4.0;

    Vec3 cTrans = cOrig + Vec3{ g.tx, g.ty, g.tz };

    // Ve vector tinh tien v tu cOrig -> cTrans
    setCol(hexCol(0xFF7043));
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex3d(cOrig.x, cOrig.y, cOrig.z);
    glVertex3d(cTrans.x, cTrans.y, cTrans.z);
    glEnd();

    // Mui ten vector
    glPushMatrix();
    glTranslated(cTrans.x, cTrans.y, cTrans.z);
    setCol(hexCol(0xFF7043));
    glutSolidCone(0.18, 0.45, 12, 1);
    glPopMatrix();

    char buf[64];
    snprintf(buf, sizeof buf, "Vector v(%.2f, %.2f, %.2f)", g.tx, g.ty, g.tz);
    drawString3D((cOrig.x + cTrans.x) / 2.0 + 0.2, (cOrig.y + cTrans.y) / 2.0 + 0.2, (cOrig.z + cTrans.z) / 2.0, buf, hexCol(0xFF7043), F_BOLD);
}

// 4. La ban 3D Mini o goc man hinh (Orientation Compass)
static void drawMiniCompass(double cx, double cy, double r) {
    double pitchRad = g.camPitch * PI / 180.0;
    double yawRad   = g.camYaw * PI / 180.0;

    auto projAxis = [&](double x, double y, double z) -> std::pair<double, double> {
        double x1 = x * std::cos(-yawRad) + z * std::sin(-yawRad);
        double y1 = y;
        double z1 = -x * std::sin(-yawRad) + z * std::cos(-yawRad);
        double x2 = x1;
        double y2 = y1 * std::cos(-pitchRad) - z1 * std::sin(-pitchRad);
        return { cx + x2 * r, cy + y2 * r };
    };

    circle2D(cx, cy, r * 1.35, hexCol(0x191D24, 0.88f));
    strokeRect2D(cx - r * 1.4, cy - r * 1.4, r * 2.8, r * 2.8, hexCol(BORDER_COLOR), 1.0);

    auto ox = projAxis(1, 0, 0);
    auto oy = projAxis(0, 1, 0);
    auto oz = projAxis(0, 0, 1);

    line2D(cx, cy, ox.first, ox.second, hexCol(0xFF5252), 2.5);
    line2D(cx, cy, oy.first, oy.second, hexCol(0x38EF7D), 2.5);
    line2D(cx, cy, oz.first, oz.second, hexCol(0x4D96FF), 2.5);

    drawText(ox.first + 3, ox.second - 4, "X", hexCol(0xFF5252), F_NORMAL);
    drawText(oy.first + 3, oy.second - 4, "Y", hexCol(0x38EF7D), F_NORMAL);
    drawText(oz.first + 3, oz.second - 4, "Z", hexCol(0x4D96FF), F_NORMAL);
}

// ============================ 6. GUI HUD RENDERING ===========================

static void renderHUD() {
    g.btns.clear();
    g.slds.clear();

    double pw = panelW();
    double px = g.W - pw;

    // Header thanh tieu de tren cung
    fillRect2D(0, g.H - HDR_H, g.W, HDR_H, hexCol(PANEL_BG));
    line2D(0, g.H - HDR_H, g.W, g.H - HDR_H, hexCol(BORDER_COLOR), 1.5);

    drawText(24, g.H - 32, "VKU - LAB 3D: HINH CHOP ABCD & BIEN DOI 3D (26/10)", hexCol(TEXT_WHITE), F_BOLD);

    std::string subTitle = "Hinh chop 4 mat tam giac ABCD | Chieu Song song & Phoi canh (XOY, YOZ, ZOX) | Tinh tien, Ti le quanh dinh, Quay Ox, Oy, Oz";
    drawText(24, g.H - 50, subTitle, hexCol(ACCENT_CYAN), F_NORMAL);

    // Banner nhac nho chup anh gui Zalo
    std::string zaloHint = "Luu y: Nhan [S] click chuot tang/giam ti le; Nhan [R] / [Mui ten] de quay. Chup anh gui Zalo!";
    drawText(g.W - pw - textWidth(zaloHint) - 30, g.H - 35, zaloHint, hexCol(ACCENT_ORANGE), F_NORMAL);

    // Sidebar Ben Phai
    fillRect2D(px, 0, pw, g.H - HDR_H, hexCol(PANEL_BG));
    line2D(px, 0, px, g.H - HDR_H, hexCol(BORDER_COLOR), 1.5);

    double cy = g.H - HDR_H - 24;
    double cardPad = 14;
    double cardW = pw - cardPad * 2;

    // CARD 1: PHEP BIEN DOI DANG CHON (T, S, R)
    {
        double ch = 90;
        cy -= ch;
        fillRect2D(px + cardPad, cy, cardW, ch, hexCol(CARD_BG));
        strokeRect2D(px + cardPad, cy, cardW, ch, hexCol(BORDER_COLOR));

        drawText(px + cardPad + 12, cy + ch - 22, "1. CHON PHEP BIEN DOI 3D (Phim T, S, R)", hexCol(ACCENT_GOLD), F_NORMAL);

        double bw = (cardW - 24 - 16) / 3.0;
        renderButton(px + cardPad + 12, cy + 18, bw, 32, "T: Tinh Tien", g.mode == MODE_TRANSLATE, [] {
            g.mode = MODE_TRANSLATE;
        });
        renderButton(px + cardPad + 12 + bw + 8, cy + 18, bw, 32, "S: Ti Le Dinh", g.mode == MODE_SCALE, [] {
            g.mode = MODE_SCALE;
        });
        renderButton(px + cardPad + 12 + (bw + 8) * 2, cy + 18, bw, 32, "R: Quay Truc", g.mode == MODE_ROTATE, [] {
            g.mode = MODE_ROTATE;
        });
    }

    // CARD 2: MO HINH BIEU DIEN (KHUNG KET NOI / MAT DA GIAC)
    {
        double ch = 110;
        cy -= (ch + 12);
        fillRect2D(px + cardPad, cy, cardW, ch, hexCol(CARD_BG));
        strokeRect2D(px + cardPad, cy, cardW, ch, hexCol(BORDER_COLOR));

        drawText(px + cardPad + 12, cy + ch - 20, "2. MO HINH BIEU DIEN HINH CHOP (Phim M)", hexCol(ACCENT_GOLD), F_NORMAL);

        double bw = (cardW - 24 - 16) / 3.0;
        renderButton(px + cardPad + 12, cy + ch - 54, bw, 28, "Khung Wire", g.model == MODEL_WIREFRAME, [] {
            g.model = MODEL_WIREFRAME;
        });
        renderButton(px + cardPad + 12 + bw + 8, cy + ch - 54, bw, 28, "Mat Da Giac", g.model == MODEL_POLYGON, [] {
            g.model = MODEL_POLYGON;
        });
        renderButton(px + cardPad + 12 + (bw + 8) * 2, cy + ch - 54, bw, 28, "Ca Hai (Dep)", g.model == MODEL_BOTH, [] {
            g.model = MODEL_BOTH;
        });

        // Nut bat/tat khung net dut hinh goc ABCD
        std::string origLabel = g.showOriginal ? "Khung Hinh Goc ABCD: [BAT]" : "Khung Hinh Goc ABCD: [TAT]";
        renderButton(px + cardPad + 12, cy + 12, cardW - 24, 26, origLabel, g.showOriginal, [] {
            g.showOriginal = !g.showOriginal;
        });
    }

    // CARD 3: PHEP CHIEU & MAT PHANG CHIEU (XOY, YOZ, ZOX)
    {
        double ch = 126;
        cy -= (ch + 12);
        fillRect2D(px + cardPad, cy, cardW, ch, hexCol(CARD_BG));
        strokeRect2D(px + cardPad, cy, cardW, ch, hexCol(BORDER_COLOR));

        drawText(px + cardPad + 12, cy + ch - 22, "3. PHEP CHIEU & MAT PHANG CHIEU (Phim P, 1, 2, 3)", hexCol(ACCENT_GOLD), F_NORMAL);

        // Chon Song song / Phoi canh
        double bw2 = (cardW - 24 - 10) / 2.0;
        renderButton(px + cardPad + 12, cy + ch - 58, bw2, 28, "Chieu Song Song", g.projType == PROJ_PARALLEL, [] {
            g.projType = PROJ_PARALLEL;
        });
        renderButton(px + cardPad + 12 + bw2 + 10, cy + ch - 58, bw2, 28, "Chieu Phoi Canh", g.projType == PROJ_PERSPECTIVE, [] {
            g.projType = PROJ_PERSPECTIVE;
        });

        // Chon Mat phang chieu
        double bw4 = (cardW - 24 - 18) / 4.0;
        renderButton(px + cardPad + 12, cy + 16, bw4, 28, "1: XOY", g.projPlane == PLANE_XOY, [] {
            g.projPlane = PLANE_XOY;
        });
        renderButton(px + cardPad + 12 + bw4 + 6, cy + 16, bw4, 28, "2: YOZ", g.projPlane == PLANE_YOZ, [] {
            g.projPlane = PLANE_YOZ;
        });
        renderButton(px + cardPad + 12 + (bw4 + 6) * 2, cy + 16, bw4, 28, "3: ZOX", g.projPlane == PLANE_ZOX, [] {
            g.projPlane = PLANE_ZOX;
        });
        renderButton(px + cardPad + 12 + (bw4 + 6) * 3, cy + 16, bw4, 28, "4: 3D All", g.projPlane == PLANE_3D, [] {
            g.projPlane = PLANE_3D;
        });
    }

    // CARD 4: DIEU KHIEN CHI TIET THEO CHE DO (SLIDERS & OPTIONS)
    {
        double ch = 190;
        cy -= (ch + 12);
        fillRect2D(px + cardPad, cy, cardW, ch, hexCol(CARD_BG));
        strokeRect2D(px + cardPad, cy, cardW, ch, hexCol(BORDER_COLOR));

        if (g.mode == MODE_SCALE) {
            drawText(px + cardPad + 12, cy + ch - 22, "DIEU KHIEN TI LE (TAM LA 1 DINH)", hexCol(ACCENT_CYAN), F_BOLD);

            // Chon dinh lam tam ti le
            drawText(px + cardPad + 12, cy + ch - 46, "Chon dinh lam tam ti le P0 (Phim V):", hexCol(TEXT_MUTED), F_NORMAL);
            double bw4 = (cardW - 24 - 18) / 4.0;
            renderButton(px + cardPad + 12, cy + ch - 78, bw4, 26, "Dinh A", g.pivotIdx == 0, [] { g.pivotIdx = 0; });
            renderButton(px + cardPad + 12 + bw4 + 6, cy + ch - 78, bw4, 26, "Dinh B", g.pivotIdx == 1, [] { g.pivotIdx = 1; });
            renderButton(px + cardPad + 12 + (bw4 + 6) * 2, cy + ch - 78, bw4, 26, "Dinh C", g.pivotIdx == 2, [] { g.pivotIdx = 2; });
            renderButton(px + cardPad + 12 + (bw4 + 6) * 3, cy + ch - 78, bw4, 26, "Dinh D", g.pivotIdx == 3, [] { g.pivotIdx = 3; });

            // Slider he so ti le
            renderSliderRow(px + cardPad + 12, cy + 50, cardW - 24, 3, "He so S:", 0.2, 3.0, true);

            drawText(px + cardPad + 12, cy + 20, "Chuot: Click Trai de TANG (+0.2), Phai de GIAM (-0.2)", hexCol(ACCENT_ORANGE), F_NORMAL);

        } else if (g.mode == MODE_ROTATE) {
            drawText(px + cardPad + 12, cy + ch - 22, "DIEU KHIEN QUAY QUANH TRUC Ox, Oy, Oz", hexCol(ACCENT_CYAN), F_BOLD);

            // Chon truc quay va nut bat/tat vong quay Gizmo
            double bw4 = (cardW - 24 - 18) / 4.0;
            renderButton(px + cardPad + 12, cy + ch - 54, bw4, 26, "X: Ox", g.activeAxis == AXIS_X, [] { g.activeAxis = AXIS_X; });
            renderButton(px + cardPad + 12 + bw4 + 6, cy + ch - 54, bw4, 26, "Y: Oy", g.activeAxis == AXIS_Y, [] { g.activeAxis = AXIS_Y; });
            renderButton(px + cardPad + 12 + (bw4 + 6) * 2, cy + ch - 54, bw4, 26, "Z: Oz", g.activeAxis == AXIS_Z, [] { g.activeAxis = AXIS_Z; });
            renderButton(px + cardPad + 12 + (bw4 + 6) * 3, cy + ch - 54, bw4, 26, g.showRotationRing ? "Vong: ON" : "Vong: OFF", g.showRotationRing, [] {
                g.showRotationRing = !g.showRotationRing;
            });

            renderSliderRow(px + cardPad + 12, cy + 74, cardW - 24, 4, "Goc Ox:", -180.0, 180.0, true);
            renderSliderRow(px + cardPad + 12, cy + 44, cardW - 24, 5, "Goc Oy:", -180.0, 180.0, true);
            renderSliderRow(px + cardPad + 12, cy + 14, cardW - 24, 6, "Goc Oz:", -180.0, 180.0, true);

        } else if (g.mode == MODE_TRANSLATE) {
            drawText(px + cardPad + 12, cy + ch - 22, "DIEU KHIEN TINH TIEN 3D", hexCol(ACCENT_CYAN), F_BOLD);
            drawText(px + cardPad + 12, cy + ch - 42, "Click chuot vao khung 3D de dat vi tri moi!", hexCol(ACCENT_ORANGE), F_NORMAL);

            renderSliderRow(px + cardPad + 12, cy + 86, cardW - 24, 0, "Tx (Ox):", -5.0, 5.0, true);
            renderSliderRow(px + cardPad + 12, cy + 54, cardW - 24, 1, "Ty (Oy):", -5.0, 5.0, true);
            renderSliderRow(px + cardPad + 12, cy + 22, cardW - 24, 2, "Tz (Oz):", -5.0, 5.0, true);
        }
    }

    // CARD 5: BANG TOA DO CAC DINH & MA TRAN 4x4
    {
        double ch = 190;
        cy -= (ch + 12);
        fillRect2D(px + cardPad, cy, cardW, ch, hexCol(CARD_BG));
        strokeRect2D(px + cardPad, cy, cardW, ch, hexCol(BORDER_COLOR));

        drawText(px + cardPad + 12, cy + ch - 20, "BANG TOA DO CAC DINH HINH CHOP ABCD:", hexCol(ACCENT_GOLD), F_NORMAL);

        auto transV = getTransformedVertices();
        char buf[128];
        double tableY = cy + ch - 42;

        drawText(px + cardPad + 12, tableY, "Dinh Goc: A(0, 2.2, 0) | B(-1.8,-1.2, 1.4) | C(1.8,-1.2, 1.4) | D(0,-1.2,-1.8)", hexCol(TEXT_MUTED), F_NORMAL);

        tableY -= 22;
        drawText(px + cardPad + 12, tableY, "TOA DO SAU BIEN DOI (A', B', C', D'):", hexCol(ACCENT_CYAN), F_NORMAL);

        for (size_t i = 0; i < transV.size(); i++) {
            snprintf(buf, sizeof buf, "%c': (%5.2f, %5.2f, %5.2f)", (char)('A' + i), transV[i].x, transV[i].y, transV[i].z);
            double colX = (i % 2 == 0) ? px + cardPad + 16 : px + cardPad + 220;
            double rowY = tableY - 20 - (i / 2) * 20;
            drawText(colX, rowY, buf, hexCol((int)i == g.pivotIdx && g.mode == MODE_SCALE ? ACCENT_GOLD : TEXT_WHITE), F_NORMAL);
        }

        // Hien thi ma tran bien doi 4x4
        double matY = tableY - 65;
        drawText(px + cardPad + 12, matY, "MA TRAN BIEN DOI 4x4 DANG AP DUNG (P' = P * M):", hexCol(TEXT_MUTED), F_NORMAL);

        Mat4 M = currentTransformMatrix();
        for (int r = 0; r < 4; r++) {
            snprintf(buf, sizeof buf, "[ %5.2f  %5.2f  %5.2f  %5.2f ]", M.m[r][0], M.m[r][1], M.m[r][2], M.m[r][3]);
            drawText(px + cardPad + 24, matY - 16 - r * 15, buf, hexCol(0xA8B5C8), F_NORMAL);
        }
    }

    // Thanh nut goc nhin nhanh (Quick Camera Views) & Auto-Spin
    {
        double btnY = 56;
        double bw5 = (cardW - 16) / 5.0;
        renderButton(px + cardPad, btnY, bw5, 26, "3D (0)", false, [] {
            g.camPitch = 22.0; g.camYaw = -40.0; g.camDist = 13.0;
        });
        renderButton(px + cardPad + bw5 + 4, btnY, bw5, 26, "Bang", false, [] {
            g.camPitch = 89.0; g.camYaw = 0.0;
        });
        renderButton(px + cardPad + (bw5 + 4) * 2, btnY, bw5, 26, "Dung", false, [] {
            g.camPitch = 0.0; g.camYaw = 0.0;
        });
        renderButton(px + cardPad + (bw5 + 4) * 3, btnY, bw5, 26, "Canh", false, [] {
            g.camPitch = 0.0; g.camYaw = 90.0;
        });
        renderButton(px + cardPad + (bw5 + 4) * 4, btnY, bw5, 26, g.autoSpin ? "Dung" : "Quay A", g.autoSpin, [] {
            g.autoSpin = !g.autoSpin;
        });
    }

    // Nut Reset bien doi
    {
        double btnY = 16;
        double bw = (cardW - 10) / 2.0;
        renderButton(px + cardPad, btnY, bw, 32, "Reset Bien Doi", false, [] {
            resetTransform();
        });
        renderButton(px + cardPad + bw + 10, btnY, bw, 32, "Reset Camera", false, [] {
            g.camPitch = 25.0; g.camYaw = -35.0; g.camDist = 12.0;
        });
    }

    // Mini Compass la ban huong 3D tren goc trai
    drawMiniCompass(50, vh() - 50, 24);

    // Mini Legend tren goc trai viewport
    {
        fillRect2D(16, 16, 360, 116, hexCol(PANEL_BG, 0.88f));
        strokeRect2D(16, 16, 360, 116, hexCol(BORDER_COLOR));

        drawText(26, 112, "HUONG DAN PHIM & CHUOT:", hexCol(ACCENT_GOLD), F_NORMAL);
        drawText(26, 94, "- T / S / R: Chuyen che do Tinh tien / Ti le / Quay", hexCol(TEXT_WHITE), F_NORMAL);
        drawText(26, 76, "- Mui ten: Tang/giam goc quay | X, Y, Z: Doi truc quay", hexCol(TEXT_WHITE), F_NORMAL);
        drawText(26, 58, "- Click chuot (Trai/Phai): Tang/giam he so ti le S", hexCol(ACCENT_CYAN), F_NORMAL);
        drawText(26, 40, "- A: Bat/tat Tu dong quay 3D | O: Bat/tat khung goc", hexCol(ACCENT_ORANGE), F_NORMAL);
        drawText(26, 22, "- 0, 7, 8, 9: Xem goc 3D, Hinh chieu Bang, Dung, Canh", hexCol(TEXT_MUTED), F_NORMAL);
    }
}

// ============================ 7. GLUT CALLBACKS ==============================

static void display() {
    glClearColor(0.067f, 0.078f, 0.102f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ----------------- 1. VIEWPORT 3D (BEN TRAI) -----------------
    glViewport(0, 0, (GLsizei)vw(), (GLsizei)vh());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = vw() / vh();

    if (g.projType == PROJ_PERSPECTIVE) {
        gluPerspective(45.0, aspect, 0.5, 100.0);
    } else {
        double orthoSize = g.camDist * 0.45;
        glOrtho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, -50.0, 100.0);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Tinh vi tri camera orbit
    double pitchRad = g.camPitch * PI / 180.0;
    double yawRad   = g.camYaw * PI / 180.0;
    double eyeX = g.camDist * std::cos(pitchRad) * std::sin(yawRad);
    double eyeY = g.camDist * std::sin(pitchRad);
    double eyeZ = g.camDist * std::cos(pitchRad) * std::cos(yawRad);

    gluLookAt(eyeX, eyeY, eyeZ,
              g.camTargetX, g.camTargetY, g.camTargetZ,
              0.0, 1.0, 0.0);

    // Thiet lap anh sang chieu
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Two-sided lighting de ca 2 mat deu sang dep
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    GLfloat globalAmbient[] = { 0.45f, 0.45f, 0.50f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Den chinh (Key light tu tren xuong)
    GLfloat lightPos0[] = { 8.0f, 14.0f, 10.0f, 1.0f };
    GLfloat lightDiff0[] = { 0.90f, 0.90f, 0.90f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff0);
    glEnable(GL_LIGHT0);

    // Den phu (Fill light tu phia sau de khong bi den mat nao)
    GLfloat lightPos1[] = { -8.0f, -6.0f, -10.0f, 1.0f };
    GLfloat lightDiff1[] = { 0.45f, 0.45f, 0.50f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiff1);
    glEnable(GL_LIGHT1);

    // Ve luoi va he truc toa do Oxyz
    drawGridPlane(g.projPlane);
    drawCoordinateAxes();

    // Ve hinh chop goc ABCD (Ghost reference)
    drawPyramid(g.origVertices, true);

    // Ve hinh chop sau bien doi A'B'C'D'
    auto transVertices = getTransformedVertices();
    drawPyramid(transVertices, false);

    // Ve phep chieu (bong va tia chieu len mat phang XOY, YOZ, ZOX)
    drawProjectionView(transVertices);

    // Ve cac Gizmo truc quan ho tro quan sat bien doi
    drawRotationGizmo();
    drawScalingGizmo(transVertices);
    drawTranslationGizmo(transVertices);

    // ----------------- 2. GIAO DIEN 2D HUD (FULL WINDOW) -----------------
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glViewport(0, 0, g.W, g.H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, g.W, 0, g.H);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    renderHUD();

    glutSwapBuffers();
}

static void reshape(int w, int h) {
    g.W = w;
    g.H = h;
    glutPostRedisplay();
}

// Chuyen toa do click chuot tren man hinh sang vi tri tinh tien 3D
static void pickTranslationPoint(int mouseX, int mouseY) {
    // Tinh ty le chuot trong vung 3D viewport
    double normX = (mouseX / vw()) * 2.0 - 1.0;
    double normY = ((vh() - mouseY) / vh()) * 2.0 - 1.0;

    // Anh xa tuong doi sang mat phang Oxy trong he toa do the gioi
    g.tx = std::round(normX * 4.0 * 20.0) / 20.0;
    g.ty = std::round(normY * 4.0 * 20.0) / 20.0;
}

static void mouse(int button, int state, int x, int y) {
    g.mx = x;
    g.my = g.H - y;

    if (state == GLUT_DOWN) {
        // Kiem tra click vao Slider
        g.dragSlider = -1;
        for (const auto& s : g.slds) {
            if (g.mx >= s.x && g.mx <= s.x + s.w && g.my >= s.y && g.my <= s.y + s.h) {
                g.dragSlider = s.id;
                handleSliderDrag(s);
                glutPostRedisplay();
                return;
            }
        }

        // Kiem tra click vao Nut GUI
        for (const auto& b : g.btns) {
            if (g.mx >= b.x && g.mx <= b.x + b.w && g.my >= b.y && g.my <= b.y + b.h) {
                if (b.fn) b.fn();
                glutPostRedisplay();
                return;
            }
        }

        // Tuong tac trong vung ve 3D
        if (x < vw() && (g.H - y) < vh()) {
            if (g.mode == MODE_SCALE) {
                // Click chuot trai / phai de tang/giam he so ti le theo yeu cau de bai
                if (button == GLUT_LEFT_BUTTON) {
                    g.scaleFactor = std::min(4.0, g.scaleFactor + 0.2);
                } else if (button == GLUT_RIGHT_BUTTON) {
                    g.scaleFactor = std::max(0.2, g.scaleFactor - 0.2);
                }
            } else if (g.mode == MODE_TRANSLATE) {
                // Click chuot de xac dinh vi tri tinh tien theo de bai
                if (button == GLUT_LEFT_BUTTON) {
                    pickTranslationPoint(x, y);
                }
            } else if (g.mode == MODE_ROTATE) {
                // Xoay quanh goc nhin hoac truc
                if (button == GLUT_LEFT_BUTTON) {
                    g.mouseRotating = true;
                } else if (button == GLUT_RIGHT_BUTTON) {
                    g.mousePanning = true;
                }
            }

            g.lx = x;
            g.ly = y;
        }
    } else if (state == GLUT_UP) {
        g.dragSlider = -1;
        g.mouseRotating = false;
        g.mousePanning = false;
    }

    glutPostRedisplay();
}

static void motion(int x, int y) {
    g.mx = x;
    g.my = g.H - y;

    if (g.dragSlider >= 0) {
        for (const auto& s : g.slds) {
            if (s.id == g.dragSlider) {
                handleSliderDrag(s);
                glutPostRedisplay();
                return;
            }
        }
    }

    if (g.mouseRotating) {
        double dx = x - g.lx;
        double dy = y - g.ly;
        g.camYaw   += dx * 0.5;
        g.camPitch = std::max(-85.0, std::min(85.0, g.camPitch + dy * 0.5));
    } else if (g.mousePanning) {
        double dx = x - g.lx;
        double dy = y - g.ly;
        g.camTargetX -= dx * 0.01;
        g.camTargetY += dy * 0.01;
    }

    g.lx = x;
    g.ly = y;
    glutPostRedisplay();
}

static void wheel(int, int dir, int x, int y) {
    if (x < vw()) {
        g.camDist = std::max(3.0, std::min(40.0, g.camDist - dir * 0.8));
        glutPostRedisplay();
    }
}

static void keyboard(unsigned char k, int, int) {
    if (k >= 'A' && k <= 'Z') k = (unsigned char)(k - 'A' + 'a');

    switch (k) {
        case 't': g.mode = MODE_TRANSLATE; break;
        case 's': g.mode = MODE_SCALE; break;
        case 'r': g.mode = MODE_ROTATE; break;

        case 'm':
            g.model = (RenderModel)(((int)g.model + 1) % 3);
            break;

        case 'o': case 'g':
            g.showOriginal = !g.showOriginal;
            break;

        case 'p':
            g.projType = (g.projType == PROJ_PARALLEL ? PROJ_PERSPECTIVE : PROJ_PARALLEL);
            break;

        case '1': g.projPlane = PLANE_XOY; break;
        case '2': g.projPlane = PLANE_YOZ; break;
        case '3': g.projPlane = PLANE_ZOX; break;
        case '4': g.projPlane = PLANE_3D; break;

        case 'v':
            g.pivotIdx = (g.pivotIdx + 1) % 4;
            break;

        case 'x': g.activeAxis = AXIS_X; break;
        case 'y': g.activeAxis = AXIS_Y; break;
        case 'z': g.activeAxis = AXIS_Z; break;

        case 'a':
            g.autoSpin = !g.autoSpin;
            break;

        case 'k': case 'h':
            g.showRotationRing = !g.showRotationRing;
            break;

        case '0':
            g.camPitch = 22.0; g.camYaw = -40.0; g.camDist = 13.0;
            break;
        case '7': // Nhin tu Tren xuong (Top XOZ)
            g.camPitch = 89.0; g.camYaw = 0.0;
            break;
        case '8': // Nhin Chinh dien (Front XOY)
            g.camPitch = 0.0; g.camYaw = 0.0;
            break;
        case '9': // Nhin Ben hong (Side YOZ)
            g.camPitch = 0.0; g.camYaw = 90.0;
            break;

        case '[':
            g.faceOpacity = std::max(0.3, g.faceOpacity - 0.1);
            break;
        case ']':
            g.faceOpacity = std::min(1.0, g.faceOpacity + 0.1);
            break;

        case '+': case '=':
            if (g.mode == MODE_SCALE) g.scaleFactor = std::min(4.0, g.scaleFactor + 0.1);
            break;
        case '-': case '_':
            if (g.mode == MODE_SCALE) g.scaleFactor = std::max(0.1, g.scaleFactor - 0.1);
            break;

        case ' ':
            resetTransform();
            break;

        case 27: // ESC
            exit(0);
            break;
    }

    glutPostRedisplay();
}

static void special(int k, int, int) {
    bool shift = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
    double deltaAngle = shift ? 15.0 : 5.0;

    if (g.mode == MODE_ROTATE) {
        if (k == GLUT_KEY_LEFT) {
            if (g.activeAxis == AXIS_X) g.rotX -= deltaAngle;
            else if (g.activeAxis == AXIS_Y) g.rotY -= deltaAngle;
            else if (g.activeAxis == AXIS_Z) g.rotZ -= deltaAngle;
        } else if (k == GLUT_KEY_RIGHT) {
            if (g.activeAxis == AXIS_X) g.rotX += deltaAngle;
            else if (g.activeAxis == AXIS_Y) g.rotY += deltaAngle;
            else if (g.activeAxis == AXIS_Z) g.rotZ += deltaAngle;
        } else if (k == GLUT_KEY_UP) {
            g.rotX += deltaAngle;
        } else if (k == GLUT_KEY_DOWN) {
            g.rotX -= deltaAngle;
        }
    } else if (g.mode == MODE_SCALE) {
        if (k == GLUT_KEY_UP || k == GLUT_KEY_RIGHT) {
            g.scaleFactor = std::min(4.0, g.scaleFactor + 0.1);
        } else if (k == GLUT_KEY_DOWN || k == GLUT_KEY_LEFT) {
            g.scaleFactor = std::max(0.1, g.scaleFactor - 0.1);
        }
    } else if (g.mode == MODE_TRANSLATE) {
        double deltaTrans = shift ? 0.5 : 0.1;
        if (k == GLUT_KEY_LEFT)  g.tx -= deltaTrans;
        if (k == GLUT_KEY_RIGHT) g.tx += deltaTrans;
        if (k == GLUT_KEY_UP)    g.ty += deltaTrans;
        if (k == GLUT_KEY_DOWN)  g.ty -= deltaTrans;
    }

    glutPostRedisplay();
}

// Timer cap nhat hoat hoa Auto-Spin muot ma 60 FPS
static void timerCallback(int val) {
    if (g.autoSpin) {
        g.camYaw += 0.5;
        if (g.camYaw > 360.0) g.camYaw -= 360.0;
        glutPostRedisplay();
    }
    glutTimerFunc(16, timerCallback, 0);
}

// ============================ 8. MAIN ENTRY ==================================
int main(int argc, char** argv) {
    // Chay test tu dong neu co doi so --test
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--test")) {
            return runTests();
        }
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(g.W, g.H);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("VKU - LAB 3D: HINH CHOP ABCD & BIEN DOI 3D (26/10)");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glShadeModel(GL_SMOOTH);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutMouseWheelFunc(wheel);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(16, timerCallback, 0);

    glutMainLoop();
    return 0;
}
