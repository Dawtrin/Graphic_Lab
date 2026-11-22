// =============================================================================
// TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN (VKU)
// KHOA KY THUAT MAY TINH VA DIEN TU - BO MON DO HOA MAY TINH
// =============================================================================
// LAB 4: CHUONG TRINH QUAN SAT VAT THE 3D (3D VIEWING & TRANSFORMATION LAB)
//
// YEU CAU DE BAI:
// 4.1. Su dung phep quay 3D quanh truc Ox, Oy, Oz de quan sat vat the
// 4.2. Dung ma tran bien doi he quan sat de quan sat vat the bang cach:
//      - Thay doi phep chieu: Song song (Orthographic) hoac Phoi canh (Perspective)
//      - Thay doi goc quan sat theta (Azimuth), Phi (Elevation) de xoay vat the
//      - Thay doi vi tri quan sat (Diem nhin / Camera Eye & Look-At Target)
//      - Phong to, thu nho vat the (Scaling & Camera Distance / Zoom)
//
// Yeu cau ky thuat:
//      - Vat the duoc bieu dien theo Mo hinh mat da giac (Polygon Mesh Model)
//      - Dung su kien phim, chuot de dieu chinh goc quay, vi tri quan sat,...
//      - Bo sung mo hinh da giac dac biet: Linh dac nhiem cam sung (Commando)
//
// Chuan toan hoc (VKU Computer Graphics):
//      Vector hang P = [x y z 1], P' = P x M_4x4
//
// Bien dich:
//      cmake -B build
//      cmake --build build --target Lab4_3D_main
// Chay ung dung:
//      Lab4_3D_main.exe         (Giao dien do hoa 3D tuong tac)
//      Lab4_3D_main.exe --test  (Chay bo kiem thu tu dong Self-Tests)
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
// 1. TOAN HOC 3D & MA TRAN THUAN NHAT (3D MATH & 4x4 MATRICES)
// Quy uoc Vector hang (Row-Vector Convention): P = [x y z 1], P' = P x M
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

// --- 4.1: CAC PHEP QUAY 3D QUANH TRUC Ox, Oy, Oz ---
// 1. Quay quanh truc Ox mot goc alpha (rad)
static Mat4 rotX4(double rad) {
    Mat4 r = ident4();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[1][1] = c;  r.m[1][2] = s;
    r.m[2][1] = -s; r.m[2][2] = c;
    return r;
}

// 2. Quay quanh truc Oy mot goc beta (rad)
static Mat4 rotY4(double rad) {
    Mat4 r = ident4();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][2] = -s;
    r.m[2][0] = s;  r.m[2][2] = c;
    return r;
}

// 3. Quay quanh truc Oz mot goc gamma (rad)
static Mat4 rotZ4(double rad) {
    Mat4 r = ident4();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c;  r.m[0][1] = s;
    r.m[1][0] = -s; r.m[1][1] = c;
    return r;
}

// Ma tran quay tong hop quanh 3 truc (Euler angles)
static Mat4 rotationEuler4(double degX, double degY, double degZ) {
    double radX = degX * PI / 180.0;
    double radY = degY * PI / 180.0;
    double radZ = degZ * PI / 180.0;
    return rotZ4(radZ) * rotY4(radY) * rotX4(radX);
}

// Ma tran tinh tien 3D: T(tx, ty, tz)
static Mat4 translation4(double tx, double ty, double tz) {
    Mat4 r = ident4();
    r.m[3][0] = tx;
    r.m[3][1] = ty;
    r.m[3][2] = tz;
    return r;
}

// Ma tran ti le 3D (Phong to, thu nho vat the): S(sx, sy, sz)
static Mat4 scaling4(double sx, double sy, double sz) {
    Mat4 r = ident4();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    r.m[2][2] = sz;
    return r;
}

// --- 4.2: MA TRAN BIEN DOI HE QUAN SAT (VIEWING TRANSFORMATION MATRIX) ---
// Tinh toa do diem nhin (Eye Position) tu toa do cau (theta, phi, R) quanh Look-At target C
static Vec3 computeEyePosition(const Vec3& target, double thetaDeg, double phiDeg, double radius) {
    double theta = thetaDeg * PI / 180.0; // Goc phuong vi (Azimuth / Yaw)
    double phi   = phiDeg   * PI / 180.0; // Goc ngang (Elevation / Pitch)

    double x = target.x + radius * std::cos(phi) * std::sin(theta);
    double y = target.y + radius * std::sin(phi);
    double z = target.z + radius * std::cos(phi) * std::cos(theta);
    return { x, y, z };
}
static Mat4 viewingMatrix4(const Vec3& eye, const Vec3& target, const Vec3& upGuide = { 0, 1, 0 }) {
    Vec3 f = normalize(target - eye); 
    Vec3 n = normalize(eye - target);

    Vec3 up = upGuide;
    if (std::fabs(dot(n, up)) > 0.999) {
        up = { 0, 0, -1 };
    }

    Vec3 u = normalize(cross(up, n)); 
    Vec3 v = cross(n, u);           

    Mat4 M = ident4();
    M.m[0][0] = u.x;  M.m[0][1] = v.x;  M.m[0][2] = n.x;
    M.m[1][0] = u.y;  M.m[1][1] = v.y;  M.m[1][2] = n.y;
    M.m[2][0] = u.z;  M.m[2][1] = v.z;  M.m[2][2] = n.z;
    M.m[3][0] = -dot(eye, u);
    M.m[3][1] = -dot(eye, v);
    M.m[3][2] = -dot(eye, n);
    return M;
}

// Phep chieu hinh hoc truc tiep sang mat phang chieu 2D (View Plane / Screen Plane)
static Vec3 projectToScreen(const Vec3& pv, bool isPerspective, double d = 4.0, double zoom = 1.0) {
    if (!isPerspective) {
        return { pv.x * zoom, pv.y * zoom, 0.0 };
    } else {
        double z = -pv.z;
        if (z < 0.2) z = 0.2;
        double factor = (d / z) * zoom;
        return { pv.x * factor, pv.y * factor, z };
    }
}

// =============================================================================
// 2. MO HINH MAT DA GIAC (POLYGON MESH MODEL)
// =============================================================================

struct Face3D {
    std::vector<int> vertexIndices;
    Vec3 normal;
};

struct Mesh3D {
    std::string name;
    std::vector<Vec3> vertices;
    std::vector<Face3D> faces;
    std::vector<std::pair<int, int>> edges;
    std::vector<unsigned> faceColors; // Mau sac rieng cho tung mat
};

// Tinh toan vector phap tuyen cho tat ca cac mat da giac
static void updateMeshNormals(Mesh3D& mesh) {
    for (auto& f : mesh.faces) {
        if (f.vertexIndices.size() >= 3) {
            Vec3 v0 = mesh.vertices[f.vertexIndices[0]];
            Vec3 v1 = mesh.vertices[f.vertexIndices[1]];
            Vec3 v2 = mesh.vertices[f.vertexIndices[2]];
            f.normal = normalize(cross(v1 - v0, v2 - v0));
        } else {
            f.normal = { 0, 1, 0 };
        }
    }
}

// 1. Khoi tao Hinh chop tu dien ABCD (Pyramid / Tetrahedron)
static Mesh3D createPyramidMesh() {
    Mesh3D m;
    m.name = "Hinh chop ABCD (Tetrahedron)";
    m.vertices = {
        {  0.0,  2.2,  0.0 }, // 0: A (Dinh chop - Apex)
        { -1.8, -1.2,  1.4 }, // 1: B (Day truoc trai)
        {  1.8, -1.2,  1.4 }, // 2: C (Day truoc phai)
        {  0.0, -1.2, -1.8 }  // 3: D (Day sau)
    };
    m.faces = {
        { { 0, 1, 2 }, {} }, // ABC (Mat truoc)
        { { 0, 2, 3 }, {} }, // ACD (Mat phai)
        { { 0, 3, 1 }, {} }, // ADB (Mat trai)
        { { 1, 3, 2 }, {} }  // BDC (Mat day)
    };
    m.edges = {
        { 0, 1 }, { 0, 2 }, { 0, 3 },
        { 1, 2 }, { 2, 3 }, { 3, 1 }
    };
    m.faceColors = {
        0x00E5BE, // Mat truoc: Xanh ngoc sang
        0xFFA62B, // Mat phai: Vang cam
        0x9D4EDD, // Mat trai: Tim sang
        0x3A86FF  // Mat day: Xanh Cobalt
    };
    updateMeshNormals(m);
    return m;
}

