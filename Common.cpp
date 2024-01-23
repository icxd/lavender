#include "Common.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

[[noreturn]] void panic(const char *file, usz line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "PANIC: %s:%lu: ", file, line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}
