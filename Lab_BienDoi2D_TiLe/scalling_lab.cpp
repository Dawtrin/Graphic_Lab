// =============================================================================
// LAB 3: 2D AFFINE TRANSFORMATIONS (CAC PHEP BIEN DOI 2D) - VKU Computer Graphics
// Single-file C++17 / OpenGL / FreeGLUT application.
//
// 4 Transformations implemented:
//   1. Tinh tien (Translation) by vector v = (tx, ty)
//   2. Ti le (Scaling) with pivot (x0, y0) and factors (Sx, Sy)
//   3. Quay (Rotation) with pivot (x0, y0) by angle theta
//   4. Doi xung (Reflection) across arbitrary line y = mx + b
//
// Application: Find image of Triangle ABC (and other polygons)
// Mouse interaction:
//   - Click to define translation vector (2 points or 1 point)
//   - Click to define scaling pivot (x0, y0)
//   - Click to define rotation center (x0, y0) and drag / click for angle
//   - Click 2 points to define reflection line y = mx + b
//
// Convention (VKU slides): ROW vectors, matrices multiply on the RIGHT.
//     P = [x y 1],  Q = P x M
//     Translation T(tx, ty) has (tx, ty) in the LAST ROW.
//
// Build:
//   cmake --build build --target Lab_BienDoi2D_TiLe_scalling_lab
//   (hoac: g++ -std=c++17 -O2 scalling_lab.cpp -o scaling_lab.exe -lfreeglut -lopengl32 -lglu32)
// Run GUI : scaling_lab.exe
// Run tests: scaling_lab.exe --test
// =============================================================================

#include <GL/freeglut.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <numeric>
#include <string>
#include <vector>

static const double PI = 3.14159265358979323846;

// ============================ 1. MATH (no OpenGL) ============================
struct Pt { double x = 0, y = 0; };
struct Mat3 { double m[3][3]; };

static Mat3 ident() {
    Mat3 r{};
    for (int i = 0; i < 3; i++) r.m[i][i] = 1.0;
    return r;
}

static Mat3 operator*(const Mat3& a, const Mat3& b) {
    Mat3 r{};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

static Pt operator*(const Pt& p, const Mat3& M) {
    return { p.x * M.m[0][0] + p.y * M.m[1][0] + M.m[2][0],
             p.x * M.m[0][1] + p.y * M.m[1][1] + M.m[2][1] };
}

// Basic elementary row-vector matrices
static Mat3 translation(double tx, double ty) {
    Mat3 r = ident();
    r.m[2][0] = tx;
    r.m[2][1] = ty;
    return r;
}

static Mat3 scaling(double sx, double sy) {
    Mat3 r = ident();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    return r;
}

static Mat3 rotation(double rad) {
    Mat3 r = ident();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][1] = s;
    r.m[1][0] = -s; r.m[1][1] = c;
    return r;
}

static Mat3 reflectOx() {
    Mat3 r = ident();
    r.m[1][1] = -1.0; // y -> -y
    return r;
}

// 1. Tinh tien (Translation)
static Mat3 translateMat(double tx, double ty) {
    return translation(tx, ty);
}
static Pt translateDirect(Pt p, double tx, double ty) {
    return { p.x + tx, p.y + ty };
}

// 2. Ti le voi tam (x0, y0) (Scaling)
static Mat3 scalePivotMat(Pt t, double sx, double sy) {
    return translation(-t.x, -t.y) * scaling(sx, sy) * translation(t.x, t.y);
}
static Pt scaleDirect(Pt p, Pt t, double sx, double sy) {
    return { t.x + (p.x - t.x) * sx, t.y + (p.y - t.y) * sy };
}

// 3. Quay quanh tam (x0, y0) (Rotation)
static Mat3 rotatePivotMat(Pt t, double rad) {
    return translation(-t.x, -t.y) * rotation(rad) * translation(t.x, t.y);
}
static Pt rotateDirect(Pt p, Pt t, double rad) {
    double c = std::cos(rad), s = std::sin(rad);
    double dx = p.x - t.x, dy = p.y - t.y;
    return { t.x + dx * c - dy * s, t.y + dx * s + dy * c };
}

// 4. Doi xung qua duong thang y = mx + b (Reflection)
// Pipeline: T(0, -b) * R(-alpha) * reflectOx() * R(alpha) * T(0, b), alpha = atan(m)
static Mat3 reflectLineMat(double m, double b) {
    double alpha = std::atan(m);
    return translation(0, -b) * rotation(-alpha) * reflectOx() * rotation(alpha) * translation(0, b);
}
static Pt reflectLineDirect(Pt p, double m, double b) {
    double m2 = m * m;
    double denom = m2 + 1.0;
    return {
        ((1.0 - m2) * p.x + 2.0 * m * p.y - 2.0 * m * b) / denom,
        (2.0 * m * p.x + (m2 - 1.0) * p.y + 2.0 * b) / denom
    };
}

static std::vector<Pt> applyM(const std::vector<Pt>& v, const Mat3& M) {
    std::vector<Pt> r;
    r.reserve(v.size());
    for (const auto& p : v) r.push_back(p * M);
    return r;
}

static double signedArea(const std::vector<Pt>& p) { // shoelace formula
    double a = 0; size_t n = p.size();
    for (size_t i = 0; i < n; i++) {
        const Pt& u = p[i]; const Pt& v = p[(i + 1) % n];
        a += u.x * v.y - v.x * u.y;
    }
    return a / 2.0;
}

struct BB { double x0 = 0, y0 = 0, x1 = 0, y1 = 0; bool ok = false; };
static BB bboxOf(const std::vector<Pt>& p) {
    BB b;
    for (const auto& q : p) {
        if (!b.ok) { b.x0 = b.x1 = q.x; b.y0 = b.y1 = q.y; b.ok = true; }
        else {
            b.x0 = std::min(b.x0, q.x); b.x1 = std::max(b.x1, q.x);
            b.y0 = std::min(b.y0, q.y); b.y1 = std::max(b.y1, q.y);
        }
    }
    return b;
}