// 2. Khoi tao Khoi lap phuong (Cube)
static Mesh3D createCubeMesh() {
    Mesh3D m;
    m.name = "Khoi lap phuong (Cube 6 mat)";
    double s = 1.35;
    m.vertices = {
        { -s, -s,  s }, {  s, -s,  s }, {  s,  s,  s }, { -s,  s,  s }, // 0..3: Mat truoc
        { -s, -s, -s }, {  s, -s, -s }, {  s,  s, -s }, { -s,  s, -s }  // 4..7: Mat sau
    };
    m.faces = {
        { { 0, 1, 2, 3 }, {} }, // Truoc (+Z)
        { { 5, 4, 7, 6 }, {} }, // Sau (-Z)
        { { 4, 0, 3, 7 }, {} }, // Trai (-X)
        { { 1, 5, 6, 2 }, {} }, // Phai (+X)
        { { 3, 2, 6, 7 }, {} }, // Tren (+Y)
        { { 4, 5, 1, 0 }, {} }  // Duoi (-Y)
    };
    m.edges = {
        {0,1},{1,2},{2,3},{3,0}, // Truoc
        {4,5},{5,6},{6,7},{7,4}, // Sau
        {0,4},{1,5},{2,6},{3,7}  // Noi
    };
    m.faceColors = {
        0x00D2D3, // Truoc: Cyan
        0xFF6B6B, // Sau: Do san ho
        0xF368E0, // Trai: Hong tim
        0x10AC84, // Phai: Xanh ngoc
        0xFF9F43, // Tren: Cam tuoi
        0x54A0FF  // Duoi: Xanh troi
    };
    updateMeshNormals(m);
    return m;
}

// 3. Khoi tao Mo hinh Ngoi nha 3D (VKU 3D House Model)
static Mesh3D createHouseMesh() {
    Mesh3D m;
    m.name = "Ngoi nha 3D (VKU House Model)";
    m.vertices = {
        { -1.4, -1.2,  1.2 }, {  1.4, -1.2,  1.2 }, {  1.4,  0.6,  1.2 }, { -1.4,  0.6,  1.2 }, // Truoc
        { -1.4, -1.2, -1.2 }, {  1.4, -1.2, -1.2 }, {  1.4,  0.6, -1.2 }, { -1.4,  0.6, -1.2 }, // Sau
        {  0.0,  1.9,  1.2 }, // 8: Dinh mai truoc
        {  0.0,  1.9, -1.2 }  // 9: Dinh mai sau
    };
    m.faces = {
        { { 0, 1, 2, 3 }, {} },       // Tuong truoc
        { { 5, 4, 7, 6 }, {} },       // Tuong sau
        { { 4, 0, 3, 7 }, {} },       // Tuong trai
        { { 1, 5, 6, 2 }, {} },       // Tuong phai
        { { 4, 5, 1, 0 }, {} },       // San nha
        { { 3, 2, 8 }, {} },          // Mat mai tam giac truoc
        { { 6, 7, 9 }, {} },          // Mat mai tam giac sau
        { { 2, 6, 9, 8 }, {} },       // Mai nghieng phai
        { { 7, 3, 8, 9 }, {} }        // Mai nghieng trai
    };
    m.edges = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7},
        {3,8},{2,8},{7,9},{6,9},{8,9}
    };
    m.faceColors = {
        0x38EF7D, 0x11998E, 0x00B4DB, 0x0083B0, 0x4B6584,
        0xFF5252, 0xD63031, 0xFFA62B, 0xE17055
    };
    updateMeshNormals(m);
    return m;
}

// 4. Khoi tao Khoi bat dien deu (Octahedron)
static Mesh3D createOctahedronMesh() {
    Mesh3D m;
    m.name = "Khoi bat dien deu (Octahedron)";
    double r = 1.8;
    m.vertices = {
        {  0,  r,  0 }, // 0: Dinh tren
        {  r,  0,  0 }, // 1: +X
        {  0,  0,  r }, // 2: +Z
        { -r,  0,  0 }, // 3: -X
        {  0,  0, -r }, // 4: -Z
        {  0, -r,  0 }  // 5: Dinh duoi
    };
    m.faces = {
        { { 0, 1, 2 }, {} }, { { 0, 2, 3 }, {} }, { { 0, 3, 4 }, {} }, { { 0, 4, 1 }, {} },
        { { 5, 2, 1 }, {} }, { { 5, 3, 2 }, {} }, { { 5, 4, 3 }, {} }, { { 5, 1, 4 }, {} }
    };
    m.edges = {
        {0,1},{0,2},{0,3},{0,4},
        {1,2},{2,3},{3,4},{4,1},
        {5,1},{5,2},{5,3},{5,4}
    };
    m.faceColors = {
        0xFF6B6B, 0x4ECDC4, 0xFFE66D, 0x1A535C,
        0xF7FFF7, 0x70A1FF, 0x2ED573, 0xFFA502
    };
    updateMeshNormals(m);
    return m;
}

// 5. Khoi tao Mo hinh Linh dac nhiem cam sung (Special Forces Commando holding an Assault Rifle)
static Mesh3D createSoldierMesh() {
    Mesh3D m;
    m.name = "Linh dac nhiem cam sung (Commando)";

    auto addBox = [&](Vec3 p0, Vec3 p1, unsigned col) {
        int baseIdx = (int)m.vertices.size();
        double x0 = std::min(p0.x, p1.x), x1 = std::max(p0.x, p1.x);
        double y0 = std::min(p0.y, p1.y), y1 = std::max(p0.y, p1.y);
        double z0 = std::min(p0.z, p1.z), z1 = std::max(p0.z, p1.z);

        // 8 dinh
        m.vertices.push_back({ x0, y0, z1 }); // 0: Truoc duoi trai
        m.vertices.push_back({ x1, y0, z1 }); // 1: Truoc duoi phai
        m.vertices.push_back({ x1, y1, z1 }); // 2: Truoc tren phai
        m.vertices.push_back({ x0, y1, z1 }); // 3: Truoc tren trai
        m.vertices.push_back({ x0, y0, z0 }); // 4: Sau duoi trai
        m.vertices.push_back({ x1, y0, z0 }); // 5: Sau duoi phai
        m.vertices.push_back({ x1, y1, z0 }); // 6: Sau tren phai
        m.vertices.push_back({ x0, y1, z0 }); // 7: Sau tren trai

        // 6 mat da giac
        m.faces.push_back({ { baseIdx + 0, baseIdx + 1, baseIdx + 2, baseIdx + 3 }, {} }); // Truoc (+Z)
        m.faces.push_back({ { baseIdx + 5, baseIdx + 4, baseIdx + 7, baseIdx + 6 }, {} }); // Sau (-Z)
        m.faces.push_back({ { baseIdx + 4, baseIdx + 0, baseIdx + 3, baseIdx + 7 }, {} }); // Trai (-X)
        m.faces.push_back({ { baseIdx + 1, baseIdx + 5, baseIdx + 6, baseIdx + 2 }, {} }); // Phai (+X)
        m.faces.push_back({ { baseIdx + 3, baseIdx + 2, baseIdx + 6, baseIdx + 7 }, {} }); // Tren (+Y)
        m.faces.push_back({ { baseIdx + 4, baseIdx + 5, baseIdx + 1, baseIdx + 0 }, {} }); // Duoi (-Y)

        for (int k = 0; k < 6; k++) {
            m.faceColors.push_back(col);
        }

        // 12 canh khung
        m.edges.push_back({ baseIdx + 0, baseIdx + 1 });
        m.edges.push_back({ baseIdx + 1, baseIdx + 2 });
        m.edges.push_back({ baseIdx + 2, baseIdx + 3 });
        m.edges.push_back({ baseIdx + 3, baseIdx + 0 });
        m.edges.push_back({ baseIdx + 4, baseIdx + 5 });
        m.edges.push_back({ baseIdx + 5, baseIdx + 6 });
        m.edges.push_back({ baseIdx + 6, baseIdx + 7 });
        m.edges.push_back({ baseIdx + 7, baseIdx + 4 });
        m.edges.push_back({ baseIdx + 0, baseIdx + 4 });
        m.edges.push_back({ baseIdx + 1, baseIdx + 5 });
        m.edges.push_back({ baseIdx + 2, baseIdx + 6 });
        m.edges.push_back({ baseIdx + 3, baseIdx + 7 });
    };

    // 1. Dau & Mat na trum dau (Head & Tactical Balaclava)
    addBox({ -0.30, 1.25, -0.28 }, { 0.30, 1.78, 0.28 }, 0x3D4A3E); // Olive camo

    // 2. Mu bao hiem chien thuat (Tactical Combat Helmet)
    addBox({ -0.36, 1.55, -0.34 }, { 0.36, 1.95, 0.34 }, 0x243324); // Xanh quan doi dam

    // 3. Kinh nhin dem / Kinh râm chien thuat (Night Vision Visor)
    addBox({ -0.26, 1.48, 0.28 }, { 0.26, 1.64, 0.42 }, 0x00E5BE); // Kinh phan quang xanh ngoc

    // 4. Than & Ao giap chien thuat (Torso & Body Armor)
    addBox({ -0.52, 0.15, -0.32 }, { 0.52, 1.25, 0.32 }, 0x27382B); // Ao giap camo

    // 5. Ngan tui tieu lien / hop dan truoc nguc (Tactical Ammo Pouches)
    addBox({ -0.38, 0.45, 0.32 }, { 0.38, 0.95, 0.44 }, 0x1E2B1E);

    // 6. That lung chien thuat (Tactical Duty Belt)
    addBox({ -0.54, 0.05, -0.34 }, { 0.54, 0.20, 0.34 }, 0x181F18); // Den dam

    // 7. Balo chien thuat sau lung (Backpack)
    addBox({ -0.38, 0.30, -0.62 }, { 0.38, 1.15, -0.32 }, 0x2C3D2F);

    // 8. Quan linh & 2 Chan (Tactical Pants & Legs)
    addBox({ -0.48, -1.15, -0.22 }, { -0.10, 0.05, 0.22 }, 0x3A4A3B); // Chan trai
    addBox({  0.10, -1.15, -0.22 }, {  0.48, 0.05, 0.22 }, 0x3A4A3B); // Chan phai

    // 9. Giay chien thuat / Ung quan su (Combat Boots)
    addBox({ -0.50, -1.50, -0.26 }, { -0.08, -1.15, 0.38 }, 0x16181B); // Giay trai
    addBox({  0.08, -1.50, -0.26 }, {  0.50, -1.15, 0.38 }, 0x16181B); // Giay phai

    // 10. Tay phai (Right Arm - giu tay cam co sung)
    addBox({ 0.52, 0.75, -0.10 }, { 0.82, 1.20, 0.28 }, 0x2F4032); // Bap tay phai
    addBox({ 0.38, 0.65,  0.28 }, { 0.68, 0.95, 0.72 }, 0x3D4A3E); // Cang tay phai

    // 11. Tay trai (Left Arm - vuon ra do nong sung)
    addBox({ -0.82, 0.75, -0.10 }, { -0.52, 1.20, 0.28 }, 0x2F4032); // Bap tay trai
    addBox({ -0.55, 0.65,  0.28 }, { -0.05, 0.95, 0.85 }, 0x3D4A3E); // Cang tay trai

    // 12. Khau sung truong dac nhiem (Assault Rifle / Carbine)
    // - Bang sung (Stock)
    addBox({ 0.12, 0.72, 0.10 }, { 0.24, 0.92, 0.52 }, 0x1A1E24);
    // - Than sung (Receiver & Body)
    addBox({ 0.06, 0.68, 0.52 }, { 0.26, 0.95, 1.35 }, 0x2B313A);
    // - Nong sung dai vuon ve phia truoc (Long Barrel)
    addBox({ 0.12, 0.78, 1.35 }, { 0.20, 0.86, 2.35 }, 0x485460);
    // - Dau nong giam giat (Muzzle Brake)
    addBox({ 0.10, 0.76, 2.35 }, { 0.22, 0.88, 2.52 }, 0x111317);
    // - Bang dan cong dac trung (Curved Magazine)
    addBox({ 0.11, 0.26, 0.82 }, { 0.21, 0.68, 1.05 }, 0x1F242B);
    // - Tay cam co sung (Pistol Grip)
    addBox({ 0.11, 0.45, 0.54 }, { 0.21, 0.72, 0.70 }, 0x1A1E24);
    // - Ong ngam quang hoc Laser (Tactical Scope / Red Dot Sight)
    addBox({ 0.12, 0.95, 0.75 }, { 0.20, 1.12, 1.15 }, 0xFF4757); // Ong ngam do
    // - Op lot tay nong sung (Tactical Handguard Desert Tan)
    addBox({ 0.08, 0.72, 1.10 }, { 0.24, 0.90, 1.45 }, 0xC8A165); // Vang sa mac

    updateMeshNormals(m);
    return m;
}

