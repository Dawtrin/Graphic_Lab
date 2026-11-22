// =============================================================================
// TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN (VKU)
// KHOA KY THUAT MAY TINH VA DIEN TU - BO MON DO HOA MAY TINH
// =============================================================================
// LAB 5: CHUONG TRINH VE CAC DUONG VA MAT CONG TRONG KHONG GIAN 3D
//        (3D CURVES AND SURFACES LAB: RULED, REVOLUTION, BEZIER CURVE & SURFACE)
//
// YEU CAU DE BAI:
// 5.1. Cac mat co quy tac (Ruled Surfaces): Cylinder, Cone, Hyperbolic Paraboloid,...
// 5.2. Cac mat tron xoay (Surfaces of Revolution): Sphere, Ellipsoid, Hyperboloid, Toroid,...
// 5.3. Duong cong Bezier (Bezier Curves): Da thuc Bernstein & Thuat toan De Casteljau
// 5.4. Mat cong Bezier (Bezier Surfaces): Lưới diem kiem soat 4x4 (Bicubic Bezier Surface)
//
// YEU CAU TUONG TAC:
// - Dung su kien phim, chuot de dieu chinh goc quay khi quan sat (Yaw, Pitch, Orbit)
// - Phong to, thu nho vat the (Zoom & Scaling)
// - Thay doi mau sac vat the (Presets & RGB Sliders), do trong suot (Opacity), do bong
//
// BIEN DICH VA CHAY:
// - Bien dich: cmake -B build && cmake --build build --target Lab_5_main
// - Chay do hoa: build\Lab_5_main.exe
// - Chay kiem thu toan hoc: build\Lab_5_main.exe --test
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

// =============================================================================
// 1. TOAN HOC 3D & MA TRAN (3D MATH & MATRICES)
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

static Vec3 lerp(const Vec3& a, const Vec3& b, double t) {
    return a + (b - a) * t;
}

// Ma tran 4x4 theo chuan Row-vector VKU: P = [x y z 1], P' = P x M
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

static Mat4 rotationEuler4(double degX, double degY, double degZ) {
    double rx = degX * PI / 180.0;
    double ry = degY * PI / 180.0;
    double rz = degZ * PI / 180.0;
    return rotZ4(rz) * rotY4(ry) * rotX4(rx);
}

static Mat4 translation4(double tx, double ty, double tz) {
    Mat4 r = ident4();
    r.m[3][0] = tx;
    r.m[3][1] = ty;
    r.m[3][2] = tz;
    return r;
}

static Mat4 scaling4(double sx, double sy, double sz) {
    Mat4 r = ident4();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    r.m[2][2] = sz;
    return r;
}

static Vec3 computeEyePosition(const Vec3& target, double thetaDeg, double phiDeg, double radius) {
    double theta = thetaDeg * PI / 180.0;
    double phi   = phiDeg   * PI / 180.0;
    double x = target.x + radius * std::cos(phi) * std::sin(theta);
    double y = target.y + radius * std::sin(phi);
    double z = target.z + radius * std::cos(phi) * std::cos(theta);
    return { x, y, z };
}

// =============================================================================
// 2. CAU TRUC DU LIEU MAT CONG VA LUOI DA GIAC (SURFACE & MESH STRUCTURES)
// =============================================================================

struct VertexUV {
    Vec3 pos;
    Vec3 norm;
    double u = 0, v = 0;
};

struct SurfaceMesh {
    std::string name;
    std::string equation;
    std::vector<std::vector<VertexUV>> grid; // [i][j] theo 2 tham so u, v
    std::vector<std::pair<Vec3, Vec3>> ruleLines; // Duong sinh thang (cho mat co quy tac)
    std::vector<std::vector<Vec3>> capFaces; // Cac mat day/nap (triangle fans)
};

enum LabSection {
    SECTION_RULED       = 0, // 5.1. Cac mat co quy tac
    SECTION_REVOLUTION  = 1, // 5.2. Cac mat tron xoay
    SECTION_BEZIER_CURV = 2, // 5.3. Duong cong Bezier
    SECTION_BEZIER_SURF = 3  // 5.4. Mat cong Bezier
};

enum RuledType {
    RULED_CYLINDER   = 0,
    RULED_CONE       = 1,
    RULED_HYPER_PARA = 2, // Hyperbolic Paraboloid (Yen ngua)
    RULED_CONOID     = 3
};

enum RevType {
    REV_SPHERE          = 0,
    REV_ELLIPSOID       = 1,
    REV_HYPERBOLOID     = 2, // Hyperboloid tron xoay 1 tang (One-sheet)
    REV_HYPERBOLOID_2   = 3, // Hyperboloid tron xoay 2 tang (Two-sheet)
    REV_TOROID          = 4, // Torus (Mat xuyen)
    REV_PARABOLOID      = 5
};

enum BezierCurveType {
    CURVE_CUBIC_S     = 0, // Bậc 3 hinh chu S
    CURVE_CUBIC_ARCH  = 1, // Bậc 3 vom cao
    CURVE_QUADRATIC   = 2, // Bậc 2 parabol
    CURVE_3D_SPIRAL   = 3  // Bậc 4 xoan luon trong khong gian 3D
};

enum BezierSurfacePreset {
    SURF_PRESET_WAVE   = 0, // Song luon doi nui
    SURF_PRESET_SADDLE = 1, // Yen ngua muot
    SURF_PRESET_DOME   = 2, // Vom non tron mem
    SURF_PRESET_VALLEY = 3  // Mang trung doi
};

enum RenderStyle {
    STYLE_SOLID     = 0,
    STYLE_WIREFRAME = 1,
    STYLE_BOTH      = 2
};

// =============================================================================
// 3. THUAT TOAN TOAN HOC CHO LAB 5
// =============================================================================

// --- 5.1: CAC MAT CO QUY TAC (RULED SURFACES) ---
// Mot mat goi la mat co quy tac neu qua moi diem tren mat do co it nhat mot duong thang
// nam tron ven tren mat (duong sinh - generatrix / ruling line).

// 1. Mat tru (Cylinder): x = R*cos(u), y = v, z = R*sin(u)
static SurfaceMesh createCylinder(double R, double H, int slices = 36, int stacks = 12) {
    SurfaceMesh m;
    m.name = "Mat tru tron (Cylinder)";
    m.equation = "x = R*cos(u),  y = v,  z = R*sin(u)  |  u in [0, 2*pi], v in [-H/2, H/2]";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    for (int j = 0; j <= stacks; j++) {
        double v = -H * 0.5 + H * ((double)j / stacks);
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices;
            double cu = std::cos(u);
            double su = std::sin(u);
            Vec3 pos{ R * cu, v, R * su };
            Vec3 norm{ cu, 0, su };
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }

    // Cac duong sinh thang doc theo mat tru
    int ruleCount = 16;
    for (int k = 0; k < ruleCount; k++) {
        double u = k * 2.0 * PI / ruleCount;
        Vec3 p0{ R * std::cos(u), -H * 0.5, R * std::sin(u) };
        Vec3 p1{ R * std::cos(u),  H * 0.5, R * std::sin(u) };
        m.ruleLines.push_back({ p0, p1 });
    }

    // Nap day duoi va nap day tren
    std::vector<Vec3> botCap, topCap;
    botCap.push_back({ 0, -H * 0.5, 0 });
    topCap.push_back({ 0,  H * 0.5, 0 });
    for (int i = 0; i <= slices; i++) {
        double u = i * 2.0 * PI / slices;
        botCap.push_back({ R * std::cos(-u), -H * 0.5, R * std::sin(-u) });
        topCap.push_back({ R * std::cos(u),   H * 0.5, R * std::sin(u) });
    }
    m.capFaces.push_back(botCap);
    m.capFaces.push_back(topCap);

    return m;
}

// 2. Mat non (Cone): x = (1 - s)*R*cos(u), y = -H/2 + s*H, z = (1 - s)*R*sin(u)
static SurfaceMesh createCone(double R, double H, int slices = 36, int stacks = 12) {
    SurfaceMesh m;
    m.name = "Mat non tron (Cone)";
    m.equation = "x = (1 - s)*R*cos(u),  y = -H/2 + s*H,  z = (1 - s)*R*sin(u)  (s in [0, 1])";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    double sideHypot = std::sqrt(R * R + H * H);
    double ny = R / sideHypot;
    double nr = H / sideHypot;

    for (int j = 0; j <= stacks; j++) {
        double s = (double)j / stacks; // 0 o day, 1 o dinh
        double r = (1.0 - s) * R;
        double y = -H * 0.5 + s * H;
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices;
            double cu = std::cos(u);
            double su = std::sin(u);
            Vec3 pos{ r * cu, y, r * su };
            Vec3 norm{ nr * cu, ny, nr * su };
            if (s > 0.999) norm = { 0, 1, 0 };
            m.grid[j][i] = { pos, normalize(norm), (double)i / slices, s };
        }
    }

    // Cac duong sinh noi tu dinh non toi vong day
    int ruleCount = 16;
    Vec3 apex{ 0, H * 0.5, 0 };
    for (int k = 0; k < ruleCount; k++) {
        double u = k * 2.0 * PI / ruleCount;
        Vec3 basePt{ R * std::cos(u), -H * 0.5, R * std::sin(u) };
        m.ruleLines.push_back({ apex, basePt });
    }

    // Nap day tron
    std::vector<Vec3> botCap;
    botCap.push_back({ 0, -H * 0.5, 0 });
    for (int i = 0; i <= slices; i++) {
        double u = i * 2.0 * PI / slices;
        botCap.push_back({ R * std::cos(-u), -H * 0.5, R * std::sin(-u) });
    }
    m.capFaces.push_back(botCap);

    return m;
}

// 3. Mat yen ngua (Hyperbolic Paraboloid - Mat ke doi): z = (x^2 / a^2) - (y^2 / b^2)
static SurfaceMesh createHyperbolicParaboloid(double a, double b, double size = 2.0, int res = 24) {
    SurfaceMesh m;
    m.name = "Mat Yen ngua (Hyperbolic Paraboloid - Mat ke doi)";
    m.equation = "z = (x^2 / a^2) - (y^2 / b^2)  hoac  x+z=u, x-z=v, y=u*v/c (2 he duong sinh)";

    m.grid.resize(res + 1, std::vector<VertexUV>(res + 1));
    for (int j = 0; j <= res; j++) {
        double y = -size + 2.0 * size * ((double)j / res);
        for (int i = 0; i <= res; i++) {
            double x = -size + 2.0 * size * ((double)i / res);
            double z = (x * x) / (a * a) - (y * y) / (b * b);
            // Dao ham tinh phap tuyen: dz/dx = 2x/a^2, dz/dy = -2y/b^2
            Vec3 norm = normalize(Vec3{ -2.0 * x / (a * a), 2.0 * y / (b * b), 1.0 });
            m.grid[j][i] = { { x, z, y }, norm, (double)i / res, (double)j / res };
        }
    }

    // 2 he duong sinh thang (Family 1 & Family 2 ruling lines)
    int nLines = 10;
    for (int k = 0; k <= nLines; k++) {
        double u = -size + 2.0 * size * ((double)k / nLines);
        // He duong thang sinh 1: x/a + y/b = u, z = u * (x/a - y/b)
        Vec3 p0{ u, (u * u) / (a * a) - (size * size) / (b * b), -size };
        Vec3 p1{ u, (u * u) / (a * a) - (size * size) / (b * b),  size };
        m.ruleLines.push_back({ p0, p1 });

        Vec3 q0{ -size, (size * size) / (a * a) - (u * u) / (b * b), u };
        Vec3 q1{  size, (size * size) / (a * a) - (u * u) / (b * b), u };
        m.ruleLines.push_back({ q0, q1 });
    }

    return m;
}

