#pragma once

#include <string>
#include <uuid/uuid.h>

inline std::string generate_uuid() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

int parse_log_level(const std::string& level)
{
    if (level == "trace" || level == "debug") return LOG_DEBUG;
    if (level == "info") return LOG_INFO;
    if (level == "warn") return LOG_WARNING;
    if (level == "error") return LOG_ERR;
    return -1;
}