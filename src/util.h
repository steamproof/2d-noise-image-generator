#pragma once
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static inline std::string lo(std::string s) {
    for (char& c : s) {
        c = (char) std::tolower((unsigned char) c);
    }
    return s;
}

static inline bool st(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

static inline bool en(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

static inline std::string trim(const std::string& s) {
    int l = 0;
    while (l < (int) s.size() && std::isspace((unsigned char) s[l])) {
        l++;
    }
    int r = (int) s.size();
    while (r > l && std::isspace((unsigned char) s[r - 1])) {
        r--;
    }
    return s.substr(l, r - l);
}

static inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> a;
    std::string cur;
    for (char c : s) {
        if (c == d) {
            a.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    a.push_back(cur);
    return a;
}

template <class T>
static inline T clampv(T x, T l, T r) {
    if (x < l) {
        return l;
    }
    if (x > r) {
        return r;
    }
    return x;
}

static inline std::string read_all(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("failed to open: " + path);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static inline void write_all(const std::string& path, const std::string& s) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("failed to write: " + path);
    }
    f.write(s.data(), (std::streamsize) s.size());
}

static inline bool parse_i(const std::string& s, int& x) {
    errno = 0;
    char* e = nullptr;
    long v = std::strtol(s.c_str(), &e, 10);
    if (errno != 0 || e == s.c_str() || *e != '\0') {
        return false;
    }
    if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max()) {
        return false;
    }
    x = (int) v;
    return true;
}

static inline bool parse_f(const std::string& s, float& x) {
    errno = 0;
    char* e = nullptr;
    float v = std::strtof(s.c_str(), &e);
    if (errno != 0 || e == s.c_str() || *e != '\0') {
        return false;
    }
    x = v;
    return true;
}

static inline std::string ext_of(const std::string& path) {
    int p = (int) path.size();
    while (p > 0 && path[p - 1] != '/' && path[p - 1] != '\\') {
        if (path[p - 1] == '.') {
            return lo(path.substr(p));
        }
        p--;
    }
    return "";
}