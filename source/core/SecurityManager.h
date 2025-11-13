#pragma once
#include <vector>
#include "../include/odf_types.hpp"

class SecurityManager {
    unsigned char enc[256];
    unsigned char dec[256];
public:
    SecurityManager() {
        for (int i = 0; i < 256; i++) { enc[i] = (unsigned char)i; dec[i] = (unsigned char)i; }
    }
    void set_table(const std::vector<unsigned char>& table) {
        for (int i = 0; i < 256 && i < (int)table.size(); i++) enc[i] = table[i];
        for (int i = 0; i < 256; i++) dec[enc[i]] = (unsigned char)i;
    }
    void encode(std::vector<unsigned char>& data) const {
        for (unsigned long long i = 0; i < data.size(); i++) data[i] = enc[data[i]];
    }
    void decode(std::vector<unsigned char>& data) const {
        for (unsigned long long i = 0; i < data.size(); i++) data[i] = dec[data[i]];
    }
};
