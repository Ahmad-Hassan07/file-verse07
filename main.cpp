#include <iostream>
#include <string>
#include <vector>

#include "source/include/odf_types.hpp"
#include "source/include/ofs_internal.h"

#include "source/api/ofs_api.h"

#include "source/data_structures/SinglyLinkedList.h"
#include "source/data_structures/Stack.h"
#include "source/data_structures/Queue.h"
#include "source/data_structures/AVLTree.h"
#include "source/data_structures/HashTable.h"
#include "source/data_structures/DirectoryNode.h"

int main() {
    void* instance = nullptr;
    int rc = fs_init(&instance, "filesystem.omni", "compiled/default.uconf");
    std::cout << "fs_init rc = " << rc << std::endl;

    if (rc == (int)OFSErrorCodes::SUCCESS) {
        std::cout << "fs_init success" << std::endl;
    } else {
        std::cout << "fs_init failed" << std::endl;
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
