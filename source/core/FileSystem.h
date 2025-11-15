#pragma once
#include <vector>
#include "../include/odf_types.hpp"
#include "../include/ofs_internal.h"
#include "BlockManager.h"
#include "FreeSpaceManager.h"
#include "SecurityManager.h"
#include "DirectoryManager.h"
#include "UserManager.h"

class FileSystem {
    std::vector<unsigned char> omni;
    std::vector<unsigned char> bitmap_area;
    OMNIHeader header;
    BlockManager block_mgr;
    FreeSpaceManager free_mgr;
    SecurityManager sec_mgr;
    DirectoryManager dir_mgr;
    UserManager user_mgr;
    MountLayout layout;
public:
    FileSystem() : header() {}

    bool load_empty(const FSConfig& cfg);
    bool format_new(const FSConfig& cfg, const char* omni_path);

    BlockManager& blocks() { return block_mgr; }
    FreeSpaceManager& freemap() { return free_mgr; }
    SecurityManager& security() { return sec_mgr; }
    DirectoryManager& directory() { return dir_mgr; }
    UserManager& users() { return user_mgr; }
    OMNIHeader& hdr() { return header; }
    MountLayout& mount() { return layout; }
};