// 4. Mat Conoid: Sinh boi duong thang chuyen dong luon song
static SurfaceMesh createConoid(double R, double H, int slices = 32, int stacks = 14) {
    SurfaceMesh m;
    m.name = "Mat Conoid (Ruled Conoid)";
    m.equation = "x = v*cos(u),  y = v*sin(u),  z = H*sin(2*u)  (Duong sinh song song mat phang)";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    for (int j = 0; j <= stacks; j++) {
        double v = ((double)j / stacks) * R;
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices;
            double x = v * std::cos(u);
            double z = v * std::sin(u);
            double y = H * 0.5 * std::sin(2.0 * u);
            Vec3 pos{ x, y, z };
            Vec3 norm = normalize(Vec3{ -H * std::cos(2 * u) * std::sin(u), 1.0, H * std::cos(2 * u) * std::cos(u) });
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }

    int ruleCount = 18;
    for (int k = 0; k < ruleCount; k++) {
        double u = k * 2.0 * PI / ruleCount;
        Vec3 p0{ 0, H * 0.5 * std::sin(2.0 * u), 0 };
        Vec3 p1{ R * std::cos(u), H * 0.5 * std::sin(2.0 * u), R * std::sin(u) };
        m.ruleLines.push_back({ p0, p1 });
    }

    return m;
}

// --- 5.2: CAC MAT TRON XOAY (SURFACES OF REVOLUTION) ---
// Tao nen bang cach quay mot duong cong phang quanh mot truc co dinh.

// 1. Mat cau (Sphere): x = R*cos(v)*cos(u), y = R*sin(v), z = R*cos(v)*sin(u)
static SurfaceMesh createSphere(double R, int slices = 36, int stacks = 20) {
    SurfaceMesh m;
    m.name = "Mat cau (Sphere)";
    m.equation = "x = R*cos(v)*cos(u),  y = R*sin(v),  z = R*cos(v)*sin(u)  |  v in [-pi/2, pi/2]";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    for (int j = 0; j <= stacks; j++) {
        double v = -PI * 0.5 + PI * ((double)j / stacks); // Vi tuyen
        double cv = std::cos(v), sv = std::sin(v);
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices; // Kinh tuyen
            double cu = std::cos(u), su = std::sin(u);
            Vec3 pos{ R * cv * cu, R * sv, R * cv * su };
            Vec3 norm{ cv * cu, sv, cv * su };
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }
    return m;
}

// 2. Mat Ellipsoid: x = a*cos(v)*cos(u), y = b*sin(v), z = c*cos(v)*sin(u)
static SurfaceMesh createEllipsoid(double a, double b, double c, int slices = 36, int stacks = 20) {
    SurfaceMesh m;
    m.name = "Mat Ellipsoid";
    m.equation = "x = a*cos(v)*cos(u),  y = b*sin(v),  z = c*cos(v)*sin(u)  |  (x/a)^2 + (y/b)^2 + (z/c)^2 = 1";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    for (int j = 0; j <= stacks; j++) {
        double v = -PI * 0.5 + PI * ((double)j / stacks);
        double cv = std::cos(v), sv = std::sin(v);
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices;
            double cu = std::cos(u), su = std::sin(u);
            Vec3 pos{ a * cv * cu, b * sv, c * cv * su };
            Vec3 norm = normalize(Vec3{ (cv * cu) / a, sv / b, (cv * su) / c });
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }
    return m;
}

// 3. Mat Hyperboloid tron xoay 1 tang (One-sheet Hyperboloid of Revolution)
static SurfaceMesh createHyperboloidOneSheet(double a, double b, double H = 3.0, int slices = 36, int stacks = 20) {
    SurfaceMesh m;
    m.name = "Mat Hyperboloid 1 tang (One-sheet Hyperboloid)";
    m.equation = "x^2/a^2 + z^2/a^2 - y^2/b^2 = 1  |  r(y) = a*sqrt(1 + (y/b)^2), quay quanh Oy";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    for (int j = 0; j <= stacks; j++) {
        double y = -H * 0.5 + H * ((double)j / stacks);
        double r = a * std::sqrt(1.0 + (y * y) / (b * b));
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices;
            double cu = std::cos(u), su = std::sin(u);
            Vec3 pos{ r * cu, y, r * su };
            // Phap tuyen: grad(F) = (2x/a^2, -2y/b^2, 2z/a^2)
            Vec3 norm = normalize(Vec3{ pos.x / (a * a), -pos.y / (b * b), pos.z / (a * a) });
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }

    // Hyperboloid 1 tang cung la mat ke doi: them duong sinh thang xoan tuyet dep!
    int ruleCount = 16;
    for (int k = 0; k < ruleCount; k++) {
        double u = k * 2.0 * PI / ruleCount;
        double y0 = -H * 0.5, y1 = H * 0.5;
        double r0 = a * std::sqrt(1.0 + (y0 * y0) / (b * b));
        double r1 = a * std::sqrt(1.0 + (y1 * y1) / (b * b));
        double deltaU = std::atan2(H * 0.5, b);
        Vec3 p0{ r0 * std::cos(u - deltaU), y0, r0 * std::sin(u - deltaU) };
        Vec3 p1{ r1 * std::cos(u + deltaU), y1, r1 * std::sin(u + deltaU) };
        m.ruleLines.push_back({ p0, p1 });
    }

    return m;
}

// 4. Mat Hyperboloid tron xoay 2 tang (Two-sheet Hyperboloid of Revolution)
static SurfaceMesh createHyperboloidTwoSheets(double a, double b, double H = 3.6, int slices = 36, int stacks = 14) {
    SurfaceMesh m;
    m.name = "Mat Hyperboloid 2 tang (Two-sheet Hyperboloid)";
    m.equation = "y^2/b^2 - (x^2+z^2)/a^2 = 1  |  2 ban phan rieng biet doi xung qua goc O";

    double yMin = b;
    double yMax = H * 0.5;
    if (yMax < yMin + 0.3) yMax = yMin + 1.2;

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    for (int j = 0; j <= stacks; j++) {
        double y = yMin + (yMax - yMin) * ((double)j / stacks);
        double r = a * std::sqrt(std::max(0.0, (y * y) / (b * b) - 1.0));
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices;
            double cu = std::cos(u), su = std::sin(u);
            Vec3 pos{ r * cu, y, r * su };
            Vec3 norm = normalize(Vec3{ -pos.x / (a * a), pos.y / (b * b), -pos.z / (a * a) });
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }

    // Ban phan doi xung nua duoi (-Y) dung cac duong sinh doc de the hien
    for (int k = 0; k < 16; k++) {
        double u = k * 2.0 * PI / 16;
        double cu = std::cos(u), su = std::sin(u);
        for (int j = 0; j < stacks; j++) {
            double y1 = -(yMin + (yMax - yMin) * ((double)j / stacks));
            double y2 = -(yMin + (yMax - yMin) * ((double)(j + 1) / stacks));
            double r1 = a * std::sqrt(std::max(0.0, (y1 * y1) / (b * b) - 1.0));
            double r2 = a * std::sqrt(std::max(0.0, (y2 * y2) / (b * b) - 1.0));
            m.ruleLines.push_back({ { r1 * cu, y1, r1 * su }, { r2 * cu, y2, r2 * su } });
        }
    }

    return m;
}

// 5. Mat xuyen (Torus / Toroid): Quay duong tron quanh truc dong phang
static SurfaceMesh createToroid(double R, double r, int slices = 40, int stacks = 24) {
    SurfaceMesh m;
    m.name = "Mat xuyen (Toroid / Torus)";
    m.equation = "x = (R + r*cos(v))*cos(u),  y = r*sin(v),  z = (R + r*cos(v))*sin(u)";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    for (int j = 0; j <= stacks; j++) {
        double v = j * 2.0 * PI / stacks; // Goc quanh tiet dien ong
        double cv = std::cos(v), sv = std::sin(v);
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices; // Goc xoay quanh truc Oy
            double cu = std::cos(u), su = std::sin(u);
            Vec3 pos{ (R + r * cv) * cu, r * sv, (R + r * cv) * su };
            Vec3 norm{ cv * cu, sv, cv * su };
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }
    return m;
}

// 5. Mat Paraboloid tron xoay (Paraboloid of Revolution)
static SurfaceMesh createParaboloid(double R, double H, int slices = 36, int stacks = 16) {
    SurfaceMesh m;
    m.name = "Mat Paraboloid tron xoay";
    m.equation = "y = (x^2 + z^2) * k  |  Quay parabol quanh truc doi xung Oy";

    m.grid.resize(stacks + 1, std::vector<VertexUV>(slices + 1));
    double k = H / (R * R);
    for (int j = 0; j <= stacks; j++) {
        double r = R * std::sqrt((double)j / stacks);
        double y = -H * 0.5 + k * r * r;
        for (int i = 0; i <= slices; i++) {
            double u = i * 2.0 * PI / slices;
            double cu = std::cos(u), su = std::sin(u);
            Vec3 pos{ r * cu, y, r * su };
            Vec3 norm = normalize(Vec3{ -2.0 * k * r * cu, 1.0, -2.0 * k * r * su });
            m.grid[j][i] = { pos, norm, (double)i / slices, (double)j / stacks };
        }
    }
    return m;
}

// --- 5.3: DUONG CONG BEZIER (BEZIER CURVES) ---
// Cong thuc da thuc Bernstein: B_{i,n}(t) = C(n, i) * (1-t)^(n-i) * t^i
// Cong thuc De Casteljau: P_i^(k)(t) = (1-t)*P_i^(k-1) + t*P_{i+1}^(k-1)

static double combination(int n, int k) {
    if (k < 0 || k > n) return 0.0;
    if (k == 0 || k == n) return 1.0;
    double res = 1.0;
    for (int i = 1; i <= k; i++) {
        res = res * (n - i + 1) / i;
    }
    return res;
}

static double bernstein(int i, int n, double t) {
    return combination(n, i) * std::pow(1.0 - t, n - i) * std::pow(t, i);
}

// Tinh diem tren duong cong Bezier bang da thuc Bernstein
static Vec3 evaluateBezierCurveBernstein(const std::vector<Vec3>& ctrlPts, double t) {
    int n = (int)ctrlPts.size() - 1;
    Vec3 pt{ 0, 0, 0 };
    for (int i = 0; i <= n; i++) {
        double b = bernstein(i, n, t);
        pt = pt + ctrlPts[i] * b;
    }
    return pt;
}

// Thuat toan De Casteljau: Tra ve tat ca cac cap diem trung gian de truc quan hoa
static std::vector<std::vector<Vec3>> deCasteljauSteps(const std::vector<Vec3>& ctrlPts, double t) {
    std::vector<std::vector<Vec3>> levels;
    levels.push_back(ctrlPts);
    int n = (int)ctrlPts.size();
    for (int k = 1; k < n; k++) {
        std::vector<Vec3> nextLevel;
        const auto& prev = levels.back();
        for (size_t i = 0; i + 1 < prev.size(); i++) {
            nextLevel.push_back(lerp(prev[i], prev[i + 1], t));
        }
        levels.push_back(nextLevel);
    }
    return levels;
}

// --- 5.4: MAT CONG BEZIER (BEZIER SURFACES) ---
// Mat cong Bicubic Bezier bac 3x3 gom 16 diem kiem soat P_{i, j} (i=0..3, j=0..3)
// S(u, v) = sum_{i=0}^3 sum_{j=0}^3 B_{i,3}(u) * B_{j,3}(v) * P_{i, j}

struct BezierSurfaceModel {
    std::string name;
    std::string equation = "S(u,v) = sum_{i=0}^3 sum_{j=0}^3 B_i^3(u)*B_j^3(v)*P_{i,j}  (u,v in [0, 1])";
    std::array<std::array<Vec3, 4>, 4> controlNet;
    int selectedI = 1;
    int selectedJ = 1;
};

// Dao ham bac 1 cua da thuc Bernstein bac 3: dB_i^3(t)/dt
static double dBernstein3(int i, double t) {
    switch (i) {
        case 0: return -3.0 * (1.0 - t) * (1.0 - t);
        case 1: return 3.0 * (1.0 - t) * (1.0 - t) - 6.0 * t * (1.0 - t);
        case 2: return 6.0 * t * (1.0 - t) - 3.0 * t * t;
        case 3: return 3.0 * t * t;
        default: return 0.0;
    }
}

