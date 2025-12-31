#include "args.h"
#include "util.h"
#include <stdexcept>

Args Args::parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; i++) {
        std::string s = argv[i];
        if (!st(s, "--")) {
            throw std::runtime_error("unexpected positional arg: " + s);
        }
        if (s == "--") {
            throw std::runtime_error("unexpected '--'");
        }
        std::string k = s.substr(2);
        std::string v;
        if (st(k, "no-")) {
            a.m[k] = {};
            continue;
        }
        if (i + 1 < argc) {
            std::string t = argv[i + 1];
            if (!st(t, "--")) {
                v = t;
                i++;
                a.m[k].push_back(v);
                continue;
            }
        }
        a.m[k] = {};
    }
    return a;
}

bool Args::has(const std::string& k) const {
    return m.find(k) != m.end();
}

std::string Args::get1(const std::string& k, const std::string& def) const {
    auto it = m.find(k);
    if (it == m.end()) {
        return def;
    }
    if (it->second.empty()) {
        return def;
    }
    return it->second.back();
}