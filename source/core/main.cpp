// #include <iostream>
// #include <vector>
// #include <cstring>
// #include <cassert>


// #include "FileSystem.cpp"   // user/session tests

// // ===========================================================
// // Helper macro
// // ===========================================================
// #define CHECK(x,msg) \
//     if(!(x)){ std::cout << "[FAIL] " << msg << "\n"; return 1; } \
//     else { std::cout << "[ OK ] " << msg << "\n"; }

// // ===========================================================
// // TEST 1 — MetadataManager Basic
// // ===========================================================
// int test_metadata_basic() {
//     std::cout << "\n==== TEST 1: MetadataManager Basic ====\n";

//     const int COUNT = 50;
//     uint8_t buffer[COUNT * sizeof(MetadataEntry)];
//     memset(buffer, 0, sizeof(buffer));

//     MetadataManager mm;
//     CHECK(mm.init(buffer, 0, COUNT), "init()");

//     int idx = mm.allocate_entry();
//     CHECK(idx == 0, "first allocate returns index 0");

//     MetadataEntry e;
//     e.valid_flag = 1;
//     e.type_flag = 0;
//     e.parent_index = 0;
//     strncpy(e.short_name, "hello", 12);

//     CHECK(mm.write_entry(idx, e), "write_entry()");
    
//     MetadataEntry r;
//     CHECK(mm.read_entry(idx, r), "read_entry()");
//     CHECK(strcmp(r.short_name, "hello") == 0, "short_name saved");

//     return 0;
// }

// // ===========================================================
// // TEST 2 — Short Name Truncation <= 10 chars
// // ===========================================================
// int test_shortname() {
//     std::cout << "\n==== TEST 2: Short Name <= 10 chars ====\n";

//     uint8_t buf[1000];
//     memset(buf, 0, sizeof(buf));

//     MetadataManager mm;
//     mm.init(buf, 0, 20);

//     int idx = mm.allocate_entry();
//     MetadataEntry e;
//     e.valid_flag = 1;
//     strncpy(e.short_name, "extralongfilename", 10);
// e.short_name[10] = '\0';


//     mm.write_entry(idx, e);

//     MetadataEntry r;
//     mm.read_entry(idx, r);

//     std::string s = r.short_name;
//     std::cout << "Stored short_name: \"" << s << "\" (len=" << s.size() << ")\n";

//     CHECK(s.size() <= 10, "truncated to <= 10 chars");

//     return 0;
// }

// // ===========================================================
// // TEST 3 — DirectoryTree Resolve
// // ===========================================================
// int test_directory_tree() {
//     std::cout << "\n==== TEST 3: DirectoryTree ====\n";

//     uint8_t buf[5000];
//     memset(buf, 0, sizeof(buf));

//     MetadataManager mm;
//     mm.init(buf, 0, 100);

//     // create root
//     MetadataEntry root{};
//     root.valid_flag = 1;
//     root.type_flag = 1;
//     mm.write_entry(0, root);

//     // create folder /docs
//     MetadataEntry docs{};
//     docs.valid_flag = 1;
//     docs.type_flag = 1;
//     docs.parent_index = 0;
//     strncpy(docs.short_name, "docs", 12);
//     mm.write_entry(1, docs);

//     // create /docs/x
//     MetadataEntry x{};
//     x.valid_flag = 1;
//     x.type_flag = 1;
//     x.parent_index = 1;
//     strncpy(x.short_name, "x", 12);
//     mm.write_entry(2, x);

//     DirectoryTree tree;
//     tree.init(&mm);
//     tree.rebuild();

//     CHECK(tree.resolve("/docs/x") == 2, "resolve /docs/x");
//     CHECK(tree.resolve("/DOCS/x") == 2, "case‐insensitive");
//     CHECK(tree.resolve("/docs/noexist") == -1, "resolve fail");

//     return 0;
// }

// // ===========================================================
// // TEST 4 — Duplicate Name Detection
// // ===========================================================
// int test_duplicate() {
//     std::cout << "\n==== TEST 4: Duplicate Name Detection ====\n";

//     uint8_t buf[5000];
//     memset(buf, 0, sizeof(buf));

//     MetadataManager mm;
//     mm.init(buf, 0, 100);

//     MetadataEntry root{};
//     root.valid_flag = 1;
//     mm.write_entry(0, root);

//     MetadataEntry d1{};
//     d1.valid_flag = 1;
//     d1.parent_index = 0;
//     strncpy(d1.short_name, "same", 12);
//     mm.write_entry(1, d1);

