#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "MetadataManager.h"

struct DirNode {
    int meta_index;
    int parent_index;
    std::string name;
    std::vector<int> children;
};

class DirectoryTree {
private:
    MetadataManager* meta;

public:
    std::unordered_map<int, DirNode> nodes;

    DirectoryTree();

    void init(MetadataManager* mm);
    void rebuild();

    static std::string normalize(const std::string& path);
    static std::vector<std::string> split(const std::string& path);

    int resolve(const std::string& path);
    void add_child(int parent_idx, int child_idx);
    void remove_child(int parent_idx, int child_idx);
    bool is_empty_dir(int meta_idx);
};
