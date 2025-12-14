/*
 * Init command - Create a new package from template
 */

#include "nex.h"
#include <ctype.h>

static void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

static void sanitize_name(char *str) {
    for (int i = 0; str[i]; i++) {
        if (!isalnum(str[i]) && str[i] != '-') {
            str[i] = '-';
        }
    }
    to_lowercase(str);
}

int cmd_init(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    char name[MAX_NAME_LEN] = {0};
    char author[MAX_NAME_LEN] = {0};
    char description[MAX_DESCRIPTION_LEN] = {0};
    char runtime[32] = "python";
    char entrypoint[MAX_PATH_LEN] = {0};
    
    printf("\n");
    printf("  \033[33m📦 Create a new nex package\033[0m\n");
    printf("  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
    
    /* Package name */
    printf("  Package name: ");
    fflush(stdout);
    if (fgets(name, sizeof(name), stdin) == NULL) return 1;
    name[strcspn(name, "\n")] = '\0';
    sanitize_name(name);
    
    if (strlen(name) == 0) {
        print_error("Package name is required");
        return 1;
    }
    
    /* Author/username */
    printf("  Your username: ");
    fflush(stdout);
    if (fgets(author, sizeof(author), stdin) == NULL) return 1;
    author[strcspn(author, "\n")] = '\0';
    to_lowercase(author);
    
    if (strlen(author) == 0) {
        print_error("Username is required");
        return 1;
    }
    
    /* Description */
    printf("  Description: ");
    fflush(stdout);
    if (fgets(description, sizeof(description), stdin) == NULL) return 1;
    description[strcspn(description, "\n")] = '\0';
    
    /* Runtime */
    printf("  Runtime (python/node/bash) [python]: ");
    fflush(stdout);
    char runtime_input[32];
    if (fgets(runtime_input, sizeof(runtime_input), stdin) != NULL) {
        runtime_input[strcspn(runtime_input, "\n")] = '\0';
        if (strlen(runtime_input) > 0) {
            strncpy(runtime, runtime_input, sizeof(runtime) - 1);
        }
    }
    
    /* Set default entrypoint based on runtime */
    if (strcmp(runtime, "python") == 0) {
        snprintf(entrypoint, sizeof(entrypoint), "%s.py", name);
    } else if (strcmp(runtime, "node") == 0) {
        snprintf(entrypoint, sizeof(entrypoint), "index.js");
    } else if (strcmp(runtime, "bash") == 0) {
        snprintf(entrypoint, sizeof(entrypoint), "%s.sh", name);
    } else {
        snprintf(entrypoint, sizeof(entrypoint), "main");
    }
    
    /* Entrypoint */
    printf("  Entry file [%s]: ", entrypoint);
    fflush(stdout);
    char entry_input[MAX_PATH_LEN];
    if (fgets(entry_input, sizeof(entry_input), stdin) != NULL) {
        entry_input[strcspn(entry_input, "\n")] = '\0';
        if (strlen(entry_input) > 0) {
            strncpy(entrypoint, entry_input, sizeof(entrypoint) - 1);
        }
    }
    
    /* Build package ID */
    char package_id[MAX_NAME_LEN];
    snprintf(package_id, sizeof(package_id), "%s.%s", author, name);
    
    printf("\n  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
    printf("  Creating package: \033[1m%s\033[0m\n\n", package_id);
    
    /* Create manifest.json */
    FILE *f = fopen("manifest.json", "w");
    if (!f) {
        print_error("Failed to create manifest.json");
        return 1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"$schema\": \"https://raw.githubusercontent.com/devkiraa/nex/main/registry/schema/package.schema.json\",\n");
    fprintf(f, "  \"id\": \"%s\",\n", package_id);
    fprintf(f, "  \"name\": \"%s\",\n", name);
    fprintf(f, "  \"version\": \"0.1.0\",\n");
    fprintf(f, "  \"description\": \"%s\",\n", description);
    fprintf(f, "  \"author\": {\n");
    fprintf(f, "    \"name\": \"%s\",\n", author);
    fprintf(f, "    \"github\": \"%s\"\n", author);
    fprintf(f, "  },\n");
    fprintf(f, "  \"license\": \"MIT\",\n");
    fprintf(f, "  \"repository\": \"https://github.com/%s/%s\",\n", author, name);
    fprintf(f, "  \"runtime\": {\n");
    fprintf(f, "    \"type\": \"%s\"\n", runtime);
    fprintf(f, "  },\n");
    fprintf(f, "  \"entrypoint\": \"%s\",\n", entrypoint);
    fprintf(f, "  \"commands\": {\n");
    if (strcmp(runtime, "python") == 0) {
        fprintf(f, "    \"default\": \"python %s\",\n", entrypoint);
        fprintf(f, "    \"install\": \"pip install -r requirements.txt\"\n");
    } else if (strcmp(runtime, "node") == 0) {
        fprintf(f, "    \"default\": \"node %s\",\n", entrypoint);
        fprintf(f, "    \"install\": \"npm install\"\n");
    } else {
        fprintf(f, "    \"default\": \"./%s\"\n", entrypoint);
    }
    fprintf(f, "  },\n");
    fprintf(f, "  \"keywords\": []\n");
    fprintf(f, "}\n");
    fclose(f);
    printf("  \033[32m✓\033[0m Created manifest.json\n");
    
    /* Create entrypoint file if it doesn't exist */
    if (access(entrypoint, F_OK) != 0) {
        f = fopen(entrypoint, "w");
        if (f) {
            if (strcmp(runtime, "python") == 0) {
                fprintf(f, "#!/usr/bin/env python3\n");
                fprintf(f, "\"\"\"\n%s - %s\n\"\"\"\n\n", name, description);
                fprintf(f, "import argparse\n\n");
                fprintf(f, "def main():\n");
                fprintf(f, "    parser = argparse.ArgumentParser(description='%s')\n", description);
                fprintf(f, "    args = parser.parse_args()\n");
                fprintf(f, "    print('Hello from %s!')\n\n", name);
                fprintf(f, "if __name__ == '__main__':\n");
                fprintf(f, "    main()\n");
            } else if (strcmp(runtime, "node") == 0) {
                fprintf(f, "#!/usr/bin/env node\n\n");
                fprintf(f, "/**\n * %s - %s\n */\n\n", name, description);
                fprintf(f, "console.log('Hello from %s!');\n", name);
            } else if (strcmp(runtime, "bash") == 0) {
                fprintf(f, "#!/bin/bash\n");
                fprintf(f, "# %s - %s\n\n", name, description);
                fprintf(f, "echo \"Hello from %s!\"\n", name);
            }
            fclose(f);
            printf("  \033[32m✓\033[0m Created %s\n", entrypoint);
        }
    }
    
    /* Create requirements.txt for Python */
    if (strcmp(runtime, "python") == 0) {
        if (access("requirements.txt", F_OK) != 0) {
            f = fopen("requirements.txt", "w");
            if (f) {
                fprintf(f, "# Add your dependencies here\n");
                fclose(f);
                printf("  \033[32m✓\033[0m Created requirements.txt\n");
            }
        }
    }
    
    /* Create package.json for Node */
    if (strcmp(runtime, "node") == 0) {
        if (access("package.json", F_OK) != 0) {
            f = fopen("package.json", "w");
            if (f) {
                fprintf(f, "{\n");
                fprintf(f, "  \"name\": \"%s\",\n", name);
                fprintf(f, "  \"version\": \"0.1.0\",\n");
                fprintf(f, "  \"description\": \"%s\",\n", description);
                fprintf(f, "  \"main\": \"%s\",\n", entrypoint);
                fprintf(f, "  \"scripts\": {\n");
                fprintf(f, "    \"start\": \"node %s\"\n", entrypoint);
                fprintf(f, "  }\n");
                fprintf(f, "}\n");
                fclose(f);
                printf("  \033[32m✓\033[0m Created package.json\n");
            }
        }
    }
    
    /* Create README.md */
    if (access("README.md", F_OK) != 0) {
        f = fopen("README.md", "w");
        if (f) {
            fprintf(f, "# %s\n\n", name);
            fprintf(f, "%s\n\n", description);
            fprintf(f, "## Installation\n\n");
            fprintf(f, "```bash\n");
            fprintf(f, "nex install %s\n", name);
            fprintf(f, "```\n\n");
            fprintf(f, "## Usage\n\n");
            fprintf(f, "```bash\n");
            fprintf(f, "nex run %s\n", name);
            fprintf(f, "```\n\n");
            fprintf(f, "## License\n\n");
            fprintf(f, "MIT\n");
            fclose(f);
            printf("  \033[32m✓\033[0m Created README.md\n");
        }
    }
    
    printf("\n  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
    printf("  \033[32m✓ Package initialized!\033[0m\n\n");
    printf("  Next steps:\n");
    printf("    1. Edit %s with your code\n", entrypoint);
    printf("    2. Test locally: %s %s\n", runtime, entrypoint);
    printf("    3. Publish: nex publish\n");
    printf("\n");
    
    return 0;
}