// =============================================================================
// 3. APPLICATION STATE & CONTROLS
// =============================================================================

enum LabSection {
    SECTION_ROTATION_3D = 0, // 4.1. Phep quay 3D quanh Ox, Oy, Oz
    SECTION_VIEW_TRANS  = 1  // 4.2. Ma tran bien doi he quan sat
};

enum ProjectionMode {
    PROJ_PARALLEL    = 0, // Chieu song song (Orthographic)
    PROJ_PERSPECTIVE = 1  // Chieu phoi canh (Perspective)
};

enum RenderStyle {
    STYLE_WIREFRAME = 0, // Mo hinh khung ket noi (Wireframe)
    STYLE_POLYGON   = 1, // Mo hinh mat da giac (Solid Shaded)
    STYLE_BOTH      = 2  // Ket hop ca hai (Shaded + Wireframe Edges)
};

enum ActiveRotAxis {
    AXIS_OX = 0,
    AXIS_OY = 1,
    AXIS_OZ = 2
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
    // Danh sach mo hinh vat the da giac
    std::vector<Mesh3D> meshes;
    int currentMeshIdx = 4; // Mac dinh chon Linh dac nhiem cam sung

    // Chuyen muc Lab (4.1 hoac 4.2)
    LabSection activeSection = SECTION_VIEW_TRANS;

    // 4.1. PHEP QUAY 3D QUANH TRUC Ox, Oy, Oz
    double rotOx = 15.0;  // Goc quay quanh Ox (deg)
    double rotOy = 30.0;  // Goc quay quanh Oy (deg)
    double rotOz = 0.0;   // Goc quay quanh Oz (deg)
    ActiveRotAxis activeAxis = AXIS_OY;
    bool autoSpinOx = false;
    bool autoSpinOy = false;
    bool autoSpinOz = false;

    // 4.2. BIEN DOI HE QUAN SAT
    // Phep chieu
    ProjectionMode projMode = PROJ_PERSPECTIVE;
    double focalLengthD = 4.5; // Khoang cach tu mat toi mat phang chieu d

    // Goc quan sat
    double thetaDeg = 35.0; // Goc phuong vi theta (0..360 deg)
    double phiDeg   = 20.0; // Goc ngẩng phi (-89..+89 deg)

    // Vi tri quan sat (Look-At target & Eye distance)
    Vec3 target = { 0.0, 0.2, 0.5 }; // Tam quan sat C (Look-At giua than linh va sung)
    double eyeRadius = 10.0;         // Khoang cach quan sat R

    // Phong to, thu nho vat the (Scaling)
    double objScale = 1.0;           // He so ti le vat the S

    // Kieu bieu dien mat da giac
    RenderStyle renderStyle = STYLE_BOTH;
    double faceOpacity = 0.90;

    // Hien thi bo tro
    bool showAxes = true;
    bool showGrid = true;
    bool showLabels = true;
    bool showNormals = false;
    bool showRotationGizmo = true;
    bool showMatrixHUD = true;
    bool autoOrbit = false; // Tu dong xoay goc quan sat

    // Kich thuoc man hinh & Tuong tac chuot
    int windowW = 1380, windowH = 860;
    double mouseX = 0, mouseY = 0;
    double lastMouseX = 0, lastMouseY = 0;
    bool isLeftDragging = false;
    bool isRightDragging = false;
    int activeSliderId = -1;

    std::vector<UIControlBtn> buttons;
    std::vector<UIControlSlider> sliders;
} g;

static const double HEADER_H = 60.0;
static double sidePanelW() { return 470.0; }
static double viewportW()  { return g.windowW - sidePanelW(); }
static double viewportH()  { return g.windowH - HEADER_H; }

// Lay vat the mesh dang duoc chon
static Mesh3D& currentMesh() {
    return g.meshes[g.currentMeshIdx];
}

// Reset ve gia tri mac dinh
static void resetToDefaults() {
    g.rotOx = 15.0;
    g.rotOy = 30.0;
    g.rotOz = 0.0;
    g.thetaDeg = 35.0;
    g.phiDeg = 20.0;
    g.eyeRadius = 10.0;
    g.target = { 0.0, 0.2, 0.5 };
    g.objScale = 1.0;
    g.projMode = PROJ_PERSPECTIVE;
    g.focalLengthD = 4.5;
    g.autoOrbit = false;
    g.autoSpinOx = false;
    g.autoSpinOy = false;
    g.autoSpinOz = false;
}

// =============================================================================
// 4. KIEM THU TU DONG (SELF-TESTS SUITE)
// Chay headless bang tham so --test de kiem tra toan hoc 100%
// =============================================================================

