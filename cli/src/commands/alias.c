/*
 * Alias command - Manage package shortcuts
 */

#include "nex.h"
#include "cJSON.h"

static int get_aliases_path(char *path, size_t size) {
    char home[MAX_PATH_LEN];
    if (config_get_home_dir(home, sizeof(home)) != 0) {
        return -1;
    }
    snprintf(path, size, "%s%caliases.json", home, PATH_SEPARATOR);
    return 0;
}

static cJSON* load_aliases(void) {
    char path[MAX_PATH_LEN];
    if (get_aliases_path(path, sizeof(path)) != 0) {
        return cJSON_CreateObject();
    }
    
    FILE *f = fopen(path, "r");
    if (!f) {
        return cJSON_CreateObject();
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *data = malloc(size + 1);
    if (!data) {
        fclose(f);
        return cJSON_CreateObject();
    }
    
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);
    
    cJSON *json = cJSON_Parse(data);
    free(data);
    
    return json ? json : cJSON_CreateObject();
}

static int save_aliases(cJSON *aliases) {
    char path[MAX_PATH_LEN];
    if (get_aliases_path(path, sizeof(path)) != 0) {
        return -1;
    }
    
    char *str = cJSON_Print(aliases);
    if (!str) return -1;
    
    FILE *f = fopen(path, "w");
    if (!f) {
        free(str);
        return -1;
    }
    
    fputs(str, f);
    fclose(f);
    free(str);
    
    return 0;
}

/* Resolve alias to package ID - exported for use by other commands */
int resolve_alias(const char *name, char *package_id, size_t size) {
    cJSON *aliases = load_aliases();
    cJSON *item = cJSON_GetObjectItem(aliases, name);
    
    if (item && cJSON_IsString(item)) {
        strncpy(package_id, item->valuestring, size - 1);
        package_id[size - 1] = '\0';
        cJSON_Delete(aliases);
        return 1;  /* Found alias */
    }
    
    cJSON_Delete(aliases);
    return 0;  /* Not an alias */
}

int cmd_alias(int argc, char *argv[]) {
    if (argc < 1) {
        /* List all aliases */
        printf("\n\033[33m🔗 Package Aliases:\033[0m\n\n");
        
        cJSON *aliases = load_aliases();
        
        cJSON *item;
        int count = 0;
        cJSON_ArrayForEach(item, aliases) {
            if (cJSON_IsString(item)) {
                printf("  \033[1m%-15s\033[0m → %s\n", item->string, item->valuestring);
                count++;
            }
        }
        
        if (count == 0) {
            printf("  \033[90mNo aliases defined.\033[0m\n");
        }
        
        printf("\n\033[90mUsage:\033[0m\n");
        printf("  nex alias <shortcut> <package>   Create an alias\n");
        printf("  nex alias --remove <shortcut>    Remove an alias\n");
        printf("\n\033[90mExample:\033[0m\n");
        printf("  nex alias pp pagepull\n");
        printf("  nex run pp --help\n");
        printf("\n");
        
        cJSON_Delete(aliases);
        return 0;
    }
    
    /* Remove an alias */
    if (strcmp(argv[0], "--remove") == 0 || strcmp(argv[0], "-r") == 0) {
        if (argc < 2) {
            print_error("Usage: nex alias --remove <shortcut>");
            return 1;
        }
        
        cJSON *aliases = load_aliases();
        
        if (!cJSON_HasObjectItem(aliases, argv[1])) {
            print_error("Alias '%s' does not exist", argv[1]);
            cJSON_Delete(aliases);
            return 1;
        }
        
        cJSON_DeleteItemFromObject(aliases, argv[1]);
        save_aliases(aliases);
        cJSON_Delete(aliases);
        
        print_success("Removed alias '%s'", argv[1]);
        return 0;
    }
    
    /* Create an alias */
    if (argc >= 2) {
        const char *shortcut = argv[0];
        const char *package = argv[1];
        
        /* Validate shortcut name */
        if (strchr(shortcut, '.') != NULL) {
            print_error("Alias cannot contain '.' character");
            return 1;
        }
        
        /* Resolve package name first */
        char resolved_id[MAX_NAME_LEN];
        if (package_resolve_name(package, resolved_id, sizeof(resolved_id)) != 0) {
            print_error("Package '%s' not found", package);
            return 1;
        }
        
        cJSON *aliases = load_aliases();
        
        /* Check if alias already exists */
        if (cJSON_HasObjectItem(aliases, shortcut)) {
            cJSON_DeleteItemFromObject(aliases, shortcut);
        }
        
        cJSON_AddStringToObject(aliases, shortcut, resolved_id);
        save_aliases(aliases);
        cJSON_Delete(aliases);
        
        printf("\n");
        print_success("Created alias: %s → %s", shortcut, resolved_id);
        printf("  You can now use: nex run %s\n\n", shortcut);
        
        return 0;
    }
    
    /* Single argument - show what an alias points to */
    if (argc == 1) {
        cJSON *aliases = load_aliases();
        cJSON *item = cJSON_GetObjectItem(aliases, argv[0]);
        
        if (item && cJSON_IsString(item)) {
            printf("%s → %s\n", argv[0], item->valuestring);
        } else {
            printf("'%s' is not an alias\n", argv[0]);
        }
        
        cJSON_Delete(aliases);
    }
    
    return 0;
}
