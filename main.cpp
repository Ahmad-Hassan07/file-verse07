#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "source/include/odf_types.hpp"
#include "source/include/ofs_internal.h"

#include "source/api/ofs_api.h"
#include "source/core/config_parser.h"
#include "source/core/FileSystem.h"

#include "source/data_structures/SinglyLinkedList.h"
#include "source/data_structures/Stack.h"
#include "source/data_structures/Queue.h"
#include "source/data_structures/AVLTree.h"
#include "source/data_structures/HashTable.h"
#include "source/data_structures/DirectoryNode.h"

int main() {
    FSConfig cfg;
    const char* cfg_path = "compiled/default.uconf";
    bool ok_cfg = parse_uconf(cfg_path, cfg);
    std::cout << "parse_uconf(" << cfg_path << ") = " << (ok_cfg ? 1 : 0) << std::endl;
    if (ok_cfg) {
        std::cout << "total_size = " << cfg.total_size << std::endl;
        std::cout << "block_size = " << cfg.block_size << std::endl;
        std::cout << "max_users = " << cfg.max_users << std::endl;
        std::cout << "max_inodes = " << cfg.max_inodes << std::endl;
    }

    const char* omni_path = "compiled/test.omni";
    int rc_fmt = fs_format(omni_path, cfg_path);
    std::cout << "fs_format rc = " << rc_fmt << std::endl;

    if (rc_fmt == (int)OFSErrorCodes::SUCCESS) {
        std::ifstream f(omni_path, std::ios::binary | std::ios::ate);
        if (f.is_open()) {
            std::streamsize sz = f.tellg();
            std::cout << "formatted omni size = " << (long long)sz << std::endl;
            f.close();
        } else {
            std::cout << "could not open formatted omni file" << std::endl;
        }
    }

    void* instance = nullptr;
    int rc_init = fs_init(&instance, omni_path, cfg_path);
    std::cout << "fs_init rc = " << rc_init << std::endl;

    if (rc_init == (int)OFSErrorCodes::SUCCESS) {
        std::cout << "fs_init success" << std::endl;
    } else {
        std::cout << "fs_init failed" << std::endl;
    }

    if (instance) {
        FileSystem* fs = (FileSystem*)instance;
        FreeSpaceManager& fm = fs->freemap();
        BlockManager& bm = fs->blocks();

        unsigned int b1 = 0, b2 = 0, b3 = 0;
        bool a1 = fm.allocate_one(b1);
        bool a2 = fm.allocate_one(b2);
        bool a3 = fm.allocate_one(b3);
        std::cout << "block allocations: " << a1 << " " << b1 << " , " << a2 << " " << b2 << " , " << a3 << " " << b3 << std::endl;

        if (a1) {
            std::vector<unsigned char> blk;
            bm.read_block(b1, blk);
            std::cout << "read_block size for b1 = " << blk.size() << std::endl;
        }
    }

    Stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    std::cout << "Stack pop order:";
    while (!s.empty()) {
        int v = 0;
        s.pop(v);
        std::cout << " " << v;
    }
    std::cout << std::endl;

    Queue<int> q;
    q.enqueue(10);
    q.enqueue(20);
    q.enqueue(30);
    std::cout << "Queue dequeue order:";
    while (!q.empty()) {
        int v = 0;
        q.dequeue(v);
        std::cout << " " << v;
    }
    std::cout << std::endl;

    AVLTree<int, std::string> tree;
    tree.insert(2, "two");
    tree.insert(1, "one");
    tree.insert(3, "three");
    std::vector<int> keys = tree.inorder_keys();
    std::cout << "AVL inorder keys:";
    for (unsigned long long i = 0; i < keys.size(); i++) {
        std::cout << " " << keys[i];
    }
    std::cout << std::endl;
    std::string value;
    if (tree.get(2, value)) {
        std::cout << "AVL get(2) = " << value << std::endl;
    }

    HashTable<unsigned long long, int> ht;
    ht.put(11, 100);
    ht.put(22, 200);
    ht.put(33, 300);
    int out = 0;
    std::cout << "HashTable lookups:" << std::endl;
    if (ht.get(11, out)) {
        std::cout << " key 11 -> " << out << std::endl;
    }
    if (ht.get(22, out)) {
        std::cout << " key 22 -> " << out << std::endl;
    }
    if (ht.get(33, out)) {
        std::cout << " key 33 -> " << out << std::endl;
    }

    char root_name[12];
    for (int i = 0; i < 12; i++) root_name[i] = 0;
    std::string rn = "root";
    for (int i = 0; i < (int)rn.size() && i < 11; i++) root_name[i] = rn[i];

    DirectoryNode root(root_name, 1, nullptr);

    char child1_name[12];
    char child2_name[12];
    for (int i = 0; i < 12; i++) { child1_name[i] = 0; child2_name[i] = 0; }
    std::string c1 = "docs";
    std::string c2 = "logs";
    for (int i = 0; i < (int)c1.size() && i < 11; i++) child1_name[i] = c1[i];
    for (int i = 0; i < (int)c2.size() && i < 11; i++) child2_name[i] = c2[i];

    root.add_child(child1_name, 2);
    root.add_child(child2_name, 3);

    std::vector<DirChild> children = root.list_children();
    std::cout << "Directory children:" << std::endl;
    for (unsigned long long i = 0; i < children.size(); i++) {
        std::cout << " entry_index=" << children[i].entry_index << " name=";
        for (int j = 0; j < 12 && children[i].name[j] != 0; j++) {
            std::cout << children[i].name[j];
        }
        std::cout << std::endl;
    }

    if (instance) {
        fs_shutdown(instance);
        std::cout << "fs_shutdown done" << std::endl;
    }

    return 0;
}
