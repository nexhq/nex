/*
 * Init command - Create a new package from template
 */

#include "nex.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define access _access
#define getcwd _getcwd
#define F_OK 0
#else
#include <unistd.h>
#endif

static void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = (char)tolower((unsigned char)str[i]);
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
    console_init();
    int yes_mode = 0;
    
    /* Check for -y/--yes flag */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) {
            yes_mode = 1;
            break;
        }
    }

    char name[MAX_NAME_LEN] = {0};
    char version[32] = "0.1.0";
    char author[MAX_NAME_LEN] = {0};
    char description[MAX_DESCRIPTION_LEN] = {0};
    char license[32] = "MIT";
    char runtime[32] = "python";
    char entrypoint[MAX_PATH_LEN] = {0};
    char cwd[MAX_PATH_LEN] = {0};
    
    /* Get current directory name for default package name */
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char *p = strrchr(cwd, PATH_SEPARATOR);
        if (p) {
            strncpy(name, p + 1, sizeof(name) - 1);
        } else {
            strncpy(name, cwd, sizeof(name) - 1);
        }
    } else {
        strcpy(name, "my-package");
    }
    sanitize_name(name);

    if (yes_mode) {
        /* Defaults for yes mode */
        strcpy(author, "user");
        strcpy(description, "A new nex package");
        strcpy(runtime, "python");
        strcpy(entrypoint, "main.py");
    } else {
        printf("\n");
        printf("  \033[33m📦 Create a new nex package\033[0m\n");
        printf("  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
        printf("  This utility will walk you through creating a nex.json file.\n");
        printf("  It only covers the most common items, and tries to guess sensible defaults.\n\n");
        printf("  Press ^C at any time to quit.\n\n");
        
        /* Package name */
        printf("  package name: (%s) ", name);
        fflush(stdout);
        char input[MAX_NAME_LEN];
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            if (strlen(input) > 0) {
                strncpy(name, input, sizeof(name) - 1);
                sanitize_name(name);
            }
        }
        
        /* Version */
        printf("  version: (%s) ", version);
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            if (strlen(input) > 0) {
                strncpy(version, input, sizeof(version) - 1);
            }
        }

        /* Description */
        printf("  description: ");
        fflush(stdout);
        if (fgets(description, sizeof(description), stdin) != NULL) {
            description[strcspn(description, "\n")] = '\0';
        }

        /* Author/username */
        char default_author[32] = "user";
        /* Try to guess author from git config if we could, but for now default to 'user' or empty */
        printf("  author: (%s) ", default_author);
        fflush(stdout);
        if (fgets(author, sizeof(author), stdin) != NULL) {
            author[strcspn(author, "\n")] = '\0';
        }
        if (strlen(author) == 0) {
            strcpy(author, default_author);
        }
        to_lowercase(author);
        
        /* Runtime */
        printf("  runtime (python/node/bash) [%s]: ", runtime);
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            if (strlen(input) > 0) {
                strncpy(runtime, input, sizeof(runtime) - 1);
            }
        }
    }
    
    /* Set default entrypoint based on runtime if not already set or in interactive mode */
    char default_entry[32];
    if (strcmp(runtime, "python") == 0) {
        snprintf(default_entry, sizeof(default_entry), "main.py");
    } else if (strcmp(runtime, "node") == 0) {
        snprintf(default_entry, sizeof(default_entry), "index.js");
    } else if (strcmp(runtime, "bash") == 0) {
        snprintf(default_entry, sizeof(default_entry), "main.sh");
    } else {
        snprintf(default_entry, sizeof(default_entry), "main");
    }

    if (!yes_mode) {
        /* Entrypoint */
        printf("  entry point: (%s) ", default_entry);
        fflush(stdout);
        char input[MAX_PATH_LEN];
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            if (strlen(input) > 0) {
                strncpy(entrypoint, input, sizeof(entrypoint) - 1);
            } else {
                strcpy(entrypoint, default_entry);
            }
        }
        
        /* License */
        printf("  license: (%s) ", license);
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            if (strlen(input) > 0) {
                strncpy(license, input, sizeof(license) - 1);
            }
        }
    } else {
        strcpy(entrypoint, default_entry);
    }
    
    /* Build package ID */
    char package_id[MAX_NAME_LEN];
    snprintf(package_id, sizeof(package_id), "%s.%s", author, name);
    
    if (!yes_mode) {
        printf("\n  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
        printf("  About to write to %s\\nex.json:\n\n", cwd);
        printf("  {\n");
        printf("    \"name\": \"%s\",\n", name);
        printf("    \"version\": \"%s\",\n", version);
        printf("    \"description\": \"%s\",\n", description);
        printf("    \"main\": \"%s\",\n", entrypoint);
        printf("    \"author\": \"%s\",\n", author);
        printf("    \"license\": \"%s\"\n", license);
        printf("  }\n\n");
        
        printf("  Is this OK? (yes) ");
        fflush(stdout);
        char input[10];
        if (fgets(input, sizeof(input), stdin) != NULL) {
            if (tolower(input[0]) == 'n') {
                printf("  Aborted.\n");
                return 0;
            }
        }
    }
    
    /* Create nex.json */
    FILE *f = fopen("nex.json", "w");
    if (!f) {
        print_error("Failed to create nex.json");
        return 1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"$schema\": \"https://raw.githubusercontent.com/nexhq/nex/main/registry/schema/package.schema.json\",\n");
    fprintf(f, "  \"id\": \"%s\",\n", package_id);
    fprintf(f, "  \"name\": \"%s\",\n", name);
    fprintf(f, "  \"version\": \"%s\",\n", version);
    fprintf(f, "  \"description\": \"%s\",\n", description);
    fprintf(f, "  \"author\": {\n");
    fprintf(f, "    \"name\": \"%s\",\n", author);
    fprintf(f, "    \"github\": \"%s\"\n", author);
    fprintf(f, "  },\n");
    fprintf(f, "  \"license\": \"%s\",\n", license);
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
    
    if (!yes_mode) {
        printf("  \033[32m✓\033[0m Created nex.json\n");
    }
    
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
            if (!yes_mode) printf("  \033[32m✓\033[0m Created %s\n", entrypoint);
        }
    }
    
    /* Create requirements.txt for Python */
    if (strcmp(runtime, "python") == 0) {
        if (access("requirements.txt", F_OK) != 0) {
            f = fopen("requirements.txt", "w");
            if (f) {
                fprintf(f, "# Add your dependencies here\n");
                fclose(f);
                if (!yes_mode) printf("  \033[32m✓\033[0m Created requirements.txt\n");
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
                fprintf(f, "  \"version\": \"%s\",\n", version);
                fprintf(f, "  \"description\": \"%s\",\n", description);
                fprintf(f, "  \"main\": \"%s\",\n", entrypoint);
                fprintf(f, "  \"scripts\": {\n");
                fprintf(f, "    \"start\": \"node %s\"\n", entrypoint);
                fprintf(f, "  }\n");
                fprintf(f, "}\n");
                fclose(f);
                if (!yes_mode) printf("  \033[32m✓\033[0m Created package.json\n");
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
            fprintf(f, "%s\n", license);
            fclose(f);
            if (!yes_mode) printf("  \033[32m✓\033[0m Created README.md\n");
        }
    }
    
    if (!yes_mode) {
        printf("\n  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
        printf("  \033[32m✓ Package initialized!\033[0m\n\n");
        printf("  Next steps:\n");
        printf("    1. Edit %s with your code\n", entrypoint);
        printf("    2. Test locally: %s %s\n", runtime, entrypoint);
        printf("    3. Publish: nex publish\n");
        printf("\n");
    } else {
        printf("Created a new nex package in %s\n", cwd);
    }
    
    return 0;
}
