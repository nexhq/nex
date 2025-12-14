/*
 * Package Manager - Handles package operations
 */

#include "nex.h"
#include "cJSON.h"

/* Windows compatibility */
#ifdef _WIN32
#define strcasecmp _stricmp
#endif

/* Build manifest URL from package ID */
static int build_manifest_url(const char *package_id, char *url, size_t url_size) {
    /* Package ID format: author.package-name */
    char author[MAX_NAME_LEN] = {0};
    char name[MAX_NAME_LEN] = {0};
    
    const char *dot = strchr(package_id, '.');
    if (!dot) {
        print_error("Invalid package ID format. Expected: author.package-name");
        return -1;
    }
    
    size_t author_len = dot - package_id;
    if (author_len >= MAX_NAME_LEN) author_len = MAX_NAME_LEN - 1;
    strncpy(author, package_id, author_len);
    author[author_len] = '\0';
    
    strncpy(name, dot + 1, MAX_NAME_LEN - 1);
    name[MAX_NAME_LEN - 1] = '\0';
    
    /* First letter for directory */
    char first_letter = author[0];
    if (first_letter >= 'A' && first_letter <= 'Z') {
        first_letter = first_letter - 'A' + 'a';
    }
    
    snprintf(url, url_size, "%s/packages/%c/%s/%s/manifest.json",
        REGISTRY_BASE_URL, first_letter, author, name);
    
    return 0;
}

/* Resolve short name or full ID to full package ID */
int package_resolve_name(const char *name_or_id, char *resolved_id, size_t resolved_size) {
    /* If it already contains a dot, assume it's a full ID */
    if (strchr(name_or_id, '.') != NULL) {
        strncpy(resolved_id, name_or_id, resolved_size - 1);
        resolved_id[resolved_size - 1] = '\0';
        return 0;
    }
    
    /* Fetch registry index to find the package */
    HttpResponse *response = http_get(REGISTRY_INDEX_URL);
    if (!response) {
        print_error("Failed to fetch registry");
        return -1;
    }
    
    if (response->status_code != 200) {
        print_error("Failed to fetch registry (HTTP %ld)", response->status_code);
        http_response_free(response);
        return -1;
    }
    
    /* Parse JSON and search for matching shortName or name */
    cJSON *json = cJSON_Parse(response->data);
    http_response_free(response);
    
    if (!json) {
        print_error("Failed to parse registry");
        return -1;
    }
    
    cJSON *packages = cJSON_GetObjectItemCaseSensitive(json, "packages");
    if (!cJSON_IsArray(packages)) {
        cJSON_Delete(json);
        print_error("Invalid registry format");
        return -1;
    }
    
    char found_id[MAX_NAME_LEN] = {0};
    int match_count = 0;
    
    cJSON *pkg;
    cJSON_ArrayForEach(pkg, packages) {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(pkg, "id");
        cJSON *shortName = cJSON_GetObjectItemCaseSensitive(pkg, "shortName");
        
        /* Check shortName first (preferred) */
        if (cJSON_IsString(shortName) && strcasecmp(shortName->valuestring, name_or_id) == 0) {
            if (cJSON_IsString(id)) {
                strncpy(found_id, id->valuestring, MAX_NAME_LEN - 1);
                match_count++;
            }
        }
        /* Also check the package name part of the ID (after the dot) */
        else if (cJSON_IsString(id)) {
            const char *dot = strchr(id->valuestring, '.');
            if (dot && strcasecmp(dot + 1, name_or_id) == 0) {
                strncpy(found_id, id->valuestring, MAX_NAME_LEN - 1);
                match_count++;
            }
        }
    }
    
    cJSON_Delete(json);
    
    if (match_count == 0) {
        print_error("Package '%s' not found in registry", name_or_id);
        return -1;
    }
    
    if (match_count > 1) {
        print_error("Multiple packages match '%s'. Use full ID (author.package-name)", name_or_id);
        return -1;
    }
    
    strncpy(resolved_id, found_id, resolved_size - 1);
    resolved_id[resolved_size - 1] = '\0';
    
    return 0;
}

