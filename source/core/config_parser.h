// config_parser.h
#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <cstdint>
#include "../include/ofs_internal.h"   

// Plain-old-data config struct – safe to memset to 0.

// Parse a .uconf file into FSConfig.
// Returns true on success, false on any error (file missing, parse error, invalid values).
bool parse_uconf(const char* path, FSConfig& cfg);

#endif // CONFIG_PARSER_H
