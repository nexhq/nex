/*
 * Publish command - Submit a package to the registry
 */

#include "nex.h"
#include "cJSON.h"

int cmd_publish(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("  \033[33m📤 Publish Package to Registry\033[0m\n");
    printf("  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
    
    /* Check if manifest.json exists */
    FILE *f = fopen("manifest.json", "r");
    if (!f) {
        print_error("No manifest.json found in current directory");
        printf("\nRun 'nex init' to create a new package first.\n\n");
        return 1;
    }
    
    /* Read and parse manifest */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *data = malloc(size + 1);
    if (!data) {
        fclose(f);
        print_error("Out of memory");
        return 1;
    }
    
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);
    
    cJSON *manifest = cJSON_Parse(data);
    free(data);
    
    if (!manifest) {
        print_error("Invalid manifest.json");
        return 1;
    }
    
    /* Validate required fields */
    cJSON *id = cJSON_GetObjectItem(manifest, "id");
    cJSON *name = cJSON_GetObjectItem(manifest, "name");
    cJSON *version = cJSON_GetObjectItem(manifest, "version");
    cJSON *description = cJSON_GetObjectItem(manifest, "description");
    cJSON *repository = cJSON_GetObjectItem(manifest, "repository");
    
    int valid = 1;
    
    printf("  Validating manifest...\n\n");
    
    if (!id || !cJSON_IsString(id)) {
        printf("  \033[31m✗\033[0m Missing 'id' field\n");
        valid = 0;
    } else {
        printf("  \033[32m✓\033[0m ID: %s\n", id->valuestring);
    }
    
    if (!name || !cJSON_IsString(name)) {
        printf("  \033[31m✗\033[0m Missing 'name' field\n");
        valid = 0;
    } else {
        printf("  \033[32m✓\033[0m Name: %s\n", name->valuestring);
    }
    
    if (!version || !cJSON_IsString(version)) {
        printf("  \033[31m✗\033[0m Missing 'version' field\n");
        valid = 0;
    } else {
        printf("  \033[32m✓\033[0m Version: %s\n", version->valuestring);
    }
    
    if (!description || !cJSON_IsString(description)) {
        printf("  \033[31m✗\033[0m Missing 'description' field\n");
        valid = 0;
    } else {
        printf("  \033[32m✓\033[0m Description: %s\n", description->valuestring);
    }
    
    if (!repository || !cJSON_IsString(repository)) {
        printf("  \033[31m✗\033[0m Missing 'repository' field\n");
        valid = 0;
    } else {
        printf("  \033[32m✓\033[0m Repository: %s\n", repository->valuestring);
    }
    
    printf("\n");
    
    if (!valid) {
        print_error("Manifest validation failed");
        printf("\nFix the issues above and try again.\n\n");
        cJSON_Delete(manifest);
        return 1;
    }
    
    printf("  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
    
    /* Generate submission instructions */
    printf("  \033[32m✓ Manifest is valid!\033[0m\n\n");
    
    printf("  To publish your package:\n\n");
    
    printf("  \033[33m1. Push your code to GitHub:\033[0m\n");
    printf("     git add .\n");
    printf("     git commit -m \"Release %s\"\n", version->valuestring);
    printf("     git push origin main\n");
    printf("     git tag v%s\n", version->valuestring);
    printf("     git push origin v%s\n\n", version->valuestring);
    
    printf("  \033[33m2. Fork the nex registry:\033[0m\n");
    printf("     https://github.com/devkiraa/nex\n\n");
    
    printf("  \033[33m3. Add your package:\033[0m\n");
    
    /* Figure out registry path from package ID */
    const char *pkg_id = id->valuestring;
    char first_letter = pkg_id[0];
    const char *dot = strchr(pkg_id, '.');
    char author[MAX_NAME_LEN] = {0};
    char pkg_name[MAX_NAME_LEN] = {0};
    
    if (dot) {
        size_t author_len = dot - pkg_id;
        strncpy(author, pkg_id, author_len);
        strncpy(pkg_name, dot + 1, sizeof(pkg_name) - 1);
    }
    
    printf("     Create: registry/packages/%c/%s/%s/manifest.json\n", 
           first_letter, author, pkg_name);
    printf("     Copy your manifest.json there\n\n");
    
    printf("  \033[33m4. Update registry/index.json:\033[0m\n");
    printf("     Add your package entry with shortName\n\n");
    
    printf("  \033[33m5. Submit a Pull Request:\033[0m\n");
    printf("     Title: \"Add package: %s\"\n\n", pkg_id);
    
    printf("  \033[90mNote: Automated publishing coming soon!\033[0m\n\n");
    
    cJSON_Delete(manifest);
    return 0;
}