static int runSelfTests() {
    int total = 0, fail = 0;
    auto chk = [&](bool c, const char* name) {
        total++;
        if (!c) fail++;
        printf(" [%s] %s\n", c ? "PASS" : "FAIL", name);
    };
    auto eqV = [](const Vec3& a, const Vec3& b, double eps = 1e-6) {
        return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps && std::fabs(a.z - b.z) < eps;
    };
    auto eqD = [](double a, double b, double eps = 1e-6) {
        return std::fabs(a - b) < eps;
    };

    printf("=================================================================\n");
    printf("   VKU LAB 4: 3D VIEWING & TRANSFORMATION MATH SELF-TESTS        \n");
    printf("=================================================================\n");

    // --- PHAN 4.1: PHEP QUAY 3D QUANH TRUC Ox, Oy, Oz ---
    printf("\n--- KIEM THU 4.1: PHEP QUAY 3D QUANH TRUC Ox, Oy, Oz ---\n");
    chk(eqV(Vec3{ 1, 2, 3 } * rotX4(0), { 1, 2, 3 }), "R1.1: Quay Ox goc 0 rad khong doi");
    chk(eqV(Vec3{ 1, 2, 3 } * rotY4(0), { 1, 2, 3 }), "R1.2: Quay Oy goc 0 rad khong doi");
    chk(eqV(Vec3{ 1, 2, 3 } * rotZ4(0), { 1, 2, 3 }), "R1.3: Quay Oz goc 0 rad khong doi");

    chk(eqV(Vec3{ 1, 0, 0 } * rotX4(PI / 2.0), { 1, 0, 0 }), "R2.1: Diem tren truc Ox bat bien qua phep quay Ox");
    chk(eqV(Vec3{ 0, 1, 0 } * rotX4(PI / 2.0), { 0, 0, 1 }), "R2.2: Quay 90 deg quanh Ox: (0,1,0) -> (0,0,1)");
    chk(eqV(Vec3{ 0, 0, 1 } * rotX4(PI / 2.0), { 0, -1, 0 }), "R2.3: Quay 90 deg quanh Ox: (0,0,1) -> (0,-1,0)");

    chk(eqV(Vec3{ 0, 1, 0 } * rotY4(PI / 2.0), { 0, 1, 0 }), "R3.1: Diem tren truc Oy bat bien qua phep quay Oy");
    chk(eqV(Vec3{ 0, 0, 1 } * rotY4(PI / 2.0), { 1, 0, 0 }), "R3.2: Quay 90 deg quanh Oy: (0,0,1) -> (1,0,0)");
    chk(eqV(Vec3{ 1, 0, 0 } * rotY4(PI / 2.0), { 0, 0, -1 }), "R3.3: Quay 90 deg quanh Oy: (1,0,0) -> (0,0,-1)");

    chk(eqV(Vec3{ 0, 0, 1 } * rotZ4(PI / 2.0), { 0, 0, 1 }), "R4.1: Diem tren truc Oz bat bien qua phep quay Oz");
    chk(eqV(Vec3{ 1, 0, 0 } * rotZ4(PI / 2.0), { 0, 1, 0 }), "R4.2: Quay 90 deg quanh Oz: (1,0,0) -> (0,1,0)");
    chk(eqV(Vec3{ 0, 1, 0 } * rotZ4(PI / 2.0), { -1, 0, 0 }), "R4.3: Quay 90 deg quanh Oz: (0,1,0) -> (-1,0,0)");

    {
        Vec3 p1{ 1.0, 2.0, 3.0 }, p2{ -2.0, 1.5, 0.5 };
        double d0 = length(p1 - p2);
        Mat4 R = rotationEuler4(37.5, -45.0, 60.0);
        double d1 = length((p1 * R) - (p2 * R));
        chk(eqD(d0, d1), "R5: Phep quay 3D tong hop bao toan khoang cach giua 2 diem");
    }

    // --- PHAN 4.2: MA TRAN HE QUAN SAT, PHEP CHIEU & THU PHONG ---
    printf("\n--- KIEM THU 4.2: HE QUAN SAT, PHEP CHIEU & THU PHONG ---\n");
    {
        Vec3 origin{ 0, 0, 0 };
        Vec3 e1 = computeEyePosition(origin, 0.0, 0.0, 10.0);
        chk(eqV(e1, { 0, 0, 10 }), "V1.1: Toa do cau (theta=0, phi=0, R=10) -> Eye=(0, 0, 10)");

        Vec3 e2 = computeEyePosition(origin, 90.0, 0.0, 10.0);
        chk(eqV(e2, { 10, 0, 0 }), "V1.2: Toa do cau (theta=90, phi=0, R=10) -> Eye=(10, 0, 0)");

        Vec3 e3 = computeEyePosition(origin, 0.0, 90.0, 10.0);
        chk(eqV(e3, { 0, 10, 0 }), "V1.3: Toa do cau (theta=0, phi=90, R=10) -> Eye=(0, 10, 0)");
    }

    {
        Vec3 eye{ 0, 0, 10 };
        Vec3 target{ 0, 0, 0 };
        Mat4 V = viewingMatrix4(eye, target, { 0, 1, 0 });
        Vec3 pTargetInView = target * V;
        chk(eqD(pTargetInView.x, 0.0) && eqD(pTargetInView.y, 0.0), "V2.1: Target nam chinh giua tam he quan sat (xv=0, yv=0)");
        chk(eqD(pTargetInView.z, -10.0), "V2.2: Chieu sau Target trong VCS bang dung khoang cach -R (-10)");

        Vec3 pEyeInView = eye * V;
        chk(eqV(pEyeInView, { 0, 0, 0 }), "V2.3: Diem nhin Eye chuyen thanh goc toa do VCS (0, 0, 0)");
    }

    {
        Vec3 pv1{ 2.0, 3.0, -5.0 };
        Vec3 pv2{ 2.0, 3.0, -15.0 };
        Vec3 scr1 = projectToScreen(pv1, false, 4.0, 1.0);
        Vec3 scr2 = projectToScreen(pv2, false, 4.0, 1.0);
        chk(eqV(scr1, scr2), "P1.1: Chieu song song: Toa do chieu khong phu thuoc vao do sau z");
        chk(eqD(scr1.x, 2.0) && eqD(scr1.y, 3.0), "P1.2: Chieu song song: (x_screen, y_screen) == (xv, yv)");
    }

    {
        Vec3 pvNear{ 2.0, 2.0, -4.0 };
        Vec3 pvFar { 2.0, 2.0, -8.0 };
        double d = 4.0;
        Vec3 scrNear = projectToScreen(pvNear, true, d, 1.0);
        Vec3 scrFar  = projectToScreen(pvFar,  true, d, 1.0);
        chk(eqD(scrNear.x, 2.0) && eqD(scrNear.y, 2.0), "P2.1: Phoi canh: Diem cach mat phang tieu cu d khong bi thu phong");
        chk(eqD(scrFar.x, 1.0) && eqD(scrFar.y, 1.0),   "P2.2: Phoi canh: Diem o xa gap doi bi thu nho 1/2");
    }

    {
        Vec3 p{ 1.0, 2.0, -1.0 };
        double s = 2.5;
        Mat4 S = scaling4(s, s, s);
        Vec3 pScaled = p * S;
        chk(eqV(pScaled, { 2.5, 5.0, -2.5 }), "S1.1: Ma tran ti le S(2.5) phong to dung ty le cac truc");

        double sTiny = 0.5;
        Mat4 STiny = scaling4(sTiny, sTiny, sTiny);
        chk(eqV(p * STiny, { 0.5, 1.0, -0.5 }), "S1.2: Ma tran ti le S(0.5) thu nho dung ty le");
    }

    {
        Mesh3D pyr = createPyramidMesh();
        chk(pyr.vertices.size() == 4, "M1.1: Hinh chop ABCD co dung 4 dinh");
        chk(pyr.faces.size() == 4,    "M1.2: Hinh chop ABCD co dung 4 mat da giac");
        chk(pyr.edges.size() == 6,    "M1.3: Hinh chop ABCD co dung 6 canh khung");

        Mesh3D cube = createCubeMesh();
        chk(cube.vertices.size() == 8, "M1.4: Khoi lap phuong co dung 8 dinh");
        chk(cube.faces.size() == 6,    "M1.5: Khoi lap phuong co dung 6 mat da giac");
        chk(cube.edges.size() == 12,   "M1.6: Khoi lap phuong co dung 12 canh");

        Mesh3D house = createHouseMesh();
        chk(house.vertices.size() == 10, "M1.7: Mo hinh Ngoi nha 3D co dung 10 dinh");
        chk(house.faces.size() == 9,     "M1.8: Mo hinh Ngoi nha 3D co dung 9 mat");

        Mesh3D soldier = createSoldierMesh();
        chk(!soldier.vertices.empty(),   "M1.9: Mo hinh Linh dac nhiem cam sung khoi tao thanh cong");
        chk(soldier.faces.size() >= 50,  "M1.10: Mo hinh Linh dac nhiem co day du cac mat da giac chi tiet");
        chk(!soldier.edges.empty(),      "M1.11: Mo hinh Linh dac nhiem co day du khung canh 3D");
    }

    printf("=================================================================\n");
    printf("  KET QUA KIEM THU: %d / %d tests PASSED!\n", total - fail, total);
    printf("=================================================================\n\n");
    return fail ? 1 : 0;
}

// =============================================================================
// 5. DO HOA 2D & UI HELPERS (COLORS, BUTTONS, SLIDERS)
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

static const unsigned BG_DARK_COLOR   = 0x0F1318;
static const unsigned PANEL_BG_COLOR  = 0x161B22;
static const unsigned CARD_BG_COLOR   = 0x1F2633;
static const unsigned ACCENT_CYAN     = 0x3DD6B5;
static const unsigned ACCENT_ORANGE   = 0xF79F37;
static const unsigned ACCENT_YELLOW   = 0xFFD166;
static const unsigned TEXT_MAIN       = 0xF0F6FC;
static const unsigned TEXT_MUTED      = 0x8B949E;
static const unsigned BORDER_LINE     = 0x30363D;

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

static std::string fmt(double v, int prec = 1) {
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
    ColorRGBA bg = active ? hexCol(0x235E55) : hovered ? hexCol(0x2D3748) : hexCol(CARD_BG_COLOR);
    ColorRGBA bd = active ? hexCol(ACCENT_CYAN) : hexCol(BORDER_LINE);
    ColorRGBA tx = active ? hexCol(0xFFFFFF) : enabled ? hexCol(TEXT_MAIN) : hexCol(TEXT_MUTED);

    fillRect2D(x, y, w, h, bg);
    strokeRect2D(x, y, w, h, bd, active ? 1.8 : 1.0);
    double tw = textWidth(text, FONT_MD);
    drawText(x + (w - tw) * 0.5, y + h * 0.5 - 4.0, text, tx, FONT_MD);

    if (enabled) {
        g.buttons.push_back({ x, y, w, h, text, onClick, active });
    }
}

