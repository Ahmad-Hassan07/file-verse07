#include "directory_tree.h"
#include <algorithm>
#include <iostream>

DirectoryTree::DirectoryTree() {
    meta = nullptr;
}

void DirectoryTree::init(MetadataManager *mm) {
    meta = mm;
}

std::string DirectoryTree::normalize(const std::string &path) {
    std::string out;
    out.reserve(path.size());

    bool slash_prev = false;
    for (char c : path) {
        char lc = std::tolower((unsigned char)c);
        if (lc == '/') {
            if (!slash_prev) out.push_back('/');
            slash_prev = true;
        } else {
            out.push_back(lc);
            slash_prev = false;
        }
    }
    if (out.size() > 1 && out.back() == '/') out.pop_back();
    if (out.empty()) out = "/";
    return out;
}

std::vector<std::string> DirectoryTree::split(const std::string &path) {
    std::vector<std::string> parts;
    if (path == "/") return parts;

    size_t start = 1;
    for (size_t i = 1; i <= path.size(); i++) {
        if (i == path.size() || path[i] == '/') {
            std::string seg = path.substr(start, i - start);
            if (!seg.empty()) parts.push_back(seg);
            start = i + 1;
        }
    }
    return parts;
}

void DirectoryTree::rebuild() {
    nodes.clear();

    // FIRST PASS: register ONLY directories as nodes
    for (uint32_t i = 0; i < meta->capacity(); i++) {
        const MetadataEntry &e = meta->get_const(i);
        if (!e.valid_flag) continue;

        if (e.type_flag == 1) { // DIRECTORY ONLY
            DirNode n;
            n.meta_index = i;
            n.parent_index = e.parent_index;
            n.name = e.short_name;

            std::transform(n.name.begin(), n.name.end(), n.name.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            nodes[i] = n;
        }
    }

    // SECOND PASS: add all children (files + directories)
    for (uint32_t i = 0; i < meta->capacity(); i++) {
        const MetadataEntry &e = meta->get_const(i);
        if (!e.valid_flag) continue;

        int parent = e.parent_index;

        // parent must be a directory node
        if (nodes.count(parent)) {
            nodes[parent].children.push_back(i);
        }
    }
}


int DirectoryTree::resolve(const std::string &path) {
    if (!meta) return -1;

    std::string p = normalize(path);
    if (p == "/") return 0;

    auto parts = split(p);
    int curr = 0;

    for (auto &seg : parts) {
        bool found = false;
        if (!nodes.count(curr)) return -1;

        for (auto child : nodes[curr].children) {
            const MetadataEntry &e = meta->get_const(child);

            std::string nm = e.short_name;
            std::transform(nm.begin(), nm.end(), nm.begin(),
                [](unsigned char c){ return std::tolower(c); });

            if (nm == seg) {
                curr = child;
                found = true;
                break;
            }
        }

        if (!found) return -1;
    }
    return curr;
}

void DirectoryTree::add_child(int parent_idx, int child_idx) {
    if (!nodes.count(parent_idx)) return;
    nodes[parent_idx].children.push_back(child_idx);

    const MetadataEntry &e = meta->get_const(child_idx);
    if (e.type_flag == 1) {
        DirNode n;
        n.meta_index = child_idx;
        n.parent_index = parent_idx;
        n.name = e.short_name;
        std::transform(n.name.begin(), n.name.end(), n.name.begin(),
            [](unsigned char c){ return std::tolower(c); });
        nodes[child_idx] = n;
    }
}
void DirectoryTree::remove_child(int parent_idx, int child_idx) {
    if (!nodes.count(parent_idx)) return;

    auto &vec = nodes[parent_idx].children;
    vec.erase(std::remove(vec.begin(), vec.end(), child_idx), vec.end());

    // only remove from nodes if it is a directory
    if (nodes.count(child_idx)) {
        nodes.erase(child_idx);
    }
}

bool DirectoryTree::is_empty_dir(int meta_idx) {
    if (!nodes.count(meta_idx)) return false;
    return nodes[meta_idx].children.empty();
}
