/*
 * Config command - Manage nex configuration
 */

#include "nex.h"
#include "cJSON.h"

static int get_config_path(char *path, size_t size) {
    char home[MAX_PATH_LEN];
    if (config_get_home_dir(home, sizeof(home)) != 0) {
        return -1;
    }
    snprintf(path, size, "%s%cconfig.json", home, PATH_SEPARATOR);
    return 0;
}

static cJSON* load_config(void) {
    char path[MAX_PATH_LEN];
    if (get_config_path(path, sizeof(path)) != 0) {
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

static int save_config(cJSON *config) {
    char path[MAX_PATH_LEN];
    if (get_config_path(path, sizeof(path)) != 0) {
        return -1;
    }
    
    char *str = cJSON_Print(config);
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

int cmd_config(int argc, char *argv[]) {
    if (argc < 1) {
        /* Show all config */
        printf("\n\033[33mNex Configuration:\033[0m\n\n");
        
        cJSON *config = load_config();
        
        cJSON *item;
        int count = 0;
        cJSON_ArrayForEach(item, config) {
            if (cJSON_IsString(item)) {
                printf("  %s = %s\n", item->string, item->valuestring);
            } else if (cJSON_IsBool(item)) {
                printf("  %s = %s\n", item->string, cJSON_IsTrue(item) ? "true" : "false");
            }
            count++;
        }
        
        if (count == 0) {
            printf("  \033[90mNo configuration set.\033[0m\n");
        }
        
        printf("\n\033[90mUsage:\033[0m\n");
        printf("  nex config <key>              Get a value\n");
        printf("  nex config <key> <value>      Set a value\n");
        printf("  nex config --unset <key>      Remove a value\n");
        printf("\n\033[90mAvailable keys:\033[0m\n");
        printf("  registry_url      Custom registry URL\n");
        printf("  global_path       Path for global packages\n");
        printf("  auto_update       Auto-check for CLI updates (true/false)\n");
        printf("\n");
        
        cJSON_Delete(config);
        return 0;
    }
    
    /* Unset a key */
    if (strcmp(argv[0], "--unset") == 0) {
        if (argc < 2) {
            print_error("Usage: nex config --unset <key>");
            return 1;
        }
        
        cJSON *config = load_config();
        cJSON_DeleteItemFromObject(config, argv[1]);
        save_config(config);
        cJSON_Delete(config);
        
        print_success("Removed '%s' from config", argv[1]);
        return 0;
    }
    
    /* Get a value */
    if (argc == 1) {
        cJSON *config = load_config();
        cJSON *item = cJSON_GetObjectItem(config, argv[0]);
        
        if (!item) {
            printf("%s: \033[90m(not set)\033[0m\n", argv[0]);
        } else if (cJSON_IsString(item)) {
            printf("%s\n", item->valuestring);
        } else if (cJSON_IsBool(item)) {
            printf("%s\n", cJSON_IsTrue(item) ? "true" : "false");
        }
        
        cJSON_Delete(config);
        return 0;
    }
    
    /* Set a value */
    if (argc >= 2) {
        cJSON *config = load_config();
        
        /* Check for boolean values */
        if (strcmp(argv[1], "true") == 0) {
            cJSON_DeleteItemFromObject(config, argv[0]);
            cJSON_AddBoolToObject(config, argv[0], 1);
        } else if (strcmp(argv[1], "false") == 0) {
            cJSON_DeleteItemFromObject(config, argv[0]);
            cJSON_AddBoolToObject(config, argv[0], 0);
        } else {
            cJSON_DeleteItemFromObject(config, argv[0]);
            cJSON_AddStringToObject(config, argv[0], argv[1]);
        }
        
        save_config(config);
        cJSON_Delete(config);
        
        print_success("Set %s = %s", argv[0], argv[1]);
        return 0;
    }
    
    return 0;
}