static void drawSlider(double x, double y, double w, double h, int id, const std::string& label, double minV, double maxV, double* pVal, const std::string& unit) {
    double val = *pVal;
    drawText(x, y + 8, label, hexCol(TEXT_MAIN), FONT_MD);

    double t0 = x + 110.0;
    double t1 = x + w - 80.0;
    double cy = y + h * 0.5;

    line2D(t0, cy, t1, cy, hexCol(0x3B4657), 3.0);

    if (minV < 0 && maxV > 0) {
        double zx = t0 + (0.0 - minV) / (maxV - minV) * (t1 - t0);
        line2D(zx, cy - 5, zx, cy + 5, hexCol(0x6A7B92), 1.0);
    }

    double t = (val - minV) / (maxV - minV);
    t = std::max(0.0, std::min(1.0, t));
    double hx = t0 + t * (t1 - t0);

    circle2D(hx, cy, 6.0, hexCol(ACCENT_CYAN));
    circle2D(hx, cy, 3.0, hexCol(0xFFFFFF));

    std::string valStr = fmt(val) + " " + unit;
    drawText(t1 + 10, y + 8, valStr, hexCol(ACCENT_YELLOW), FONT_MD);

    g.sliders.push_back({ t0 - 8, y, (t1 - t0) + 16, h, label, minV, maxV, pVal, unit, id });
}

// =============================================================================
// 6. 3D RENDERING PIPELINE & GIZMOS
// =============================================================================

static void drawString3D(double x, double y, double z, const std::string& s, ColorRGBA c, void* font = FONT_MD) {
    setCol(c);
    glRasterPos3d(x, y, z);
    for (char ch : s) glutBitmapCharacter(font, ch);
}

static void drawAxisArrow3D(int axis, double len, ColorRGBA col) {
    setCol(col);
    glLineWidth(2.5f);
    double tipLen = 0.45;
    double tipRad = 0.12;
    double shaft = len - tipLen;

    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    if (axis == 0) glVertex3d(shaft, 0, 0);
    else if (axis == 1) glVertex3d(0, shaft, 0);
    else glVertex3d(0, 0, shaft);
    glEnd();

    glPushMatrix();
    if (axis == 0) {
        glTranslated(shaft, 0, 0);
        glRotated(90.0, 0, 1, 0);
    } else if (axis == 1) {
        glTranslated(0, shaft, 0);
        glRotated(-90.0, 1, 0, 0);
    } else {
        glTranslated(0, 0, shaft);
    }
    glutSolidCone(tipRad, tipLen, 16, 2);
    glPopMatrix();
}

static void drawCoordinateAxes3D() {
    if (!g.showAxes) return;
    double L = 4.8;
    drawAxisArrow3D(0, L, hexCol(0xFF4757));
    drawString3D(L + 0.3, 0, 0, "Ox (+X)", hexCol(0xFF4757), FONT_MD);

    drawAxisArrow3D(1, L, hexCol(0x2ED573));
    drawString3D(0, L + 0.3, 0, "Oy (+Y)", hexCol(0x2ED573), FONT_MD);

    drawAxisArrow3D(2, L, hexCol(0x1E90FF));
    drawString3D(0, 0, L + 0.3, "Oz (+Z)", hexCol(0x1E90FF), FONT_MD);
}

static void drawGroundGrid3D() {
    if (!g.showGrid) return;
    int steps = 6;
    double stepSize = 1.0;
    double maxL = steps * stepSize;
    double floorY = -1.6;

    glLineWidth(1.0f);
    setCol(hexCol(0x2C3A4B, 0.45f));
    glBegin(GL_LINES);
    for (int i = -steps; i <= steps; i++) {
        double d = i * stepSize;
        glVertex3d(-maxL, floorY, d); glVertex3d(maxL, floorY, d);
        glVertex3d(d, floorY, -maxL); glVertex3d(d, floorY, maxL);
    }
    glEnd();
}

static void drawRotationGizmoRings() {
    if (!g.showRotationGizmo) return;
    double r = 3.2;
    int segs = 48;

    glLineWidth(g.activeAxis == AXIS_OX ? 3.0f : 1.4f);
    setCol(hexCol(0xFF4757, g.activeAxis == AXIS_OX ? 0.95f : 0.40f));
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++) {
        double a = i * 2.0 * PI / segs;
        glVertex3d(0, r * std::cos(a), r * std::sin(a));
    }
    glEnd();

    glLineWidth(g.activeAxis == AXIS_OY ? 3.0f : 1.4f);
    setCol(hexCol(0x2ED573, g.activeAxis == AXIS_OY ? 0.95f : 0.40f));
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++) {
        double a = i * 2.0 * PI / segs;
        glVertex3d(r * std::cos(a), 0, r * std::sin(a));
    }
    glEnd();

    glLineWidth(g.activeAxis == AXIS_OZ ? 3.0f : 1.4f);
    setCol(hexCol(0x1E90FF, g.activeAxis == AXIS_OZ ? 0.95f : 0.40f));
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++) {
        double a = i * 2.0 * PI / segs;
        glVertex3d(r * std::cos(a), r * std::sin(a), 0);
    }
    glEnd();
}

// Ve vat the 3D theo Mo hinh mat da giac (Polygon Mesh Model)
static void drawPolygonMesh(const Mesh3D& mesh, const std::vector<Vec3>& xformedVerts) {
    if (xformedVerts.empty()) return;

    // 1. Ve cac mat da giac (Solid Shaded Faces)
    if (g.renderStyle == STYLE_POLYGON || g.renderStyle == STYLE_BOTH) {
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        for (size_t i = 0; i < mesh.faces.size(); i++) {
            const auto& f = mesh.faces[i];
            if (f.vertexIndices.size() < 3) continue;

            Vec3 v0 = xformedVerts[f.vertexIndices[0]];
            Vec3 v1 = xformedVerts[f.vertexIndices[1]];
            Vec3 v2 = xformedVerts[f.vertexIndices[2]];
            Vec3 norm = normalize(cross(v1 - v0, v2 - v0));

            unsigned hexC = i < mesh.faceColors.size() ? mesh.faceColors[i] : 0x3DD6B5;
            ColorRGBA col = hexCol(hexC, (float)g.faceOpacity);
            setCol(col);

            GLfloat matSpec[] = { 0.3f, 0.3f, 0.3f, 1.0f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0f);

            if (f.vertexIndices.size() == 3) {
                glBegin(GL_TRIANGLES);
                glNormal3d(norm.x, norm.y, norm.z);
                for (int idx : f.vertexIndices) {
                    const auto& pt = xformedVerts[idx];
                    glVertex3d(pt.x, pt.y, pt.z);
                }
                glEnd();
            } else if (f.vertexIndices.size() == 4) {
                glBegin(GL_QUADS);
                glNormal3d(norm.x, norm.y, norm.z);
                for (int idx : f.vertexIndices) {
                    const auto& pt = xformedVerts[idx];
                    glVertex3d(pt.x, pt.y, pt.z);
                }
                glEnd();
            } else {
                glBegin(GL_POLYGON);
                glNormal3d(norm.x, norm.y, norm.z);
                for (int idx : f.vertexIndices) {
                    const auto& pt = xformedVerts[idx];
                    glVertex3d(pt.x, pt.y, pt.z);
                }
                glEnd();
            }
        }

        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_LIGHTING);
    }

    // 2. Ve khung vien canh (Wireframe Edges)
    if (g.renderStyle == STYLE_WIREFRAME || g.renderStyle == STYLE_BOTH) {
        setCol(hexCol(0xFFFFFF, 0.95f));
        glLineWidth(2.4f);
        glBegin(GL_LINES);
        for (const auto& e : mesh.edges) {
            const auto& p1 = xformedVerts[e.first];
            const auto& p2 = xformedVerts[e.second];
            glVertex3d(p1.x, p1.y, p1.z);
            glVertex3d(p2.x, p2.y, p2.z);
        }
        glEnd();
    }

    // 3. Ve vector phap tuyen (Face Normals)
    if (g.showNormals) {
        setCol(hexCol(0xFFD166, 0.9f));
        glLineWidth(1.8f);
        glBegin(GL_LINES);
        for (const auto& f : mesh.faces) {
            if (f.vertexIndices.size() < 3) continue;
            Vec3 center{ 0, 0, 0 };
            for (int idx : f.vertexIndices) center = center + xformedVerts[idx];
            center = center / (double)f.vertexIndices.size();

            Vec3 v0 = xformedVerts[f.vertexIndices[0]];
            Vec3 v1 = xformedVerts[f.vertexIndices[1]];
            Vec3 v2 = xformedVerts[f.vertexIndices[2]];
            Vec3 norm = normalize(cross(v1 - v0, v2 - v0));

            Vec3 tip = center + norm * 0.7;
            glVertex3d(center.x, center.y, center.z);
            glVertex3d(tip.x, tip.y, tip.z);
        }
        glEnd();
    }

    // 4. Nhieu nhan ten cac dinh (cho cac vat the it dinh nhu Chop, Cube de khong roi mat)
    if (g.showLabels && xformedVerts.size() <= 12) {
        for (size_t i = 0; i < xformedVerts.size(); i++) {
            const auto& pt = xformedVerts[i];
            char lbl[16];
            if (xformedVerts.size() <= 4) snprintf(lbl, sizeof lbl, "%c", (char)('A' + i));
            else snprintf(lbl, sizeof lbl, "V%d", (int)i);
            drawString3D(pt.x + 0.15, pt.y + 0.15, pt.z + 0.15, lbl, hexCol(0xFFFFFF), FONT_MD);
        }
    }
}

