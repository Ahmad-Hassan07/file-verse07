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
            std::cout << "formatted omni size = " << (long long) f.tellg() << std::endl;
            f.close();
        }
    }

    void* instance = nullptr;

    int rc_init = fs_init(&instance, omni_path, cfg_path);
    std::cout << "fs_init rc = " << rc_init << std::endl;

    if (rc_init == 0)
        std::cout << "fs_init success" << std::endl;
    else
        std::cout << "fs_init failed" << std::endl;

    if (!instance) {
        std::cout << "instance null after fs_init" << std::endl;
        return 0;
    }

    std::cout << "FileSystem instance successfully created!" << std::endl;

    // ---------- BLOCK ALLOCATION TEST ----------
    std::cout << "Attempting to allocate blocks using free space manager..." << std::endl;

    FileSystem* fs = (FileSystem*)instance;

    FreeSpaceManager& fm = fs->freemap();
    BlockManager& bm = fs->blocks();

    unsigned int b1 = 0, b2 = 0, b3 = 0;

    bool a1 = fm.allocate_one(b1);
    bool a2 = fm.allocate_one(b2);
    bool a3 = fm.allocate_one(b3);

    std::cout << "Block allocation results: "
              << a1 << " " << b1 << " , "
              << a2 << " " << b2 << " , "
              << a3 << " " << b3 << std::endl;

    // ---------- OPTIONAL: block read test ----------
    if (a1) {
        std::vector<unsigned char> blk;
        bm.read_block(b1, blk);
        std::cout << "Read block size = " << blk.size() << std::endl;
    }

    // ---------- BASIC DATA STRUCTURE TESTS ----------
    std::cout << "\nTesting stack..." << std::endl;
    Stack<int> s;
    s.push(1); s.push(2); s.push(3);
    while (!s.empty()) {
        int v; s.pop(v);
        std::cout << v << " ";
    }
    std::cout << std::endl;

    std::cout << "Testing queue..." << std::endl;
    Queue<int> q;
    q.enqueue(10); q.enqueue(20); q.enqueue(30);
    while (!q.empty()) {
        int v; q.dequeue(v);
        std::cout << v << " ";
    }
    std::cout << std::endl;

    std::cout << "Testing AVL..." << std::endl;
    AVLTree<int,std::string> t;
    t.insert(2,"two");
    t.insert(1,"one");
    t.insert(3,"three");
    std::vector<int> keys = t.inorder_keys();
    for (int x: keys) std::cout << x << " ";
    std::cout << std::endl;

    std::string getv;
    if (t.get(2,getv))
        std::cout << "AVL get(2) = " << getv << std::endl;

    std::cout << "Testing hash table..." << std::endl;
    HashTable<unsigned long long,int> ht;
    ht.put(11,100);
    ht.put(22,200);
    ht.put(33,300);

    int out;
    if (ht.get(11,out)) std::cout << "11->" << out << std::endl;
    if (ht.get(22,out)) std::cout << "22->" << out << std::endl;
    if (ht.get(33,out)) std::cout << "33->" << out << std::endl;

    // done
    fs_shutdown(instance);
    std::cout << "fs_shutdown done" << std::endl;

    return 0;
}