int package_parse_manifest(const char *json_str, PackageInfo *info) {
    if (!json_str || !info) return -1;
    
    memset(info, 0, sizeof(PackageInfo));
    
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        print_error("Failed to parse manifest JSON");
        return -1;
    }
    
    /* Required fields */
    cJSON *id = cJSON_GetObjectItemCaseSensitive(json, "id");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
    cJSON *description = cJSON_GetObjectItemCaseSensitive(json, "description");
    cJSON *repository = cJSON_GetObjectItemCaseSensitive(json, "repository");
    cJSON *entrypoint = cJSON_GetObjectItemCaseSensitive(json, "entrypoint");
    
    if (!cJSON_IsString(id) || !cJSON_IsString(version) || !cJSON_IsString(repository)) {
        print_error("Manifest missing required fields");
        cJSON_Delete(json);
        return -1;
    }
    
    strncpy(info->id, id->valuestring, MAX_NAME_LEN - 1);
    strncpy(info->version, version->valuestring, MAX_VERSION_LEN - 1);
    strncpy(info->repository, repository->valuestring, MAX_URL_LEN - 1);
    
    if (cJSON_IsString(name)) {
        strncpy(info->name, name->valuestring, MAX_NAME_LEN - 1);
    } else {
        strncpy(info->name, info->id, MAX_NAME_LEN - 1);
    }
    
    if (cJSON_IsString(description)) {
        strncpy(info->description, description->valuestring, MAX_DESCRIPTION_LEN - 1);
    }
    
    if (cJSON_IsString(entrypoint)) {
        strncpy(info->entrypoint, entrypoint->valuestring, MAX_PATH_LEN - 1);
    }
    
    /* Author */
    cJSON *author = cJSON_GetObjectItemCaseSensitive(json, "author");
    if (cJSON_IsObject(author)) {
        cJSON *author_name = cJSON_GetObjectItemCaseSensitive(author, "name");
        if (cJSON_IsString(author_name)) {
            strncpy(info->author, author_name->valuestring, MAX_NAME_LEN - 1);
        }
    } else if (cJSON_IsString(author)) {
        strncpy(info->author, author->valuestring, MAX_NAME_LEN - 1);
    }
    
    /* Runtime */
    cJSON *runtime = cJSON_GetObjectItemCaseSensitive(json, "runtime");
    if (cJSON_IsObject(runtime)) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(runtime, "type");
        cJSON *ver = cJSON_GetObjectItemCaseSensitive(runtime, "version");
        
        if (cJSON_IsString(type)) {
            info->runtime = runtime_from_string(type->valuestring);
        }
        if (cJSON_IsString(ver)) {
            strncpy(info->runtime_version, ver->valuestring, MAX_VERSION_LEN - 1);
        }
    }
    
    /* Commands */
    cJSON *commands = cJSON_GetObjectItemCaseSensitive(json, "commands");
    if (cJSON_IsObject(commands)) {
        cJSON *cmd;
        cJSON_ArrayForEach(cmd, commands) {
            if (info->command_count < MAX_COMMANDS && cJSON_IsString(cmd)) {
                strncpy(info->commands[info->command_count].name, cmd->string, MAX_NAME_LEN - 1);
                strncpy(info->commands[info->command_count].command, cmd->valuestring, MAX_COMMAND_LEN - 1);
                info->command_count++;
            }
        }
    }
    
    /* Keywords */
    cJSON *keywords = cJSON_GetObjectItemCaseSensitive(json, "keywords");
    if (cJSON_IsArray(keywords)) {
        cJSON *kw;
        cJSON_ArrayForEach(kw, keywords) {
            if (info->keyword_count < MAX_KEYWORDS && cJSON_IsString(kw)) {
                strncpy(info->keywords[info->keyword_count], kw->valuestring, MAX_NAME_LEN - 1);
                info->keyword_count++;
            }
        }
    }
    
    cJSON_Delete(json);
    return 0;
}

int package_fetch_manifest(const char *package_id, PackageInfo *info) {
    char url[MAX_URL_LEN];
    
    if (build_manifest_url(package_id, url, sizeof(url)) != 0) {
        return -1;
    }
    
    HttpResponse *response = http_get(url);
    if (!response) {
        return -1;
    }
    
    if (response->status_code != 200) {
        print_error("Package not found (HTTP %ld)", response->status_code);
        http_response_free(response);
        return -1;
    }
    
    int result = package_parse_manifest(response->data, info);
    http_response_free(response);
    
    return result;
}

