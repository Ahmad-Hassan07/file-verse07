// config_parser.cpp
#include "config_parser.h"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>

// Helper: trim whitespace from both ends of a std::string
static inline void trim(std::string& s) {
    auto not_space = [](int ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
}

// Helper: strip optional surrounding quotes
static inline std::string strip_quotes(const std::string& s) {
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                          (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Helper: case-insensitive compare
static inline bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool parse_uconf(const char* path, FSConfig& cfg) {
    if (!path) return false;

    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    std::string current_section;

    while (std::getline(in, line)) {
        // Remove comments starting with '#' or ';'
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line.erase(comment_pos);
        }
        comment_pos = line.find(';');
        if (comment_pos != std::string::npos) {
            line.erase(comment_pos);
        }

        trim(line);
        if (line.empty()) continue;

        // Section header: [filesystem], [security], [server]
        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            trim(current_section);
            continue;
        }

        // Key = value
        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            // malformed line
            continue;
        }

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        trim(key);
        trim(value);

        // Now parse based on section + key
        if (iequals(current_section, "filesystem")) {
            if (iequals(key, "total_size")) {
                cfg.total_size = static_cast<uint64_t>(std::stoull(value));
            } else if (iequals(key, "header_size")) {
                cfg.header_size = static_cast<uint32_t>(std::stoul(value));
            } else if (iequals(key, "block_size")) {
                cfg.block_size = static_cast<uint32_t>(std::stoul(value));
            } else if (iequals(key, "max_files")) {
                cfg.max_files = static_cast<uint32_t>(std::stoul(value));
            } else if (iequals(key, "max_filename_length")) {
                cfg.max_filename_length = static_cast<uint32_t>(std::stoul(value));
            }
        } else if (iequals(current_section, "security")) {
            if (iequals(key, "max_users")) {
                cfg.max_users = static_cast<uint32_t>(std::stoul(value));
            } else if (iequals(key, "admin_username")) {
                std::string v = strip_quotes(value);
                std::memset(cfg.admin_username, 0, sizeof(cfg.admin_username));
                std::strncpy(cfg.admin_username, v.c_str(), sizeof(cfg.admin_username) - 1);
            } else if (iequals(key, "admin_password")) {
                std::string v = strip_quotes(value);
                std::memset(cfg.admin_password, 0, sizeof(cfg.admin_password));
                std::strncpy(cfg.admin_password, v.c_str(), sizeof(cfg.admin_password) - 1);
            } else if (iequals(key, "require_auth")) {
                std::string v = value;
                std::transform(v.begin(), v.end(), v.begin(),
                               [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                if (v == "true" || v == "1") {
                    cfg.require_auth = 1;
                } else {
                    cfg.require_auth = 0;
                }
            } else if (iequals(key, "key") || iequals(key, "private_key")) {
                std::string v = strip_quotes(value);
                std::memset(cfg.private_key, 0, sizeof(cfg.private_key));
                // Copy up to 64 bytes of key data; we don't guarantee null-termination.
                std::memcpy(cfg.private_key, v.data(), std::min(v.size(), sizeof(cfg.private_key)));
            }
        } else if (iequals(current_section, "server")) {
            if (iequals(key, "port")) {
                cfg.server_port = static_cast<uint16_t>(std::stoul(value));
            } else if (iequals(key, "max_connections")) {
                cfg.max_connections = static_cast<uint32_t>(std::stoul(value));
            } else if (iequals(key, "queue_timeout")) {
                cfg.queue_timeout = static_cast<uint32_t>(std::stoul(value));
            }
        }
    }

    // Basic sanity checks: we require at least these to be non-zero
    if (cfg.total_size == 0 ||
        cfg.header_size == 0 ||
        cfg.block_size == 0 ||
        cfg.max_files == 0 ||
        cfg.max_users == 0 ||
        cfg.server_port == 0) {
        return false;
    }

    return true;
}
