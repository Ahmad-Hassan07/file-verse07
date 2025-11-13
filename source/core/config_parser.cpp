#include "config_parser.h"
#include <fstream>
#include <sstream>
#include <cctype>

static std::string trim(const std::string& s) {
    unsigned long long i = 0;
    unsigned long long j = s.size();
    while (i < j && std::isspace((unsigned char)s[i])) i++;
    while (j > i && std::isspace((unsigned char)s[j - 1])) j--;
    return s.substr(i, j - i);
}

static std::string strip_quotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

static bool to_bool(const std::string& s) {
    std::string v = s;
    for (unsigned long long i = 0; i < v.size(); i++) {
        char c = v[i];
        if (c >= 'A' && c <= 'Z') v[i] = (char)(c - 'A' + 'a');
    }
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

bool parse_uconf(const char* path, FSConfig& out) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::string line;
    std::string section;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        char c0 = line[0];
        if (c0 == '#' || c0 == ';') continue;
        if (c0 == '[') {
            unsigned long long pos = line.find(']');
            if (pos != std::string::npos && pos > 1) {
                section = line.substr(1, pos - 1);
            } else {
                section.clear();
            }
            continue;
        }
        unsigned long long eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        value = strip_quotes(value);
        if (section == "filesystem") {
            if (key == "total_size") {
                out.total_size = std::stoull(value);
            } else if (key == "header_size") {
                out.header_size = std::stoull(value);
            } else if (key == "block_size") {
                out.block_size = std::stoull(value);
            } else if (key == "max_files") {
                unsigned long long v = std::stoull(value);
                out.max_inodes = (uint32_t)v;
            }
        } else if (section == "security") {
            if (key == "max_users") {
                unsigned long long v = std::stoull(value);
                out.max_users = (uint32_t)v;
            } else if (key == "admin_username") {
                for (int i = 0; i < 32; i++) out.admin_username[i] = 0;
                for (unsigned long long i = 0; i < value.size() && i < 31; i++) out.admin_username[i] = value[i];
            } else if (key == "admin_password") {
                for (int i = 0; i < 32; i++) out.admin_password[i] = 0;
                for (unsigned long long i = 0; i < value.size() && i < 31; i++) out.admin_password[i] = value[i];
            } else if (key == "require_auth") {
                out.require_auth = to_bool(value) ? 1u : 0u;
            } else if (key == "private_key") {
                for (int i = 0; i < 64; i++) out.private_key[i] = 0;
                for (unsigned long long i = 0; i < value.size() && i < 63; i++) out.private_key[i] = value[i];
            }
        } else if (section == "server") {
            if (key == "port") {
                unsigned long long v = std::stoull(value);
                out.server_port = (uint32_t)v;
            } else if (key == "max_connections") {
                unsigned long long v = std::stoull(value);
                out.max_connections = (uint32_t)v;
            }
        }
    }
    if (out.total_size == 0 || out.block_size == 0 || out.max_users == 0 || out.max_inodes == 0) return false;
    return true;
}