/* Fetch manifest and return raw JSON (caller must free) */
static char* package_fetch_manifest_raw(const char *package_id) {
    char url[MAX_URL_LEN];
    
    if (build_manifest_url(package_id, url, sizeof(url)) != 0) {
        return NULL;
    }
    
    HttpResponse *response = http_get(url);
    if (!response) {
        return NULL;
    }
    
    if (response->status_code != 200) {
        http_response_free(response);
        return NULL;
    }
    
    char *json = malloc(response->size + 1);
    if (json) {
        memcpy(json, response->data, response->size);
        json[response->size] = '\0';
    }
    
    http_response_free(response);
    return json;
}

int package_install(const char *package_id) {
    PackageInfo info;
    
    /* Fetch manifest and keep raw JSON */
    char *manifest_json = package_fetch_manifest_raw(package_id);
    if (!manifest_json) {
        print_error("Failed to fetch package manifest");
        return -1;
    }
    
    if (package_parse_manifest(manifest_json, &info) != 0) {
        free(manifest_json);
        return -1;
    }
    
    /* Get install directory */
    char packages_dir[MAX_PATH_LEN];
    if (config_get_packages_dir(packages_dir, sizeof(packages_dir)) != 0) {
        free(manifest_json);
        return -1;
    }
    
    char install_path[MAX_PATH_LEN];
    snprintf(install_path, sizeof(install_path), "%s%c%s", 
        packages_dir, PATH_SEPARATOR, package_id);
    
    /* Clone repository */
    char cmd[MAX_COMMAND_LEN];
    snprintf(cmd, sizeof(cmd), "git clone --depth 1 \"%s\" \"%s\"",
        info.repository, install_path);
    
    print_info("Cloning from %s", info.repository);
    
    if (run_command(cmd) != 0) {
        print_error("Failed to clone repository");
        free(manifest_json);
        return -1;
    }
    
    /* Save manifest.json to install directory */
    char manifest_path[MAX_PATH_LEN];
    snprintf(manifest_path, sizeof(manifest_path), "%s%cmanifest.json",
        install_path, PATH_SEPARATOR);
    
    FILE *mf = fopen(manifest_path, "w");
    if (mf) {
        fputs(manifest_json, mf);
        fclose(mf);
    }
    free(manifest_json);
    
    /* Run install command if specified */
    for (int i = 0; i < info.command_count; i++) {
        if (strcmp(info.commands[i].name, "install") == 0) {
            print_info("Running install command...");
            
            char install_cmd[MAX_COMMAND_LEN];
            snprintf(install_cmd, sizeof(install_cmd), "cd \"%s\" && %s",
                install_path, info.commands[i].command);
            
            if (run_command(install_cmd) != 0) {
                print_error("Install command failed");
                /* Don't fail - package is still installed */
            }
            break;
        }
    }
    
    /* Save local package info */
    LocalPackage local;
    strncpy(local.id, package_id, MAX_NAME_LEN - 1);
    strncpy(local.version, info.version, MAX_VERSION_LEN - 1);
    strncpy(local.install_path, install_path, MAX_PATH_LEN - 1);
    local.is_installed = 1;
    
    config_save_local_package(&local);
    
    return 0;
}

int package_remove(const char *package_id) {
    LocalPackage local;
    
    if (!package_is_installed(package_id, &local)) {
        print_error("Package not installed");
        return -1;
    }
    
    /* Remove directory */
    char cmd[MAX_COMMAND_LEN];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\"", local.install_path);
#else
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", local.install_path);
#endif
    
    if (run_command(cmd) != 0) {
        print_error("Failed to remove package directory");
        return -1;
    }
    
    /* Remove from config */
    config_remove_local_package(package_id);
    
    return 0;
}

int package_is_installed(const char *package_id, LocalPackage *local) {
    char packages_dir[MAX_PATH_LEN];
    if (config_get_packages_dir(packages_dir, sizeof(packages_dir)) != 0) {
        return 0;
    }
    
    char install_path[MAX_PATH_LEN];
    snprintf(install_path, sizeof(install_path), "%s%c%s",
        packages_dir, PATH_SEPARATOR, package_id);
    
    /* Check if directory exists */
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(install_path);
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return 0;
    }
#else
    struct stat st;
    if (stat(install_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return 0;
    }
#endif
    
    if (local) {
        strncpy(local->id, package_id, MAX_NAME_LEN - 1);
        strncpy(local->install_path, install_path, MAX_PATH_LEN - 1);
        strcpy(local->version, "installed");  /* TODO: Read from local manifest */
        local->is_installed = 1;
    }
    
    return 1;
}