// =============================================================================
// 7. MA TRAN TONG HOP & TIEN TRINH QUAN SAT (PIPELINE)
// =============================================================================

static Mat4 computeModelMatrix() {
    Mat4 mScale = scaling4(g.objScale, g.objScale, g.objScale);
    Mat4 mRot   = rotationEuler4(g.rotOx, g.rotOy, g.rotOz);
    return mScale * mRot;
}

static std::vector<Vec3> getTransformedMeshVertices(const Mesh3D& mesh) {
    Mat4 M = computeModelMatrix();
    std::vector<Vec3> res;
    res.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
        res.push_back(v * M);
    }
    return res;
}

// =============================================================================
// 8. VE VIEWPORT QUAN SAT & GIAO DIEN DIEU KHIEN (MAIN DISPLAY)
// =============================================================================

static void renderHeader() {
    fillRect2D(0, 0, g.windowW, HEADER_H, hexCol(0x11161F));
    line2D(0, HEADER_H, g.windowW, HEADER_H, hexCol(BORDER_LINE), 1.0);

    drawText(20, 24, "VKU GRAPHICS - LAB 4: QUAN SAT VAT THE 3D", hexCol(ACCENT_CYAN), FONT_LG);
    drawText(20, 44, "Bieu dien mat da giac | Quay 3D Ox, Oy, Oz | Ma tran he quan sat | Chieu song song & Phoi canh", hexCol(TEXT_MUTED), FONT_SM);

    double btnW = 190.0, btnH = 34.0, btnY = 13.0;
    double tX = g.windowW - sidePanelW() - 410.0;
    drawButton(tX, btnY, btnW, btnH, "4.1: QUAY 3D (Ox, Oy, Oz)", g.activeSection == SECTION_ROTATION_3D, [] {
        g.activeSection = SECTION_ROTATION_3D;
    });
    drawButton(tX + btnW + 12.0, btnY, btnW, btnH, "4.2: HE QUAN SAT & CHIEU", g.activeSection == SECTION_VIEW_TRANS, [] {
        g.activeSection = SECTION_VIEW_TRANS;
    });

    drawButton(g.windowW - 130.0, btnY, 110.0, btnH, "RESET (R)", false, [] {
        resetToDefaults();
    });
}

static void renderSidebarUI() {
    double pX = viewportW();
    double pY = HEADER_H;
    double pW = sidePanelW();
    double pH = viewportH();

    fillRect2D(pX, pY, pW, pH, hexCol(PANEL_BG_COLOR));
    line2D(pX, pY, pX, pY + pH, hexCol(BORDER_LINE), 1.5);

    double curY = pY + 14.0;
    double cardW = pW - 32.0;
    double cX = pX + 16.0;

    // --- CARD 1: CHON VAT THE DA GIAC & KIEU HIEN THI ---
    {
        double cardH = 138.0;
        fillRect2D(cX, curY, cardW, cardH, hexCol(CARD_BG_COLOR));
        strokeRect2D(cX, curY, cardW, cardH, hexCol(BORDER_LINE), 1.0);

        drawText(cX + 12, curY + 16, "1. MO HINH MAT DA GIAC (POLYGON MESH)", hexCol(ACCENT_YELLOW), FONT_MD);

        // Dong 1: 3 nut
        double bW3_top = (cardW - 30.0) / 3.0, bH = 26.0, by1 = curY + 28.0;
        drawButton(cX + 10, by1, bW3_top, bH, "Chop ABCD", g.currentMeshIdx == 0, [] { g.currentMeshIdx = 0; });
        drawButton(cX + 10 + bW3_top + 5, by1, bW3_top, bH, "Lap phuong", g.currentMeshIdx == 1, [] { g.currentMeshIdx = 1; });
        drawButton(cX + 10 + (bW3_top + 5) * 2, by1, bW3_top, bH, "Ngoi nha 3D", g.currentMeshIdx == 2, [] { g.currentMeshIdx = 2; });

        // Dong 2: 2 nut (Bat dien deu, Linh dac nhiem cam sung)
        double bW2 = (cardW - 25.0) / 2.0, by2 = curY + 58.0;
        drawButton(cX + 10, by2, bW2, bH, "Bat dien deu", g.currentMeshIdx == 3, [] { g.currentMeshIdx = 3; });
        drawButton(cX + 10 + bW2 + 5, by2, bW2, bH, "Linh dac nhiem cam sung", g.currentMeshIdx == 4, [] { g.currentMeshIdx = 4; });

        // Dong 3: Kieu bieu dien: Wireframe / Polygon / Both
        double bW3 = (cardW - 30.0) / 3.0, by3 = curY + 98.0;
        drawButton(cX + 10, by3, bW3, bH, "Khung day (W)", g.renderStyle == STYLE_WIREFRAME, [] { g.renderStyle = STYLE_WIREFRAME; });
        drawButton(cX + 10 + bW3 + 5, by3, bW3, bH, "Mat da giac", g.renderStyle == STYLE_POLYGON, [] { g.renderStyle = STYLE_POLYGON; });
        drawButton(cX + 10 + (bW3 + 5) * 2, by3, bW3, bH, "Ket hop (Both)", g.renderStyle == STYLE_BOTH, [] { g.renderStyle = STYLE_BOTH; });

        curY += cardH + 12.0;
    }

    // --- CARD 2: NOI DUNG CHUYEN DE (4.1 HOAC 4.2) ---
    if (g.activeSection == SECTION_ROTATION_3D) {
        double cardH = 240.0;
        fillRect2D(cX, curY, cardW, cardH, hexCol(CARD_BG_COLOR));
        strokeRect2D(cX, curY, cardW, cardH, hexCol(BORDER_LINE), 1.0);

        drawText(cX + 12, curY + 16, "4.1: PHEP QUAY 3D QUANH TRUC Ox, Oy, Oz", hexCol(ACCENT_CYAN), FONT_MD);

        double bW = (cardW - 36.0) / 3.0, by = curY + 28.0;
        drawButton(cX + 10, by, bW, 26.0, "Truc Ox (X)", g.activeAxis == AXIS_OX, [] { g.activeAxis = AXIS_OX; });
        drawButton(cX + 10 + bW + 8, by, bW, 26.0, "Truc Oy (Y)", g.activeAxis == AXIS_OY, [] { g.activeAxis = AXIS_OY; });
        drawButton(cX + 10 + (bW + 8) * 2, by, bW, 26.0, "Truc Oz (Z)", g.activeAxis == AXIS_OZ, [] { g.activeAxis = AXIS_OZ; });

        double sY = curY + 68.0;
        drawSlider(cX + 10, sY, cardW - 20, 24.0, 101, "Goc quay Ox:", -180.0, 180.0, &g.rotOx, "deg");
        drawSlider(cX + 10, sY + 30.0, cardW - 20, 24.0, 102, "Goc quay Oy:", -180.0, 180.0, &g.rotOy, "deg");
        drawSlider(cX + 10, sY + 60.0, cardW - 20, 24.0, 103, "Goc quay Oz:", -180.0, 180.0, &g.rotOz, "deg");

        double qY = curY + 168.0, qW = (cardW - 40.0) / 4.0;
        drawButton(cX + 10, qY, qW, 26.0, "-15 deg", false, [] {
            if (g.activeAxis == AXIS_OX) g.rotOx -= 15.0;
            else if (g.activeAxis == AXIS_OY) g.rotOy -= 15.0;
            else g.rotOz -= 15.0;
        });
        drawButton(cX + 10 + qW + 6, qY, qW, 26.0, "+15 deg", false, [] {
            if (g.activeAxis == AXIS_OX) g.rotOx += 15.0;
            else if (g.activeAxis == AXIS_OY) g.rotOy += 15.0;
            else g.rotOz -= 15.0;
        });
        drawButton(cX + 10 + (qW + 6) * 2, qY, qW, 26.0, "Ve 0 deg", false, [] {
            if (g.activeAxis == AXIS_OX) g.rotOx = 0.0;
            else if (g.activeAxis == AXIS_OY) g.rotOy = 0.0;
            else g.rotOz = 0.0;
        });
        drawButton(cX + 10 + (qW + 6) * 3, qY, qW, 26.0, "Reset ca 3", false, [] {
            g.rotOx = 0.0; g.rotOy = 0.0; g.rotOz = 0.0;
        });

        double spY = curY + 204.0, spW = (cardW - 30.0) / 3.0;
        drawButton(cX + 10, spY, spW, 24.0, "Spin Ox", g.autoSpinOx, [] { g.autoSpinOx = !g.autoSpinOx; });
        drawButton(cX + 10 + spW + 5, spY, spW, 24.0, "Spin Oy", g.autoSpinOy, [] { g.autoSpinOy = !g.autoSpinOy; });
        drawButton(cX + 10 + (spW + 5) * 2, spY, spW, 24.0, "Spin Oz", g.autoSpinOz, [] { g.autoSpinOz = !g.autoSpinOz; });

        curY += cardH + 12.0;
    } else {
        double cardH = 265.0;
        fillRect2D(cX, curY, cardW, cardH, hexCol(CARD_BG_COLOR));
        strokeRect2D(cX, curY, cardW, cardH, hexCol(BORDER_LINE), 1.0);

        drawText(cX + 12, curY + 16, "4.2: BIEN DOI HE QUAN SAT & PHEP CHIEU", hexCol(ACCENT_CYAN), FONT_MD);

        double pW2 = (cardW - 28.0) / 2.0, py = curY + 28.0;
        drawButton(cX + 10, py, pW2, 28.0, "Chieu Song song (P)", g.projMode == PROJ_PARALLEL, [] {
            g.projMode = PROJ_PARALLEL;
        });
        drawButton(cX + 10 + pW2 + 8, py, pW2, 28.0, "Chieu Phoi canh (P)", g.projMode == PROJ_PERSPECTIVE, [] {
            g.projMode = PROJ_PERSPECTIVE;
        });

        double sY = curY + 68.0;
        drawSlider(cX + 10, sY, cardW - 20, 24.0, 201, "Goc quan sat Theta:", 0.0, 360.0, &g.thetaDeg, "deg");
        drawSlider(cX + 10, sY + 30.0, cardW - 20, 24.0, 202, "Goc quan sat Phi:", -89.0, 89.0, &g.phiDeg, "deg");
        drawSlider(cX + 10, sY + 60.0, cardW - 20, 24.0, 203, "Khoang cach R:", 3.0, 25.0, &g.eyeRadius, "");
        drawSlider(cX + 10, sY + 90.0, cardW - 20, 24.0, 204, "He so Ti le Scale:", 0.2, 3.0, &g.objScale, "x");

        if (g.projMode == PROJ_PERSPECTIVE) {
            drawSlider(cX + 10, sY + 120.0, cardW - 20, 24.0, 205, "Tieu cu d (FOV):", 1.5, 12.0, &g.focalLengthD, "");
        } else {
            drawText(cX + 10, sY + 128.0, "* Chieu song song: Cac tia chieu luon song song", hexCol(TEXT_MUTED), FONT_SM);
        }

        double prY = curY + 224.0, prW = (cardW - 36.0) / 4.0;
        drawButton(cX + 10, prY, prW, 26.0, "Truoc (Front)", false, [] { g.thetaDeg = 0; g.phiDeg = 0; });
        drawButton(cX + 10 + prW + 5, prY, prW, 26.0, "Tren (Top)", false, [] { g.thetaDeg = 0; g.phiDeg = 88.0; });
        drawButton(cX + 10 + (prW + 5) * 2, prY, prW, 26.0, "Ben (Side)", false, [] { g.thetaDeg = 90.0; g.phiDeg = 0; });
        drawButton(cX + 10 + (prW + 5) * 3, prY, prW, 26.0, "Phoi canh", false, [] { g.thetaDeg = 45.0; g.phiDeg = 30.0; });

        curY += cardH + 12.0;
    }

    // --- CARD 3: MA TRAN HIEN TAI & TOA DO DIEM NHIN (LIVE MATRIX INSPECTOR) ---
    {
        double cardH = 220.0;
        fillRect2D(cX, curY, cardW, cardH, hexCol(CARD_BG_COLOR));
        strokeRect2D(cX, curY, cardW, cardH, hexCol(BORDER_LINE), 1.0);

        Vec3 eye = computeEyePosition(g.target, g.thetaDeg, g.phiDeg, g.eyeRadius);
        Mat4 Mmodel = computeModelMatrix();
        Mat4 Mview  = viewingMatrix4(eye, g.target);

        drawText(cX + 12, curY + 16, "THONG SO HE QUAN SAT & MA TRAN LIVE", hexCol(ACCENT_YELLOW), FONT_MD);

        std::string eyeStr = "Diem nhin Eye: E(" + fmt(eye.x) + ", " + fmt(eye.y) + ", " + fmt(eye.z) + ")";
        drawText(cX + 12, curY + 36, eyeStr, hexCol(ACCENT_CYAN), FONT_SM);

        std::string tgtStr = "Tam quan sat LookAt: C(" + fmt(g.target.x) + ", " + fmt(g.target.y) + ", " + fmt(g.target.z) + ") | R = " + fmt(g.eyeRadius);
        drawText(cX + 12, curY + 52, tgtStr, hexCol(TEXT_MUTED), FONT_SM);

        if (g.activeSection == SECTION_ROTATION_3D) {
            drawText(cX + 12, curY + 76, "Ma tran Quay 3D Model: M_rot = Rz * Ry * Rx", hexCol(TEXT_MAIN), FONT_SM);
            for (int r = 0; r < 4; r++) {
                std::string row = "| ";
                for (int c = 0; c < 4; c++) {
                    char buf[16];
                    snprintf(buf, sizeof buf, "%6.2f ", Mmodel.m[r][c]);
                    row += buf;
                }
                row += "|";
                drawText(cX + 16, curY + 96 + r * 16, row, hexCol(0x38EF7D), FONT_SM);
            }
        } else {
            drawText(cX + 12, curY + 76, "Ma tran He quan sat: M_view = T(-E) x [U V N]", hexCol(TEXT_MAIN), FONT_SM);
            for (int r = 0; r < 4; r++) {
                std::string row = "| ";
                for (int c = 0; c < 4; c++) {
                    char buf[16];
                    snprintf(buf, sizeof buf, "%6.2f ", Mview.m[r][c]);
                    row += buf;
                }
                row += "|";
                drawText(cX + 16, curY + 96 + r * 16, row, hexCol(ACCENT_CYAN), FONT_SM);
            }
        }

        drawText(cX + 12, curY + 172, "Phim: [1] Muc 4.1  [2] Muc 4.2  [P] Chieu  [M] Doi mau", hexCol(TEXT_MUTED), FONT_SM);
        drawText(cX + 12, curY + 188, "Chuot: Keo trai=Xoay | Keo phai=Pan | Cuon=Thu phong", hexCol(ACCENT_ORANGE), FONT_SM);
        drawText(cX + 12, curY + 204, "Mui ten: Tang/giam goc quay | Space: Tu dong xoay", hexCol(TEXT_MUTED), FONT_SM);
    }
}

