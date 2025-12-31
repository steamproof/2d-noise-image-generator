#include "colormap.h"
#include "util.h"
#include <cmath>
#include <stdexcept>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static RGB hex2(const std::string& s) {
    std::string t = s;
    if (!t.empty() && t[0] == '#') {
        t = t.substr(1);
    }
    if (t.size() != 6) {
        throw std::runtime_error("bad color: " + s);
    }
    auto h = [&](char c) -> int {
        if ('0' <= c && c <= '9') {
            return c - '0';
        }
        if ('a' <= c && c <= 'f') {
            return 10 + (c - 'a');
        }
        if ('A' <= c && c <= 'F') {
            return 10 + (c - 'A');
        }
        return -1;
    };
    int a = h(t[0]), b = h(t[1]), c = h(t[2]), d = h(t[3]), e = h(t[4]), f = h(t[5]);
    if (a < 0 || b < 0 || c < 0 || d < 0 || e < 0 || f < 0) {
        throw std::runtime_error("bad color: " + s);
    }
    RGB x;
    x.r = (uint8_t) ((a << 4) | b);
    x.g = (uint8_t) ((c << 4) | d);
    x.b = (uint8_t) ((e << 4) | f);
    return x;
}

static RGB lerp(const RGB& a, const RGB& b, float t) {
    RGB c;
    c.r = (uint8_t) clampv((int) std::lround(a.r + (b.r - a.r) * t), 0, 255);
    c.g = (uint8_t) clampv((int) std::lround(a.g + (b.g - a.g) * t), 0, 255);
    c.b = (uint8_t) clampv((int) std::lround(a.b + (b.b - a.b) * t), 0, 255);
    return c;
}

static std::vector<Stop> preset(const std::string& name) {
    std::string n = lo(name);
    if (n == "grayscale") {
        return {{0.0f, {0, 0, 0}}, {1.0f, {255, 255, 255}}};
    }
    if (n == "terrain") {
        return {{0.0f, hex2("#081d3a")}, {0.35f, hex2("#1e90ff")}, {0.50f, hex2("#f5deb3")}, {0.70f, hex2("#228b22")}, {0.90f, hex2("#7f7f7f")}, {1.0f, hex2("#ffffff")}};
    }
    if (n == "viridis") {
        return {{0.0f, hex2("#440154")}, {0.25f, hex2("#3b528b")}, {0.50f, hex2("#21918c")}, {0.75f, hex2("#5ec962")}, {1.0f, hex2("#fde725")}};
    }
    if (n == "magma") {
        return {{0.0f, hex2("#000004")}, {0.25f, hex2("#3b0f70")}, {0.50f, hex2("#8c2981")}, {0.75f, hex2("#de4968")}, {1.0f, hex2("#fcfdbf")}};
    }
    if (n == "turbo") {
        return {{0.0f, hex2("#30123b")}, {0.25f, hex2("#4456c7")}, {0.50f, hex2("#2ab9a2")}, {0.75f, hex2("#f9e721")}, {1.0f, hex2("#900c00")}};
    }
    if (n == "icefire") {
        return {{0.0f, hex2("#000000")}, {0.35f, hex2("#2b83ba")}, {0.50f, hex2("#ffffff")}, {0.65f, hex2("#d7191c")}, {1.0f, hex2("#000000")}};
    }
    return {};
}

static std::vector<Stop> parse_stops(const std::string& s) {
    std::string t = s.substr((int) std::string("stops:").size());
    std::vector<std::string> a = split(t, ',');
    std::vector<Stop> r;
    for (std::string x : a) {
        x = trim(x);
        if (x.empty()) {
            continue;
        }
        std::vector<std::string> p = split(x, ':');
        if ((int) p.size() != 2) {
            throw std::runtime_error("bad stop: " + x);
        }
        float pos = 0.0f;
        if (!parse_f(trim(p[0]), pos)) {
            throw std::runtime_error("bad stop pos: " + x);
        }
        Stop stp;
        stp.p = pos;
        stp.c = hex2(trim(p[1]));
        r.push_back(stp);
    }
    if (r.empty()) {
        throw std::runtime_error("empty stops");
    }
    std::sort(r.begin(), r.end(), [&](const Stop& a, const Stop& b) { return a.p < b.p; });
    return r;
}