static std::string fmt(double v, int prec = 2) {
    if (std::fabs(v) < 5e-10) v = 0;
    char b[64]; snprintf(b, sizeof b, "%.*f", prec, v);
    std::string s = b;
    if (s.find('.') != std::string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    if (s == "-0") s = "0";
    return s;
}

static std::string pt2s(Pt p) { return "(" + fmt(p.x) + ", " + fmt(p.y) + ")"; }

// Presets: 0 Triangle ABC, 1 VKU Slide 113 house, 2 Rectangle, 3 Concave polygon, 4 Custom
static std::vector<Pt> presetPts(int i) {
    switch (i) {
        case 0: return { {1, 1}, {5, 1}, {3, 4} };                   // Tam giac ABC de bai yeu cau
        case 1: return { {2, 3}, {4, 3}, {4, 5}, {3, 6}, {2, 5} };   // VKU Slide 113 house
        case 2: return { {2, 2}, {6, 2}, {6, 4}, {2, 4} };           // Rectangle ABCD
        case 3: return { {1, 1}, {5, 1}, {6, 3}, {4, 2.5}, {3, 5}, {1, 3} }; // Da giac lom
        default: return {};
    }
}

// =============================== 2. SELF TESTS ===============================
static int runTests() {
    int fail = 0, total = 0;
    auto chk = [&](bool c, const char* n) { total++; if (!c) fail++; printf("[%s] %s\n", c ? "PASS" : "FAIL", n); };
    auto eq = [](Pt a, Pt b) { return std::fabs(a.x - b.x) < 1e-7 && std::fabs(a.y - b.y) < 1e-7; };

    printf("=================================================================\n");
    printf("  RUNNING LAB 3 SELF TESTS: 2D AFFINE TRANSFORMATIONS (VKU)\n");
    printf("=================================================================\n");

    // --- A. TRANSLATION TESTS ---
    chk(eq(Pt{3, 4} * translation(0, 0), {3, 4}), "T1 Tinh tien identity (tx=0, ty=0)");
    chk(eq(Pt{2, 3} * translation(3, -2), {5, 1}), "T2 Tinh tien vector (3, -2): (2,3)->(5,1)");
    {
        bool ok = true;
        for (double tx : {-5.0, 0.0, 2.5}) for (double ty : {-3.0, 1.0, 4.0})
            for (auto& p : presetPts(0))
                ok = ok && eq(p * translateMat(tx, ty), translateDirect(p, tx, ty));
        chk(ok, "T3 Tinh tien: row-vector matrix == direct formula");
    }
    {
        auto tri = presetPts(0);
        auto moved = applyM(tri, translateMat(4, -3));
        chk(std::fabs(signedArea(tri) - signedArea(moved)) < 1e-9, "T4 Tinh tien: bao toan dien tich tam giac ABC");
    }

    // --- B. SCALING TESTS ---
    chk(eq(Pt{3, 4} * scaling(1, 1), {3, 4}), "S1 Ti le identity (1, 1)");
    chk(eq(Pt{2, 4} * scaling(2, 0.5), {4, 2}), "S2 Ti le khong deu (2, 0.5)");
    chk(eq(Pt{2, 3} * scaling(2, 2), {4, 6}), "S3 Phong to deu (2, 2)");
    chk(eq(Pt{4, 6} * scaling(0.5, 0.5), {2, 3}), "S4 Thu nho deu (0.5, 0.5)");
    chk(eq(Pt{4, 6} * scalePivotMat({2, 2}, 2, 0.5), {6, 4}), "S5 Ti le quanh tam T(2,2)");
    {
        bool fixed = true;
        for (double sx : {-2.0, 0.0, 0.5, 3.0}) for (double sy : {-1.0, 0.0, 2.0}) {
            Pt t{1.7, -2.3};
            fixed = fixed && eq(t * scalePivotMat(t, sx, sy), t) && eq(scaleDirect(t, t, sx, sy), t);
        }
        chk(fixed, "S6 Ti le: tam ti le T bat bien (invariant)");
    }
    {
        bool areaOk = true;
        for (int pi = 0; pi < 4; pi++) for (double sx : {-2.0, 0.5, 1.0, 2.5}) for (double sy : {-1.5, 0.5, 3.0}) {
            auto s = presetPts(pi); auto d = applyM(s, scalePivotMat({1, 2}, sx, sy));
            areaOk = areaOk && std::fabs(std::fabs(signedArea(d)) - std::fabs(signedArea(s)) * std::fabs(sx * sy)) < 1e-9;
        }
        chk(areaOk, "S7 Ti le: Dien tich' = Dien tich * |Sx * Sy|");
    }
    {
        auto d = applyM(presetPts(1), scaling(2, 0.5));
        chk(eq(d[0], {4, 1.5}) && eq(d[1], {8, 1.5}), "S8 Slide 113 house: A(2,3)->(4,1.5), B(4,3)->(8,1.5)");
    }

    // --- C. ROTATION TESTS ---
    chk(eq(Pt{3, 4} * rotation(0), {3, 4}), "R1 Quay goc 0 rad identity");
    chk(eq(Pt{1, 0} * rotation(PI / 2.0), {0, 1}), "R2 Quay goc 90 quanh O: (1,0)->(0,1)");
    chk(eq(Pt{0, 1} * rotation(PI / 2.0), {-1, 0}), "R3 Quay goc 90 quanh O: (0,1)->(-1,0)");
    chk(eq(Pt{2, 3} * rotation(PI), {-2, -3}), "R4 Quay goc 180 quanh O: (2,3)->(-2,-3)");
    chk(eq(Pt{4, 2} * rotatePivotMat({2, 2}, PI / 2.0), {2, 4}), "R5 Quay 90 quanh tam T(2,2): (4,2)->(2,4)");
    {
        bool fixed = true;
        for (double deg : {-180.0, -90.0, 30.0, 45.0, 90.0, 270.0}) {
            Pt t{3.5, -1.2}; double rad = deg * PI / 180.0;
            fixed = fixed && eq(t * rotatePivotMat(t, rad), t) && eq(rotateDirect(t, t, rad), t);
        }
        chk(fixed, "R6 Quay: tam quay T bat bien (invariant)");
    }
    {
        bool ok = true;
        for (double deg : {-90.0, 30.0, 45.0, 60.0, 90.0, 180.0}) {
            double rad = deg * PI / 180.0;
            for (auto& p : presetPts(0))
                ok = ok && eq(p * rotatePivotMat({2, 1}, rad), rotateDirect(p, {2, 1}, rad));
        }
        chk(ok, "R7 Quay: row-vector matrix == direct formula");
    }
    {
        auto tri = presetPts(0);
        auto rotTri = applyM(tri, rotatePivotMat({3, 3}, 47.0 * PI / 180.0));
        chk(std::fabs(signedArea(tri) - signedArea(rotTri)) < 1e-9, "R8 Quay: bao toan dien tich va chieu quay");
    }

    // --- D. REFLECTION (DOI XUNG) TESTS ---
    chk(eq(Pt{3, 4} * reflectLineMat(0, 0), {3, -4}), "M1 Doi xung qua truc Ox (y = 0): (3,4)->(3,-4)");
    chk(eq(Pt{3, 1} * reflectLineMat(1, 0), {1, 3}), "M2 Doi xung qua y = x: (3,1)->(1,3)");
    chk(eq(Pt{2, 5} * reflectLineMat(-1, 0), {-5, -2}), "M3 Doi xung qua y = -x: (2,5)->(-5,-2)");
    chk(eq(Pt{4, 5} * reflectLineMat(0, 2), {4, -1}), "M4 Doi xung qua y = 2: (4,5)->(4,-1)");
    chk(eq(Pt{1, 1} * reflectLineMat(2, 1), {-0.6, 1.8}), "M5 Doi xung qua y = 2x+1: (1,1)->(-0.6, 1.8)");
    {
        // Any point on line y = mx + b must be invariant
        bool onLineOk = true;
        for (double m : {-2.0, -0.5, 0.0, 1.0, 3.0}) for (double b : {-3.0, 0.0, 2.0}) {
            Pt pOnLine{2.0, m * 2.0 + b};
            onLineOk = onLineOk && eq(pOnLine * reflectLineMat(m, b), pOnLine) && eq(reflectLineDirect(pOnLine, m, b), pOnLine);
        }
        chk(onLineOk, "M6 Doi xung: moi diem tren duong thang y = mx + b bat bien");
    }
    {
        // Matrix pipeline == direct formula
        bool ok = true;
        for (double m : {-1.5, -0.5, 0.0, 1.0, 2.0}) for (double b : {-2.0, 0.0, 3.0})
            for (auto& p : presetPts(0))
                ok = ok && eq(p * reflectLineMat(m, b), reflectLineDirect(p, m, b));
        chk(ok, "M7 Doi xung: matrix pipeline (5 buoc) == direct formula");
    }
    {
        // Involution: M * M == Identity (doi xung 2 lan tro ve ban dau)
        bool invol = true;
        for (double m : {-2.0, 0.0, 1.0, 2.5}) for (double b : {-1.0, 2.0}) {
            Mat3 M = reflectLineMat(m, b);
            Mat3 M2 = M * M;
            for (auto& p : presetPts(0)) invol = invol && eq(p * M2, p);
        }
        chk(invol, "M8 Doi xung 2 lan ve lai hinh cu (M x M = I)");
    }
    {
        // Reflection reverses orientation: Area' = -Area
        auto tri = presetPts(0);
        auto reflTri = applyM(tri, reflectLineMat(1.5, -1.0));
        chk(std::fabs(signedArea(reflTri) + signedArea(tri)) < 1e-9, "M9 Doi xung: dien tich khong doi, dao huong (det = -1)");
    }

    // --- E. APPLICATION ON TRIANGLE ABC ---
    {
        auto abc = presetPts(0); // A(1,1), B(5,1), C(3,4)
        auto trABC = applyM(abc, translateMat(2, 3));
        chk(eq(trABC[0], {3, 4}) && eq(trABC[1], {7, 4}) && eq(trABC[2], {5, 7}),
            "ABC1 Anh tam giac ABC qua phep tinh tien v(2,3)");

        auto scABC = applyM(abc, scalePivotMat(abc[0], 2, 0.5)); // tam A(1,1)
        chk(eq(scABC[0], {1, 1}) && eq(scABC[1], {9, 1}) && eq(scABC[2], {5, 2.5}),
            "ABC2 Anh tam giac ABC qua phep ti le quanh A(1,1) voi Sx=2, Sy=0.5");

        auto rotABC = applyM(abc, rotatePivotMat(abc[0], PI / 2.0)); // tam A(1,1), 90 deg
        chk(eq(rotABC[0], {1, 1}) && eq(rotABC[1], {1, 5}) && eq(rotABC[2], {-2, 3}),
            "ABC3 Anh tam giac ABC qua phep quay 90 quanh A(1,1)");

        auto refABC = applyM(abc, reflectLineMat(1, 0)); // y = x
        chk(eq(refABC[0], {1, 1}) && eq(refABC[1], {1, 5}) && eq(refABC[2], {4, 3}),
            "ABC4 Anh tam giac ABC qua phep doi xung qua y = x");
    }

    printf("=================================================================\n");
    printf("  TEST RESULT: %d / %d tests passed\n", total - fail, total);
    printf("=================================================================\n");
    return fail ? 1 : 0;
}

// ============================ 3. APPLICATION STATE ===========================
enum TransType {
    TT_TRANSLATE = 0,
    TT_SCALE     = 1,
    TT_ROTATE    = 2,
    TT_REFLECT   = 3
};
static const char* TRANS_NAME[4] = { "Tinh Tien", "Ti Le (x0,y0)", "Quay (x0,y0)", "Doi Xung y=mx+b" };

struct Btn { double x, y, w, h; std::function<void()> fn; };
struct Sld { double x, y, w, h; int id; double lo, hi; };

struct App {
    std::vector<Pt> src;
    int preset = 0; // Default to Triangle ABC

    TransType transType = TT_TRANSLATE;

    // Translation parameters
    double tx = 3.0, ty = 2.0;
    Pt vStart{ 0, 0 };

    // Scaling parameters
    double sx = 1.5, sy = 1.5;
    double scalePivX = 1.0, scalePivY = 1.0;
    bool uniformScale = false;

    // Rotation parameters
    double rotAngle = 45.0; // degrees
    double rotPivX = 1.0, rotPivY = 1.0;

    // Reflection parameters (y = mx + b)
    double lineM = 1.0;
    double lineB = 0.0;
    Pt lineP1{ 0, 0 }, lineP2{ 4, 4 };

    // Mouse interaction tracking
    int clickStep = 0; // 0: waiting 1st click, 1: waiting 2nd click
    Pt clickTempPt{ 0, 0 };

    int sel = 0; // selected parameter
    int vtx = 0; // selected vertex
    bool grid = true, corr = true, bbox = false, labels = true;
    bool stepMode = false, pres = false, addMode = false;
    int step = 0;

    double cx = 3.0, cy = 2.5, ppu = 65.0;
    int W = 1300, H = 840;
    double mx = 0, my = 0, lx = 0, ly = 0;
    int dragSlider = -1;
    bool panning = false;

    std::vector<Btn> btns;
    std::vector<Sld> slds;
} g;

static const double HDR = 62;
static double panelW() { return g.pres ? 370 : 430; }
static double vw() { return g.W - panelW(); }
static double vh() { return g.H - HDR; }

static Pt w2s(Pt p) { return { vw() / 2.0 + (p.x - g.cx) * g.ppu, vh() / 2.0 + (p.y - g.cy) * g.ppu }; }
static Pt s2w(double x, double y) { return { g.cx + (x - vw() / 2.0) / g.ppu, g.cy + (y - vh() / 2.0) / g.ppu }; }
static bool inView(double x, double y) { return x >= 0 && x < vw() && y >= 0 && y < vh(); }
static std::string vname(size_t i) {
    return std::string(1, char('A' + i % 26)) + (i >= 26 ? std::to_string(i / 26) : "");
}

// Current transformation matrix
static Mat3 currentMat() {
    switch (g.transType) {
        case TT_TRANSLATE: return translateMat(g.tx, g.ty);
        case TT_SCALE:     return scalePivotMat({ g.scalePivX, g.scalePivY }, g.sx, g.sy);
        case TT_ROTATE:    return rotatePivotMat({ g.rotPivX, g.rotPivY }, g.rotAngle * PI / 180.0);
        case TT_REFLECT:   return reflectLineMat(g.lineM, g.lineB);
    }
    return ident();
}

static Pt applyCurrentDirect(Pt p) {
    switch (g.transType) {
        case TT_TRANSLATE: return translateDirect(p, g.tx, g.ty);
        case TT_SCALE:     return scaleDirect(p, { g.scalePivX, g.scalePivY }, g.sx, g.sy);
        case TT_ROTATE:    return rotateDirect(p, { g.rotPivX, g.rotPivY }, g.rotAngle * PI / 180.0);
        case TT_REFLECT:   return reflectLineDirect(p, g.lineM, g.lineB);
    }
    return p;
}

static std::vector<Pt> finalPts() {
    return applyM(g.src, currentMat());
}

static int getNumStages() {
    switch (g.transType) {
        case TT_TRANSLATE: return 2;
        case TT_SCALE:     return 4;
        case TT_ROTATE:    return 4;
        case TT_REFLECT:   return 6;
    }
    return 2;
}

static std::vector<Pt> getStagePts(int k) {
    std::vector<Pt> r = g.src;
    if (k <= 0) return r;

    switch (g.transType) {
        case TT_TRANSLATE:
            if (k >= 1) r = applyM(r, translation(g.tx, g.ty));
            break;
        case TT_SCALE:
            if (k >= 1) r = applyM(r, translation(-g.scalePivX, -g.scalePivY));
            if (k >= 2) r = applyM(r, scaling(g.sx, g.sy));
            if (k >= 3) r = applyM(r, translation(g.scalePivX, g.scalePivY));
            break;
        case TT_ROTATE:
            if (k >= 1) r = applyM(r, translation(-g.rotPivX, -g.rotPivY));
            if (k >= 2) r = applyM(r, rotation(g.rotAngle * PI / 180.0));
            if (k >= 3) r = applyM(r, translation(g.rotPivX, g.rotPivY));
            break;
        case TT_REFLECT: {
            double alpha = std::atan(g.lineM);
            if (k >= 1) r = applyM(r, translation(0, -g.lineB));
            if (k >= 2) r = applyM(r, rotation(-alpha));
            if (k >= 3) r = applyM(r, reflectOx());
            if (k >= 4) r = applyM(r, rotation(alpha));
            if (k >= 5) r = applyM(r, translation(0, g.lineB));
            break;
        }
    }
    return r;
}

static double pipelineError() {
    int maxK = getNumStages() - 1;
    auto st = getStagePts(maxK);
    double e = 0;
    for (size_t i = 0; i < g.src.size(); i++) {
        Pt d = applyCurrentDirect(g.src[i]);
        e = std::max(e, std::max(std::fabs(st[i].x - d.x), std::fabs(st[i].y - d.y)));
    }
    return e;
}

static void fitView() {
    std::vector<Pt> all = g.src;
    auto add = [&](const std::vector<Pt>& v) { all.insert(all.end(), v.begin(), v.end()); };
    add(finalPts());
    all.push_back({ 0, 0 });

    if (g.transType == TT_SCALE) all.push_back({ g.scalePivX, g.scalePivY });
    else if (g.transType == TT_ROTATE) all.push_back({ g.rotPivX, g.rotPivY });
    else if (g.transType == TT_TRANSLATE) {
        all.push_back(g.vStart);
        all.push_back({ g.vStart.x + g.tx, g.vStart.y + g.ty });
    } else if (g.transType == TT_REFLECT) {
        all.push_back(g.lineP1);
        all.push_back(g.lineP2);
    }

    BB b = bboxOf(all);
    double bw = std::max(b.x1 - b.x0, 3.0), bh = std::max(b.y1 - b.y0, 3.0);
    g.cx = (b.x0 + b.x1) / 2.0;
    g.cy = (b.y0 + b.y1) / 2.0;
    g.ppu = std::max(12.0, std::min(240.0, std::min(vw() / (bw * 1.35), vh() / (bh * 1.35))));
}

static void resetTransform() {
    g.tx = 3.0; g.ty = 2.0; g.vStart = { 0, 0 };
    g.sx = 1.5; g.sy = 1.5;
    g.rotAngle = 45.0;
    g.lineM = 1.0; g.lineB = 0.0; g.lineP1 = { 0, 0 }; g.lineP2 = { 4, 4 };
    g.clickStep = 0;

    if (!g.src.empty()) {
        g.scalePivX = g.src[0].x; g.scalePivY = g.src[0].y;
        g.rotPivX = g.src[0].x;   g.rotPivY = g.src[0].y;
    } else {
        g.scalePivX = 0; g.scalePivY = 0;
        g.rotPivX = 0; g.rotPivY = 0;
    }
    g.stepMode = false; g.step = 0;
    fitView();
}

static void loadPreset(int i) {
    g.preset = i; g.addMode = false; g.vtx = 0; g.stepMode = false; g.step = 0;
    g.clickStep = 0;
    if (i != 4) g.src = presetPts(i);

    if (!g.src.empty()) {
        g.scalePivX = g.src[0].x; g.scalePivY = g.src[0].y;
        g.rotPivX = g.src[0].x;   g.rotPivY = g.src[0].y;
        g.vStart = g.src[0];
    }
    fitView();
}

static void setTransType(TransType t) {
    g.transType = t;
    g.clickStep = 0;
    g.stepMode = false;
    g.step = 0;
    fitView();
}

static void nextStep() {
    int maxS = getNumStages();
    if (!g.stepMode) { g.stepMode = true; g.step = 0; }
    else g.step = (g.step + 1) % maxS;
}
static void prevStep() {
    int maxS = getNumStages();
    if (!g.stepMode) { g.stepMode = true; g.step = 0; }
    else g.step = (g.step - 1 + maxS) % maxS;
}

// ============================== 4. RENDERING ================================
struct C { float r, g, b, a; };
static C hex(unsigned h, float a = 1.0f) {
    return { ((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a };
}

static const unsigned BG = 0x111318, PANEL = 0x191D24, PANEL2 = 0x222834, TXT = 0xF1F3F5, TXT2 = 0x9BA5B4,
                      GRIDC = 0x2C333F, ORIG = 0x8FA4B8, TRAN = 0x48D7BE, PIV = 0xF5B942,
                      XAX = 0xE87979, YAX = 0x77A9F7, ERR = 0xFF7272, LINE_COL = 0xFFA94D;

static void* F12 = GLUT_BITMAP_HELVETICA_12;
static void* F18 = GLUT_BITMAP_HELVETICA_18;

static void setc(C c) { glColor4f(c.r, c.g, c.b, c.a); }
static void fillRect(double x, double y, double w, double h, C c) {
    setc(c); glBegin(GL_QUADS);
    glVertex2d(x, y); glVertex2d(x + w, y); glVertex2d(x + w, y + h); glVertex2d(x, y + h);
    glEnd();
}
static void strokeRect(double x, double y, double w, double h, C c, double lw = 1) {
    setc(c); glLineWidth((float)lw); glBegin(GL_LINE_LOOP);
    glVertex2d(x, y); glVertex2d(x + w, y); glVertex2d(x + w, y + h); glVertex2d(x, y + h);
    glEnd();
}
static void line(double x1, double y1, double x2, double y2, C c, double lw = 1) {
    setc(c); glLineWidth((float)lw); glBegin(GL_LINES);
    glVertex2d(x1, y1); glVertex2d(x2, y2);
    glEnd();
}
static void disc(double x, double y, double r, C c) {
    setc(c); glBegin(GL_TRIANGLE_FAN); glVertex2d(x, y);
    for (int i = 0; i <= 24; i++) {
        double a = i * 2.0 * PI / 24.0;
        glVertex2d(x + r * std::cos(a), y + r * std::sin(a));
    }
    glEnd();
}
static void ring(double x, double y, double r, C c, double lw = 1.5) {
    setc(c); glLineWidth((float)lw); glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 28; i++) {
        double a = i * 2.0 * PI / 28.0;
        glVertex2d(x + r * std::cos(a), y + r * std::sin(a));
    }
    glEnd();
}
static void arrow(double x1, double y1, double x2, double y2, C c, double lw = 2.0, double headSize = 10.0) {
    line(x1, y1, x2, y2, c, lw);
    double dx = x2 - x1, dy = y2 - y1;
    double len = std::hypot(dx, dy);
    if (len < 1e-4) return;
    dx /= len; dy /= len;
    double px = -dy, py = dx;
    setc(c);
    glBegin(GL_TRIANGLES);
    glVertex2d(x2, y2);
    glVertex2d(x2 - headSize * dx + (headSize * 0.5) * px, y2 - headSize * dy + (headSize * 0.5) * py);
    glVertex2d(x2 - headSize * dx - (headSize * 0.5) * px, y2 - headSize * dy - (headSize * 0.5) * py);
    glEnd();
}
static double tw(const std::string& s, void* f = F12) {
    return glutBitmapLength(f, (const unsigned char*)s.c_str());
}
static void txt(double x, double y, const std::string& s, C c, void* f = F12) {
    setc(c); glRasterPos2d(x, y);
    for (unsigned char ch : s) glutBitmapCharacter(f, ch);
}

static void button(double x, double y, double w, double h, const std::string& label, bool active, std::function<void()> fn, bool enabled = true) {
    bool hov = enabled && g.mx >= x && g.mx <= x + w && g.my >= y && g.my <= y + h;
    fillRect(x, y, w, h, active ? hex(0x24655D) : hov ? hex(0x2D3746) : hex(PANEL2));
    strokeRect(x, y, w, h, active ? hex(TRAN) : hex(0x3B4657));
    txt(x + (w - tw(label)) / 2.0, y + h / 2.0 - 4, label, hex(enabled ? TXT : 0x5D6675));
    if (enabled) g.btns.push_back({ x, y, w, h, fn });
}

static double getParamVal(int id) {
    if (id == 0) return g.tx;
    if (id == 1) return g.ty;
    if (id == 2) return g.sx;
    if (id == 3) return g.sy;
    if (id == 4) return g.scalePivX;
    if (id == 5) return g.scalePivY;
    if (id == 6) return g.rotAngle;
    if (id == 7) return g.rotPivX;
    if (id == 8) return g.rotPivY;
    if (id == 9) return g.lineM;
    if (id == 10) return g.lineB;
    return 0;
}
static void setParamVal(int id, double v) {
    if (id == 0) g.tx = std::round(v * 20.0) / 20.0;
    else if (id == 1) g.ty = std::round(v * 20.0) / 20.0;
    else if (id == 2) {
        g.sx = std::round(v * 20.0) / 20.0;
        if (g.uniformScale) g.sy = g.sx;
    }
    else if (id == 3) {
        g.sy = std::round(v * 20.0) / 20.0;
        if (g.uniformScale) g.sx = g.sy;
    }
    else if (id == 4) g.scalePivX = std::round(v * 20.0) / 20.0;
    else if (id == 5) g.scalePivY = std::round(v * 20.0) / 20.0;
    else if (id == 6) g.rotAngle = std::round(v);
    else if (id == 7) g.rotPivX = std::round(v * 20.0) / 20.0;
    else if (id == 8) g.rotPivY = std::round(v * 20.0) / 20.0;
    else if (id == 9) g.lineM = std::round(v * 20.0) / 20.0;
    else if (id == 10) g.lineB = std::round(v * 20.0) / 20.0;
}

static void sliderRow(double x, double y, double w, int id, const char* label, double lo, double hi, bool en) {
    const double h = 24; double v = getParamVal(id);
    txt(x, y + 8, (g.sel == id ? std::string("> ") : std::string("  ")) + label, hex(g.sel == id ? PIV : en ? TXT : 0x5D6675));
    double t0 = x + 72, t1 = x + w - 85, cy = y + h / 2.0;
    line(t0, cy, t1, cy, hex(en ? 0x4A5465 : 0x2C333F), 3);
    if (lo < 0 && hi > 0) {
        double zx = t0 + (0 - lo) / (hi - lo) * (t1 - t0);
        line(zx, cy - 6, zx, cy + 6, hex(0x6A7587), 1);
    }
    double hx = t0 + (v - lo) / (hi - lo) * (t1 - t0);
    disc(hx, cy, 6, hex(en ? TRAN : 0x4A5465));
    txt(t1 + 10, y + 8, fmt(v) + (id == 6 ? " deg" : ""), hex(en ? TXT : 0x5D6675));
    button(x + w - 24, y, 24, h, "0", false, [id] { setParamVal(id, 0); }, en);
    if (en) g.slds.push_back({ t0 - 8, y, t1 - t0 + 16, h, id, lo, hi });
}
static void sliderDrag(const Sld& s) {
    double t = (g.mx - (s.x + 8)) / (s.w - 16);
    t = std::max(0.0, std::min(1.0, t));
    setParamVal(s.id, s.lo + t * (s.hi - s.lo));
    g.sel = s.id;
}

static std::vector<std::array<int, 3>> triangulate(const std::vector<Pt>& p) { // ear clipping
    std::vector<std::array<int, 3>> out; int n = (int)p.size(); if (n < 3) return out;
    double A = signedArea(p); if (std::fabs(A) < 1e-12) return out;
    std::vector<int> idx(n); std::iota(idx.begin(), idx.end(), 0);
    if (A < 0) std::reverse(idx.begin(), idx.end());
    auto cr = [&](Pt a, Pt b, Pt c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); };
    int guard = 0;
    while (idx.size() > 3 && guard++ < 2000) {
        bool cut = false; size_t m = idx.size();
        for (size_t i = 0; i < m && !cut; i++) {
            int a = idx[(i + m - 1) % m], b = idx[i], c = idx[(i + 1) % m];
            if (cr(p[a], p[b], p[c]) <= 1e-12) continue;
            bool in = false;
            for (int k : idx) if (k != a && k != b && k != c && cr(p[a], p[b], p[k]) >= -1e-12 && cr(p[b], p[c], p[k]) >= -1e-12 && cr(p[c], p[a], p[k]) >= -1e-12) { in = true; break; }
            if (in) continue;
            out.push_back({ a, b, c }); idx.erase(idx.begin() + i); cut = true;
        }
        if (!cut) return out;
    }
    if (idx.size() == 3) out.push_back({ idx[0], idx[1], idx[2] });
    return out;
}

static void drawPoly(const std::vector<Pt>& pts, C col, float fillA, const std::string& suffix, double lw) {
    if (pts.empty()) return;
    std::vector<Pt> s;
    for (const auto& p : pts) s.push_back(w2s(p));

    if (fillA > 0) {
        C f = col; f.a = fillA; setc(f);
        glBegin(GL_TRIANGLES);
        for (const auto& t : triangulate(pts))
            for (int k : t) glVertex2d(s[k].x, s[k].y);
        glEnd();
    }
    setc(col); glLineWidth((float)lw);
    glBegin(pts.size() > 2 ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (const auto& p : s) glVertex2d(p.x, p.y);
    glEnd();

    Pt cen{ 0, 0 };
    for (const auto& p : s) { cen.x += p.x / s.size(); cen.y += p.y / s.size(); }

    for (size_t i = 0; i < s.size(); i++) {
        disc(s[i].x, s[i].y, 4.5, col);
        if (!g.labels) continue;
        double dx = s[i].x - cen.x, dy = s[i].y - cen.y, d = std::max(1e-6, std::hypot(dx, dy));
        dx /= d; dy /= d;
        std::string L = vname(i) + suffix + " " + pt2s(pts[i]);
        double lx = s[i].x + dx * 16.0, ly = s[i].y + dy * 16.0 - 4.0;
        if (dx < -0.2) lx -= tw(L);
        else if (std::fabs(dx) <= 0.2) lx -= tw(L) / 2.0;
        txt(lx, ly, L, col);
    }
}

static void dashed(bool on) {
    if (on) { glEnable(GL_LINE_STIPPLE); glLineStipple(1, 0x0F0F); }
    else glDisable(GL_LINE_STIPPLE);
}

static void drawGrid() {
    Pt a = s2w(0, 0), b = s2w(vw(), vh());
    double target = 50.0 / g.ppu;
    double mag = std::pow(10, std::floor(std::log10(target))), st = mag * 10;
    for (double c : { 1.0, 2.0, 5.0, 10.0 }) if (c * mag >= target) { st = c * mag; break; }
    Pt o = w2s({ 0, 0 });
    double lyy = std::max(4.0, std::min(vh() - 14, o.y - 15));
    double lxx = std::max(4.0, std::min(vw() - 46, o.x + 6));
    for (double x = std::floor(a.x / st) * st; x <= b.x; x += st) {
        double sx = w2s({ x, 0 }).x;
        line(sx, 0, sx, vh(), hex(GRIDC), 1);
        if (std::fabs(x) > st * 0.01) txt(sx - tw(fmt(x)) / 2.0, lyy, fmt(x), hex(TXT2));
    }
    for (double y = std::floor(a.y / st) * st; y <= b.y; y += st) {
        double sy = w2s({ 0, y }).y;
        line(0, sy, vw(), sy, hex(GRIDC), 1);
        if (std::fabs(y) > st * 0.01) txt(lxx, sy + 3, fmt(y), hex(TXT2));
    }
}

static void drawAxes() {
    Pt o = w2s({ 0, 0 });
    if (o.y >= 0 && o.y <= vh()) {
        line(0, o.y, vw() - 4, o.y, hex(XAX), 2);
        setc(hex(XAX)); glBegin(GL_TRIANGLES);
        glVertex2d(vw() - 2, o.y); glVertex2d(vw() - 16, o.y + 6); glVertex2d(vw() - 16, o.y - 6);
        glEnd();
        txt(vw() - 26, o.y + 12, "X", hex(XAX), F18);
    }
    if (o.x >= 0 && o.x <= vw()) {
        line(o.x, 0, o.x, vh() - 4, hex(YAX), 2);
        setc(hex(YAX)); glBegin(GL_TRIANGLES);
        glVertex2d(o.x, vh() - 2); glVertex2d(o.x + 6, vh() - 16); glVertex2d(o.x - 6, vh() - 16);
        glEnd();
        txt(o.x + 12, vh() - 28, "Y", hex(YAX), F18);
    }
    if (inView(o.x, o.y)) {
        disc(o.x, o.y, 4, hex(TXT));
        txt(o.x + 7, o.y - 16, "O(0,0)", hex(TXT), F12);
    }
}

// Draw Reflection Line across viewport: y = mx + b
static void drawReflectionLine() {
    Pt a = s2w(0, 0), b = s2w(vw(), vh());
    double xMin = a.x - 5, xMax = b.x + 5;
    Pt pLeft{ xMin, g.lineM * xMin + g.lineB };
    Pt pRight{ xMax, g.lineM * xMax + g.lineB };
    Pt s1 = w2s(pLeft), s2 = w2s(pRight);

    // Glowing line effect
    line(s1.x, s1.y, s2.x, s2.y, hex(LINE_COL, 0.4f), 5.0);
    line(s1.x, s1.y, s2.x, s2.y, hex(LINE_COL, 1.0f), 2.5);

    // Points P1 and P2 used to define the line
    Pt sp1 = w2s(g.lineP1), sp2 = w2s(g.lineP2);
    disc(sp1.x, sp1.y, 5, hex(PIV));
    ring(sp1.x, sp1.y, 8, hex(PIV));
    txt(sp1.x + 8, sp1.y + 8, "P1" + pt2s(g.lineP1), hex(PIV));

    disc(sp2.x, sp2.y, 5, hex(PIV));
    ring(sp2.x, sp2.y, 8, hex(PIV));
    txt(sp2.x + 8, sp2.y + 8, "P2" + pt2s(g.lineP2), hex(PIV));

    // Equation label
    std::string eq = "d: y = " + fmt(g.lineM) + "x " + (g.lineB >= 0 ? "+ " + fmt(g.lineB) : "- " + fmt(-g.lineB));
    Pt mid = w2s({ (g.lineP1.x + g.lineP2.x) / 2.0, (g.lineP1.y + g.lineP2.y) / 2.0 });
    txt(mid.x + 12, mid.y + 14, eq, hex(LINE_COL), F18);
}

// Draw Translation Vector
static void drawTranslationVector() {
    Pt sStart = w2s(g.vStart);
    Pt sEnd = w2s({ g.vStart.x + g.tx, g.vStart.y + g.ty });
    arrow(sStart.x, sStart.y, sEnd.x, sEnd.y, hex(PIV), 3.0, 12.0);
    disc(sStart.x, sStart.y, 5, hex(PIV));
    txt(sStart.x - 10, sStart.y - 16, "V_start" + pt2s(g.vStart), hex(PIV));
    txt(sEnd.x + 12, sEnd.y + 6, "v = (" + fmt(g.tx) + ", " + fmt(g.ty) + ")", hex(PIV), F18);
}

// Draw Pivot for Scale or Rotate
static void drawPivotMarker(Pt t, const std::string& name) {
    Pt s = w2s(t);
    line(s.x - 12, s.y, s.x + 12, s.y, hex(PIV), 2);
    line(s.x, s.y - 12, s.x, s.y + 12, hex(PIV), 2);
    ring(s.x, s.y, 8, hex(PIV), 2);
    txt(s.x + 14, s.y + 6, name + " " + pt2s(t), hex(PIV), F18);
}

// Draw Rotation Angle Arc
static void drawRotationArc(Pt center, double deg) {
    Pt sc = w2s(center);
    double r = 40.0;
    double rad = deg * PI / 180.0;
    int n = std::max(8, (int)(std::fabs(deg) / 5.0));
    setc(hex(PIV, 0.8f));
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= n; i++) {
        double a = i * rad / n;
        glVertex2d(sc.x + r * std::cos(a), sc.y + r * std::sin(a));
    }
    glEnd();
    // Arrow at arc end
    double ex = sc.x + r * std::cos(rad), ey = sc.y + r * std::sin(rad);
    double tangX = -std::sin(rad) * (rad >= 0 ? 1 : -1);
    double tangY = std::cos(rad) * (rad >= 0 ? 1 : -1);
    arrow(ex - tangX * 10, ey - tangY * 10, ex, ey, hex(PIV), 2.0, 8.0);
}

static const char* getStepTitle(int step) {
    switch (g.transType) {
        case TT_TRANSLATE:
            if (step == 0) return "STEP A - Hinh goc P";
            return "STEP B - Tinh tien theo vector v: P' = P x T(tx, ty)";
        case TT_SCALE:
            if (step == 0) return "STEP A - Hinh goc P voi tam ti le T(x0, y0)";
            if (step == 1) return "STEP B - Doi tam T ve goc toa do O: P1 = P - T";
            if (step == 2) return "STEP C - Co gian quanh goc O: P2 = P1 x S";
            return "STEP D - Doi nguoc ve tam T: P' = P2 + T";
        case TT_ROTATE:
            if (step == 0) return "STEP A - Hinh goc P voi tam quay T(x0, y0)";
            if (step == 1) return "STEP B - Doi tam quay T ve goc toa do O: P1 = P - T";
            if (step == 2) return "STEP C - Quay quanh goc O goc theta: P2 = P1 x R";
            return "STEP D - Doi nguoc ve tam T: P' = P2 + T";
        case TT_REFLECT:
            if (step == 0) return "STEP A - Hinh goc P va duong thang d: y = mx + b";
            if (step == 1) return "STEP B - Tinh tien d ve di qua goc O: P1 = P x T(0, -b)";
            if (step == 2) return "STEP C - Quay d ve trung truc hoanh Ox: P2 = P1 x R(-alpha)";
            if (step == 3) return "STEP D - Lay doi xung qua truc Ox: P3 = P2 x Refl_Ox";
            if (step == 4) return "STEP E - Quay nguoc lai goc +alpha: P4 = P3 x R(+alpha)";
            return "STEP F - Tinh tien ve lai vi tri cu: P' = P4 x T(0, +b)";
    }
    return "";
}

static const char* getStepDesc(int step) {
    switch (g.transType) {
        case TT_TRANSLATE:
            if (step == 0) return "Hinh hoc ban dau, chua thuc hien bien doi.";
            return "Q = P x T(tx, ty): x' = x + tx,  y' = y + ty.";
        case TT_SCALE:
            if (step == 0) return "T(x0, y0) la tam ti le co dinh; hinh chua bien doi.";
            if (step == 1) return "Tinh tien (-x0, -y0) de tam T roi vao goc toa do O.";
            if (step == 2) return "Co gian quanh O: x2 = x1 * Sx,  y2 = y1 * Sy.";
            return "Tinh tien (+x0, +y0) dua tam ve lai vi tri cu; giong cong thuc truc tiep.";
        case TT_ROTATE:
            if (step == 0) return "T(x0, y0) la tam quay co dinh; goc quay theta.";
            if (step == 1) return "Tinh tien (-x0, -y0) dua tam quay T ve goc O.";
            if (step == 2) return "Quay quanh O goc theta: x2 = x1*cos - y1*sin, y2 = x1*sin + y1*cos.";
            return "Tinh tien (+x0, +y0) tra tam ve vi tri cu; ket qua trung cong thuc truc tiep.";
        case TT_REFLECT:
            if (step == 0) return "Duong thang d: y = mx + b co he so goc m, goc nghieng alpha = arctan(m).";
            if (step == 1) return "Doi duong thang ve di qua O bang phep tinh tien theo truc Y: T(0, -b).";
            if (step == 2) return "Quay duong thang quanh O mot goc -alpha de d trung hoan toan truc Ox.";
            if (step == 3) return "Lay doi xung qua truc Ox: phep bien doi [x  -y] (dao dau toa do Y).";
            if (step == 4) return "Quay nguoc lai goc +alpha de tra duong thang ve huong song song ban dau.";
            return "Tinh tien nguoc lai +b dua d va hinh ve vi tri ban dau. Hoan tat chuoi 5 buoc!";
    }
    return "";
}

static void drawViewport() {
    glEnable(GL_SCISSOR_TEST); glScissor(0, 0, (GLsizei)vw(), (GLsizei)vh());
    fillRect(0, 0, vw(), vh(), hex(BG));

    if (g.grid) drawGrid();
    drawAxes();

    int maxK = getNumStages() - 1;
    int k = g.stepMode ? g.step : maxK;
    std::vector<Pt> dst = g.stepMode ? getStagePts(k) : finalPts();

    // Specific transformation visual helpers
    if (g.transType == TT_REFLECT) {
        drawReflectionLine();
    } else if (g.transType == TT_TRANSLATE) {
        drawTranslationVector();
    } else if (g.transType == TT_SCALE) {
        drawPivotMarker({ g.scalePivX, g.scalePivY }, "Tam Ti Le T");
    } else if (g.transType == TT_ROTATE) {
        drawPivotMarker({ g.rotPivX, g.rotPivY }, "Tam Quay T");
        drawRotationArc({ g.rotPivX, g.rotPivY }, g.rotAngle);
    }

    // Live mouse click preview
    if (g.clickStep == 1 && inView(g.mx, g.my)) {
        Pt curW = s2w(g.mx, g.my);
        curW.x = std::round(curW.x * 4.0) / 4.0;
        curW.y = std::round(curW.y * 4.0) / 4.0;
        Pt sStart = w2s(g.clickTempPt), sCur = w2s(curW);

        dashed(true);
        if (g.transType == TT_TRANSLATE) {
            arrow(sStart.x, sStart.y, sCur.x, sCur.y, hex(ERR), 2.0, 10.0);
            txt(sCur.x + 10, sCur.y + 10, "Vector moi: (" + fmt(curW.x - g.clickTempPt.x) + ", " + fmt(curW.y - g.clickTempPt.y) + ")", hex(ERR));
        } else if (g.transType == TT_REFLECT) {
            line(sStart.x, sStart.y, sCur.x, sCur.y, hex(ERR), 2.0);
            txt(sCur.x + 10, sCur.y + 10, "Chon P2 cho duong thang...", hex(ERR));
        }
        dashed(false);
    }

    // Bounding Box
    if (g.bbox) {
        dashed(true);
        BB a = bboxOf(g.src);
        if (a.ok) { Pt p = w2s({ a.x0, a.y0 }), q = w2s({ a.x1, a.y1 }); strokeRect(p.x, p.y, q.x - p.x, q.y - p.y, hex(ORIG, 0.8f)); }
        if (k > 0) { BB b = bboxOf(dst); if (b.ok) { Pt p = w2s({ b.x0, b.y0 }), q = w2s({ b.x1, b.y1 }); strokeRect(p.x, p.y, q.x - p.x, q.y - p.y, hex(TRAN, 0.8f)); } }
        dashed(false);
    }

    // Corresponding points link
    if (g.corr && k > 0 && !g.src.empty() && dst.size() == g.src.size()) {
        dashed(true);
        for (size_t i = 0; i < g.src.size(); i++) {
            Pt a = w2s(g.src[i]), b = w2s(dst[i]);
            line(a.x, a.y, b.x, b.y, hex(0x6C7686, 0.9f), 1.0);
        }
        dashed(false);
    }

    // Draw polygons
    std::string suf = (g.stepMode && k < maxK) ? std::to_string(k) : "'";
    drawPoly(g.src, hex(ORIG), 0.15f, "", 2.0);
    if (k > 0 || !g.stepMode) drawPoly(dst, hex(TRAN), 0.25f, suf, 2.5);

    // Selected vertex ring
    if (g.vtx >= 0 && g.vtx < (int)g.src.size()) {
        Pt a = w2s(g.src[g.vtx]); ring(a.x, a.y, 9, hex(TXT));
        if (k > 0 && g.vtx < (int)dst.size()) { Pt b = w2s(dst[g.vtx]); ring(b.x, b.y, 9, hex(TXT)); }
    }

    // Viewport legends & instructions overlay
    fillRect(12, vh() - 92, 180, 80, hex(PANEL, 0.88f));
    disc(24, vh() - 26, 5, hex(ORIG)); txt(36, vh() - 30, "Hinh goc P (Tam giac ABC)", hex(TXT));
    disc(24, vh() - 48, 5, hex(TRAN)); txt(36, vh() - 52, g.stepMode ? "Giai doan hien tai" : "Hinh anh Q (A'B'C')", hex(TXT));
    disc(24, vh() - 70, 5, hex(PIV));  txt(36, vh() - 74, "Diem chuot / Yeu to bien doi", hex(TXT));

    // Interaction banner
    std::string hint = "";
    if (g.addMode) hint = "ADD MODE: Click viewport them dinh da giac (Nhan Esc de ket thuc)";
    else if (g.transType == TT_TRANSLATE) {
        hint = (g.clickStep == 1) ? "CLICK CHUOT: Dang cho diem thu 2 de chot vector tinh tien..." : "CLICK CHUOT: Click 2 diem tren viewport de tao vector tinh tien v";
    } else if (g.transType == TT_SCALE) {
        hint = "CLICK CHUOT: Click len viewport de dat tam ti le (x0, y0)";
    } else if (g.transType == TT_ROTATE) {
        hint = "CLICK CHUOT: Click viewport dat tam quay (x0, y0) | Chinh goc quay bang Slider/Nut";
    } else if (g.transType == TT_REFLECT) {
        hint = (g.clickStep == 1) ? "CLICK CHUOT: Dang cho diem P2 tren duong thang y = mx + b..." : "CLICK CHUOT: Click 2 diem tren viewport de tao duong thang doi xung y = mx + b";
    }
    txt(205, vh() - 26, hint, hex(PIV), F18);

    // Step mode banner
    if (g.stepMode) {
        double bh = g.pres ? 78 : 64;
        fillRect(0, 0, vw(), bh, hex(PANEL, 0.92f));
        txt(16, bh - 24, getStepTitle(g.step), hex(TRAN), F18);
        txt(16, bh - 46, getStepDesc(g.step), hex(TXT), g.pres ? F18 : F12);
        if (g.step == maxK) {
            double err = pipelineError();
            txt(vw() - 340, bh - 24, "max |pipeline - direct| = " + (err < 1e-9 ? std::string("0 (OK)") : fmt(err, 9)), hex(TRAN));
        }
    }

    Pt curM = s2w(g.mx, g.my);
    if (inView(g.mx, g.my)) {
        txt(vw() - 210, g.stepMode ? (g.pres ? 86 : 72) : 12, "Toa do chuot: x=" + fmt(curM.x) + "  y=" + fmt(curM.y), hex(TXT2));
    }

    glDisable(GL_SCISSOR_TEST);
}

static void drawHeader() {
    fillRect(0, g.H - HDR, g.W, HDR, hex(PANEL));
    line(0, g.H - HDR, g.W, g.H - HDR, hex(0x2C333F), 1);

    txt(16, g.H - 24, "LAB 3: CAC PHEP BIEN DOI 2D & TIM ANH TAM GIAC ABC", hex(TXT), F18);
    txt(16, g.H - 46, "VKU Computer Graphics Lab - Tinh Tien | Ti Le (x0,y0) | Quay (x0,y0) | Doi Xung y=mx+b", hex(TXT2));

    std::string transInfo = "";
    if (g.transType == TT_TRANSLATE) transInfo = "TINH TIEN | v = (" + fmt(g.tx) + ", " + fmt(g.ty) + ")";
    else if (g.transType == TT_SCALE) transInfo = "TI LE | Tam T(" + fmt(g.scalePivX) + ", " + fmt(g.scalePivY) + ")  Sx=" + fmt(g.sx) + "  Sy=" + fmt(g.sy);
    else if (g.transType == TT_ROTATE) transInfo = "QUAY | Tam T(" + fmt(g.rotPivX) + ", " + fmt(g.rotPivY) + ")  Goc=" + fmt(g.rotAngle) + " deg";
    else if (g.transType == TT_REFLECT) transInfo = "DOI XUNG | d: y = " + fmt(g.lineM) + "x " + (g.lineB >= 0 ? "+ " + fmt(g.lineB) : "- " + fmt(-g.lineB));

    txt(580, g.H - 24, transInfo, hex(PIV), F18);

    double a0 = std::fabs(signedArea(g.src)), a1 = std::fabs(signedArea(finalPts()));
    std::string status = "Dien tich S: " + fmt(a0) + " -> S': " + fmt(a1);
    if (g.transType == TT_REFLECT) status += "  (Dao huong - Orientation reversed)";
    txt(580, g.H - 46, status, hex(TRAN));

    double x = g.W - 12 - 130 - 85 - 130 - 12, y = g.H - HDR + 15;
    button(x, y, 130, 30, "Reset Bien Doi", false, [] { resetTransform(); });
    button(x + 136, y, 85, 30, "Fit View", false, [] { fitView(); });
    button(x + 227, y, 130, 30, "Trinh Chieu", g.pres, [] {
        g.pres = !g.pres;
        if (g.pres) { g.stepMode = true; g.step = 0; }
        fitView();
    });
}

static void drawPanel() {
    double pw = panelW(), px = g.W - pw, top = g.H - HDR;
    fillRect(px, 0, pw, top, hex(PANEL));
    line(px, 0, px, top, hex(0x2C333F), 1);

    double x0 = px + 14, w = pw - 28, y = top - 4;
    void* F = g.pres ? F18 : F12; double LH = g.pres ? 22 : 16;
    auto sect = [&](const char* t) { y -= 17; txt(x0, y, t, hex(PIV)); y -= 3; };
    auto ln = [&](const std::string& s, C c) { if (y < 24) return; y -= LH; txt(x0, y, s, c, F); };
    auto row = [&](int n, double h, const std::function<void(int, double, double, double)>& f) {
        y -= h + 4; double bw = (w - (n - 1) * 5) / n;
        for (int i = 0; i < n; i++) f(i, x0 + i * (bw + 5), y, bw);
    };

    // 1. CHOOSE TRANSFORMATION TYPE
    sect("1. CHON PHEP BIEN DOI 2D (Phim T / S / R / M)");
    row(4, 28, [&](int i, double bx, double by, double bw) {
        button(bx, by, bw, 28, TRANS_NAME[i], g.transType == i, [i] { setTransType((TransType)i); });
    });

    // 2. PARAMETERS & SLIDERS
    sect("2. THAM SO BIEN DOI");
    if (g.transType == TT_TRANSLATE) {
        y -= 26; sliderRow(x0, y, w, 0, "Vector tx", -10, 10, true);
        y -= 26; sliderRow(x0, y, w, 1, "Vector ty", -10, 10, true);
        row(3, 22, [&](int i, double bx, double by, double bw) {
            if (i == 0) button(bx, by, bw, 22, "v=(2, 3)", g.tx == 2 && g.ty == 3, [] { g.tx = 2; g.ty = 3; fitView(); });
            else if (i == 1) button(bx, by, bw, 22, "v=(4, -2)", g.tx == 4 && g.ty == -2, [] { g.tx = 4; g.ty = -2; fitView(); });
            else button(bx, by, bw, 22, "Dat v tu O", false, [] { g.vStart = {0, 0}; fitView(); });
        });
    } else if (g.transType == TT_SCALE) {
        y -= 26; sliderRow(x0, y, w, 2, g.uniformScale ? "S (Sx)" : "Sx", -3, 3, true);
        y -= 26; sliderRow(x0, y, w, 3, g.uniformScale ? "S (Sy)" : "Sy", -3, 3, true);
        y -= 26; sliderRow(x0, y, w, 4, "Tam x0", -8, 8, true);
        y -= 26; sliderRow(x0, y, w, 5, "Tam y0", -8, 8, true);
        row(3, 22, [&](int i, double bx, double by, double bw) {
            if (i == 0) button(bx, by, bw, 22, g.uniformScale ? "Uniform: BAT" : "Uniform", g.uniformScale, [] {
                g.uniformScale = !g.uniformScale;
                if (g.uniformScale) g.sy = g.sx;
            });
            else if (i == 1) button(bx, by, bw, 22, "Tam O(0,0)", g.scalePivX == 0 && g.scalePivY == 0, [] { g.scalePivX = 0; g.scalePivY = 0; });
            else button(bx, by, bw, 22, "Tam la dinh A", false, [] {
                if (!g.src.empty()) { g.scalePivX = g.src[0].x; g.scalePivY = g.src[0].y; }
            });
        });
    } else if (g.transType == TT_ROTATE) {
        y -= 26; sliderRow(x0, y, w, 6, "Goc theta", -360, 360, true);
        y -= 26; sliderRow(x0, y, w, 7, "Tam x0", -8, 8, true);
        y -= 26; sliderRow(x0, y, w, 8, "Tam y0", -8, 8, true);
        row(5, 22, [&](int i, double bx, double by, double bw) {
            double angs[5] = { 30, 45, 90, 180, -90 };
            const char* nms[5] = { "30", "45", "90", "180", "-90" };
            button(bx, by, bw, 22, nms[i], g.rotAngle == angs[i], [angs, i] { g.rotAngle = angs[i]; });
        });
    } else if (g.transType == TT_REFLECT) {
        y -= 26; sliderRow(x0, y, w, 9, "He so m", -5, 5, true);
        y -= 26; sliderRow(x0, y, w, 10, "Tung do b", -8, 8, true);
        row(5, 22, [&](int i, double bx, double by, double bw) {
            if (i == 0) button(bx, by, bw, 22, "y=x+1", g.lineM == 1 && g.lineB == 1, [] { g.lineM = 1; g.lineB = 1; g.lineP1 = {0,1}; g.lineP2 = {3,4}; fitView(); });
            else if (i == 1) button(bx, by, bw, 22, "y = x", g.lineM == 1 && g.lineB == 0, [] { g.lineM = 1; g.lineB = 0; g.lineP1 = {0,0}; g.lineP2 = {4,4}; fitView(); });
            else if (i == 2) button(bx, by, bw, 22, "y = -x", g.lineM == -1 && g.lineB == 0, [] { g.lineM = -1; g.lineB = 0; g.lineP1 = {0,0}; g.lineP2 = {-4,4}; fitView(); });
            else if (i == 3) button(bx, by, bw, 22, "y = 0", g.lineM == 0 && g.lineB == 0, [] { g.lineM = 0; g.lineB = 0; g.lineP1 = {0,0}; g.lineP2 = {4,0}; fitView(); });
            else button(bx, by, bw, 22, "y = 2", g.lineM == 0 && g.lineB == 2, [] { g.lineM = 0; g.lineB = 2; g.lineP1 = {0,2}; g.lineP2 = {4,2}; fitView(); });
        });
    }

    // 3. PRESETS (Hinh hoc)
    sect("3. CHON HINH HOC (Phim 1-5)");
    const char* pn[5] = { "Tam giac ABC", "Nha Slide 113", "Hinh chu nhat", "Da giac lom", "Tu ve (Custom)" };
    row(3, 24, [&](int i, double bx, double by, double bw) {
        button(bx, by, bw, 24, pn[i], g.preset == i, [i] { loadPreset(i); });
    });
    row(2, 24, [&](int i, double bx, double by, double bw) {
        int idx = i + 3;
        button(bx, by, bw, 24, pn[idx], g.preset == idx, [idx] { loadPreset(idx); });
    });
    if (g.preset == 4) {
        row(3, 22, [&](int i, double bx, double by, double bw) {
            if (i == 0) button(bx, by, bw, 22, g.addMode ? "Them dinh: ON" : "Them dinh", g.addMode, [] { g.addMode = !g.addMode; });
            else if (i == 1) button(bx, by, bw, 22, "Undo dinh", false, [] { if (!g.src.empty()) g.src.pop_back(); g.vtx = 0; });
            else button(bx, by, bw, 22, "Xoa het", false, [] { g.src.clear(); g.vtx = 0; g.addMode = true; });
        });
    }

    // 4. STEP-BY-STEP PIPELINE
    sect("4. XEM TUNG BUOC PIPELINE (Phim Space / Backspace)");
    int maxS = getNumStages();
    row(4, 24, [&](int i, double bx, double by, double bw) {
        if (i == 0) button(bx, by, bw, 24, "Lui (Prev)", false, [] { prevStep(); });
        else if (i == 1) button(bx, by, bw, 24, "Toi (Next)", false, [] { nextStep(); });
        else if (i == 2) button(bx, by, bw, 24, "Dau tien", false, [] { g.stepMode = true; g.step = 0; });
        else button(bx, by, bw, 24, "Tat (Off)", !g.stepMode, [] { g.stepMode = false; });
    });
    y -= 16;
    txt(x0, y, g.stepMode ? "Giai doan " + std::to_string(g.step + 1) + " / " + std::to_string(maxS) : "Che do xem: Hien thi ket qua cuoi cung", hex(g.stepMode ? TRAN : TXT2), F18);
    y -= 10;
    for (int i = 0; i < maxS; i++) {
        double bw = (w - (maxS - 1) * 4) / maxS;
        fillRect(x0 + i * (bw + 4), y, bw, 5, hex(g.stepMode && i <= g.step ? TRAN : 0x3B4657));
    }
    if (g.stepMode) {
        ln(getStepTitle(g.step), hex(TXT));
        ln(getStepDesc(g.step), hex(TXT2));
    }

    // 5. ANH CUA TAM GIAC ABC
    sect("5. ANH CUA TAM GIAC ABC (Dinh goc -> Dinh anh)");
    if (!g.src.empty()) {
        auto fin = finalPts();
        for (size_t i = 0; i < g.src.size() && i < 4; i++) {
            std::string lineStr = vname(i) + pt2s(g.src[i]) + "  ->  " + vname(i) + "'" + pt2s(fin[i]);
            ln(lineStr, i == (size_t)g.vtx ? hex(PIV) : hex(TRAN));
        }
    }

    // 6. MA TRAN BIEN DOI 3x3 LIVE (Row vector)
    sect("6. MA TRAN BIEN DOI M (3x3 - Row Vector)");
    Mat3 M = currentMat();
    char rowBuf[128];
    snprintf(rowBuf, sizeof(rowBuf), "[ %6.2f  %6.2f  %6.2f ]", M.m[0][0], M.m[0][1], M.m[0][2]); ln(rowBuf, hex(TXT));
    snprintf(rowBuf, sizeof(rowBuf), "[ %6.2f  %6.2f  %6.2f ]", M.m[1][0], M.m[1][1], M.m[1][2]); ln(rowBuf, hex(TXT));
    snprintf(rowBuf, sizeof(rowBuf), "[ %6.2f  %6.2f  %6.2f ]", M.m[2][0], M.m[2][1], M.m[2][2]); ln(rowBuf, hex(PIV));

    // 7. DISPLAY TOGGLES
    sect("7. HIEN THI (Phim G / C / B / L)");
    row(4, 22, [&](int i, double bx, double by, double bw) {
        bool* flags[4] = { &g.grid, &g.corr, &g.bbox, &g.labels };
        const char* names[4] = { "Luoi", "Noi dinh", "BBox", "Nhan" };
        button(bx, by, bw, 22, names[i], *flags[i], [flags, i] { *flags[i] = !*flags[i]; });
    });

    txt(x0, 8, "Phim: T TinhTien  S TiLe  R Quay  M DoiXung  1-5 Hinh  Space Buoc", hex(TXT2));
}

static void display() {
    glViewport(0, 0, g.W, g.H);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0, g.W, 0, g.H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    glClearColor(17 / 255.f, 19 / 255.f, 24 / 255.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    g.btns.clear();
    g.slds.clear();

    drawViewport();
    drawHeader();
    drawPanel();

    glutSwapBuffers();
}

// ============================== 5. INPUT =====================================
static void reshape(int w, int h) {
    g.W = std::max(w, 950);
    g.H = std::max(h, 600);
    glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y) {
    g.mx = x; g.my = g.H - y; g.lx = g.mx; g.ly = g.my;
    if (state == GLUT_UP) {
        g.dragSlider = -1;
        g.panning = false;
        glutPostRedisplay();
        return;
    }
    int mod = glutGetModifiers();

    if (button == GLUT_MIDDLE_BUTTON) {
        if (inView(g.mx, g.my)) g.panning = true;
    } else if (button == GLUT_LEFT_BUTTON) {
        // Hit UI elements first
        for (auto& s : g.slds) {
            if (g.mx >= s.x && g.mx <= s.x + s.w && g.my >= s.y && g.my <= s.y + s.h) {
                g.dragSlider = s.id; sliderDrag(s); glutPostRedisplay(); return;
            }
        }
        for (auto& b : g.btns) {
            if (g.mx >= b.x && g.mx <= b.x + b.w && g.my >= b.y && g.my <= b.y + b.h) {
                auto fn = b.fn; fn(); glutPostRedisplay(); return;
            }
        }

        // Viewport interaction
        if (inView(g.mx, g.my)) {
            if (mod & GLUT_ACTIVE_SHIFT) {
                g.panning = true;
            } else {
                Pt w = s2w(g.mx, g.my);
                w.x = std::round(w.x * 4.0) / 4.0;
                w.y = std::round(w.y * 4.0) / 4.0;

                if (g.addMode) {
                    g.src.push_back(w);
                    g.vtx = (int)g.src.size() - 1;
                } else if (g.transType == TT_TRANSLATE) {
                    if (g.clickStep == 0) {
                        g.clickTempPt = w;
                        g.vStart = w;
                        g.clickStep = 1;
                    } else {
                        g.tx = std::round((w.x - g.clickTempPt.x) * 20.0) / 20.0;
                        g.ty = std::round((w.y - g.clickTempPt.y) * 20.0) / 20.0;
                        g.clickStep = 0;
                        g.stepMode = false;
                    }
                } else if (g.transType == TT_SCALE) {
                    g.scalePivX = w.x;
                    g.scalePivY = w.y;
                    g.stepMode = false;
                } else if (g.transType == TT_ROTATE) {
                    g.rotPivX = w.x;
                    g.rotPivY = w.y;
                    g.stepMode = false;
                } else if (g.transType == TT_REFLECT) {
                    if (g.clickStep == 0) {
                        g.clickTempPt = w;
                        g.lineP1 = w;
                        g.clickStep = 1;
                    } else {
                        g.lineP2 = w;
                        if (std::fabs(g.lineP2.x - g.clickTempPt.x) < 1e-3) {
                            g.lineP2.x += 0.25;
                        }
                        g.lineM = (g.lineP2.y - g.clickTempPt.y) / (g.lineP2.x - g.clickTempPt.x);
                        g.lineB = g.clickTempPt.y - g.lineM * g.clickTempPt.x;
                        g.lineM = std::round(g.lineM * 20.0) / 20.0;
                        g.lineB = std::round(g.lineB * 20.0) / 20.0;
                        g.clickStep = 0;
                        g.stepMode = false;
                    }
                }
            }
        }
    } else if (button == GLUT_RIGHT_BUTTON && inView(g.mx, g.my)) {
        // Select nearest vertex
        double best = 20; auto fin = finalPts();
        for (size_t i = 0; i < g.src.size(); i++) {
            for (const Pt& q : { g.src[i], fin[i] }) {
                Pt s = w2s(q);
                double d = std::hypot(s.x - g.mx, s.y - g.my);
                if (d < best) { best = d; g.vtx = (int)i; }
            }
        }
    }
    glutPostRedisplay();
}

static void motion(int x, int y) {
    g.mx = x; g.my = g.H - y;
    if (g.dragSlider >= 0) {
        for (auto& s : g.slds)
            if (s.id == g.dragSlider) sliderDrag(s);
    }
    if (g.panning) {
        g.cx -= (g.mx - g.lx) / g.ppu;
        g.cy -= (g.my - g.ly) / g.ppu;
    }
    g.lx = g.mx; g.ly = g.my;
    glutPostRedisplay();
}

static void wheel(int, int dir, int x, int y) {
    g.mx = x; g.my = g.H - y;
    if (!inView(g.mx, g.my)) return;
    Pt before = s2w(g.mx, g.my);
    g.ppu = std::max(8.0, std::min(400.0, g.ppu * std::pow(1.12, dir)));
    Pt after = s2w(g.mx, g.my);
    g.cx += before.x - after.x;
    g.cy += before.y - after.y;
    glutPostRedisplay();
}

static void keyboard(unsigned char k, int, int) {
    if (k >= 'A' && k <= 'Z') k = (unsigned char)(k - 'A' + 'a');
    switch (k) {
        case 't': setTransType(TT_TRANSLATE); break;
        case 's': setTransType(TT_SCALE); break;
        case 'q': case 'r': setTransType(TT_ROTATE); break;
        case 'm': case 'd': setTransType(TT_REFLECT); break;

        case '1': loadPreset(0); break;
        case '2': loadPreset(1); break;
        case '3': loadPreset(2); break;
        case '4': loadPreset(3); break;
        case '5': loadPreset(4); break;

        case 'f': fitView(); break;
        case 'g': g.grid = !g.grid; break;
        case 'c': g.corr = !g.corr; break;
        case 'b': g.bbox = !g.bbox; break;
        case 'l': g.labels = !g.labels; break;
        case 'h':
            g.pres = !g.pres;
            if (g.pres) { g.stepMode = true; g.step = 0; }
            fitView();
            break;
        case ' ': nextStep(); break;
        case 8: prevStep(); break;
        case 9: if (!g.src.empty()) g.vtx = (g.vtx + 1) % (int)g.src.size(); break;
        case 27:
            if (g.clickStep > 0) g.clickStep = 0;
            else if (g.addMode) g.addMode = false;
            else if (g.pres) g.pres = false;
            else if (g.stepMode) g.stepMode = false;
            break;
    }
    glutPostRedisplay();
}

static void special(int k, int, int) {
    bool shift = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
    if (k == GLUT_KEY_UP) g.sel = (g.sel + 10) % 11;
    else if (k == GLUT_KEY_DOWN) g.sel = (g.sel + 1) % 11;
    else if (k == GLUT_KEY_LEFT || k == GLUT_KEY_RIGHT) {
        double inc = (shift ? 1.0 : 0.25);
        if (g.sel == 6) inc = (shift ? 15.0 : 5.0); // angle
        if (k == GLUT_KEY_LEFT) inc = -inc;
        setParamVal(g.sel, getParamVal(g.sel) + inc);
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++)
        if (!std::strcmp(argv[i], "--test")) return runTests();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(g.W, g.H);
    glutCreateWindow("LAB 3 - 2D AFFINE TRANSFORMATIONS (VKU Computer Graphics)");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    loadPreset(0); // Khoi dong voi Tam Giac ABC

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutMouseWheelFunc(wheel);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);

    glutMainLoop();
    return 0;
}