// Tinh diem va vector phap tuyen tren mat cong Bezier
static VertexUV evaluateBezierSurfacePoint(const std::array<std::array<Vec3, 4>, 4>& net, double u, double v) {
    Vec3 pt{ 0, 0, 0 };
    Vec3 du{ 0, 0, 0 };
    Vec3 dv{ 0, 0, 0 };

    double bu[4], bv[4], dbu[4], dbv[4];
    for (int i = 0; i < 4; i++) {
        bu[i]  = bernstein(i, 3, u);
        bv[i]  = bernstein(i, 3, v);
        dbu[i] = dBernstein3(i, u);
        dbv[i] = dBernstein3(i, v);
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const Vec3& P = net[i][j];
            pt = pt + P * (bu[i] * bv[j]);
            du = du + P * (dbu[i] * bv[j]);
            dv = dv + P * (bu[i] * dbv[j]);
        }
    }

    Vec3 norm = normalize(cross(dv, du));
    return { pt, norm, u, v };
}

static SurfaceMesh generateBezierSurfaceMesh(const std::array<std::array<Vec3, 4>, 4>& net, int resU = 24, int resV = 24) {
    SurfaceMesh m;
    m.name = "Mat cong Bicubic Bezier 4x4";
    m.equation = "S(u,v) = sum_{i=0}^3 sum_{j=0}^3 B_i^3(u)*B_j^3(v)*P_{i,j}  (16 diem kiem soat)";

    m.grid.resize(resV + 1, std::vector<VertexUV>(resU + 1));
    for (int j = 0; j <= resV; j++) {
        double v = (double)j / resV;
        for (int i = 0; i <= resU; i++) {
            double u = (double)i / resU;
            m.grid[j][i] = evaluateBezierSurfacePoint(net, u, v);
        }
    }
    return m;
}

// =============================================================================
// 4. PRESETS CHO DUONG VA MAT CONG BEZIER
// =============================================================================

static std::vector<Vec3> createBezierCurvePreset(BezierCurveType t) {
    switch (t) {
        case CURVE_CUBIC_S:
            return {
                { -2.4, -1.6,  0.0 }, // P0
                { -1.8,  2.2,  1.2 }, // P1
                {  1.8, -2.2, -1.2 }, // P2
                {  2.4,  1.6,  0.0 }  // P3
            };
        case CURVE_CUBIC_ARCH:
            return {
                { -2.5, -1.5,  0.0 }, // P0
                { -1.5,  2.5,  1.5 }, // P1
                {  1.5,  2.5, -1.5 }, // P2
                {  2.5, -1.5,  0.0 }  // P3
            };
        case CURVE_QUADRATIC:
            return {
                { -2.2, -1.2, -1.0 }, // P0
                {  0.0,  2.4,  1.5 }, // P1
                {  2.2, -1.2, -1.0 }  // P2
            };
        case CURVE_3D_SPIRAL:
            return {
                { -2.5, -1.8, -1.5 }, // P0
                { -1.0,  1.8, -2.0 }, // P1
                {  1.5,  2.2,  1.0 }, // P2
                {  2.5, -0.5,  1.8 }, // P3
                {  0.5, -2.0,  0.0 }  // P4
            };
    }
    return { { -2, 0, 0 }, { -1, 2, 0 }, { 1, 2, 0 }, { 2, 0, 0 } };
}

static std::array<std::array<Vec3, 4>, 4> createBezierSurfacePreset(BezierSurfacePreset p) {
    std::array<std::array<Vec3, 4>, 4> net;
    double step = 1.4;
    for (int i = 0; i < 4; i++) {
        double x = (i - 1.5) * step;
        for (int j = 0; j < 4; j++) {
            double z = (j - 1.5) * step;
            double y = 0.0;
            if (p == SURF_PRESET_WAVE) {
                y = 1.2 * std::sin(x * 1.2) * std::cos(z * 1.2);
            } else if (p == SURF_PRESET_SADDLE) {
                y = 0.45 * (x * x - z * z);
            } else if (p == SURF_PRESET_DOME) {
                double r2 = x * x + z * z;
                y = 2.0 * std::exp(-r2 * 0.35) - 0.6;
            } else if (p == SURF_PRESET_VALLEY) {
                y = 0.9 * std::cos(x * 1.5);
            }
            net[i][j] = { x, y, z };
        }
    }
    return net;
}

// =============================================================================
// 5. APPLICATION STATE & THEME PALETTES
// =============================================================================

struct ColorRGBA {
    float r, g, b, a;
};

static ColorRGBA hexCol(unsigned h, float a = 1.0f) {
    return {
        ((h >> 16) & 255) / 255.0f,
        ((h >> 8) & 255)  / 255.0f,
        (h & 255)         / 255.0f,
        a
    };
}

static const unsigned BG_DARK_COLOR   = 0x0B0F14;
static const unsigned PANEL_BG_COLOR  = 0x141A22;
static const unsigned CARD_BG_COLOR   = 0x1C2430;
static const unsigned ACCENT_CYAN     = 0x00E5BE;
static const unsigned ACCENT_ORANGE   = 0xFF9F43;
static const unsigned ACCENT_YELLOW   = 0xFFD166;
static const unsigned TEXT_MAIN       = 0xF0F6FC;
static const unsigned TEXT_MUTED      = 0x8B949E;
static const unsigned BORDER_LINE     = 0x2D3748;

struct ColorPreset {
    const char* name;
    unsigned hexColor;
};

static const std::vector<ColorPreset> COLOR_PRESETS = {
    { "Cyan Ngoc",   0x00D2D3 },
    { "Do San Ho",   0xFF6B6B },
    { "Luc Bao",     0x10AC84 },
    { "Vang Kim",    0xFFD166 },
    { "Xanh Duong",  0x3A86FF },
    { "Cam Chay",    0xFF9F43 },
    { "Tim Hoa Ca",  0x9D4EDD },
    { "Bac Titan",   0xC8D6E5 }
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
    // Chuyen muc Lab 5
    LabSection activeSection = SECTION_RULED;

    // 5.1. Mat co quy tac
    RuledType ruledType = RULED_CYLINDER;
    double cylinderR = 1.4, cylinderH = 2.8;
    double coneR = 1.6, coneH = 2.8;
    double hpA = 1.2, hpB = 1.2, hpSize = 1.8;
    double conoidR = 1.8, conoidH = 1.4;

    // 5.2. Mat tron xoay
    RevType revType = REV_SPHERE;
    double sphereR = 1.7;
    double ellipA = 2.0, ellipB = 1.2, ellipC = 1.5;
    double hyperA = 1.1, hyperB = 1.4, hyperH = 3.0;
    double torusR = 1.8, torusTubeR = 0.65;
    double paraR = 1.8, paraH = 2.5;

    // 5.3. Duong cong Bezier
    BezierCurveType curveType = CURVE_CUBIC_S;
    std::vector<Vec3> curveCtrlPts;
    int selectedCurvePtIdx = 1;
    double curveT = 0.5;
    bool animateCurveT = false;
    double animSpeed = 0.4;
    int animDir = 1;

    // 5.4. Mat cong Bezier
    BezierSurfacePreset surfPreset = SURF_PRESET_WAVE;
    std::array<std::array<Vec3, 4>, 4> surfControlNet;
    int surfSelI = 1;
    int surfSelJ = 1;

    // Do min luoi tham so UV (Slices / Stacks)
    int uvSlices = 36;
    int uvStacks = 18;

    // He quan sat & Camera (Look-At Orbit)
    double thetaDeg = 45.0; // Azimuth
    double phiDeg   = 25.0; // Elevation
    double eyeRadius = 9.0; // Distance / Zoom
    Vec3 target = { 0.0, 0.0, 0.0 };
    double objScale = 1.0;  // Scale vat the
    bool autoOrbit = false; // Tu dong xoay

    // Kieu bieu dien & Hieu ung
    RenderStyle renderStyle = STYLE_BOTH;
    int colorPresetIdx = 0;
    double customR = 0.0, customG = 0.82, customB = 0.82; // RGB [0..1]
    double faceOpacity = 0.92;
    double shininess = 40.0;

    // Toggle hien thi bo tro
    bool showAxes = true;
    bool showGrid = true;
    bool showNormals = false;
    bool showRules = true;
    bool showControlNet = true;
    bool showDeCasteljau = true;
    bool showHUD = true;

    // Kich thuoc cua so & Chuot
    int windowW = 1400, windowH = 880;
    double mouseX = 0, mouseY = 0;
    double lastMouseX = 0, lastMouseY = 0;
    bool isLeftDragging = false;
    bool isRightDragging = false;
    int activeSliderId = -1;

    std::vector<UIControlBtn> buttons;
    std::vector<UIControlSlider> sliders;
} g;

static const double HEADER_H = 60.0;
static double sidePanelW() { return 460.0; }
static double viewportW()  { return g.windowW - sidePanelW(); }
static double viewportH()  { return g.windowH - HEADER_H; }

// =============================================================================
// 6. TU DONG KIEM THU TOAN HOC (SELF-TESTS SUITE --test)
// =============================================================================

