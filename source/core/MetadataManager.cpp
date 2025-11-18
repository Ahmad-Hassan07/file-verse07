#include "MetadataManager.h"
#include <cstring>
#include <iostream>

bool MetadataManager::init(void* file, uint64_t off, uint32_t count) {
    base = file;
    offset = off;
    max_entries = count;
    return true;
}

int MetadataManager::allocate_entry() {
    for (uint32_t i = 0; i < max_entries; i++) {
        MetadataEntry e;
        read_entry(i, e);
        if (e.valid_flag == 0) {
            return i;
        }
    }
    return -1;
}

bool MetadataManager::free_entry(int idx) {
    if (idx < 0 || idx >= (int)max_entries) return false;
    MetadataEntry zero{};
    memset(&zero, 0, sizeof(MetadataEntry));
    return write_entry(idx, zero);
}

bool MetadataManager::write_entry(int idx, const MetadataEntry& e) {
    if (idx < 0 || idx >= (int)max_entries) return false;
    uint8_t* ptr = (uint8_t*)base + offset + idx * sizeof(MetadataEntry);
    memcpy(ptr, &e, sizeof(MetadataEntry));
    return true;
}

bool MetadataManager::read_entry(int idx, MetadataEntry& e) {
    if (idx < 0 || idx >= (int)max_entries) return false;
    uint8_t* ptr = (uint8_t*)base + offset + idx * sizeof(MetadataEntry);
    memcpy(&e, ptr, sizeof(MetadataEntry));
    return true;
}

int MetadataManager::find_in_dir(uint32_t parent, const std::string& name) {
    char shortname[12];
    MetadataManager::to_short_name(name, shortname);  // ⭐ USE THE CORRECT RULE

    for (uint32_t i = 0; i < max_entries; i++) {
        MetadataEntry e;
        read_entry(i, e);

        if (!e.valid_flag) continue;
        if (e.parent_index != parent) continue;

        if (strncmp(e.short_name, shortname, 12) == 0)
            return i;
    }
    return -1;
}
