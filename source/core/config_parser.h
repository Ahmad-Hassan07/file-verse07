// config_parser.h
#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <cstdint>

// Plain-old-data config struct – safe to memset to 0.
struct FSConfig {
    // [filesystem]
    uint64_t total_size;          // bytes
    uint32_t header_size;         // bytes
    uint32_t block_size;          // bytes
    uint32_t max_files;           // max number of files (or inodes)
    uint32_t max_filename_length; // <= 10 recommended

    // [security]
    uint32_t max_users;
    char admin_username[32];      // null-terminated string
    char admin_password[32];      // null-terminated string
    uint8_t require_auth;         // 0 = false, 1 = true
    char private_key[64];         // 64 raw bytes; may or may not be null-terminated

    // [server]
    uint16_t server_port;         // e.g., 8080
    uint32_t max_connections;     // max simultaneous connections
    uint32_t queue_timeout;       // seconds
};

// Parse a .uconf file into FSConfig.
// Returns true on success, false on any error (file missing, parse error, invalid values).
bool parse_uconf(const char* path, FSConfig& cfg);

#endif // CONFIG_PARSER_H
