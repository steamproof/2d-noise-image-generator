#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct RGB {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

struct Stop {
    float p = 0.0f;
    RGB c;
};

struct Colormap {
    bool ramp = false;
    std::vector<Stop> s;
    std::vector<RGB> r;
    static Colormap parse(const std::string& spec);
    RGB at(float t) const;
};
