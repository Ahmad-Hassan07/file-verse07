// #include <iostream>
// #include <string>
// #include "source/api/ofs_api.h"

// int main() {
//     std::string omni_path = "filesystem.omni";
//     std::string config_path = "compiled/default.uconf";

//     std::cout << "OFS Test Menu\n";

//     while (true) {
//         std::cout << "\n1. Format file system\n";
//         std::cout << "2. Init file system\n";
//         std::cout << "3. Exit\n";
//         std::cout << "Choice: ";
//         int choice;
//         if (!(std::cin >> choice)) return 0;
//         if (choice == 3) return 0;

//         if (choice == 1) {
//             int rc = fs_format(omni_path.c_str(), config_path.c_str());
//             std::cout << "fs_format rc = " << rc << " (" << get_error_message(rc) << ")\n";
//         } else if (choice == 2) {
//             void* instance = 0;
//             int rc = fs_init(&instance, omni_path.c_str(), config_path.c_str());
//             std::cout << "fs_init rc = " << rc << " (" << get_error_message(rc) << ")\n";
//             if (rc != 0) continue;

//             void* current_session = 0;

//             while (true) {
//                 if (!current_session) {
//                     std::cout << "\nNo user logged in.\n";
//                     std::cout << "1. Login\n";
//                     std::cout << "2. Back to main menu\n";
//                     std::cout << "Choice: ";
//                     int c;
//                     std::cin >> c;
//                     if (c == 2) break;
//                     if (c == 1) {
//                         std::string u;
//                         std::string p;
//                         std::cout << "Username: ";
//                         std::cin >> u;
//                         std::cout << "Password: ";
//                         std::cin >> p;
//                         void* sess = 0;
//                         int lrc = user_login(&sess, u.c_str(), p.c_str());
//                         if (lrc == 0) {
//                             current_session = sess;
//                             SessionInfo info;
//                             int src = get_session_info(current_session, &info);
//                             if (src == 0) {
//                                 std::cout << "Login success. Role ";
//                                 if (info.user.role == UserRole::ADMIN) std::cout << "ADMIN\n";
//                                 else std::cout << "NORMAL\n";
//                             } else {
//                                 std::cout << "get_session_info rc = " << src << " (" << get_error_message(src) << ")\n";
//                             }
//                         } else {
//                             std::cout << "user_login rc = " << lrc << " (" << get_error_message(lrc) << ")\n";
//                         }
//                     }
//                 } else {
//                     SessionInfo info;
//                     int src = get_session_info(current_session, &info);
//                     if (src != 0) {
//                         std::cout << "Session invalid. Logging out.\n";
//                         current_session = 0;
//                         continue;
//                     }
//                     bool is_admin = (info.user.role == UserRole::ADMIN);
//                     std::cout << "\nLogged in as " << info.user.username;
//                     if (is_admin) std::cout << " (ADMIN)";
//                     std::cout << "\n";
//                     std::cout << "1. Show session info\n";
//                     if (is_admin) {
//                         std::cout << "2. Create user\n";
//                         std::cout << "3. Delete user\n";
//                         std::cout << "4. List users\n";
//                         std::cout << "5. Logout\n";
//                         std::cout << "6. Back to main menu (shutdown)\n";
//                     } else {
//                         std::cout << "2. Logout\n";
//                         std::cout << "3. Back to main menu (shutdown)\n";
//                     }
//                     std::cout << "Choice: ";
//                     int c;
//                     std::cin >> c;

//                     if (is_admin) {
//                         if (c == 1) {
//                             SessionInfo s2;
//                             int irc = get_session_info(current_session, &s2);
//                             if (irc == 0) {
//                                 std::cout << "Session ID: " << s2.session_id << "\n";
//                                 std::cout << "Username: " << s2.user.username << "\n";
//                                 std::cout << "Role: " << (s2.user.role == UserRole::ADMIN ? "ADMIN" : "NORMAL") << "\n";
//                             } else {
//                                 std::cout << "get_session_info rc = " << irc << " (" << get_error_message(irc) << ")\n";
//                             }
//                         } else if (c == 2) {
//                             std::string nu;
//                             std::string np;
//                             int r;
//                             std::cout << "New username: ";
//                             std::cin >> nu;
//                             std::cout << "New password: ";
//                             std::cin >> np;
//                             std::cout << "Role (0=normal,1=admin): ";
//                             int rr;
//                             std::cin >> rr;
//                             UserRole role = rr == 1 ? UserRole::ADMIN : UserRole::NORMAL;
//                             r = user_create(current_session, nu.c_str(), np.c_str(), role);
//                             std::cout << "user_create rc = " << r << " (" << get_error_message(r) << ")\n";
//                         } else if (c == 3) {
//                             std::string du;
//                             std::cout << "Username to delete: ";
//                             std::cin >> du;
//                             int r = user_delete(current_session, du.c_str());
//                             std::cout << "user_delete rc = " << r << " (" << get_error_message(r) << ")\n";
//                         } else if (c == 4) {
//                             UserInfo* arr = 0;
//                             int count = 0;
//                             int r = user_list(current_session, &arr, &count);
//                             std::cout << "user_list rc = " << r << " (" << get_error_message(r) << ")\n";
//                             if (r == 0 && count > 0 && arr) {
//                                 std::cout << "Users:\n";
//                                 for (int i = 0; i < count; i++) {
//                                     std::cout << "- " << arr[i].username << " role ";
//                                     if (arr[i].role == UserRole::ADMIN) std::cout << "ADMIN";
//                                     else std::cout << "NORMAL";
//                                     std::cout << "\n";
//                                 }
//                                 free_buffer(arr);
//                             }
//                         } else if (c == 5) {
//                             int r = user_logout(current_session);
//                             std::cout << "user_logout rc = " << r << " (" << get_error_message(r) << ")\n";
//                             current_session = 0;
//                         } else if (c == 6) {
//                             fs_shutdown(instance);
//                             instance = 0;
//                             return 0;
//                         }
//                     } else {
//                         if (c == 1) {
//                             SessionInfo s2;
//                             int irc = get_session_info(current_session, &s2);
//                             if (irc == 0) {
//                                 std::cout << "Session ID: " << s2.session_id << "\n";
//                                 std::cout << "Username: " << s2.user.username << "\n";
//                                 std::cout << "Role: NORMAL\n";
//                             } else {
//                                 std::cout << "get_session_info rc = " << irc << " (" << get_error_message(irc) << ")\n";
//                             }
//                         } else if (c == 2) {
//                             int r = user_logout(current_session);
//                             std::cout << "user_logout rc = " << r << " (" << get_error_message(r) << ")\n";
//                             current_session = 0;
//                         } else if (c == 3) {
//                             fs_shutdown(instance);
//                             instance = 0;
//                             return 0;
//                         }
//                     }
//                 }
//             }