//     MetadataEntry d2{};
//     d2.valid_flag = 1;
//     d2.parent_index = 0;
//     strncpy(d2.short_name, "same", 12);
//     mm.write_entry(2, d2);

//     CHECK(mm.find_in_dir(0, "same") == 1, "duplicate detection picks first");

//     return 0;
// }

// // ===========================================================
// // TEST 5 — Non-Empty Directory Deletion
// // ===========================================================
// int test_nonempty_delete() {
//     std::cout << "\n==== TEST 5: Non-Empty Delete ====\n";

//     uint8_t buf[5000];
//     memset(buf, 0, sizeof(buf));

//     MetadataManager mm;
//     mm.init(buf, 0, 100);

//     // root
//     MetadataEntry root{};
//     root.valid_flag = 1;
//     root.type_flag = 1;
//     mm.write_entry(0, root);

//     // child /a
//     MetadataEntry a{};
//     a.valid_flag = 1;
//     a.type_flag = 1;
//     a.parent_index = 0;
//     strncpy(a.short_name, "a", 12);
//     mm.write_entry(1, a);

//     // file inside /a
//     MetadataEntry file{};
//     file.valid_flag = 1;
//     file.type_flag = 0;
//     file.parent_index = 1;
//     strncpy(file.short_name, "f", 12);
//     mm.write_entry(2, file);

//     DirectoryTree t;
//     t.init(&mm);
//     t.rebuild();

//     CHECK(!t.is_empty_dir(1), "/a is non-empty");

//     return 0;
// }

// // ===========================================================
// // TEST 6 — FreeSpaceManager
// // ===========================================================
// int test_free_map() {
//     std::cout << "\n==== TEST 6: FreeSpaceManager ====\n";

//     uint8_t map[16];
//     memset(map, 0, sizeof(map));

//     FreeSpaceManager fm;
//     CHECK(fm.init(map, 0, 100), "init");

//     CHECK(fm.allocate_block() == 0, "first block = 0");
//     CHECK(fm.allocate_block() == 1, "next block = 1");

//     fm.free_block(0);
//     CHECK(fm.allocate_block() == 0, "re‐use freed block");

//     return 0;
// }

// // ===========================================================
// // TEST 7 — BlockManager
// // ===========================================================
// int test_block_manager() {
//     std::cout << "\n==== TEST 7: BlockManager ====\n";

//     uint8_t file[4096 * 10];
//     memset(file, 0, sizeof(file));

//     FreeSpaceManager fm;
//     fm.init(file, 0, 10);

//     BlockManager bm;
//     CHECK(bm.init(file, 0, 4096, 10, &fm), "init block manager");

//     // allocate a block chain
//     int start = bm.allocate_block();
//     CHECK(start >= 0, "allocate block");

//     const char* msg = "HELLO WORLD";
//     CHECK(bm.write_file(start, 0, (uint8_t*)msg, strlen(msg)) >= 0, "write_file");

//     char out[64];
//     memset(out, 0, sizeof(out));
//     CHECK(bm.read_file(start, 0, (uint8_t*)out, strlen(msg)) >= 0, "read_file");
//     CHECK(strcmp(out, msg) == 0, "read matches written data");

//     return 0;
// }

// // ===========================================================
// // RUN ALL TESTS
// // ===========================================================
// int main() {
//     std::cout << "\n================== FULL TEST SUITE ==================\n";

//     CHECK(test_metadata_basic() == 0, "MetadataManager basic");
//     CHECK(test_shortname() == 0, "short_name <= 10");
//     CHECK(test_directory_tree() == 0, "DirectoryTree");
//     CHECK(test_duplicate() == 0, "Duplicate name detection");
//     CHECK(test_nonempty_delete() == 0, "Non-empty delete");
//     CHECK(test_free_map() == 0, "FreeSpaceManager");
//     CHECK(test_block_manager() == 0, "BlockManager");

//     std::cout << "\n🎉 ALL TESTS PASSED SUCCESSFULLY! 🎉\n";
//     return 0;
// }


#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "FileSystem.cpp"   // This pulls ALL .cpp files

using std::cout;
using std::endl;

// ======================================================
// Utility ASSERT macro
// ======================================================
#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            cout << "[FAIL] " << msg << endl; \
            return false; \
        } else { \
            cout << "[ OK ] " << msg << endl; \
        } \
    } while(0)