static int runSelfTests() {
    int total = 0, fail = 0;
    auto chk = [&](bool c, const char* name) {
        total++;
        if (!c) fail++;
        printf(" [%s] %s\n", c ? "PASS" : "FAIL", name);
    };
    auto eqV = [](const Vec3& a, const Vec3& b, double eps = 1e-5) {
        return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps && std::fabs(a.z - b.z) < eps;
    };
    auto eqD = [](double a, double b, double eps = 1e-5) {
        return std::fabs(a - b) < eps;
    };

    printf("=================================================================\n");
    printf("   VKU LAB 5: 3D CURVES & SURFACES MATH SELF-TESTS SUITE         \n");
    printf("=================================================================\n");

    // --- KIEM THU 5.1: CAC MAT CO QUY TAC ---
    printf("\n--- KIEM THU 5.1: CAC MAT CO QUY TAC (RULED SURFACES) ---\n");
    {
        SurfaceMesh cyl = createCylinder(1.5, 3.0, 32, 10);
        chk(!cyl.grid.empty(), "5.1.1: Mat tru (Cylinder) khoi tao luoi UV thanh cong");
        chk(cyl.grid.size() == 11 && cyl.grid[0].size() == 33, "5.1.2: Do phan giai luoi mat tru dung stacks/slices");
        // Kiem tra ban kinh tai cac diem
        bool radOk = true;
        for (const auto& row : cyl.grid) {
            for (const auto& vtx : row) {
                double r = std::sqrt(vtx.pos.x * vtx.pos.x + vtx.pos.z * vtx.pos.z);
                if (std::fabs(r - 1.5) > 1e-4) radOk = false;
            }
        }
        chk(radOk, "5.1.3: Moi diem tren mat tru deu co ban kinh R = 1.5");
        chk(!cyl.ruleLines.empty(), "5.1.4: Mat tru co day du cac duong sinh thang (Rule lines)");
        chk(cyl.capFaces.size() == 2, "5.1.5: Mat tru co 2 mat day/nap tron (Caps)");
    }

    {
        SurfaceMesh cone = createCone(1.8, 3.2, 32, 10);
        chk(!cone.grid.empty(), "5.1.6: Mat non (Cone) khoi tao thanh cong");
        // Dinh non tai y = +1.6, r = 0
        const auto& apexPt = cone.grid.back()[0];
        chk(eqD(apexPt.pos.y, 1.6) && eqD(apexPt.pos.x, 0.0) && eqD(apexPt.pos.z, 0.0),
            "5.1.7: Dinh non nam dung toa do (0, H/2, 0)");
        // Day non tai y = -1.6, r = 1.8
        const auto& basePt = cone.grid.front()[0];
        double baseR = std::sqrt(basePt.pos.x * basePt.pos.x + basePt.pos.z * basePt.pos.z);
        chk(eqD(basePt.pos.y, -1.6) && eqD(baseR, 1.8),
            "5.1.8: Day non nam tai y = -H/2 voi dung ban kinh R = 1.8");
    }

    {
        SurfaceMesh hp = createHyperbolicParaboloid(1.0, 1.0, 2.0, 16);
        chk(!hp.grid.empty(), "5.1.9: Mat Yen ngua (Hyperbolic Paraboloid) tao thanh cong");
        // Kiem tra phuong trinh z = x^2/a^2 - y^2/b^2 tai tam goc toa do (0,0)
        Vec3 mid = hp.grid[8][8].pos;
        chk(eqD(mid.x, 0.0) && eqD(mid.y, 0.0) && eqD(mid.z, 0.0),
            "5.1.10: Diem giua yen ngua di qua goc toa do (0, 0, 0)");
        chk(!hp.ruleLines.empty(), "5.1.11: Yen ngua hien thi 2 he duong sinh thang (Mat ke doi)");
    }

    // --- KIEM THU 5.2: CAC MAT TRON XOAY ---
    printf("\n--- KIEM THU 5.2: CAC MAT TRON XOAY (SURFACES OF REVOLUTION) ---\n");
    {
        SurfaceMesh sph = createSphere(2.0, 24, 12);
        chk(!sph.grid.empty(), "5.2.1: Mat cau (Sphere) tao thanh cong");
        bool sphOk = true;
        for (const auto& row : sph.grid) {
            for (const auto& vtx : row) {
                if (std::fabs(length(vtx.pos) - 2.0) > 1e-4) sphOk = false;
            }
        }
        chk(sphOk, "5.2.2: Moi diem tren mat cau cach tam mot khoang dung R = 2.0");
    }

    {
        SurfaceMesh ellip = createEllipsoid(3.0, 1.5, 2.0, 24, 12);
        chk(!ellip.grid.empty(), "5.2.3: Mat Ellipsoid tao thanh cong");
        bool ellipOk = true;
        for (const auto& row : ellip.grid) {
            for (const auto& vtx : row) {
                double val = (vtx.pos.x * vtx.pos.x) / 9.0 + (vtx.pos.y * vtx.pos.y) / 2.25 + (vtx.pos.z * vtx.pos.z) / 4.0;
                if (std::fabs(val - 1.0) > 1e-4) ellipOk = false;
            }
        }
        chk(ellipOk, "5.2.4: Moi diem tren mat Ellipsoid thoa man (x/a)^2 + (y/b)^2 + (z/c)^2 = 1");
    }

    {
        SurfaceMesh tor = createToroid(2.0, 0.5, 32, 16);
        chk(!tor.grid.empty(), "5.2.5: Mat xuyen (Toroid) tao thanh cong");
        // Diem xa nhat R + r = 2.5
        double maxDist = 0;
        for (const auto& row : tor.grid) {
            for (const auto& vtx : row) {
                double d = std::sqrt(vtx.pos.x * vtx.pos.x + vtx.pos.z * vtx.pos.z);
                if (d > maxDist) maxDist = d;
            }
        }
        chk(eqD(maxDist, 2.5), "5.2.6: Ban kinh ngoai mat xuyen bang R + r = 2.5");
    }

    {
        SurfaceMesh hyp1 = createHyperboloidOneSheet(1.0, 1.5, 3.0, 24, 12);
        chk(!hyp1.grid.empty(), "5.2.7: Mat Hyperboloid 1 tang tao thanh cong");
        // Tai eo y = 0, ban kinh r = a = 1.0
        double waistR = std::sqrt(hyp1.grid[6][0].pos.x * hyp1.grid[6][0].pos.x + hyp1.grid[6][0].pos.z * hyp1.grid[6][0].pos.z);
        chk(eqD(waistR, 1.0), "5.2.8: Vong eo mat Hyperboloid 1 tang tai y=0 co ban kinh dung bang a = 1.0");

        SurfaceMesh hyp2 = createHyperboloidTwoSheets(1.0, 1.2, 3.6, 24, 12);
        chk(!hyp2.grid.empty(), "5.2.9: Mat Hyperboloid 2 tang tao thanh cong");
        const auto& apex2 = hyp2.grid.front()[0];
        chk(eqD(apex2.pos.y, 1.2) && eqD(apex2.pos.x, 0.0), "5.2.10: Dinh nua tren mat Hyperboloid 2 tang tai y = b = 1.2 co r = 0");
    }

    // --- KIEM THU 5.3: DUONG CONG BEZIER ---
    printf("\n--- KIEM THU 5.3: DUONG CONG BEZIER (BEZIER CURVES) ---\n");
    {
        // Kiem tra tinh chat da thuc Bernstein: Tong cac da thuc luon bang 1
        for (double t = 0.0; t <= 1.0; t += 0.25) {
            double sum3 = bernstein(0, 3, t) + bernstein(1, 3, t) + bernstein(2, 3, t) + bernstein(3, 3, t);
            chk(eqD(sum3, 1.0), "5.3.1: Da thuc Bernstein thoa man tinh chat phan hoach don vi (tong bang 1)");
        }

        std::vector<Vec3> pts = { { 0, 0, 0 }, { 1, 2, 0 }, { 3, 2, 0 }, { 4, 0, 0 } };
        Vec3 startPt = evaluateBezierCurveBernstein(pts, 0.0);
        Vec3 endPt   = evaluateBezierCurveBernstein(pts, 1.0);
        chk(eqV(startPt, pts[0]), "5.3.2: Diem dau duong cong B(0) trung khop voi diem kiem soat P0");
        chk(eqV(endPt, pts[3]),   "5.3.3: Diem cuoi duong cong B(1) trung khop voi diem kiem soat P3");

        // So sanh Bernstein va De Casteljau tai t = 0.5
        Vec3 pBernstein = evaluateBezierCurveBernstein(pts, 0.5);
        auto steps = deCasteljauSteps(pts, 0.5);
        Vec3 pDeCasteljau = steps.back()[0];
        chk(eqV(pBernstein, pDeCasteljau), "5.3.4: Thuat toan Bernstein va De Casteljau cho ket qua trung khop 100%");
    }

    // --- KIEM THU 5.4: MAT CONG BEZIER ---
    printf("\n--- KIEM THU 5.4: MAT CONG BEZIER (BEZIER SURFACES) ---\n");
    {
        auto net = createBezierSurfacePreset(SURF_PRESET_WAVE);
        // 4 goc cua mat cong Bicubic Bezier phai trung 4 goc cua luoi diem kiem soat:
        // S(0,0)=P[0][0], S(1,0)=P[3][0], S(0,1)=P[0][3], S(1,1)=P[3][3]
        VertexUV s00 = evaluateBezierSurfacePoint(net, 0.0, 0.0);
        VertexUV s10 = evaluateBezierSurfacePoint(net, 1.0, 0.0);
        VertexUV s01 = evaluateBezierSurfacePoint(net, 0.0, 1.0);
        VertexUV s11 = evaluateBezierSurfacePoint(net, 1.0, 1.0);

        chk(eqV(s00.pos, net[0][0]), "5.4.1: Goc S(0,0) cua mat cong trung voi P[0][0]");
        chk(eqV(s10.pos, net[3][0]), "5.4.2: Goc S(1,0) cua mat cong trung voi P[3][0]");
        chk(eqV(s01.pos, net[0][3]), "5.4.3: Goc S(0,1) cua mat cong trung voi P[0][3]");
        chk(eqV(s11.pos, net[3][3]), "5.4.4: Goc S(1,1) cua mat cong trung voi P[3][3]");

        // Phap tuyen khong duoc suy bien (chieu dai norm xap xi 1.0)
        chk(eqD(length(s00.norm), 1.0), "5.4.5: Vector phap tuyen tai S(0,0) duoc chuan hoa hop le");

        SurfaceMesh surfMesh = generateBezierSurfaceMesh(net, 16, 16);
        chk(!surfMesh.grid.empty(), "5.4.6: Tao thanh cong toan bo luoi da giac mat cong Bezier 3D");
    }

    printf("=================================================================\n");
    printf("  KET QUA KIEM THU LAB 5: %d / %d tests PASSED!\n", total - fail, total);
    printf("=================================================================\n\n");
    return fail ? 1 : 0;
}

// =============================================================================
// 7. VE 2D GIAO DIEN NGUOI DUNG (2D UI, BUTTONS, SLIDERS, PALETTES)
// =============================================================================

static void* FONT_SM = GLUT_BITMAP_HELVETICA_10;
static void* FONT_MD = GLUT_BITMAP_HELVETICA_12;
static void* FONT_LG = GLUT_BITMAP_HELVETICA_18;

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

static double textWidth(const std::string& str, void* font = FONT_MD) {
    return glutBitmapLength(font, (const unsigned char*)str.c_str());
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

    double t0 = x + 120.0;
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
// 8. VE KHONG GIAN 3D (3D RENDERING PIPELINE)
// =============================================================================

static void drawString3D(double x, double y, double z, const std::string& s, ColorRGBA c, void* font = FONT_MD) {
    setCol(c);
    glRasterPos3d(x, y, z);
    for (char ch : s) glutBitmapCharacter(font, ch);
}

static void drawCoordinateAxes3D() {
    if (!g.showAxes) return;
    double L = 4.0;
    glLineWidth(2.2f);

    // Truc Ox - Do
    setCol(hexCol(0xFF4757));
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0); glVertex3d(L, 0, 0);
    glEnd();
    drawString3D(L + 0.2, 0, 0, "Ox (+X)", hexCol(0xFF4757), FONT_MD);

    // Truc Oy - Xanh la
    setCol(hexCol(0x2ED573));
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0); glVertex3d(0, L, 0);
    glEnd();
    drawString3D(0, L + 0.2, 0, "Oy (+Y)", hexCol(0x2ED573), FONT_MD);

    // Truc Oz - Xanh duong
    setCol(hexCol(0x1E90FF));
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0); glVertex3d(0, 0, L);
    glEnd();
    drawString3D(0, 0, L + 0.2, "Oz (+Z)", hexCol(0x1E90FF), FONT_MD);
}

static void drawGroundGrid3D() {
    if (!g.showGrid) return;
    int steps = 5;
    double stepSize = 1.0;
    double maxL = steps * stepSize;
    double floorY = -2.2;

    glLineWidth(1.0f);
    setCol(hexCol(0x2C3A4B, 0.40f));
    glBegin(GL_LINES);
    for (int i = -steps; i <= steps; i++) {
        double d = i * stepSize;
        glVertex3d(-maxL, floorY, d); glVertex3d(maxL, floorY, d);
        glVertex3d(d, floorY, -maxL); glVertex3d(d, floorY, maxL);
    }
    glEnd();
}

static ColorRGBA getCurrentObjectColor(float alphaMod = 1.0f) {
    return {
        (float)g.customR,
        (float)g.customG,
        (float)g.customB,
        (float)(g.faceOpacity * alphaMod)
    };
}

// Ve be mat tham so UV (SurfaceMesh) voi chieu sang Gouraud & Wireframe
static void drawSurfaceMesh(const SurfaceMesh& mesh) {
    if (mesh.grid.empty()) return;

    size_t rows = mesh.grid.size();
    size_t cols = mesh.grid[0].size();
    ColorRGBA objColor = getCurrentObjectColor();

    // 1. Ve mat da giac dac (Solid Shaded Surface)
    if (g.renderStyle == STYLE_SOLID || g.renderStyle == STYLE_BOTH) {
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        setCol(objColor);
        GLfloat matSpec[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float)g.shininess);

        for (size_t j = 0; j + 1 < rows; j++) {
            glBegin(GL_QUAD_STRIP);
            for (size_t i = 0; i < cols; i++) {
                const auto& v1 = mesh.grid[j][i];
                const auto& v2 = mesh.grid[j + 1][i];

                glNormal3d(v1.norm.x, v1.norm.y, v1.norm.z);
                glVertex3d(v1.pos.x, v1.pos.y, v1.pos.z);

                glNormal3d(v2.norm.x, v2.norm.y, v2.norm.z);
                glVertex3d(v2.pos.x, v2.pos.y, v2.pos.z);
            }
            glEnd();
        }

        // Ve cac mat day/nap tron (Caps)
        for (const auto& cap : mesh.capFaces) {
            if (cap.size() < 3) continue;
            glBegin(GL_TRIANGLE_FAN);
            Vec3 capNorm = normalize(cross(cap[2] - cap[0], cap[1] - cap[0]));
            glNormal3d(capNorm.x, capNorm.y, capNorm.z);
            for (const auto& pt : cap) {
                glVertex3d(pt.x, pt.y, pt.z);
            }
            glEnd();
        }

        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_LIGHTING);
    }

    // 2. Ve khung luoi toa do tham so (Wireframe Isolines)
    if (g.renderStyle == STYLE_WIREFRAME || g.renderStyle == STYLE_BOTH) {
        ColorRGBA wireCol = (g.renderStyle == STYLE_BOTH) ? hexCol(0xFFFFFF, 0.35f) : objColor;
        setCol(wireCol);
        glLineWidth(1.2f);

        // Duong tham so u (dong)
        for (size_t j = 0; j < rows; j++) {
            glBegin(GL_LINE_STRIP);
            for (size_t i = 0; i < cols; i++) {
                const auto& pt = mesh.grid[j][i].pos;
                glVertex3d(pt.x, pt.y, pt.z);
            }
            glEnd();
        }

        // Duong tham so v (cot)
        for (size_t i = 0; i < cols; i++) {
            glBegin(GL_LINE_STRIP);
            for (size_t j = 0; j < rows; j++) {
                const auto& pt = mesh.grid[j][i].pos;
                glVertex3d(pt.x, pt.y, pt.z);
            }
            glEnd();
        }
    }

    // 3. Ve cac duong sinh thang cua mat co quy tac (Ruled Generatrices)
    if (g.showRules && !mesh.ruleLines.empty()) {
        setCol(hexCol(0xFFD166, 0.90f));
        glLineWidth(2.4f);
        glBegin(GL_LINES);
        for (const auto& rl : mesh.ruleLines) {
            glVertex3d(rl.first.x, rl.first.y, rl.first.z);
            glVertex3d(rl.second.x, rl.second.y, rl.second.z);
        }
        glEnd();
    }

    // 4. Ve vector phap tuyen (Normals)
    if (g.showNormals) {
        setCol(hexCol(0xFF4757, 0.85f));
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        size_t stepR = std::max<size_t>(1, rows / 8);
        size_t stepC = std::max<size_t>(1, cols / 12);
        for (size_t j = 0; j < rows; j += stepR) {
            for (size_t i = 0; i < cols; i += stepC) {
                const auto& v = mesh.grid[j][i];
                Vec3 tip = v.pos + v.norm * 0.45;
                glVertex3d(v.pos.x, v.pos.y, v.pos.z);
                glVertex3d(tip.x, tip.y, tip.z);
            }
        }
        glEnd();
    }
}

