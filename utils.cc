#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

#if _WIN32 || _WIN64
#else
// Select GNU basename() on Linux
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libgen.h>
#endif

#include <limits.h>

#include <string.h>
#include <stdlib.h>

#include "utils.h"
#if _WIN32 || _WIN64
#define PATH_MAX 4096

int basename_r(const char* path, char*  buffer, size_t  bufflen)
{
    const char *endp, *startp;
    int         len, result;

    /* Empty or NULL string gets treated as "." */
    if (path == NULL || *path == '\0') {
        startp  = ".";
        len     = 1;
        goto Exit;
    }

    /* Strip trailing slashes */
    endp = path + strlen(path) - 1;
    while (endp > path && (*endp == '/' || *endp == '\\'))
        endp--;

    /* All slashes becomes "/" */
    if (endp == path && ( *endp == '/' || *endp == '\\')) {
        startp = "/";
        len    = 1;
        goto Exit;
    }

    /* Find the start of the base */
    startp = endp;
    while (startp > path && (*(startp - 1) != '/' && *(startp - 1) != '\\'))
        startp--;

    len = endp - startp +1;

Exit:
    result = len;
    if (buffer == NULL) {
        return result;
    }
    if (len > (int)bufflen-1) {
        len    = (int)bufflen-1;
        result = -1;
        errno  = ERANGE;
    }

    if (len >= 0) {
        memcpy( buffer, startp, len );
        buffer[len] = 0;
    }
    return result;
}

char* BaseName(const char*  path) {
    static char* bname = NULL;
    int ret;

    if (bname == NULL) {
        bname = (char *)malloc(PATH_MAX);
        if (bname == NULL)
            return(NULL);
    }
    ret = basename_r(path, bname, PATH_MAX);
    return (ret < 0) ? NULL : bname;
}
#endif

std::string extract_basename(const std::string& path)
{
    char* const temp_s = strdup(path.c_str());
#if _WIN32 || _WIN64
    std::string result(BaseName(temp_s));
#else
    std::string result(basename(temp_s));
#endif
    free(temp_s);
    return result;
}

void raise_from_system_error_code(const std::string& user_message, int err)
{
    std::ostringstream sts;
    if (user_message.size() > 0) {
        sts << user_message << ' ';
    }

    assert(0 != err);
    throw std::system_error(std::error_code(err, std::system_category()), sts.str().c_str());
}

void raise_from_errno(const std::string& user_message)
{
    raise_from_system_error_code(user_message, errno);
}
