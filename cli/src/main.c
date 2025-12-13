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

static void print_usage(void) {
    print_banner();
    printf("Usage: nex <command> [options] [arguments]\n\n");
    printf("\033[33mCommands:\033[0m\n");
    printf("  install <package>      Install a package from the registry\n");
    printf("  run <package> [cmd]    Run a package command\n");
    printf("  update [package]       Update package(s) to latest version\n");
    printf("  remove <package>       Remove an installed package\n");
    printf("  list                   List installed packages\n");
    printf("  search <query>         Search the registry\n");
    printf("  info <package>         Show package details\n");
    printf("  self-update            Update nex CLI to latest version\n");
    printf("\n\033[33mOptions:\033[0m\n");
    printf("  -v, --version          Show version\n");
    printf("  -h, --help             Show this help message\n");
    printf("\n\033[33mExamples:\033[0m\n");
    printf("  nex install john.image-converter\n");
    printf("  nex run john.image-converter convert --input file.png\n");
    printf("  nex search \"python utility\"\n");
    printf("  nex self-update\n");
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