// Ve duong cong Bezier & Truc quan hoa thuat toan De Casteljau (5.3)
static void drawBezierCurveScene() {
    const auto& ctrlPts = g.curveCtrlPts;
    if (ctrlPts.size() < 2) return;

    // 1. Ve da giac kiem soat (Control Polygon)
    if (g.showControlNet) {
        setCol(hexCol(0xF79F37, 0.75f));
        glLineWidth(1.8f);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(2, 0xAAAA);
        glBegin(GL_LINE_STRIP);
        for (const auto& pt : ctrlPts) {
            glVertex3d(pt.x, pt.y, pt.z);
        }
        glEnd();
        glDisable(GL_LINE_STIPPLE);

        // Ve cac dinh kiem soat Pi dang hinh cau nho
        for (size_t i = 0; i < ctrlPts.size(); i++) {
            const auto& pt = ctrlPts[i];
            bool isSel = ((int)i == g.selectedCurvePtIdx);
            glPushMatrix();
            glTranslated(pt.x, pt.y, pt.z);
            setCol(isSel ? hexCol(0xFF4757) : hexCol(0xFFD166));
            glutSolidSphere(isSel ? 0.16 : 0.10, 16, 16);
            glPopMatrix();

            char lbl[16];
            snprintf(lbl, sizeof lbl, "P%d", (int)i);
            drawString3D(pt.x + 0.15, pt.y + 0.15, pt.z + 0.15, lbl, isSel ? hexCol(0xFF4757) : hexCol(0xFFFFFF), FONT_MD);
        }
    }

    // 2. Ve duong cong Bezier muot ma (Smooth Curve)
    int numSamples = 120;
    ColorRGBA curveCol = getCurrentObjectColor();
    setCol(curveCol);
    glLineWidth(3.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= numSamples; i++) {
        double t = (double)i / numSamples;
        Vec3 p = evaluateBezierCurveBernstein(ctrlPts, t);
        glVertex3d(p.x, p.y, p.z);
    }
    glEnd();

    // 3. Truc quan hoa thuat toan De Casteljau tai tham so t
    if (g.showDeCasteljau) {
        auto steps = deCasteljauSteps(ctrlPts, g.curveT);

        unsigned stepColors[] = { 0x3DD6B5, 0x9D4EDD, 0x54A0FF, 0xFF6B6B };
        for (size_t level = 1; level < steps.size(); level++) {
            const auto& pts = steps[level];
            unsigned c = stepColors[(level - 1) % 4];
            setCol(hexCol(c, 0.90f));
            glLineWidth(2.0f);
            glBegin(GL_LINE_STRIP);
            for (const auto& p : pts) {
                glVertex3d(p.x, p.y, p.z);
            }
            glEnd();

            // Ve cac diem trung gian
            for (const auto& p : pts) {
                glPushMatrix();
                glTranslated(p.x, p.y, p.z);
                glutSolidSphere(0.06, 12, 12);
                glPopMatrix();
            }
        }

        // Diem hien tai B(t) tren duong cong
        Vec3 bt = steps.back()[0];
        glPushMatrix();
        glTranslated(bt.x, bt.y, bt.z);
        setCol(hexCol(0xFF0055));
        glutSolidSphere(0.15, 20, 20);
        glPopMatrix();

        char tLabel[32];
        snprintf(tLabel, sizeof tLabel, "B(t=%.2f)", g.curveT);
        drawString3D(bt.x + 0.18, bt.y + 0.18, bt.z, tLabel, hexCol(0xFF0055), FONT_MD);
    }
}

// Ve mat cong Bezier 4x4 & Luoi kiem soat (5.4)
static void drawBezierSurfaceScene() {
    SurfaceMesh surfMesh = generateBezierSurfaceMesh(g.surfControlNet, g.uvSlices, g.uvStacks);
    drawSurfaceMesh(surfMesh);

    // Ve luoi diem kiem soat 4x4 (Control Net Grid)
    if (g.showControlNet) {
        setCol(hexCol(0xF79F37, 0.70f));
        glLineWidth(1.6f);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(2, 0xAAAA);

        // Cac doan thang noi theo hang i
        for (int i = 0; i < 4; i++) {
            glBegin(GL_LINE_STRIP);
            for (int j = 0; j < 4; j++) {
                const auto& p = g.surfControlNet[i][j];
                glVertex3d(p.x, p.y, p.z);
            }
            glEnd();
        }

        // Cac doan thang noi theo cot j
        for (int j = 0; j < 4; j++) {
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i < 4; i++) {
                const auto& p = g.surfControlNet[i][j];
                glVertex3d(p.x, p.y, p.z);
            }
            glEnd();
        }
        glDisable(GL_LINE_STIPPLE);

        // Ve 16 diem kiem soat P_{i,j}
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                const auto& p = g.surfControlNet[i][j];
                bool isSel = (i == g.surfSelI && j == g.surfSelJ);

                glPushMatrix();
                glTranslated(p.x, p.y, p.z);
                setCol(isSel ? hexCol(0xFF4757) : hexCol(0xFFD166));
                glutSolidSphere(isSel ? 0.15 : 0.08, 16, 16);
                glPopMatrix();

                if (isSel) {
                    char lbl[32];
                    snprintf(lbl, sizeof lbl, "P[%d,%d]: Y=%.2f", i, j, p.y);
                    drawString3D(p.x + 0.15, p.y + 0.18, p.z + 0.15, lbl, hexCol(0xFF4757), FONT_MD);
                }
            }
        }
    }
}

// =============================================================================
// 9. VE GIAO DIEN HEADER & SIDEBAR CONTROLS
// =============================================================================