//             fs_shutdown(instance);
//             instance = 0;
//         }
//     }
// }
// main.cpp
#include "config_parser.cpp"

#include <iostream>
#include <fstream>
#include <cstring>

static void write_sample_config(const char* path) {
    std::ofstream out(path);
    out <<
        "[filesystem]\n"
        "total_size = 104857600\n"
        "header_size = 512\n"
        "block_size = 4096\n"
        "max_files = 1000\n"
        "max_filename_length = 10\n"
        "\n"
        "[security]\n"
        "max_users = 50\n"
        "admin_username = \"admin\"\n"
        "admin_password = \"admin123\"\n"
        "require_auth = true\n"
        "key = \"my_super_secret_key_1234567890\"\n"
        "\n"
        "[server]\n"
        "port = 8080\n"
        "max_connections = 20\n"
        "queue_timeout = 30\n";
}

int main() {
    const char* cfg_path = "sample.uconf";
    write_sample_config(cfg_path);

    FSConfig cfg;
    std::memset(&cfg, 0, sizeof(cfg));  // same pattern as your ofs_api.cpp

    bool ok = parse_uconf(cfg_path, cfg);
    if (!ok) {
        std::cerr << "parse_uconf FAILED\n";
        return 1;
    }

    // Print values to visually verify
    std::cout << "total_size: " << cfg.total_size << "\n";
    std::cout << "header_size: " << cfg.header_size << "\n";
    std::cout << "block_size: " << cfg.block_size << "\n";
    std::cout << "max_files: " << cfg.max_files << "\n";
    std::cout << "max_filename_length: " << cfg.max_filename_length << "\n";

    std::cout << "max_users: " << cfg.max_users << "\n";
    std::cout << "admin_username: " << cfg.admin_username << "\n";
    std::cout << "admin_password: " << cfg.admin_password << "\n";
    std::cout << "require_auth: " << (int)cfg.require_auth << "\n";

    std::cout << "server_port: " << cfg.server_port << "\n";
    std::cout << "max_connections: " << cfg.max_connections << "\n";
    std::cout << "queue_timeout: " << cfg.queue_timeout << "\n";

    // Simple assertions (manual)
    bool passed = true;

    if (cfg.total_size != 104857600ULL) { passed = false; std::cerr << "total_size mismatch\n"; }
    if (cfg.header_size != 512)         { passed = false; std::cerr << "header_size mismatch\n"; }
    if (cfg.block_size != 4096)         { passed = false; std::cerr << "block_size mismatch\n"; }
    if (cfg.max_files != 1000)          { passed = false; std::cerr << "max_files mismatch\n"; }
    if (cfg.max_filename_length != 10)  { passed = false; std::cerr << "max_filename_length mismatch\n"; }

    if (cfg.max_users != 50)            { passed = false; std::cerr << "max_users mismatch\n"; }
    if (std::strcmp(cfg.admin_username, "admin") != 0) {
        passed = false; std::cerr << "admin_username mismatch\n";
    }
    if (std::strcmp(cfg.admin_password, "admin123") != 0) {
        passed = false; std::cerr << "admin_password mismatch\n";
    }
    if (cfg.require_auth != 1) {
        passed = false; std::cerr << "require_auth mismatch\n";
    }

    if (cfg.server_port != 8080)        { passed = false; std::cerr << "server_port mismatch\n"; }
    if (cfg.max_connections != 20)      { passed = false; std::cerr << "max_connections mismatch\n"; }
    if (cfg.queue_timeout != 30)        { passed = false; std::cerr << "queue_timeout mismatch\n"; }

    if (passed) {
        std::cout << "\nAll config_parser tests PASSED ✅\n";
        return 0;
    } else {
        std::cerr << "\nSome config_parser tests FAILED ❌\n";
        return 1;
    }
}