// ======================================================
// CONFIG GENERATION
// ======================================================
FSConfig make_config() {
    FSConfig cfg;
    cfg.total_size = 10 * 1024 * 1024;   // 10 MB
    cfg.block_size = 4096;
    cfg.header_size = 512;
    cfg.max_users = 50;
    cfg.max_inodes = 0;
    cfg.max_files = 2000;
    cfg.max_filename_length = 10;
    cfg.require_auth = 1;
    cfg.server_port = 8080;
    cfg.max_connections = 64;
    cfg.queue_timeout = 30;
    strcpy(cfg.admin_username, "admin");
    strcpy(cfg.admin_password, "admin123");

    return cfg;
}

// ======================================================
// GLOBAL TEST HELPERS
// ======================================================
bool create_fs(FileSystem &fs, const char* path) {
    FSConfig cfg = make_config();
    bool r = fs.format_new(cfg, path);
    CHECK(r, "format_new()");
    return true;
}

bool load_fs(FileSystem &fs, const char* path) {
    FSConfig cfg = make_config();
    bool r = fs.load_existing(cfg, path);
    CHECK(r, "load_existing()");
    return true;
}

// ======================================================
// USER + SESSION TESTS
// ======================================================
bool test_users() {
    cout << "\n==== TEST USERS ====\n";

    FileSystem fs;
    create_fs(fs, "test.omni");
    load_fs(fs, "test.omni");

    void* admin = nullptr;
    CHECK(fs.user_login("admin", "x", &admin) == OFSErrorCodes::SUCCESS,
          "admin login");

    // create user
    CHECK(fs.user_create(admin, "u1", "pw", UserRole::NORMAL) == OFSErrorCodes::SUCCESS,
          "user_create u1");

    // duplicate user
    CHECK(fs.user_create(admin, "u1", "pw", UserRole::NORMAL) == OFSErrorCodes::ERROR_FILE_EXISTS,
          "duplicate user_create");

    // list users
    UserInfo* arr;
    int count;
    CHECK(fs.user_list(admin, &arr, &count) == OFSErrorCodes::SUCCESS,
          "user_list OK");
    CHECK(count >= 2, "user_list count >= 2");

    // login as u1
    void* s1 = nullptr;
    CHECK(fs.user_login("u1", "pw", &s1) == OFSErrorCodes::SUCCESS,
          "u1 login");

    // session info
    SessionInfo inf;
    CHECK(fs.get_session_info(s1, &inf) == OFSErrorCodes::SUCCESS,
          "session info OK");

    // delete user
    CHECK(fs.user_delete(admin, "u1") == OFSErrorCodes::SUCCESS,
          "user_delete OK");

    // login deleted user
    void* t = nullptr;
    CHECK(fs.user_login("u1", "pw", &t) == OFSErrorCodes::ERROR_NOT_FOUND,
          "login deleted user");

    CHECK(fs.user_logout(admin) == OFSErrorCodes::SUCCESS,
          "admin logout");

    return true;
}
// ======================================================
// DIRECTORY TESTS
// ======================================================
bool test_directories() {
    cout << "\n==== TEST DIRECTORIES ====\n";

    FileSystem fs;
    create_fs(fs, "test.omni");
    load_fs(fs, "test.omni");

    void* admin = nullptr;
    fs.user_login("admin", "x", &admin);

    // mkdir /docs
    CHECK(fs.dir_create(admin, "/docs") == OFSErrorCodes::SUCCESS,
          "mkdir /docs");

    // mkdir nested /docs/a/b
    CHECK(fs.dir_create(admin, "/docs/a") == OFSErrorCodes::SUCCESS,
          "mkdir /docs/a");
    CHECK(fs.dir_create(admin, "/docs/a/b") == OFSErrorCodes::SUCCESS,
          "mkdir /docs/a/b");

    // duplicate mkdir
    CHECK(fs.dir_create(admin, "/docs/a") == OFSErrorCodes::ERROR_FILE_EXISTS,
          "mkdir duplicate");

    // invalid path
    CHECK(fs.dir_create(admin, "////") == OFSErrorCodes::ERROR_INVALID_PATH,
          "mkdir invalid path");

    // delete non-empty directory
    CHECK(fs.dir_delete(admin, "/docs") == OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY,
          "non-empty delete fails");

    // list directory
    FileEntry* out;
    int count;
    CHECK(fs.dir_list(admin, "/docs", &out, &count) == OFSErrorCodes::SUCCESS,
          "dir_list /docs");
    CHECK(count >= 1, "dir_list has children");
    free(out);

    return true;
}