static void renderHeader() {
    fillRect2D(0, 0, g.windowW, HEADER_H, hexCol(0x0E131A));
    line2D(0, HEADER_H, g.windowW, HEADER_H, hexCol(BORDER_LINE), 1.5);

    drawText(20, 24, "VKU GRAPHICS - LAB 5: DUONG VA MAT CONG 3D", hexCol(ACCENT_CYAN), FONT_LG);
    drawText(20, 44, "5.1. Mat co quy tac | 5.2. Mat tron xoay | 5.3. Duong cong Bezier | 5.4. Mat cong Bezier", hexCol(TEXT_MUTED), FONT_SM);

    double btnW = 150.0, btnH = 34.0, btnY = 13.0;
    double tX = g.windowW - sidePanelW() - 480.0;

    drawButton(tX, btnY, btnW, btnH, "5.1: MAT QUY TAC", g.activeSection == SECTION_RULED, [] {
        g.activeSection = SECTION_RULED;
    });
    drawButton(tX + btnW + 8.0, btnY, btnW, btnH, "5.2: MAT TRON XOAY", g.activeSection == SECTION_REVOLUTION, [] {
        g.activeSection = SECTION_REVOLUTION;
    });
    drawButton(tX + (btnW + 8.0) * 2, btnY, btnW, btnH, "5.3: DUONG BEZIER", g.activeSection == SECTION_BEZIER_CURV, [] {
        g.activeSection = SECTION_BEZIER_CURV;
    });
    drawButton(tX + (btnW + 8.0) * 3, btnY, btnW, btnH, "5.4: MAT BEZIER", g.activeSection == SECTION_BEZIER_SURF, [] {
        g.activeSection = SECTION_BEZIER_SURF;
    });

    drawButton(g.windowW - 215.0, btnY, 95.0, btnH, g.showHUD ? "AN HUD (H)" : "HIEN HUD (H)", g.showHUD, [] {
        g.showHUD = !g.showHUD;
    });

    drawButton(g.windowW - 110.0, btnY, 90.0, btnH, "RESET (R)", false, [] {
        g.thetaDeg = 45.0;
        g.phiDeg = 25.0;
        g.eyeRadius = 9.0;
        g.objScale = 1.0;
        g.curveT = 0.5;
        g.autoOrbit = false;
        g.animateCurveT = false;
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

    // --- CARD 1: DIEU KHIEN CHUYEN MUC (SECTION ITEMS) ---
    fillRect2D(padX, curY, cardW, 95.0, hexCol(CARD_BG_COLOR));
    strokeRect2D(padX, curY, cardW, 95.0, hexCol(BORDER_LINE), 1.0);

    if (g.activeSection == SECTION_RULED) {
        drawText(padX + 12, curY + 18, "5.1. CHON MAT CO QUY TAC (RULED SURFACES):", hexCol(ACCENT_ORANGE), FONT_MD);
        double bW = (cardW - 32.0) / 2.0, bH = 28.0;
        drawButton(padX + 10, curY + 28, bW, bH, "Mat tru (Cylinder)", g.ruledType == RULED_CYLINDER, [] { g.ruledType = RULED_CYLINDER; });
        drawButton(padX + 18 + bW, curY + 28, bW, bH, "Mat non (Cone)", g.ruledType == RULED_CONE, [] { g.ruledType = RULED_CONE; });
        drawButton(padX + 10, curY + 60, bW, bH, "Yen ngua (Hyper. Para)", g.ruledType == RULED_HYPER_PARA, [] { g.ruledType = RULED_HYPER_PARA; });
        drawButton(padX + 18 + bW, curY + 60, bW, bH, "Mat Conoid", g.ruledType == RULED_CONOID, [] { g.ruledType = RULED_CONOID; });
    } else if (g.activeSection == SECTION_REVOLUTION) {
        drawText(padX + 12, curY + 18, "5.2. CHON MAT TRON XOAY (SURFACES OF REVOLUTION):", hexCol(ACCENT_ORANGE), FONT_MD);
        double bW = (cardW - 36.0) / 3.0, bH = 28.0;
        drawButton(padX + 10, curY + 28, bW, bH, "Cau (Sphere)", g.revType == REV_SPHERE, [] { g.revType = REV_SPHERE; });
        drawButton(padX + 14 + bW, curY + 28, bW, bH, "Ellipsoid", g.revType == REV_ELLIPSOID, [] { g.revType = REV_ELLIPSOID; });
        drawButton(padX + 18 + bW * 2, curY + 28, bW, bH, "Hyp. 1 tang", g.revType == REV_HYPERBOLOID, [] { g.revType = REV_HYPERBOLOID; });
        drawButton(padX + 10, curY + 60, bW, bH, "Hyp. 2 tang", g.revType == REV_HYPERBOLOID_2, [] { g.revType = REV_HYPERBOLOID_2; });
        drawButton(padX + 14 + bW, curY + 60, bW, bH, "Xuyen (Toroid)", g.revType == REV_TOROID, [] { g.revType = REV_TOROID; });
        drawButton(padX + 18 + bW * 2, curY + 60, bW, bH, "Paraboloid", g.revType == REV_PARABOLOID, [] { g.revType = REV_PARABOLOID; });
    } else if (g.activeSection == SECTION_BEZIER_CURV) {
        drawText(padX + 12, curY + 18, "5.3. CHON DUONG CONG BEZIER (BEZIER CURVE):", hexCol(ACCENT_ORANGE), FONT_MD);
        double bW = (cardW - 32.0) / 2.0, bH = 28.0;
        drawButton(padX + 10, curY + 28, bW, bH, "Bac 3: Chu S", g.curveType == CURVE_CUBIC_S, [] {
            g.curveType = CURVE_CUBIC_S;
            g.curveCtrlPts = createBezierCurvePreset(CURVE_CUBIC_S);
            g.selectedCurvePtIdx = 1;
        });
        drawButton(padX + 18 + bW, curY + 28, bW, bH, "Bac 3: Vom cong", g.curveType == CURVE_CUBIC_ARCH, [] {
            g.curveType = CURVE_CUBIC_ARCH;
            g.curveCtrlPts = createBezierCurvePreset(CURVE_CUBIC_ARCH);
            g.selectedCurvePtIdx = 1;
        });
        drawButton(padX + 10, curY + 60, bW, bH, "Bac 2: Parabol", g.curveType == CURVE_QUADRATIC, [] {
            g.curveType = CURVE_QUADRATIC;
            g.curveCtrlPts = createBezierCurvePreset(CURVE_QUADRATIC);
            g.selectedCurvePtIdx = 1;
        });
        drawButton(padX + 18 + bW, curY + 60, bW, bH, "Bac 4: Xoan 3D", g.curveType == CURVE_3D_SPIRAL, [] {
            g.curveType = CURVE_3D_SPIRAL;
            g.curveCtrlPts = createBezierCurvePreset(CURVE_3D_SPIRAL);
            g.selectedCurvePtIdx = 2;
        });
    } else if (g.activeSection == SECTION_BEZIER_SURF) {
        drawText(padX + 12, curY + 18, "5.4. PRESETS MAT CONG BEZIER 4x4:", hexCol(ACCENT_ORANGE), FONT_MD);
        double bW = (cardW - 32.0) / 2.0, bH = 28.0;
        drawButton(padX + 10, curY + 28, bW, bH, "Song luon (Wave)", g.surfPreset == SURF_PRESET_WAVE, [] {
            g.surfPreset = SURF_PRESET_WAVE;
            g.surfControlNet = createBezierSurfacePreset(SURF_PRESET_WAVE);
        });
        drawButton(padX + 18 + bW, curY + 28, bW, bH, "Yen ngua (Saddle)", g.surfPreset == SURF_PRESET_SADDLE, [] {
            g.surfPreset = SURF_PRESET_SADDLE;
            g.surfControlNet = createBezierSurfacePreset(SURF_PRESET_SADDLE);
        });
        drawButton(padX + 10, curY + 60, bW, bH, "Dinh non (Dome)", g.surfPreset == SURF_PRESET_DOME, [] {
            g.surfPreset = SURF_PRESET_DOME;
            g.surfControlNet = createBezierSurfacePreset(SURF_PRESET_DOME);
        });
        drawButton(padX + 18 + bW, curY + 60, bW, bH, "Mang trung (Valley)", g.surfPreset == SURF_PRESET_VALLEY, [] {
            g.surfPreset = SURF_PRESET_VALLEY;
            g.surfControlNet = createBezierSurfacePreset(SURF_PRESET_VALLEY);
        });
    }
    curY += 105.0;

    // --- CARD 2: DIEU KHIEN CHUYEN SAU CHO TUNG MUC ---
    double subCardH = 120.0;
    fillRect2D(padX, curY, cardW, subCardH, hexCol(CARD_BG_COLOR));
    strokeRect2D(padX, curY, cardW, subCardH, hexCol(BORDER_LINE), 1.0);

    if (g.activeSection == SECTION_RULED) {
        drawText(padX + 12, curY + 18, "THAM SO HINH HOC MAT CO QUY TAC:", hexCol(ACCENT_CYAN), FONT_MD);
        if (g.ruledType == RULED_CYLINDER) {
            drawSlider(padX + 10, curY + 28, cardW - 20, 24, 101, "Ban kinh R:", 0.5, 3.0, &g.cylinderR, "");
            drawSlider(padX + 10, curY + 58, cardW - 20, 24, 102, "Chieu cao H:", 1.0, 5.0, &g.cylinderH, "");
            drawButton(padX + 10, curY + 88, cardW - 20, 24, g.showRules ? "[x] Hien thi duong sinh thang" : "[ ] An duong sinh thang", g.showRules, [] {
                g.showRules = !g.showRules;
            });
        } else if (g.ruledType == RULED_CONE) {
            drawSlider(padX + 10, curY + 28, cardW - 20, 24, 103, "Ban kinh day R:", 0.5, 3.0, &g.coneR, "");
            drawSlider(padX + 10, curY + 58, cardW - 20, 24, 104, "Chieu cao H:", 1.0, 5.0, &g.coneH, "");
            drawButton(padX + 10, curY + 88, cardW - 20, 24, g.showRules ? "[x] Hien thi duong sinh thang" : "[ ] An duong sinh thang", g.showRules, [] {
                g.showRules = !g.showRules;
            });
        } else if (g.ruledType == RULED_HYPER_PARA) {
            drawSlider(padX + 10, curY + 28, cardW - 20, 24, 105, "Truc a:", 0.5, 2.5, &g.hpA, "");
            drawSlider(padX + 10, curY + 58, cardW - 20, 24, 106, "Truc b:", 0.5, 2.5, &g.hpB, "");
            drawButton(padX + 10, curY + 88, cardW - 20, 24, g.showRules ? "[x] 2 he duong sinh thang" : "[ ] An duong sinh", g.showRules, [] {
                g.showRules = !g.showRules;
            });
        } else {
            drawSlider(padX + 10, curY + 28, cardW - 20, 24, 107, "Ban kinh R:", 0.5, 3.0, &g.conoidR, "");
            drawSlider(padX + 10, curY + 58, cardW - 20, 24, 108, "Bien do H:", 0.5, 3.0, &g.conoidH, "");
            drawButton(padX + 10, curY + 88, cardW - 20, 24, g.showRules ? "[x] Duong sinh Conoid" : "[ ] An duong sinh", g.showRules, [] {
                g.showRules = !g.showRules;
            });
        }
    } else if (g.activeSection == SECTION_REVOLUTION) {
        drawText(padX + 12, curY + 18, "THAM SO MAT TRON XOAY:", hexCol(ACCENT_CYAN), FONT_MD);
        if (g.revType == REV_SPHERE) {
            drawSlider(padX + 10, curY + 36, cardW - 20, 26, 201, "Ban kinh R:", 0.6, 3.0, &g.sphereR, "");
        } else if (g.revType == REV_ELLIPSOID) {
            drawSlider(padX + 10, curY + 26, cardW - 20, 24, 202, "Ban truc a:", 0.5, 3.0, &g.ellipA, "");
            drawSlider(padX + 10, curY + 54, cardW - 20, 24, 203, "Ban truc b:", 0.5, 3.0, &g.ellipB, "");
            drawSlider(padX + 10, curY + 82, cardW - 20, 24, 204, "Ban truc c:", 0.5, 3.0, &g.ellipC, "");
        } else if (g.revType == REV_HYPERBOLOID) {
            drawSlider(padX + 10, curY + 26, cardW - 20, 24, 205, "Ban kinh eo a:", 0.5, 2.5, &g.hyperA, "");
            drawSlider(padX + 10, curY + 54, cardW - 20, 24, 206, "Do loe b:", 0.5, 2.5, &g.hyperB, "");
            drawSlider(padX + 10, curY + 82, cardW - 20, 24, 207, "Chieu cao H:", 1.0, 5.0, &g.hyperH, "");
        } else if (g.revType == REV_TOROID) {
            drawSlider(padX + 10, curY + 32, cardW - 20, 24, 208, "Ban kinh chinh R:", 1.0, 3.5, &g.torusR, "");
            drawSlider(padX + 10, curY + 68, cardW - 20, 24, 209, "Ban kinh ong r:", 0.1, 1.2, &g.torusTubeR, "");
        } else {
            drawSlider(padX + 10, curY + 32, cardW - 20, 24, 210, "Khau do R:", 0.8, 3.0, &g.paraR, "");
            drawSlider(padX + 10, curY + 68, cardW - 20, 24, 211, "Chieu sau H:", 0.8, 4.0, &g.paraH, "");
        }
    } else if (g.activeSection == SECTION_BEZIER_CURV) {
        drawText(padX + 12, curY + 16, "THAM SO & DE CASTELJAU:", hexCol(ACCENT_CYAN), FONT_MD);
        drawSlider(padX + 10, curY + 24, cardW - 20, 20, 301, "Tham so t:", 0.0, 1.0, &g.curveT, "");

        double bW = (cardW - 32.0) / 2.0;
        drawButton(padX + 10, curY + 48, bW, 22, g.animateCurveT ? "DUNG CHAY (Space)" : "CHAY DONG t (Space)", g.animateCurveT, [] {
            g.animateCurveT = !g.animateCurveT;
        });
        drawButton(padX + 18 + bW, curY + 48, bW, 22, g.showDeCasteljau ? "[x] De Casteljau" : "[ ] De Casteljau", g.showDeCasteljau, [] {
            g.showDeCasteljau = !g.showDeCasteljau;
        });

        // Cac nut chon truc tiep P0..P4
        double pBtnW = (cardW - 20.0 - 5 * 4.0) / 5.0;
        for (size_t i = 0; i < g.curveCtrlPts.size() && i < 5; i++) {
            char pName[8];
            snprintf(pName, sizeof pName, "P%d", (int)i);
            bool isP = ((int)i == g.selectedCurvePtIdx);
            drawButton(padX + 10 + i * (pBtnW + 4.0), curY + 74, pBtnW, 20, pName, isP, [=] {
                g.selectedCurvePtIdx = (int)i;
            });
        }

        // Sliders X, Y, Z cho diem dang chon
        if (g.selectedCurvePtIdx < (int)g.curveCtrlPts.size()) {
            drawSlider(padX + 10, curY + 98, (cardW - 24.0) / 3.0, 18, 302, "X:", -3.0, 3.0, &g.curveCtrlPts[g.selectedCurvePtIdx].x, "");
            drawSlider(padX + 12 + (cardW - 24.0) / 3.0, curY + 98, (cardW - 24.0) / 3.0, 18, 303, "Y:", -3.0, 3.0, &g.curveCtrlPts[g.selectedCurvePtIdx].y, "");
            drawSlider(padX + 14 + 2 * (cardW - 24.0) / 3.0, curY + 98, (cardW - 24.0) / 3.0, 18, 304, "Z:", -3.0, 3.0, &g.curveCtrlPts[g.selectedCurvePtIdx].z, "");
        }
        drawButton(padX + 10, curY + 120, cardW - 20, 20, g.showControlNet ? "[x] Hien Da giac kiem soat" : "[ ] An Da giac", g.showControlNet, [] {
            g.showControlNet = !g.showControlNet;
        });
    } else if (g.activeSection == SECTION_BEZIER_SURF) {
        drawText(padX + 12, curY + 16, "CHON & DIEU CHINH DIEM KIEM SOAT 4x4:", hexCol(ACCENT_CYAN), FONT_MD);

        // Luoi 4x4 nut chon diem kiem soat P_{i,j}
        double sz = 24.0, gap = 4.0;
        double startX = padX + 12.0;
        double startY = curY + 24.0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                bool isSel = (i == g.surfSelI && j == g.surfSelJ);
                char num[8];
                snprintf(num, sizeof num, "%d,%d", i, j);
                drawButton(startX + j * (sz + gap), startY + i * (sz + 2), sz, sz - 2, num, isSel, [=] {
                    g.surfSelI = i;
                    g.surfSelJ = j;
                });
            }
        }

        // Sliders dieu chinh truc tiep toa do X, Y, Z cua diem duoc chon
        double slX = startX + 4 * (sz + gap) + 12.0;
        double slW = cardW - (slX - padX) - 10.0;
        drawSlider(slX, curY + 24, slW, 20, 401, "Cao do Y:", -2.5, 2.5, &g.surfControlNet[g.surfSelI][g.surfSelJ].y, "");
        drawSlider(slX, curY + 48, slW, 20, 402, "Dich X:", -3.0, 3.0, &g.surfControlNet[g.surfSelI][g.surfSelJ].x, "");
        drawSlider(slX, curY + 72, slW, 20, 403, "Dich Z:", -3.0, 3.0, &g.surfControlNet[g.surfSelI][g.surfSelJ].z, "");
        drawButton(slX, curY + 98, slW, 22, g.showControlNet ? "[x] Hien Control Net" : "[ ] An Control Net", g.showControlNet, [] {
            g.showControlNet = !g.showControlNet;
        });
    }
    curY += subCardH + 30.0;

    // --- CARD 3: GOC QUAY & DIEU KHIEN CAMERA (ORBIT / ZOOM / SCALE) ---
    fillRect2D(padX, curY, cardW, 135.0, hexCol(CARD_BG_COLOR));
    strokeRect2D(padX, curY, cardW, 135.0, hexCol(BORDER_LINE), 1.0);

    drawText(padX + 12, curY + 16, "GOC QUAY & CAMERA (CHUOT TRAI XOAY, CUON ZOOM):", hexCol(ACCENT_CYAN), FONT_MD);
    drawSlider(padX + 10, curY + 24, cardW - 20, 20, 501, "Goc quay ngang Theta:", 0.0, 360.0, &g.thetaDeg, "deg");
    drawSlider(padX + 10, curY + 46, cardW - 20, 20, 502, "Goc quay doc Phi:", -89.0, 89.0, &g.phiDeg, "deg");
    drawSlider(padX + 10, curY + 68, cardW - 20, 20, 503, "Khoang cach Zoom:", 2.5, 25.0, &g.eyeRadius, "");
    drawSlider(padX + 10, curY + 90, cardW - 20, 20, 504, "Ti le Scale vat the:", 0.2, 2.5, &g.objScale, "x");
    drawSlider(padX + 10, curY + 112, cardW - 20, 20, 505, "Do trong suot Opacity:", 0.2, 1.0, &g.faceOpacity, "");

    curY += 145.0;

    // --- CARD 4: MAU SAC & KIEU HIEN THI (COLORS & RENDERING STYLES) ---
    fillRect2D(padX, curY, cardW, 155.0, hexCol(CARD_BG_COLOR));
    strokeRect2D(padX, curY, cardW, 155.0, hexCol(BORDER_LINE), 1.0);

    drawText(padX + 12, curY + 16, "MAU SAC & KIEU HIEN THI:", hexCol(ACCENT_CYAN), FONT_MD);

    // Kieu bieu dien: Solid / Wireframe / Both
    double sW = (cardW - 32.0) / 3.0;
    drawButton(padX + 10, curY + 24, sW, 26, "Solid", g.renderStyle == STYLE_SOLID, [] { g.renderStyle = STYLE_SOLID; });
    drawButton(padX + 14 + sW, curY + 24, sW, 26, "Wireframe", g.renderStyle == STYLE_WIREFRAME, [] { g.renderStyle = STYLE_WIREFRAME; });
    drawButton(padX + 18 + sW * 2, curY + 24, sW, 26, "Solid + Wire", g.renderStyle == STYLE_BOTH, [] { g.renderStyle = STYLE_BOTH; });

    // 8 O mau Preset nhanh
    double colBoxW = (cardW - 20.0 - 7 * 6.0) / 8.0;
    for (int k = 0; k < 8; k++) {
        double bx = padX + 10.0 + k * (colBoxW + 6.0);
        double by = curY + 56.0;
        bool isAct = (g.colorPresetIdx == k);
        ColorRGBA c = hexCol(COLOR_PRESETS[k].hexColor);
        fillRect2D(bx, by, colBoxW, 22.0, c);
        strokeRect2D(bx, by, colBoxW, 22.0, isAct ? hexCol(0xFFFFFF) : hexCol(BORDER_LINE), isAct ? 2.5 : 1.0);

        if (g.mouseX >= bx && g.mouseX <= bx + colBoxW && g.mouseY >= by && g.mouseY <= by + 22.0) {
            g.buttons.push_back({ bx, by, colBoxW, 22.0, "", [=] {
                g.colorPresetIdx = k;
                ColorRGBA sc = hexCol(COLOR_PRESETS[k].hexColor);
                g.customR = sc.r;
                g.customG = sc.g;
                g.customB = sc.b;
            }, isAct });
        }
    }

    // 3 Sliders RGB tuy bien
    drawSlider(padX + 10, curY + 84,  cardW - 20, 20, 601, "Red (R):", 0.0, 1.0, &g.customR, "");
    drawSlider(padX + 10, curY + 106, cardW - 20, 20, 602, "Green (G):", 0.0, 1.0, &g.customG, "");
    drawSlider(padX + 10, curY + 128, cardW - 20, 20, 603, "Blue (B):", 0.0, 1.0, &g.customB, "");

    curY += 165.0;

    // --- CARD 5: TOGGLE BO TRO (UTILITIES & ACTIONS) ---
    fillRect2D(padX, curY, cardW, 70.0, hexCol(CARD_BG_COLOR));
    strokeRect2D(padX, curY, cardW, 70.0, hexCol(BORDER_LINE), 1.0);

    double tW = (cardW - 32.0) / 2.0;
    drawButton(padX + 10, curY + 8, tW, 26, g.autoOrbit ? "[x] TU DONG XOAY" : "[ ] TU DONG XOAY", g.autoOrbit, [] {
        g.autoOrbit = !g.autoOrbit;
    });
    drawButton(padX + 18 + tW, curY + 8, tW, 26, g.showNormals ? "[x] PHAP TUYEN" : "[ ] PHAP TUYEN", g.showNormals, [] {
        g.showNormals = !g.showNormals;
    });
    drawButton(padX + 10, curY + 38, tW, 26, g.showAxes ? "[x] TRUC TOA DO" : "[ ] TRUC TOA DO", g.showAxes, [] {
        g.showAxes = !g.showAxes;
    });
    drawButton(padX + 18 + tW, curY + 38, tW, 26, g.showGrid ? "[x] SAN LUOI 3D" : "[ ] SAN LUOI 3D", g.showGrid, [] {
        g.showGrid = !g.showGrid;
    });
}

