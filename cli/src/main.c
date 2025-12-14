/*
 * Nex - Nimble Executor
 * A lightweight package manager for developer tools
 * 
 * Main entry point - parses arguments and dispatches to command handlers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nex.h"

static void print_banner(void) {
    printf("\n");
    printf("  \033[31m███╗   ██╗███████╗██╗  ██╗\033[0m\n");
    printf("  \033[31m████╗  ██║██╔════╝╚██╗██╔╝\033[0m\n");
    printf("  \033[31m██╔██╗ ██║█████╗   ╚███╔╝ \033[0m\n");
    printf("  \033[31m██║╚██╗██║██╔══╝   ██╔██╗ \033[0m\n");
    printf("  \033[31m██║ ╚████║███████╗██╔╝ ██╗\033[0m\n");
    printf("  \033[31m╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝\033[0m\n");
    printf("\n");
    printf("  \033[90m⚡ Nimble Executor v%s\033[0m\n", NEX_VERSION);
    printf("  \033[90m   Package manager for developer tools\033[0m\n");
    printf("\n");
}

/* Get version string from a command */
static int get_version_string(const char *cmd, char *version, size_t size) {
    FILE *fp;
    char full_cmd[256];
    
#ifdef _WIN32
    snprintf(full_cmd, sizeof(full_cmd), "%s 2>nul", cmd);
    fp = _popen(full_cmd, "r");
#else
    snprintf(full_cmd, sizeof(full_cmd), "%s 2>/dev/null", cmd);
    fp = popen(full_cmd, "r");
#endif
    
    if (!fp) return -1;
    
    if (fgets(version, (int)size, fp) == NULL) {
#ifdef _WIN32
        _pclose(fp);
#else
        pclose(fp);
#endif
        return -1;
    }
    
#ifdef _WIN32
    _pclose(fp);
#else
    pclose(fp);
#endif
    
    /* Remove trailing newline */
    size_t len = strlen(version);
    while (len > 0 && (version[len-1] == '\n' || version[len-1] == '\r')) {
        version[--len] = '\0';
    }
    
    return 0;
}

static void print_runtimes(void) {
    char version[128];
    int found_any = 0;
    
    printf("\033[33mInstalled Runtimes:\033[0m\n");
    
    /* Python */
#ifdef _WIN32
    if (get_version_string("python --version", version, sizeof(version)) == 0) {
#else
    if (get_version_string("python3 --version", version, sizeof(version)) == 0 ||
        get_version_string("python --version", version, sizeof(version)) == 0) {
#endif
        printf("  \033[32m✓\033[0m Python     %s\n", version);
        found_any = 1;
    } else {
        printf("  \033[31m✗\033[0m Python     \033[90mnot installed\033[0m\n");
    }
    
    /* Node.js */
    if (get_version_string("node --version", version, sizeof(version)) == 0) {
        printf("  \033[32m✓\033[0m Node.js    %s\n", version);
        found_any = 1;
    } else {
        printf("  \033[31m✗\033[0m Node.js    \033[90mnot installed\033[0m\n");
    }
    
    /* Git */
    if (get_version_string("git --version", version, sizeof(version)) == 0) {
        /* Extract just version number */
        char *ver = strstr(version, "version ");
        if (ver) ver += 8; else ver = version;
        printf("  \033[32m✓\033[0m Git        %s\n", ver);
        found_any = 1;
    } else {
        printf("  \033[31m✗\033[0m Git        \033[90mnot installed\033[0m\n");
    }
    
    printf("\n");
    (void)found_any;  /* Suppress unused warning */
}

static void print_usage(void) {
    print_banner();
    print_runtimes();
    printf("Usage: nex <command> [options] [arguments]\n\n");
    printf("\033[33mPackage Commands:\033[0m\n");
    printf("  install <package>      Install a package from the registry\n");
    printf("  run <package> [cmd]    Run a package command\n");
    printf("  update [package]       Update package(s) to latest version\n");
    printf("  remove <package>       Remove an installed package\n");
    printf("  list                   List installed packages\n");
    printf("  search <query>         Search the registry\n");
    printf("  info <package>         Show package details\n");
    printf("\n\033[33mDeveloper Commands:\033[0m\n");
    printf("  init                   Create a new package\n");
    printf("  publish                Submit package to registry\n");
    printf("\n\033[33mConfiguration:\033[0m\n");
    printf("  config [key] [value]   Manage nex settings\n");
    printf("  alias [name] [pkg]     Manage package shortcuts\n");
    printf("  self-update            Update nex CLI to latest version\n");
    printf("\n\033[33mOptions:\033[0m\n");
    printf("  -v, --version          Show version\n");
    printf("  -h, --help             Show this help message\n");
    printf("\n\033[33mExamples:\033[0m\n");
    printf("  nex install pagepull\n");
    printf("  nex run pagepull --url https://example.com\n");
    printf("  nex alias pp pagepull && nex run pp\n");
    printf("  nex init\n");
    printf("\n");
}

static void print_version(void) {
    printf("nex %s\n", NEX_VERSION);
}

int main(int argc, char *argv[]) {
    int result = 0;
    
    /* No arguments - show usage */
    if (argc < 2) {
        print_usage();
        return 0;
    }
    
    const char *command = argv[1];
    
    /* Check for version flag */
    if (strcmp(command, "-v") == 0 || strcmp(command, "--version") == 0) {
        print_version();
        return 0;
    }
    
    /* Check for help flag */
    if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        print_usage();
        return 0;
    }
    
    /* Initialize HTTP client */
    if (http_init() != 0) {
        print_error("Failed to initialize HTTP client");
        return 1;
    }
    
    /* Ensure config directories exist */
    if (config_ensure_directories() != 0) {
        print_error("Failed to create configuration directories");
        http_cleanup();
        return 1;
    }
    
    /* Dispatch to command handlers */
    if (strcmp(command, "install") == 0) {
        result = cmd_install(argc - 2, argv + 2);
    }
    else if (strcmp(command, "run") == 0) {
        result = cmd_run(argc - 2, argv + 2);
    }
    else if (strcmp(command, "update") == 0) {
        result = cmd_update(argc - 2, argv + 2);
    }
    else if (strcmp(command, "remove") == 0) {
        result = cmd_remove(argc - 2, argv + 2);
    }
    else if (strcmp(command, "list") == 0) {
        result = cmd_list(argc - 2, argv + 2);
    }
    else if (strcmp(command, "search") == 0) {
        result = cmd_search(argc - 2, argv + 2);
    }
    else if (strcmp(command, "info") == 0) {
        result = cmd_info(argc - 2, argv + 2);
    }
    else if (strcmp(command, "init") == 0) {
        result = cmd_init(argc - 2, argv + 2);
    }
    else if (strcmp(command, "config") == 0) {
        result = cmd_config(argc - 2, argv + 2);
    }
    else if (strcmp(command, "alias") == 0) {
        result = cmd_alias(argc - 2, argv + 2);
    }
    else if (strcmp(command, "publish") == 0) {
        result = cmd_publish(argc - 2, argv + 2);
    }
    else if (strcmp(command, "self-update") == 0) {
        result = cmd_self_update(argc - 2, argv + 2);
    }
    else {
        print_error("Unknown command: %s", command);
        printf("\nRun 'nex --help' for usage information.\n");
        result = 1;
    }
    
    /* Cleanup */
    http_cleanup();
    
    return result;
}
