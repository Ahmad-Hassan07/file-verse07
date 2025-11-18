#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include "config_parser.h"
#include "../include/ofs_internal.h"   

class MetadataManager {
private:
    void* base;             
    uint64_t offset;        
    uint32_t max_entries;   

public:
    MetadataManager() {
        base = nullptr;
        offset = 0;
        max_entries = 0;
    }
    static void to_short_name(const std::string &src, char dest[12]) {
    memset(dest, 0, 12);
    strncpy(dest, src.c_str(), 10); // EXACT spec: only 10 characters
    dest[10] = '\0';
}

    bool init(void* file, uint64_t off, uint32_t count);

    // allocation
    int allocate_entry();
    bool free_entry(int idx);

    // read/write
    bool write_entry(int idx, const MetadataEntry& e);
    bool read_entry(int idx, MetadataEntry& e);

    // directory search
    int find_in_dir(uint32_t parent, const std::string& name);

    // required by DirectoryTree
    uint32_t capacity() const { return max_entries; }

    // return const reference (DirectoryTree needs this)
    const MetadataEntry& get_const(int idx) const {
        return *(const MetadataEntry*)((uint8_t*)base + offset + idx * sizeof(MetadataEntry));
    }
};