// Ve khung thong tin HUD len goc trai tren cua viewport 3D
static void renderHUD() {
    if (!g.showHUD) return;

    double hudX = 16.0;
    double hudY = HEADER_H + 16.0;
    double hudW = 480.0;
    double hudH = 115.0;

    fillRect2D(hudX, hudY, hudW, hudH, hexCol(0x0E131AE0));
    strokeRect2D(hudX, hudY, hudW, hudH, hexCol(BORDER_LINE), 1.0);

    // Nut tat nhanh HUD truc tiep tren goc hop thong tin
    drawButton(hudX + hudW - 28.0, hudY + 6.0, 22.0, 20.0, "X", false, [] {
        g.showHUD = false;
    });

    std::string titleStr, eqStr;
    if (g.activeSection == SECTION_RULED) {
        if (g.ruledType == RULED_CYLINDER) {
            titleStr = "[5.1] MAT TRU (CYLINDER) - MAT CO QUY TAC";
            eqStr = "x = R*cos(u),  y = v,  z = R*sin(u)";
        } else if (g.ruledType == RULED_CONE) {
            titleStr = "[5.1] MAT NON (CONE) - MAT CO QUY TAC";
            eqStr = "x = (1-s)*R*cos(u),  y = -H/2 + s*H,  z = (1-s)*R*sin(u)";
        } else if (g.ruledType == RULED_HYPER_PARA) {
            titleStr = "[5.1] MAT YEN NGUA (HYPERBOLIC PARABOLOID) - MAT KE DOI";
            eqStr = "z = (x^2 / a^2) - (y^2 / b^2)  (2 he duong sinh thang)";
        } else {
            titleStr = "[5.1] MAT CONOID - MAT CO QUY TAC";
            eqStr = "x = v*cos(u),  y = v*sin(u),  z = H*sin(2*u)";
        }
    } else if (g.activeSection == SECTION_REVOLUTION) {
        if (g.revType == REV_SPHERE) {
            titleStr = "[5.2] MAT CAU (SPHERE) - MAT TRON XOAY";
            eqStr = "x = R*cos(v)*cos(u),  y = R*sin(v),  z = R*cos(v)*sin(u)";
        } else if (g.revType == REV_ELLIPSOID) {
            titleStr = "[5.2] MAT ELLIPSOID - MAT TRON XOAY";
            eqStr = "(x/a)^2 + (y/b)^2 + (z/c)^2 = 1";
        } else if (g.revType == REV_HYPERBOLOID) {
            titleStr = "[5.2] MAT HYPERBOLOID 1 TANG - MAT TRON XOAY & KE DOI";
            eqStr = "x^2/a^2 + z^2/a^2 - y^2/b^2 = 1";
        } else if (g.revType == REV_HYPERBOLOID_2) {
            titleStr = "[5.2] MAT HYPERBOLOID 2 TANG - MAT TRON XOAY";
            eqStr = "y^2/b^2 - (x^2 + z^2)/a^2 = 1  (2 ban phan)";
        } else if (g.revType == REV_TOROID) {
            titleStr = "[5.2] MAT XUYEN (TORUS / TOROID) - MAT TRON XOAY";
            eqStr = "x = (R + r*cos(v))*cos(u),  y = r*sin(v),  z = (R + r*cos(v))*sin(u)";
        } else {
            titleStr = "[5.2] MAT PARABOLOID TRON XOAY";
            eqStr = "y = (x^2 + z^2) * k";
        }
    } else if (g.activeSection == SECTION_BEZIER_CURV) {
        titleStr = "[5.3] DUONG CONG BEZIER (BEZIER CURVE)";
        eqStr = "B(t) = sum_{i=0}^n C(n,i)*(1-t)^(n-i)*t^i * P_i  |  De Casteljau";
    } else {
        titleStr = "[5.4] MAT CONG BICUBIC BEZIER 4x4 (BEZIER SURFACE)";
        eqStr = "S(u,v) = sum_{i=0}^3 sum_{j=0}^3 B_i^3(u)*B_j^3(v)*P_{i,j}";
    }

    drawText(hudX + 12, hudY + 20, titleStr, hexCol(ACCENT_CYAN), FONT_MD);
    drawText(hudX + 12, hudY + 42, "PT: " + eqStr, hexCol(ACCENT_YELLOW), FONT_SM);

    char camInfo[128];
    snprintf(camInfo, sizeof camInfo, "Camera: Theta = %.1f deg, Phi = %.1f deg, Distance = %.1f | Scale = %.2fx",
             g.thetaDeg, g.phiDeg, g.eyeRadius, g.objScale);
    drawText(hudX + 12, hudY + 64, camInfo, hexCol(TEXT_MUTED), FONT_SM);

    drawText(hudX + 12, hudY + 86, "PHIM TAT: Chuot trai: Xoay | Cuon / Chuot phai: Zoom | Space: Tu dong quay / Chay t", hexCol(TEXT_MAIN), FONT_SM);
    drawText(hudX + 12, hudY + 104, "Phim Tab: Chuyen bai 5.1 -> 5.4 | Phim 1..5: Chuyen hinh | Phim M: Doi Solid/Wire", hexCol(TEXT_MUTED), FONT_SM);
}

// =============================================================================
// 10. MAIN DISPLAY CALLBACK & EVENT HANDLERS
// =============================================================================