// ======================================================
// FILE TESTS
// ======================================================
bool test_files() {
    cout << "\n==== TEST FILES ====\n";

    FileSystem fs;
    create_fs(fs, "test.omni");
    load_fs(fs, "test.omni");

    void* admin = nullptr;
    fs.user_login("admin", "x", &admin);

    fs.dir_create(admin, "/docs");

    // create empty file
    CHECK(fs.file_create(admin, "/docs/f1", "", 0) == OFSErrorCodes::SUCCESS,
          "create empty file");

    // duplicate file
    CHECK(fs.file_create(admin, "/docs/f1", "", 0) == OFSErrorCodes::ERROR_FILE_EXISTS,
          "duplicate file");

    // write small
    const char* msg = "HelloWorld";
    CHECK(fs.file_edit(admin, "/docs/f1", msg, strlen(msg), 0) == OFSErrorCodes::SUCCESS,
          "write small");

    // read small
    char* buf;
    size_t sz;
    CHECK(fs.file_read(admin, "/docs/f1", &buf, &sz) == OFSErrorCodes::SUCCESS,
          "read small");
    CHECK(sz == strlen(msg), "read size matches");
    CHECK(strncmp(buf, msg, sz) == 0, "read data matches");
    free(buf);

    // write multi-block large file
    std::string big(9000, 'A'); // > 2 blocks
    CHECK(fs.file_edit(admin, "/docs/f1", big.c_str(), big.size(), 0) == OFSErrorCodes::SUCCESS,
          "write big multi-block");

    // read big
    CHECK(fs.file_read(admin, "/docs/f1", &buf, &sz) == OFSErrorCodes::SUCCESS,
          "read big OK");
    CHECK(sz == big.size(), "big size matches");
    CHECK(buf[0] == 'A', "first A OK");
    CHECK(buf[8999] == 'A', "last A OK");
    free(buf);

    // append
    const char* tail = "TAIL";
    CHECK(fs.file_edit(admin, "/docs/f1", tail, 4, big.size()) == OFSErrorCodes::SUCCESS,
          "append OK");

    CHECK(fs.file_read(admin, "/docs/f1", &buf, &sz) == OFSErrorCodes::SUCCESS,
          "read after append");
    CHECK(sz == big.size() + 4, "append size correct");
    CHECK(buf[sz-1] == 'L', "append data OK");
    free(buf);

    // truncate
    CHECK(fs.file_truncate(admin, "/docs/f1") == OFSErrorCodes::SUCCESS,
          "truncate OK");

    CHECK(fs.file_read(admin, "/docs/f1", &buf, &sz) == OFSErrorCodes::SUCCESS,
          "read empty after truncate");
    CHECK(sz == 0, "truncate -> size=0");
    free(buf);

    // delete file
    CHECK(fs.file_delete(admin, "/docs/f1") == OFSErrorCodes::SUCCESS,
          "delete file OK");

    // read deleted file
    CHECK(fs.file_read(admin, "/docs/f1", &buf, &sz) == OFSErrorCodes::ERROR_NOT_FOUND,
          "read deleted file fails");

    return true;
}
// ======================================================
// METADATA + STATS + EDGE CASES
// ======================================================
bool test_metadata_stats() {
    cout << "\n==== TEST METADATA + STATS ====\n";

    FileSystem fs;
    create_fs(fs, "test.omni");
    load_fs(fs, "test.omni");

    void* admin = nullptr;
    fs.user_login("admin", "x", &admin);

    fs.dir_create(admin, "/docs");
    fs.file_create(admin, "/docs/f1", "abc", 3);

    // metadata
    FileMetadata md;
    CHECK(fs.get_metadata(admin, "/docs/f1", &md) == OFSErrorCodes::SUCCESS,
          "metadata OK");
    CHECK(md.actual_size == 3, "metadata size OK");

    // permissions
    CHECK(fs.set_permissions(admin, "/docs/f1", 0777) == OFSErrorCodes::SUCCESS,
          "set permissions OK");

    FileMetadata md2;
    fs.get_metadata(admin, "/docs/f1", &md2);
    CHECK(md2.entry.permissions == 0777, "permissions updated");

    // stats
    FSStats st;
    CHECK(fs.get_stats(admin, &st) == OFSErrorCodes::SUCCESS,
          "stats OK");
    CHECK(st.total_files == 1, "stats file count = 1");
    CHECK(st.total_directories >= 1, "stats dir count >= 1");

    return true;
}

// ======================================================
// MAIN
// ======================================================
int main() {
    cout << "\n================== FULL TEST SUITE ==================\n";

    if (!test_users()) return 1;
    if (!test_directories()) return 1;
    if (!test_files()) return 1;
    if (!test_metadata_stats()) return 1;

    cout << "\n🎉 ALL PHASE-2 TESTS PASSED SUCCESSFULLY! 🎉\n";
    return 0;
}
