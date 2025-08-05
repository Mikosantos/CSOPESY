#pragma once
#include <string>
#include <sstream>
#include <iomanip>

inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    size_t last = str.find_last_not_of(" \t");
    return (first == std::string::npos) ? "" : str.substr(first, (last - first + 1));
}

inline std::string intToHex(uint32_t num) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << num;
    return ss.str();
}