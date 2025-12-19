/*
 * Utilities - Common helper functions
 */

#include "nex.h"
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

/* ANSI color codes shared definition */
static int colors_enabled = 0;

#ifdef _WIN32
#include <windows.h>
void console_init(void) {
    static int checked = 0;
    if (checked) return;
    checked = 1;

    /* Set proper encoding for box drawing characters */
    SetConsoleOutputCP(65001); // UTF-8
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(hOut, mode)) {
            colors_enabled = 1;
        }
    }
}
#else
void console_init(void) {
    /* Always enable colors on Unix-like systems, or check isatty here if desired */
    colors_enabled = 1; 
}
#endif

#define COLOR_RED     (colors_enabled ? "\033[31m" : "")
#define COLOR_GREEN   (colors_enabled ? "\033[32m" : "")
#define COLOR_YELLOW  (colors_enabled ? "\033[33m" : "")
#define COLOR_BLUE    (colors_enabled ? "\033[34m" : "")
#define COLOR_RESET   (colors_enabled ? "\033[0m" : "")

void print_error(const char *fmt, ...) {
    console_init();
    fprintf(stderr, "%s[ERROR]%s ", COLOR_RED, COLOR_RESET);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    

    fprintf(stderr, "\n");
}

void print_success(const char *fmt, ...) {
    console_init();
    printf("%s[OK]%s ", COLOR_GREEN, COLOR_RESET);
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}

void print_info(const char *fmt, ...) {
    console_init();
    printf("%s[INFO]%s ", COLOR_BLUE, COLOR_RESET);
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}

int make_directory_recursive(const char *path) {
    char tmp[MAX_PATH_LEN];
    char *p = NULL;
    size_t len;
    
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    len = strlen(tmp);
    
    /* Remove trailing separator */
    if (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
        tmp[len - 1] = '\0';
    }
    
    /* Create each directory in the path */
    for (p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
#ifdef _WIN32
            CreateDirectoryA(tmp, NULL);
#else
            mkdir(tmp, 0755);
#endif
            *p = PATH_SEPARATOR;
        }
    }
    
#ifdef _WIN32
    return CreateDirectoryA(tmp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS ? 0 : -1;
#else
    return mkdir(tmp, 0755) == 0 || errno == EEXIST ? 0 : -1;
#endif
}

RuntimeType runtime_from_string(const char *str) {
    if (!str) return RUNTIME_UNKNOWN;
    
    char lower[32];
    size_t len = strlen(str);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    
    for (size_t i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)str[i]);
    }
    lower[len] = '\0';
    
    if (strcmp(lower, "python") == 0) return RUNTIME_PYTHON;
    if (strcmp(lower, "node") == 0) return RUNTIME_NODE;
    if (strcmp(lower, "nodejs") == 0) return RUNTIME_NODE;
    if (strcmp(lower, "bash") == 0) return RUNTIME_BASH;
    if (strcmp(lower, "powershell") == 0) return RUNTIME_POWERSHELL;
    if (strcmp(lower, "binary") == 0) return RUNTIME_BINARY;
    if (strcmp(lower, "go") == 0) return RUNTIME_GO;
    
    return RUNTIME_UNKNOWN;
}

const char* runtime_to_string(RuntimeType runtime) {
    switch (runtime) {
        case RUNTIME_PYTHON: return "Python";
        case RUNTIME_NODE: return "Node.js";
        case RUNTIME_BASH: return "Bash";
        case RUNTIME_POWERSHELL: return "PowerShell";
        case RUNTIME_BINARY: return "Binary";
        case RUNTIME_GO: return "Go";
        default: return "Unknown";
    }
}

int run_command(const char *command) {
    return system(command);
}
