/*
 * Self-Update command - Check for and install nex CLI updates
 */

#include "nex.h"
#include <string.h>
#include <errno.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

/* GitHub API URL for latest release */
#define GITHUB_RELEASES_API "https://api.github.com/repos/devkiraa/nex/releases/latest"

/* Platform-specific asset names */
#ifdef _WIN32
#define ASSET_NAME "nex-windows-x64.exe"
#elif __APPLE__
#define ASSET_NAME "nex-macos-x64"
#else
#define ASSET_NAME "nex-linux-x64"
#endif

/* Simple JSON string extraction - finds "key": "value" pattern */
static int extract_json_string(const char *json, const char *key, char *value, size_t value_size) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_pattern);
    if (!key_pos) {
        return -1;
    }
    
    /* Find the colon after the key */
    const char *colon = strchr(key_pos + strlen(search_pattern), ':');
    if (!colon) {
        return -1;
    }
    
    /* Skip whitespace and find opening quote */
    const char *start = colon + 1;
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    
    if (*start != '"') {
        return -1;
    }
    start++;  /* Skip opening quote */
    
    /* Find closing quote */
    const char *end = strchr(start, '"');
    if (!end) {
        return -1;
    }
    
    size_t len = end - start;
    if (len >= value_size) {
        len = value_size - 1;
    }
    
    strncpy(value, start, len);
    value[len] = '\0';
    
    return 0;
}

/* Compare version strings (e.g., "1.0.0" vs "1.1.0") */
static int compare_versions(const char *v1, const char *v2) {
    int major1, minor1, patch1;
    int major2, minor2, patch2;
    
    if (sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1) != 3) {
        major1 = minor1 = patch1 = 0;
    }
    if (sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2) != 3) {
        major2 = minor2 = patch2 = 0;
    }
    
    if (major1 != major2) return major1 - major2;
    if (minor1 != minor2) return minor1 - minor2;
    return patch1 - patch2;
}

/* Find the download URL for the platform-specific asset */
static int find_asset_url(const char *json, char *url, size_t url_size) {
    /* Find the asset with our platform name */
    const char *asset_pos = strstr(json, ASSET_NAME);
    if (!asset_pos) {
        return -1;
    }
    
    /* Search backwards for "browser_download_url" */
    const char *search_start = json;
    
    /* Search for "browser_download_url" near the asset match */
    (void)search_start; /* Mark as unused */
    
    /* Now search forward from asset position for the URL */
    const char *url_key = strstr(asset_pos - 200 > json ? asset_pos - 200 : json, "\"browser_download_url\"");
    if (!url_key) {
        return -1;
    }
    
    /* Parse the URL value */
    const char *colon = strchr(url_key + 22, ':');
    if (!colon) {
        return -1;
    }
    
    const char *start = colon + 1;
    while (*start == ' ' || *start == '\t' || *start == '"') {
        start++;
    }
    
    const char *end = strchr(start, '"');
    if (!end) {
        return -1;
    }
    
    size_t len = end - start;
    if (len >= url_size) {
        len = url_size - 1;
    }
    
    strncpy(url, start, len);
    url[len] = '\0';
    
    return 0;
}

/* Get the path to the current executable */
static int get_executable_path(char *path, size_t size) {
#ifdef _WIN32
    DWORD len = GetModuleFileNameA(NULL, path, (DWORD)size);
    return (len > 0 && len < size) ? 0 : -1;
#elif __APPLE__
    uint32_t bufsize = (uint32_t)size;
    return _NSGetExecutablePath(path, &bufsize) == 0 ? 0 : -1;
#else
    ssize_t len = readlink("/proc/self/exe", path, size - 1);
    if (len > 0) {
        path[len] = '\0';
        return 0;
    }
    return -1;
#endif
}

/* Download file to a path */
static int download_to_file(const char *url, const char *filepath) {
    HttpResponse *response = http_get(url);
    if (!response) {
        return -1;
    }
    
    if (response->status_code != 200) {
        print_error("Download failed with status: %ld", response->status_code);
        http_response_free(response);
        return -1;
    }
    
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
#ifndef _WIN32
        if (errno == EACCES || errno == EPERM) {
            print_error("Permission denied. Try running with sudo:");
            printf("  sudo nex self-update\n");
        } else {
            print_error("Failed to create file: %s", filepath);
        }
#else
        print_error("Failed to create file: %s", filepath);
        printf("Try running as Administrator.\n");
#endif
        http_response_free(response);
        return -1;
    }
    
    size_t expected_size = response->size;
    size_t written = fwrite(response->data, 1, response->size, fp);
    fclose(fp);
    http_response_free(response);
    
    if (written != expected_size) {
        print_error("Failed to write complete file");
        return -1;
    }
    
    return 0;
}

