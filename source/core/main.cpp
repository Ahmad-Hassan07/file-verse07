#include <iostream>
#include <vector>
#include <cstring>
#include "MetadataManager.cpp"
#include "directory_tree.cpp"
#include "../include/ofs_internal.h"   

using namespace std;

static void banner(const string& s) {
    cout << "\n================ " << s << " ================\n";
}

bool test_metadata_basic() {
    banner("TEST 1 — MetadataManager Basic Alloc/Write/Read");

    const int COUNT = 32;
    vector<uint8_t> mem(COUNT * sizeof(MetadataEntry));
    MetadataManager mm;

    if (!mm.init(mem.data(), 0, COUNT))
        return false;

    int idx = mm.allocate_entry();
    if (idx != 0) { cout << "Expected index 0\n"; return false; }

    MetadataEntry e{};
    e.valid_flag = 1;
    e.type_flag = 1;
    e.parent_index = 0;
    strncpy(e.short_name, "docs", 12);

    if (!mm.write_entry(idx, e)) { cout << "write failed\n"; return false; }

    MetadataEntry r{};
    if (!mm.read_entry(idx, r)) { cout << "read failed\n"; return false; }

    if (r.valid_flag != 1 || r.type_flag != 1) return false;
    if (string(r.short_name) != "docs") return false;

    cout << "OK\n";
    return true;
}

bool test_shortname_10chars() {
    banner("TEST 2 — short_name <= 10 chars");

    vector<uint8_t> mem(64 * sizeof(MetadataEntry));
    MetadataManager mm;
    mm.init(mem.data(), 0, 64);

    int idx = mm.allocate_entry();

    MetadataEntry e{};
    e.valid_flag = 1;
    e.type_flag = 1;

    // Copy long name into short_name
    strncpy(e.short_name, "extralongf", 10);
    e.short_name[11] = '\0';   // FORCE NULL TERMINATION

    mm.write_entry(idx, e);

    MetadataEntry r{};
    mm.read_entry(idx, r);

    string name = r.short_name;

    cout << "Stored short_name: \"" << name << "\"\n";
    cout << "Length = " << name.size() << endl;

    if (name.size() > 10) {
        cout << "ERROR: short_name too long!\n";
        return false;
    }

    cout << "OK\n";
    return true;
}

bool test_directorytree_basic() {
    banner("TEST 3 — DirectoryTree Rebuild + Resolve");

    const int COUNT = 64;
    vector<uint8_t> mem(COUNT * sizeof(MetadataEntry));
    memset(mem.data(), 0, mem.size());

    MetadataManager mm;
    mm.init(mem.data(), 0, COUNT);

    // root
    MetadataEntry root{};
    root.valid_flag = 1;
    root.type_flag = 1;
    root.parent_index = 0;
    strncpy(root.short_name, "", 12);
    mm.write_entry(0, root);

    // /docs
    int d1 = mm.allocate_entry();
    MetadataEntry docs{};
    docs.valid_flag = 1;
    docs.type_flag = 1;
    docs.parent_index = 0;
    strncpy(docs.short_name, "docs", 12);
    mm.write_entry(d1, docs);

    // /docs/sub
    int d2 = mm.allocate_entry();
    MetadataEntry sub{};
    sub.valid_flag = 1;
    sub.type_flag = 1;
    sub.parent_index = d1;
    strncpy(sub.short_name, "sub", 12);
    mm.write_entry(d2, sub);

    DirectoryTree tree;
    tree.init(&mm);
    tree.rebuild();

    if (tree.resolve("/docs") != d1) return false;
    if (tree.resolve("/docs/sub") != d2) return false;
    if (tree.resolve("/DOCS/SUB") != d2) return false; // case-insensitive
    if (tree.resolve("/doesnotexist") != -1) return false;

    cout << "OK\n";
    return true;
}

bool test_duplicate_name() {
    banner("TEST 4 — Duplicate Name Detection");

    vector<uint8_t> mem(64 * sizeof(MetadataEntry));
    memset(mem.data(), 0, mem.size());

    MetadataManager mm;
    mm.init(mem.data(), 0, 64);

    // root
    MetadataEntry root{};
    root.valid_flag = 1;
    root.type_flag = 1;
    mm.write_entry(0, root);

    // /docs
    int idx1 = mm.allocate_entry();
    MetadataEntry e1{};
    e1.valid_flag = 1;
    e1.type_flag = 1;
    strncpy(e1.short_name, "docs", 12);
    mm.write_entry(idx1, e1);

    // duplicate
    int dup = mm.find_in_dir(0, "docs");
    if (dup != idx1) return false;

    cout << "OK\n";
    return true;
}

bool test_non_empty_dir_delete() {
    banner("TEST 5 — Non-Empty Directory Delete Check");

    vector<uint8_t> mem(64 * sizeof(MetadataEntry));
    memset(mem.data(), 0, mem.size());

    MetadataManager mm;
    mm.init(mem.data(), 0, 64);

    MetadataEntry root{};
    root.valid_flag = 1; root.type_flag = 1;
    mm.write_entry(0, root);

    // /docs
    int d1 = mm.allocate_entry();
    MetadataEntry docs{};
    docs.valid_flag = 1; docs.type_flag = 1; docs.parent_index = 0;
    strncpy(docs.short_name, "docs", 12);
    mm.write_entry(d1, docs);

    // /docs/a
    int f = mm.allocate_entry();
    MetadataEntry file{};
    file.valid_flag = 1; file.type_flag = 0; file.parent_index = d1;
    strncpy(file.short_name, "file", 12);
    mm.write_entry(f, file);

    DirectoryTree tree;
    tree.init(&mm);
    tree.rebuild();

    bool empty = tree.is_empty_dir(d1);
    if (empty) return false;

    cout << "OK\n";
    return true;
}

bool test_stress() {
    banner("TEST 6 — Stress Test (Random Allocation)");

    const int COUNT = 4096;
    vector<uint8_t> mem(COUNT * sizeof(MetadataEntry));
    memset(mem.data(), 0, mem.size());

    MetadataManager mm;
    mm.init(mem.data(), 0, COUNT);

    for (int i = 0; i < 2000; i++) {
        int idx = mm.allocate_entry();
        if (idx < 0) return false;

        MetadataEntry e{};
        e.valid_flag = 1;
        e.type_flag = 1;
        e.parent_index = 0;
        sprintf(e.short_name, "d%d", i);
        mm.write_entry(idx, e);
    }

    cout << "OK\n";
    return true;
}

int main() {
    cout << "====== PHASE 1 TEST SUITE ======\n";

    if (!test_metadata_basic()) return 1;
    if (!test_shortname_10chars()) return 1;
    if (!test_directorytree_basic()) return 1;
    if (!test_duplicate_name()) return 1;
    if (!test_non_empty_dir_delete()) return 1;
    if (!test_stress()) return 1;

    cout << "\n🎉 ALL PHASE-1 TESTS PASSED SUCCESSFULLY! 🎉\n";
    return 0;
}