static void display() {
    g.buttons.clear();
    g.sliders.clear();

    // 1. Ve khong gian 3D vao Viewport ben trai
    glViewport(0, 0, (GLsizei)viewportW(), (GLsizei)viewportH());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    glClearColor(0.04f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Thiet lap chieu phoi canh 3D (Perspective)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = viewportW() / viewportH();
    gluPerspective(45.0, aspect, 0.1, 100.0);

    // Thiet lap Camera theo goc quan sat theta, phi, radius
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3 eye = computeEyePosition(g.target, g.thetaDeg, g.phiDeg, g.eyeRadius);
    gluLookAt(eye.x, eye.y, eye.z, g.target.x, g.target.y, g.target.z, 0.0, 1.0, 0.0);

    // Chieu sang nguon sang Sunlight 3D
    glEnable(GL_LIGHT0);
    GLfloat lightPos[]  = { 8.0f, 14.0f, 10.0f, 1.0f };
    GLfloat lightAmb[]  = { 0.25f, 0.28f, 0.32f, 1.0f };
    GLfloat lightDiff[] = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat lightSpec[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);

    // Truc toa do va san luoi
    drawCoordinateAxes3D();
    drawGroundGrid3D();

    // Ap dung Scale vat the
    glPushMatrix();
    glScaled(g.objScale, g.objScale, g.objScale);

    // Ve vat the tuong ung voi tung chuyen muc de bai
    if (g.activeSection == SECTION_RULED) {
        SurfaceMesh m;
        if (g.ruledType == RULED_CYLINDER) {
            m = createCylinder(g.cylinderR, g.cylinderH, g.uvSlices, g.uvStacks);
        } else if (g.ruledType == RULED_CONE) {
            m = createCone(g.coneR, g.coneH, g.uvSlices, g.uvStacks);
        } else if (g.ruledType == RULED_HYPER_PARA) {
            m = createHyperbolicParaboloid(g.hpA, g.hpB, g.hpSize, g.uvSlices);
        } else {
            m = createConoid(g.conoidR, g.conoidH, g.uvSlices, g.uvStacks);
        }
        drawSurfaceMesh(m);
    } else if (g.activeSection == SECTION_REVOLUTION) {
        SurfaceMesh m;
        if (g.revType == REV_SPHERE) {
            m = createSphere(g.sphereR, g.uvSlices, g.uvStacks);
        } else if (g.revType == REV_ELLIPSOID) {
            m = createEllipsoid(g.ellipA, g.ellipB, g.ellipC, g.uvSlices, g.uvStacks);
        } else if (g.revType == REV_HYPERBOLOID) {
            m = createHyperboloidOneSheet(g.hyperA, g.hyperB, g.hyperH, g.uvSlices, g.uvStacks);
        } else if (g.revType == REV_HYPERBOLOID_2) {
            m = createHyperboloidTwoSheets(g.hyperA, g.hyperB, g.hyperH, g.uvSlices, g.uvStacks);
        } else if (g.revType == REV_TOROID) {
            m = createToroid(g.torusR, g.torusTubeR, g.uvSlices, g.uvStacks);
        } else {
            m = createParaboloid(g.paraR, g.paraH, g.uvSlices, g.uvStacks);
        }
        drawSurfaceMesh(m);
    } else if (g.activeSection == SECTION_BEZIER_CURV) {
        drawBezierCurveScene();
    } else if (g.activeSection == SECTION_BEZIER_SURF) {
        drawBezierSurfaceScene();
    }

    glPopMatrix();

    // 2. Chuyen sang che do 2D Orthographic ve Header, Sidebar & HUD
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glViewport(0, 0, g.windowW, g.windowH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, g.windowW, g.windowH, 0); // Toa do goc tren trai (0, 0)

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    renderHeader();
    renderSidebarUI();
    renderHUD();

    glutSwapBuffers();
}

static void reshape(int w, int h) {
    g.windowW = std::max(w, 800);
    g.windowH = std::max(h, 600);
    glutPostRedisplay();
}

static void timer(int v) {
    if (g.autoOrbit) {
        g.thetaDeg += 0.6;
        if (g.thetaDeg >= 360.0) g.thetaDeg -= 360.0;
        glutPostRedisplay();
    }

    if (g.animateCurveT && g.activeSection == SECTION_BEZIER_CURV) {
        g.curveT += g.animDir * 0.012;
        if (g.curveT > 1.0) {
            g.curveT = 1.0;
            g.animDir = -1;
        } else if (g.curveT < 0.0) {
            g.curveT = 0.0;
            g.animDir = 1;
        }
        glutPostRedisplay();
    }

    glutTimerFunc(16, timer, 0);
}

// =============================================================================
// 11. XU LY SU KIEN CHUOT VA BAN PHIM (MOUSE & KEYBOARD EVENTS)
// =============================================================================

static void mouse(int button, int state, int x, int y) {
    g.mouseX = x;
    g.mouseY = y;

    if (state == GLUT_DOWN) {
        // Kiem tra click cac nut bam UI
        for (const auto& btn : g.buttons) {
            if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
                if (btn.onClick) btn.onClick();
                glutPostRedisplay();
                return;
            }
        }

        // Kiem tra click cac thanh truot Slider
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

        // Xu ly chuot tren khung nhin 3D (Xoay hoac Zoom)
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

        // Cuon con lan chuot de Phong to / Thu nho (Mouse Wheel Zoom)
        if (button == 3) { // Wheel Up: Zoom in
            g.eyeRadius = std::max(2.5, g.eyeRadius - 0.5);
            glutPostRedisplay();
        } else if (button == 4) { // Wheel Down: Zoom out
            g.eyeRadius = std::min(30.0, g.eyeRadius + 0.5);
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

    // Keo Slider dang hoat dong
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

    // Keo chuot trai de xoay goc quan sat (Rotate View Orbit)
    if (g.isLeftDragging) {
        double dx = x - g.lastMouseX;
        double dy = y - g.lastMouseY;
        g.lastMouseX = x;
        g.lastMouseY = y;

        g.thetaDeg += dx * 0.45;
        if (g.thetaDeg >= 360.0) g.thetaDeg -= 360.0;
        if (g.thetaDeg < 0.0)    g.thetaDeg += 360.0;

        g.phiDeg -= dy * 0.45;
        if (g.phiDeg > 89.0)  g.phiDeg = 89.0;
        if (g.phiDeg < -89.0) g.phiDeg = -89.0;

        glutPostRedisplay();
    }

    // Keo chuot phai de Zoom
    if (g.isRightDragging) {
        double dy = y - g.lastMouseY;
        g.lastMouseY = y;
        g.eyeRadius = std::max(2.5, std::min(30.0, g.eyeRadius + dy * 0.05));
        glutPostRedisplay();
    }
}

static void passiveMotion(int x, int y) {
    g.mouseX = x;
    g.mouseY = y;
    glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case 27: // ESC
            exit(0);
            break;
        case 'r': case 'R': // Reset
            g.thetaDeg = 45.0;
            g.phiDeg = 25.0;
            g.eyeRadius = 9.0;
            g.objScale = 1.0;
            g.autoOrbit = false;
            g.animateCurveT = false;
            break;
        case ' ': // Space: Toggle auto orbit / curve animation
            if (g.activeSection == SECTION_BEZIER_CURV) {
                g.animateCurveT = !g.animateCurveT;
            } else {
                g.autoOrbit = !g.autoOrbit;
            }
            break;
        case '\t': // Tab: Chuyen chuyen muc tiep theo
            g.activeSection = (LabSection)(((int)g.activeSection + 1) % 4);
            break;
        case '+': case '=': // Phong to
            g.objScale = std::min(3.0, g.objScale + 0.1);
            break;
        case '-': case '_': // Thu nho
            g.objScale = std::max(0.2, g.objScale - 0.1);
            break;
        case '[': // Zoom camera gan lai
            g.eyeRadius = std::max(2.5, g.eyeRadius - 0.5);
            break;
        case ']': // Zoom camera ra xa
            g.eyeRadius = std::min(30.0, g.eyeRadius + 0.5);
            break;
        case 'm': case 'M': // Chuyen doi kieu ve (Solid / Wireframe / Both)
            g.renderStyle = (RenderStyle)(((int)g.renderStyle + 1) % 3);
            break;
        case 'c': case 'C': // Chon preset mau tiep theo
            g.colorPresetIdx = (g.colorPresetIdx + 1) % COLOR_PRESETS.size();
            {
                ColorRGBA sc = hexCol(COLOR_PRESETS[g.colorPresetIdx].hexColor);
                g.customR = sc.r;
                g.customG = sc.g;
                g.customB = sc.b;
            }
            break;
        case 'h': case 'H': // An/hien HUD
            g.showHUD = !g.showHUD;
            break;
        case 'a': case 'A': // Xoay trai
            g.thetaDeg -= 5.0;
            break;
        case 'd': case 'D': // Xoay phai
            g.thetaDeg += 5.0;
            break;
        case 'w': case 'W': // Xoay ngua len
            g.phiDeg = std::min(89.0, g.phiDeg + 5.0);
            break;
        case 's': case 'S': // Xoay cui xuong
            g.phiDeg = std::max(-89.0, g.phiDeg - 5.0);
            break;
        case '1': case '2': case '3': case '4': case '5': case '6':
            {
                int idx = key - '1';
                if (g.activeSection == SECTION_RULED && idx <= 3) {
                    g.ruledType = (RuledType)idx;
                } else if (g.activeSection == SECTION_REVOLUTION && idx <= 5) {
                    g.revType = (RevType)idx;
                } else if (g.activeSection == SECTION_BEZIER_CURV && idx <= 3) {
                    g.curveType = (BezierCurveType)idx;
                    g.curveCtrlPts = createBezierCurvePreset(g.curveType);
                    g.selectedCurvePtIdx = 1;
                } else if (g.activeSection == SECTION_BEZIER_SURF && idx <= 3) {
                    g.surfPreset = (BezierSurfacePreset)idx;
                    g.surfControlNet = createBezierSurfacePreset(g.surfPreset);
                }
            }
            break;
    }
    glutPostRedisplay();
}

static void special(int key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case GLUT_KEY_LEFT:
            g.thetaDeg -= 5.0;
            break;
        case GLUT_KEY_RIGHT:
            g.thetaDeg += 5.0;
            break;
        case GLUT_KEY_UP:
            g.phiDeg = std::min(89.0, g.phiDeg + 5.0);
            break;
        case GLUT_KEY_DOWN:
            g.phiDeg = std::max(-89.0, g.phiDeg - 5.0);
            break;
        case GLUT_KEY_PAGE_UP:
            g.eyeRadius = std::max(2.5, g.eyeRadius - 0.5);
            break;
        case GLUT_KEY_PAGE_DOWN:
            g.eyeRadius = std::min(30.0, g.eyeRadius + 0.5);
            break;
    }
    glutPostRedisplay();
}

// =============================================================================
// 12. HAM MAIN: KHOI DONG UNG DUNG HOAC CHAY KIEM THU
// =============================================================================

int main(int argc, char** argv) {
    // 1. Kiem tra neu co tham so --test: Chay headless khong can cua so do hoa
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            return runSelfTests();
        }
    }

    // 2. Khoi tao trang thai duong va mat cong Bezier mac dinh
    g.curveCtrlPts = createBezierCurvePreset(CURVE_CUBIC_S);
    g.surfControlNet = createBezierSurfacePreset(SURF_PRESET_WAVE);

    ColorRGBA defCol = hexCol(COLOR_PRESETS[g.colorPresetIdx].hexColor);
    g.customR = defCol.r;
    g.customG = defCol.g;
    g.customB = defCol.b;

    // 3. Khoi tao GLUT va OpenGL Window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(g.windowW, g.windowH);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("VKU GRAPHICS - LAB 5: DUONG VA MAT CONG 3D (RULED, REVOLUTION, BEZIER)");

    // 4. Dang ky cac ham callback
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passiveMotion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);

    glutTimerFunc(16, timer, 0);

    printf("=================================================================\n");
    printf(" TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN   \n");
    printf(" LAB 5: CHUONG TRINH VE DUONG VA MAT CONG TRONG KHONG GIAN 3D   \n");
    printf("=================================================================\n");
    printf(" - 5.1: Mat co quy tac: Cylinder, Cone, Hyperbolic Paraboloid, Conoid\n");
    printf(" - 5.2: Mat tron xoay: Sphere, Ellipsoid, Hyperboloid, Toroid, Paraboloid\n");
    printf(" - 5.3: Duong cong Bezier: Bernstein, De Casteljau, Animation t\n");
    printf(" - 5.4: Mat cong Bezier 4x4: Bicubic Surface, Control Net, Y-Heights\n");
    printf(" - Tuong tac: Chuot xoay, Zoom, Sliders RGB, Thay doi mau sac,..\n");
    printf("=================================================================\n\n");

    glutMainLoop();
    return 0;
}
