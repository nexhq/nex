/*
 * Nex - Nimble Executor
 * A lightweight package manager for developer tools
 * 
 * Main header file containing all type definitions and function declarations
 */

#ifndef NEX_H
#define NEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlobj.h>
#define PATH_SEPARATOR '\\'
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#define PATH_SEPARATOR '/'
#endif

/* Version */
#define NEX_VERSION "1.8.1"
#define NEX_USER_AGENT "nex/1.8.1"

/* Registry configuration */
#define REGISTRY_BASE_URL "https://raw.githubusercontent.com/nexhq/nex/main/registry"
#define REGISTRY_INDEX_URL REGISTRY_BASE_URL "/index.json"

/* Limits */
#define MAX_PATH_LEN 1024
#define MAX_NAME_LEN 128
#define MAX_VERSION_LEN 32
#define MAX_URL_LEN 512
#define MAX_DESCRIPTION_LEN 512
#define MAX_COMMAND_LEN 2048
#define MAX_COMMANDS 16
#define MAX_KEYWORDS 16

/* Runtime types */
typedef enum {
    RUNTIME_UNKNOWN = 0,
    RUNTIME_PYTHON,
    RUNTIME_NODE,
    RUNTIME_BASH,
    RUNTIME_POWERSHELL,
    RUNTIME_BINARY,
    RUNTIME_GO
} RuntimeType;

/* Package command */
typedef struct {
    char name[MAX_NAME_LEN];
    char command[MAX_COMMAND_LEN];
} PackageCommand;

/* Package information from manifest */
typedef struct {
    char id[MAX_NAME_LEN];
    char name[MAX_NAME_LEN];
    char version[MAX_VERSION_LEN];
    char description[MAX_DESCRIPTION_LEN];
    char author[MAX_NAME_LEN];
    char repository[MAX_URL_LEN];
    char entrypoint[MAX_PATH_LEN];
    RuntimeType runtime;
    char runtime_version[MAX_VERSION_LEN];
    PackageCommand commands[MAX_COMMANDS];
    int command_count;
    char keywords[MAX_KEYWORDS][MAX_NAME_LEN];
    int keyword_count;
} PackageInfo;

/* Local package state */
typedef struct {
    char id[MAX_NAME_LEN];
    char version[MAX_VERSION_LEN];
    char install_path[MAX_PATH_LEN];
    int is_installed;
} LocalPackage;

/* HTTP response */
typedef struct {
    char *data;
    size_t size;
    long status_code;
} HttpResponse;

/* ============ Function Declarations ============ */

/* Commands - see commands folder */
int cmd_install(int argc, char *argv[]);
int cmd_run(int argc, char *argv[]);
int cmd_update(int argc, char *argv[]);
int cmd_remove(int argc, char *argv[]);
int cmd_list(int argc, char *argv[]);
int cmd_search(int argc, char *argv[]);
int cmd_info(int argc, char *argv[]);
int cmd_self_update(int argc, char *argv[]);
int cmd_init(int argc, char *argv[]);
int cmd_config(int argc, char *argv[]);
int cmd_alias(int argc, char *argv[]);
int cmd_publish(int argc, char *argv[]);
int cmd_doctor(int argc, char *argv[]);
int cmd_link(int argc, char *argv[]);
int cmd_outdated(int argc, char *argv[]);
int cmd_lock(int argc, char *argv[]);
int resolve_alias(const char *name, char *package_id, size_t size);

/* Self-update helpers */
int nex_check_for_updates(int *update_available, char *latest_version, size_t version_size);
int nex_self_update(void);

/* HTTP client (http/client.c) */
int http_init(void);
void http_cleanup(void);
HttpResponse* http_get(const char *url);
void http_response_free(HttpResponse *response);

/* Package management (package/manager.c) */
int package_parse_manifest(const char *json, PackageInfo *info);
int package_fetch_manifest(const char *package_id, PackageInfo *info);
int package_install(const char *package_id);
int package_remove(const char *package_id);
int package_is_installed(const char *package_id, LocalPackage *local);
int package_execute(const char *package_id, const char *command, int argc, char *argv[]);
int package_resolve_name(const char *name_or_id, char *resolved_id, size_t resolved_size);

/* Configuration (config/config.c) */
int config_init(void);
int config_get_home_dir(char *buffer, size_t size);
int config_get_packages_dir(char *buffer, size_t size);
int config_ensure_directories(void);
int config_save_local_package(const LocalPackage *pkg);
int config_remove_local_package(const char *package_id);
int config_list_installed(LocalPackage **packages, int *count);

/* Utilities (utils/utils.c) */
void console_init(void);
void print_error(const char *fmt, ...);
void print_success(const char *fmt, ...);
void print_info(const char *fmt, ...);
int make_directory_recursive(const char *path);
RuntimeType runtime_from_string(const char *str);
const char* runtime_to_string(RuntimeType runtime);
int run_command(const char *command);

/* Runtime management (runtime/runtime.c) */
int runtime_is_installed(RuntimeType runtime);
int runtime_ensure_available(RuntimeType runtime);
int runtime_install(RuntimeType runtime);
int runtime_install_python(void);
int runtime_install_node(void);
int runtime_prompt_install(RuntimeType runtime);
const char* runtime_get_install_instructions(RuntimeType runtime);

#endif /* NEX_H */
