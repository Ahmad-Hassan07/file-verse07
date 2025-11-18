#include "DirectoryManager.h"
#include <algorithm>
#include <iostream>

DirectoryManager::DirectoryManager() {
    meta = nullptr;
    tree = nullptr;
}

void DirectoryManager::init(MetadataManager *m, DirectoryTree *t) {
    meta = m;
    tree = t;
}

// ---------------------------------------------------------------
// Validate short name: 1–10 chars (document requirement)
// ---------------------------------------------------------------
bool DirectoryManager::validate_name(const std::string &name) {
    if (name.empty()) return false;
    if (name.size() > 10) return false;  // doc requires max 10 chars
    return true;
}

// ---------------------------------------------------------------
// CREATE DIRECTORY
// ---------------------------------------------------------------
OFSErrorCodes DirectoryManager::create_dir(uint32_t owner_uid,
                                           const std::string &path)
{
    if (path.empty() || path[0] != '/')
        return OFSErrorCodes::ERROR_INVALID_PATH;

    std::string norm = DirectoryTree::normalize(path);
    if (norm == "/") return OFSErrorCodes::ERROR_INVALID_OPERATION;

    // split into components
    auto parts = DirectoryTree::split(norm);
    if (parts.empty()) return OFSErrorCodes::ERROR_INVALID_PATH;

    // parent path = everything except last
    std::string parent_path = "/";
    if (parts.size() > 1) {
        for (size_t i = 0; i < parts.size() - 1; i++)
            parent_path += parts[i] + "/";
        if (parent_path.size() > 1 && parent_path.back() == '/')
            parent_path.pop_back();
    }

    std::string name = parts.back();
    if (!validate_name(name))
        return OFSErrorCodes::ERROR_INVALID_PATH;

    // resolve parent
    int parent_idx = tree->resolve(parent_path);
    if (parent_idx < 0)
        return OFSErrorCodes::ERROR_NOT_FOUND;

    MetadataEntry par;
    meta->read_entry(parent_idx, par);
    if (par.type_flag != 1)
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    // check if name already exists
    if (meta->find_in_dir(parent_idx, name) >= 0)
        return OFSErrorCodes::ERROR_FILE_EXISTS;

    // allocate metadata entry
    int new_idx = meta->allocate_entry();
    if (new_idx < 0)
        return OFSErrorCodes::ERROR_NO_SPACE;

    // create entry
    MetadataEntry e;
    e.valid_flag = 1;
    e.type_flag = 1;     // directory
    e.parent_index = parent_idx;

    memset(e.short_name, 0, 12);
    MetadataManager::to_short_name(name, e.short_name);


    e.owner       = owner_uid;
    e.permissions = 0755;
    e.created_time = e.modified_time = time(NULL);

    meta->write_entry(new_idx, e);

    // update tree
    tree->add_child(parent_idx, new_idx);

    return OFSErrorCodes::SUCCESS;
}

// ---------------------------------------------------------------
// DELETE DIRECTORY
// ---------------------------------------------------------------
OFSErrorCodes DirectoryManager::delete_dir(uint32_t owner_uid,
                                           const std::string &path)
{
    if (path == "/" || path.empty())
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    std::string norm = DirectoryTree::normalize(path);

    int idx = tree->resolve(norm);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    MetadataEntry e;
    meta->read_entry(idx, e);

    if (e.type_flag != 1)
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    // directory must be empty
    if (!tree->is_empty_dir(idx))
        return OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY;

    // owner check
    if (owner_uid != e.owner)
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;

    // free metadata
    meta->free_entry(idx);

    // update tree
    tree->remove_child(e.parent_index, idx);

    return OFSErrorCodes::SUCCESS;
}

// ---------------------------------------------------------------
// LIST DIRECTORY
// ---------------------------------------------------------------
OFSErrorCodes DirectoryManager::list_dir(const std::string &path,
                                         std::vector<MetadataEntry> &out)
{
    std::string norm = DirectoryTree::normalize(path);

    int idx = tree->resolve(norm);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    MetadataEntry dirent;
    meta->read_entry(idx, dirent);

    if (dirent.type_flag != 1)
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    out.clear();

    // iterate over metadata table
    for (uint32_t i = 0; i < meta->capacity(); i++) {
        MetadataEntry e;
        meta->read_entry(i, e);

        if (e.valid_flag && e.parent_index == (uint32_t)idx) {
            out.push_back(e);
        }
    }

    return OFSErrorCodes::SUCCESS;
}