int nex_check_for_updates(int *update_available, char *latest_version, size_t version_size) {
    *update_available = 0;
    
    HttpResponse *response = http_get(GITHUB_RELEASES_API);
    if (!response) {
        return -1;
    }
    
    if (response->status_code != 200) {
        http_response_free(response);
        return -1;
    }
    
    /* Extract tag_name (version) from response */
    char tag[MAX_VERSION_LEN];
    if (extract_json_string(response->data, "tag_name", tag, sizeof(tag)) != 0) {
        http_response_free(response);
        return -1;
    }
    
    http_response_free(response);
    
    /* Remove 'v' prefix if present */
    const char *version = tag;
    if (tag[0] == 'v' || tag[0] == 'V') {
        version = tag + 1;
    }
    
    strncpy(latest_version, version, version_size - 1);
    latest_version[version_size - 1] = '\0';
    
    /* Compare with current version */
    if (compare_versions(version, NEX_VERSION) > 0) {
        *update_available = 1;
    }
    
    return 0;
}

int nex_self_update(void) {
    print_info("Checking for nex updates...");
    
    /* Fetch latest release info */
    HttpResponse *response = http_get(GITHUB_RELEASES_API);
    if (!response) {
        print_error("Failed to check for updates");
        return -1;
    }
    
    if (response->status_code != 200) {
        print_error("Failed to fetch release info (status: %ld)", response->status_code);
        http_response_free(response);
        return -1;
    }
    
    /* Extract version */
    char tag[MAX_VERSION_LEN];
    if (extract_json_string(response->data, "tag_name", tag, sizeof(tag)) != 0) {
        print_error("Failed to parse release info");
        http_response_free(response);
        return -1;
    }
    
    /* Remove 'v' prefix if present */
    const char *latest_version = tag;
    if (tag[0] == 'v' || tag[0] == 'V') {
        latest_version = tag + 1;
    }
    
    printf("Current version: %s\n", NEX_VERSION);
    printf("Latest version:  %s\n", latest_version);
    
    /* Compare versions */
    if (compare_versions(latest_version, NEX_VERSION) <= 0) {
        print_success("nex is already up to date!");
        http_response_free(response);
        return 0;
    }
    
    print_info("Update available! Downloading %s...", tag);
    
    /* Find download URL for our platform */
    char download_url[MAX_URL_LEN];
    if (find_asset_url(response->data, download_url, sizeof(download_url)) != 0) {
        print_error("No compatible binary found for this platform");
        http_response_free(response);
        return -1;
    }
    
    http_response_free(response);
    
    /* Get path to current executable */
    char exe_path[MAX_PATH_LEN];
    if (get_executable_path(exe_path, sizeof(exe_path)) != 0) {
        print_error("Failed to determine executable path");
        return -1;
    }
    
    /* Create temp path for new executable */
    char temp_path[MAX_PATH_LEN];
    snprintf(temp_path, sizeof(temp_path), "%s.new", exe_path);
    
    /* Download new version */
    print_info("Downloading from: %s", download_url);
    if (download_to_file(download_url, temp_path) != 0) {
        print_error("Failed to download update");
        return -1;
    }
    
#ifdef _WIN32
    /* On Windows, we need to rename the running executable first */
    char backup_path[MAX_PATH_LEN];
    snprintf(backup_path, sizeof(backup_path), "%s.old", exe_path);
    
    /* Remove old backup if exists */
    remove(backup_path);
    
    /* Rename current to backup */
    if (rename(exe_path, backup_path) != 0) {
        print_error("Failed to backup current executable");
        remove(temp_path);
        return -1;
    }
    
    /* Rename new to current */
    if (rename(temp_path, exe_path) != 0) {
        print_error("Failed to install new version");
        /* Try to restore backup */
        rename(backup_path, exe_path);
        return -1;
    }
    
    /* Schedule deletion of backup on next boot or leave it */
    print_success("Successfully updated nex to version %s!", latest_version);
    print_info("Old version saved as: %s", backup_path);
    
#else
    /* On Unix, we can replace the executable directly */
    /* Set executable permission */
    chmod(temp_path, 0755);
    
    /* Replace the executable */
    if (rename(temp_path, exe_path) != 0) {
        print_error("Failed to install new version (try running with sudo)");
        remove(temp_path);
        return -1;
    }
    
    print_success("Successfully updated nex to version %s!", latest_version);
#endif
    
    printf("\nRun 'nex --version' to verify the update.\n");
    
    return 0;
}

int cmd_self_update(int argc, char *argv[]) {
    (void)argc;  /* Unused */
    (void)argv;  /* Unused */
    
    return nex_self_update();
}
