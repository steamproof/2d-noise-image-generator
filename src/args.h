#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct Args {
    std::unordered_map<std::string, std::vector<std::string>> m;
    static Args parse(int argc, char** argv);
    bool has(const std::string& k) const;
    std::string get1(const std::string& k, const std::string& def) const;
};