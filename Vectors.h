#pragma once

struct Vec2f {
    float x;
    float y;
};

struct Vec2d {
    double x;
    double y;
};

struct Vec2i {
    Vec2i() {}

    Vec2i(int nx, int ny) : x(nx), y(ny) {}

    int x = 0;
    int y = 0;

    bool operator==(const Vec2i &b) {
        return x == b.x && y == b.y;
    }
};

struct Vec3f {
    float x;
    float y;
    float z;
};

struct Vec3d {
    double x;
    double y;
    double z;
};

struct Vec3i {
    Vec3i() {}

    Vec3i(int nx, int ny, int nz) : x(nx), y(ny), z(nz) {}

    int x = 0;
    int y = 0;
    int z = 0;
};