int package_execute(const char *package_id, const char *command, int argc, char *argv[]) {
    LocalPackage local;
    
    if (!package_is_installed(package_id, &local)) {
        print_error("Package not installed");
        return -1;
    }
    
    /* Read local manifest */
    char manifest_path[MAX_PATH_LEN];
    snprintf(manifest_path, sizeof(manifest_path), "%s%cmanifest.json",
        local.install_path, PATH_SEPARATOR);
    
    FILE *f = fopen(manifest_path, "r");
    if (!f) {
        /* Try nex.json as alternative */
        snprintf(manifest_path, sizeof(manifest_path), "%s%cnex.json",
            local.install_path, PATH_SEPARATOR);
        f = fopen(manifest_path, "r");
    }
    
    PackageInfo info;
    memset(&info, 0, sizeof(info));
    
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        char *json = malloc(size + 1);
        if (json) {
            fread(json, 1, size, f);
            json[size] = '\0';
            package_parse_manifest(json, &info);
            free(json);
        }
        fclose(f);
    }
    
    /* Check if required runtime is available */
    if (info.runtime != RUNTIME_UNKNOWN && info.runtime != RUNTIME_BINARY) {
        if (runtime_ensure_available(info.runtime) != 0) {
            print_error("Cannot run package without required runtime");
            return -1;
        }
    }
    
    /* Find command to execute */
    char exec_cmd[MAX_COMMAND_LEN] = {0};
    
    for (int i = 0; i < info.command_count; i++) {
        if (strcmp(info.commands[i].name, command) == 0) {
            strncpy(exec_cmd, info.commands[i].command, MAX_COMMAND_LEN - 1);
            break;
        }
    }
    
    /* Fallback to default entrypoint */
    if (strlen(exec_cmd) == 0 && strlen(info.entrypoint) > 0) {
        switch (info.runtime) {
            case RUNTIME_PYTHON:
                snprintf(exec_cmd, sizeof(exec_cmd), "python \"%s\"", info.entrypoint);
                break;
            case RUNTIME_NODE:
                snprintf(exec_cmd, sizeof(exec_cmd), "node \"%s\"", info.entrypoint);
                break;
            case RUNTIME_POWERSHELL:
                snprintf(exec_cmd, sizeof(exec_cmd), "powershell -File \"%s\"", info.entrypoint);
                break;
            case RUNTIME_BASH:
                snprintf(exec_cmd, sizeof(exec_cmd), "bash \"%s\"", info.entrypoint);
                break;
            default:
                snprintf(exec_cmd, sizeof(exec_cmd), "\"%s\"", info.entrypoint);
                break;
        }
    }
    
    if (strlen(exec_cmd) == 0) {
        print_error("No command '%s' found for package", command);
        return -1;
    }
    
    /* Normalize python command - use python3 if python doesn't exist */
#ifndef _WIN32
    if (info.runtime == RUNTIME_PYTHON) {
        /* Check if 'python' exists, if not use 'python3' */
        if (system("which python >/dev/null 2>&1") != 0 && 
            system("which python3 >/dev/null 2>&1") == 0) {
            /* Replace "python " with "python3 " in the command */
            char temp_cmd[MAX_COMMAND_LEN];
            char *pos = strstr(exec_cmd, "python ");
            if (pos == exec_cmd || (pos && (*(pos-1) == ' ' || *(pos-1) == '&'))) {
                size_t prefix_len = pos - exec_cmd;
                strncpy(temp_cmd, exec_cmd, prefix_len);
                temp_cmd[prefix_len] = '\0';
                strcat(temp_cmd, "python3 ");
                strcat(temp_cmd, pos + 7);  /* Skip "python " */
                strncpy(exec_cmd, temp_cmd, MAX_COMMAND_LEN - 1);
            }
        }
    }
#endif
    
    /* Build full command with args */
    char full_cmd[MAX_COMMAND_LEN * 2];
    snprintf(full_cmd, sizeof(full_cmd), "cd \"%s\" && %s", local.install_path, exec_cmd);
    
    /* Append user arguments */
    for (int i = 0; i < argc; i++) {
        strcat(full_cmd, " ");
        /* Quote arguments with spaces */
        if (strchr(argv[i], ' ')) {
            strcat(full_cmd, "\"");
            strcat(full_cmd, argv[i]);
            strcat(full_cmd, "\"");
        } else {
            strcat(full_cmd, argv[i]);
        }
    }
    
    return run_command(full_cmd);
}