static std::vector<Stop> parse_json(const std::string& path) {
    std::string s = read_all(path);
    auto skip = [&](int& i) {
        while (i < (int) s.size() && std::isspace((unsigned char) s[i])) {
            i++;
        }
    };
    auto eat = [&](int& i, char c) {
        skip(i);
        if (i >= (int) s.size() || s[i] != c) {
            throw std::runtime_error("json parse error at " + std::to_string(i));
        }
        i++;
    };
    auto str = [&](int& i) -> std::string {
        skip(i);
        eat(i, '"');
        std::string r;
        while (i < (int) s.size()) {
            char c = s[i++];
            if (c == '"') {
                break;
            }
            if (c == '\\') {
                if (i >= (int) s.size()) {
                    throw std::runtime_error("json parse error at " + std::to_string(i));
                }
                char d = s[i++];
                if (d == '"' || d == '\\' || d == '/') {
                    r += d;
                } else if (d == 'b') {
                    r += '\b';
                } else if (d == 'f') {
                    r += '\f';
                } else if (d == 'n') {
                    r += '\n';
                } else if (d == 'r') {
                    r += '\r';
                } else if (d == 't') {
                    r += '\t';
                } else {
                    throw std::runtime_error("json escape unsupported");
                }
            } else {
                r += c;
            }
        }
        return r;
    };
    auto num = [&](int& i) -> float {
        skip(i);
        int j = i;
        if (i < (int) s.size() && (s[i] == '-' || s[i] == '+')) {
            i++;
        }
        while (i < (int) s.size() && std::isdigit((unsigned char) s[i])) {
            i++;
        }
        if (i < (int) s.size() && s[i] == '.') {
            i++;
            while (i < (int) s.size() && std::isdigit((unsigned char) s[i])) {
                i++;
            }
        }
        if (i < (int) s.size() && (s[i] == 'e' || s[i] == 'E')) {
            i++;
            if (i < (int) s.size() && (s[i] == '-' || s[i] == '+')) {
                i++;
            }
            while (i < (int) s.size() && std::isdigit((unsigned char) s[i])) {
                i++;
            }
        }
        std::string t = s.substr(j, i - j);
        float x = 0.0f;
        if (!parse_f(t, x)) {
            throw std::runtime_error("json number parse error");
        }
        return x;
    };
    int i = 0;
    std::vector<Stop> r;
    while (i < (int) s.size()) {
        int j = i;
        if (s.compare(j, 6, "\"stops") == 0) {
            break;
        }
        i++;
    }
    if (i >= (int) s.size()) {
        throw std::runtime_error("json missing stops");
    }
    while (i < (int) s.size() && s[i] != '[') {
        i++;
    }
    if (i >= (int) s.size()) {
        throw std::runtime_error("json stops not array");
    }
    eat(i, '[');
    while (true) {
        skip(i);
        if (i < (int) s.size() && s[i] == ']') {
            i++;
            break;
        }
        eat(i, '{');
        float pos = 0.0f;
        std::string col;
        bool gotp = false, gotc = false;
        while (true) {
            skip(i);
            if (i < (int) s.size() && s[i] == '}') {
                i++;
                break;
            }
            std::string k = str(i);
            eat(i, ':');
            if (k == "pos") {
                pos = num(i);
                gotp = true;
            } else if (k == "color") {
                col = str(i);
                gotc = true;
            } else {
                skip(i);
                if (i < (int) s.size() && s[i] == '"') {
                    (void) str(i);
                } else if (i < (int) s.size() && (s[i] == '{' || s[i] == '[')) {
                    throw std::runtime_error("json nested unsupported");
                } else {
                    (void) num(i);
                }
            }
            skip(i);
            if (i < (int) s.size() && s[i] == ',') {
                i++;
                continue;
            }
        }
        if (!gotp || !gotc) {
            throw std::runtime_error("json stop needs pos+color");
        }
        Stop stp;
        stp.p = pos;
        stp.c = hex2(col);
        r.push_back(stp);
        skip(i);
        if (i < (int) s.size() && s[i] == ',') {
            i++;
            continue;
        }
    }
    if (r.empty()) {
        throw std::runtime_error("json empty stops");
    }
    std::sort(r.begin(), r.end(), [&](const Stop& a, const Stop& b) { return a.p < b.p; });
    return r;
}

static std::vector<RGB> load_ramp(const std::string& path) {
    int w = 0, h = 0, c = 0;
    unsigned char* p = stbi_load(path.c_str(), &w, &h, &c, 3);
    if (!p) {
        throw std::runtime_error("failed to load ramp: " + path);
    }
    std::vector<RGB> r;
    if (h == 1) {
        r.resize(w);
        for (int x = 0; x < w; x++) {
            r[x].r = p[x * 3 + 0];
            r[x].g = p[x * 3 + 1];
            r[x].b = p[x * 3 + 2];
        }
    } else if (w == 1) {
        r.resize(h);
        for (int y = 0; y < h; y++) {
            r[y].r = p[y * 3 + 0];
            r[y].g = p[y * 3 + 1];
            r[y].b = p[y * 3 + 2];
        }
    } else {
        stbi_image_free(p);
        throw std::runtime_error("ramp must be Nx1 or 1xN: " + path);
    }
    stbi_image_free(p);
    if (r.empty()) {
        throw std::runtime_error("empty ramp: " + path);
    }
    return r;
}

Colormap Colormap::parse(const std::string& spec) {
    Colormap m;
    std::string s = trim(spec);
    if (s.empty()) {
        s = "grayscale";
    }
    std::vector<Stop> p = preset(s);
    if (!p.empty()) {
        m.s = p;
        return m;
    }
    if (st(lo(s), "stops:")) {
        m.s = parse_stops(s);
        return m;
    }
    if (st(lo(s), "file:")) {
        m.ramp = true;
        m.r = load_ramp(trim(s.substr((int) std::string("file:").size())));
        return m;
    }
    if (st(lo(s), "json:")) {
        m.s = parse_json(trim(s.substr((int) std::string("json:").size())));
        return m;
    }
    throw std::runtime_error("unknown colormap spec: " + spec);
}

RGB Colormap::at(float t) const {
    t = clampv(t, 0.0f, 1.0f);
    if (ramp) {
        int n = (int) r.size();
        int i = (int) std::lround(t * (n - 1));
        i = clampv(i, 0, n - 1);
        return r[i];
    }
    if (s.empty()) {
        return {0, 0, 0};
    }
    if (t <= s.front().p) {
        return s.front().c;
    }
    if (t >= s.back().p) {
        return s.back().c;
    }
    int lo = 0, hi = (int) s.size() - 1;
    while (lo + 1 < hi) {
        int mid = (lo + hi) / 2;
        if (s[mid].p <= t) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    float a = s[lo].p, b = s[hi].p;
    float u = (b == a) ? 0.0f : (t - a) / (b - a);
    return lerp(s[lo].c, s[hi].c, u);
}