static void renderViewport3D() {
    double vpW = viewportW();
    double vpH = viewportH();

    glViewport(0, 0, (GLsizei)vpW, (GLsizei)vpH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    double aspect = vpW / (vpH > 0 ? vpH : 1.0);

    if (g.projMode == PROJ_PARALLEL) {
        double sz = g.eyeRadius * 0.45;
        glOrtho(-sz * aspect, sz * aspect, -sz, sz, 0.1, 100.0);
    } else {
        gluPerspective(45.0, aspect, 0.5, 100.0);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vec3 eye = computeEyePosition(g.target, g.thetaDeg, g.phiDeg, g.eyeRadius);
    gluLookAt(eye.x, eye.y, eye.z, g.target.x, g.target.y, g.target.z, 0.0, 1.0, 0.0);

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    GLfloat lightPos[] = { (float)eye.x, (float)(eye.y + 5.0), (float)(eye.z + 5.0), 1.0f };
    GLfloat lightAmb[] = { 0.25f, 0.28f, 0.32f, 1.0f };
    GLfloat lightDiff[] = { 0.85f, 0.85f, 0.85f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glEnable(GL_LIGHT0);

    drawGroundGrid3D();
    drawCoordinateAxes3D();

    if (g.activeSection == SECTION_ROTATION_3D) {
        drawRotationGizmoRings();
    }

    Mesh3D& mesh = currentMesh();
    std::vector<Vec3> xformed = getTransformedMeshVertices(mesh);
    drawPolygonMesh(mesh, xformed);

    glDisable(GL_DEPTH_TEST);
}

// =============================================================================
// 9. GLUT CALLBACKS (DISPLAY, RESHAPE, MOUSE, KEYBOARD, TIMER)
// =============================================================================

static void display() {
    glClearColor(0.06f, 0.075f, 0.095f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g.buttons.clear();
    g.sliders.clear();

    renderViewport3D();

    glViewport(0, 0, g.windowW, g.windowH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, g.windowW, g.windowH, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderHeader();
    renderSidebarUI();

    glutSwapBuffers();
}

static void reshape(int w, int h) {
    g.windowW = std::max(800, w);
    g.windowH = std::max(600, h);
    glutPostRedisplay();
}

static void timer(int) {
    bool needRedisplay = false;

    if (g.autoSpinOx) { g.rotOx += 1.2; if (g.rotOx > 180.0) g.rotOx -= 360.0; needRedisplay = true; }
    if (g.autoSpinOy) { g.rotOy += 1.5; if (g.rotOy > 180.0) g.rotOy -= 360.0; needRedisplay = true; }
    if (g.autoSpinOz) { g.rotOz += 1.0; if (g.rotOz > 180.0) g.rotOz -= 360.0; needRedisplay = true; }

    if (g.autoOrbit) {
        g.thetaDeg += 0.8;
        if (g.thetaDeg >= 360.0) g.thetaDeg -= 360.0;
        needRedisplay = true;
    }

    if (needRedisplay) glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

static void mouseButton(int button, int state, int x, int y) {
    g.mouseX = x;
    g.mouseY = y;

    if (state == GLUT_DOWN) {
        for (const auto& btn : g.buttons) {
            if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
                if (btn.onClick) btn.onClick();
                glutPostRedisplay();
                return;
            }
        }

        for (const auto& sld : g.sliders) {
            if (x >= sld.x && x <= sld.x + sld.w && y >= sld.y && y <= sld.y + sld.h) {
                g.activeSliderId = sld.id;
                double t = (x - sld.x) / sld.w;
                t = std::max(0.0, std::min(1.0, t));
                *(sld.pValue) = sld.minVal + t * (sld.maxVal - sld.minVal);
                glutPostRedisplay();
                return;
            }
        }

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

        if (button == 3) {
            if (g.activeSection == SECTION_ROTATION_3D) {
                g.objScale = std::min(3.0, g.objScale + 0.08);
            } else {
                g.eyeRadius = std::max(2.5, g.eyeRadius - 0.5);
            }
            glutPostRedisplay();
        } else if (button == 4) {
            if (g.activeSection == SECTION_ROTATION_3D) {
                g.objScale = std::max(0.2, g.objScale - 0.08);
            } else {
                g.eyeRadius = std::min(30.0, g.eyeRadius + 0.5);
            }
            glutPostRedisplay();
        }
    } else if (state == GLUT_UP) {
        g.isLeftDragging = false;
        g.isRightDragging = false;
        g.activeSliderId = -1;
    }
}

static void mouseMotion(int x, int y) {
    double dx = x - g.lastMouseX;
    double dy = y - g.lastMouseY;
    g.lastMouseX = x;
    g.lastMouseY = y;
    g.mouseX = x;
    g.mouseY = y;

    if (g.activeSliderId != -1) {
        for (const auto& sld : g.sliders) {
            if (sld.id == g.activeSliderId) {
                double t = (x - sld.x) / sld.w;
                t = std::max(0.0, std::min(1.0, t));
                *(sld.pValue) = sld.minVal + t * (sld.maxVal - sld.minVal);
                glutPostRedisplay();
                return;
            }
        }
    }

    if (g.isLeftDragging) {
        if (g.activeSection == SECTION_ROTATION_3D) {
            g.rotOy += dx * 0.6;
            g.rotOx += dy * 0.6;
            if (g.rotOx > 180.0)  g.rotOx -= 360.0;
            if (g.rotOx < -180.0) g.rotOx += 360.0;
            if (g.rotOy > 180.0)  g.rotOy -= 360.0;
            if (g.rotOy < -180.0) g.rotOy += 360.0;
        } else {
            g.thetaDeg += dx * 0.5;
            g.phiDeg   += dy * 0.4;
            if (g.thetaDeg >= 360.0) g.thetaDeg -= 360.0;
            if (g.thetaDeg < 0.0)    g.thetaDeg += 360.0;
            g.phiDeg = std::max(-89.0, std::min(89.0, g.phiDeg));
        }
        glutPostRedisplay();
    }

    if (g.isRightDragging) {
        double panSpeed = 0.015;
        g.target.x -= dx * panSpeed;
        g.target.y += dy * panSpeed;
        glutPostRedisplay();
    }
}

static void passiveMotion(int x, int y) {
    g.mouseX = x;
    g.mouseY = y;
    glutPostRedisplay();
}

static void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 27:
            exit(0);
            break;
        case '1':
            g.activeSection = SECTION_ROTATION_3D;
            break;
        case '2':
            g.activeSection = SECTION_VIEW_TRANS;
            break;
        case 'p':
        case 'P':
            g.projMode = (g.projMode == PROJ_PARALLEL) ? PROJ_PERSPECTIVE : PROJ_PARALLEL;
            break;
        case 'm':
        case 'M':
            g.currentMeshIdx = (g.currentMeshIdx + 1) % g.meshes.size();
            break;
        case 'w':
        case 'W':
            g.renderStyle = (RenderStyle)((g.renderStyle + 1) % 3);
            break;
        case 'x':
        case 'X':
            g.activeAxis = AXIS_OX;
            break;
        case 'y':
        case 'Y':
            g.activeAxis = AXIS_OY;
            break;
        case 'z':
        case 'Z':
            g.activeAxis = AXIS_OZ;
            break;
        case '+':
        case '=':
            if (g.activeSection == SECTION_ROTATION_3D) g.objScale = std::min(3.0, g.objScale + 0.1);
            else g.eyeRadius = std::max(2.5, g.eyeRadius - 0.5);
            break;
        case '-':
        case '_':
            if (g.activeSection == SECTION_ROTATION_3D) g.objScale = std::max(0.2, g.objScale - 0.1);
            else g.eyeRadius = std::min(30.0, g.eyeRadius + 0.5);
            break;
        case 'r':
        case 'R':
            resetToDefaults();
            break;
        case ' ':
            if (g.activeSection == SECTION_ROTATION_3D) {
                g.autoSpinOy = !g.autoSpinOy;
            } else {
                g.autoOrbit = !g.autoOrbit;
            }
            break;
    }
    glutPostRedisplay();
}

static void specialKey(int key, int, int) {
    double step = 5.0;
    if (g.activeSection == SECTION_ROTATION_3D) {
        if (key == GLUT_KEY_LEFT) {
            if (g.activeAxis == AXIS_OX) g.rotOx -= step;
            else if (g.activeAxis == AXIS_OY) g.rotOy -= step;
            else g.rotOz -= step;
        } else if (key == GLUT_KEY_RIGHT) {
            if (g.activeAxis == AXIS_OX) g.rotOx += step;
            else if (g.activeAxis == AXIS_OY) g.rotOy += step;
            else g.rotOz += step;
        } else if (key == GLUT_KEY_UP) {
            g.rotOx += step;
        } else if (key == GLUT_KEY_DOWN) {
            g.rotOx -= step;
        }
    } else {
        if (key == GLUT_KEY_LEFT) {
            g.thetaDeg -= step;
            if (g.thetaDeg < 0.0) g.thetaDeg += 360.0;
        } else if (key == GLUT_KEY_RIGHT) {
            g.thetaDeg += step;
            if (g.thetaDeg >= 360.0) g.thetaDeg -= 360.0;
        } else if (key == GLUT_KEY_UP) {
            g.phiDeg = std::min(89.0, g.phiDeg + step);
        } else if (key == GLUT_KEY_DOWN) {
            g.phiDeg = std::max(-89.0, g.phiDeg - step);
        } else if (key == GLUT_KEY_PAGE_UP) {
            g.eyeRadius = std::max(2.5, g.eyeRadius - 0.5);
        } else if (key == GLUT_KEY_PAGE_DOWN) {
            g.eyeRadius = std::min(30.0, g.eyeRadius + 0.5);
        }
    }
    glutPostRedisplay();
}

// =============================================================================
// 10. MAIN ENTRY POINT
// =============================================================================

int main(int argc, char** argv) {
    // 1. Khoi tao cac mo hinh mat da giac
    g.meshes.push_back(createPyramidMesh());     // 0: Chop ABCD
    g.meshes.push_back(createCubeMesh());        // 1: Lap phuong Cube
    g.meshes.push_back(createHouseMesh());       // 2: Ngoi nha 3D House
    g.meshes.push_back(createOctahedronMesh());  // 3: Bat dien deu
    g.meshes.push_back(createSoldierMesh());     // 4: Linh dac nhiem cam sung (Commando)

    // Mac dinh khoi dong voi mo hinh Linh dac nhiem cam sung
    g.currentMeshIdx = 4;

    // 2. Chay che do kiem thu tu dong neu co tham so --test
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            return runSelfTests();
        }
    }

    // 3. Khoi tao OpenGL & FreeGLUT Window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(g.windowW, g.windowH);
    glutInitWindowPosition(80, 40);
    glutCreateWindow("VKU - LAB 4: QUAN SAT VAT THE 3D (3D VIEWING & PROJECTIONS)");

    // 4. Dang ky cac ham callback
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(passiveMotion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutTimerFunc(16, timer, 0);

    // 5. In huong dan ra Console
    printf("=================================================================\n");
    printf("  TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN  \n");
    printf("  LAB 4: CHUONG TRINH QUAN SAT VAT THE 3D                        \n");
    printf("=================================================================\n");
    printf(" 4.1. Phep quay 3D quanh truc Ox, Oy, Oz de quan sat vat the     \n");
    printf(" 4.2. Dung ma tran bien doi he quan sat de quan sat vat the:     \n");
    printf("      - Chuyen doi phep chieu: Song song hoac Phoi canh (Phim P) \n");
    printf("      - Thay doi goc quan sat Theta, Phi (Keo chuot trai / Phim) \n");
    printf("      - Thay doi vi tri quan sat (Pan chuot phai / Slider)       \n");
    printf("      - Phong to, thu nho vat the (Cuon chuot / Slider Scale/R)  \n");
    printf(" Mo hinh: Chop ABCD, Cube, Ngoi nha, Bat dien, Linh dac nhiem     \n");
    printf("=================================================================\n");

    glutMainLoop();
    return 0;
}
