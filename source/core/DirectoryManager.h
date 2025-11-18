#pragma once
#include <string>
#include <vector>
#include "MetadataManager.h"
#include "directory_tree.h"
#include "../include/odf_types.hpp" 

class DirectoryManager {
private:
    MetadataManager *meta;
    DirectoryTree   *tree;

public:
    DirectoryManager();

    void init(MetadataManager *m, DirectoryTree *t);

    // Create a directory at an absolute path
    OFSErrorCodes create_dir(uint32_t owner_uid, const std::string &path);

    // Delete a directory at an absolute path
    OFSErrorCodes delete_dir(uint32_t owner_uid, const std::string &path);

    // List directory children
    OFSErrorCodes list_dir(const std::string &path,
                           std::vector<MetadataEntry> &out);

private:
    bool validate_name(const std::string &name);
};
