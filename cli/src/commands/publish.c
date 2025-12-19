/*
 * Publish command - Submit a package to the registry via IssueOps
 */

#include "nex.h"
#include "cJSON.h"
#include <ctype.h>

/* Simple URL encoding helper */
static void url_encode(const char *src, char *dest, size_t dest_size) {
    static const char hex[] = "0123456789ABCDEF";
    size_t i = 0, j = 0;
    
    while (src[i] && j < dest_size - 4) {
        unsigned char c = src[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            dest[j++] = c;
        } else if (c == ' ') {
            dest[j++] = '+';
        } else {
            dest[j++] = '%';
            dest[j++] = hex[c >> 4];
            dest[j++] = hex[c & 0x0F];
        }
        i++;
    }
    dest[j] = '\0';
}

static void open_url(const char *url) {
    char cmd[MAX_COMMAND_LEN];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", url);
#elif __APPLE__
    snprintf(cmd, sizeof(cmd), "open \"%s\"", url);
#else
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", url);
#endif
    system(cmd);
}

int cmd_publish(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("  \033[33m📤 Publish Package to Registry\033[0m\n");
    printf("  \033[90m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n");
    
    /* Check if nex.json exists */
    FILE *f = fopen("nex.json", "r");
    if (!f) {
        print_error("No nex.json found in current directory");
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
    
    if (!manifest) {
        free(data);
        print_error("Invalid nex.json");
        return 1;
    }
    
    /* Validate required fields */
    cJSON *id = cJSON_GetObjectItem(manifest, "id");
    
    if (!id || !cJSON_IsString(id)) {
        free(data);
        print_error("Missing 'id' field in manifest");
        return 1;
    }
    
    printf("  \033[32m✓ Manifest loaded\033[0m (%s)\n", id->valuestring);
    printf("  Preparing submission...\n\n");
    
    /* Construct Issue Body */
    /* We want the body to be just the code block with the JSON */
    char *issue_body_raw = malloc(size + 64);
    if (!issue_body_raw) {
        free(data);
        print_error("Out of memory");
        return 1;
    }
    
    sprintf(issue_body_raw, "```json\n%s\n```", data);
    
    /* Encode URL parameters */
    char *encoded_title = malloc(MAX_NAME_LEN * 3);
    char *encoded_body = malloc((size + 64) * 3);
    
    if (!encoded_title || !encoded_body) {
        print_error("Out of memory");
        return 1;
    }
    
    char title[MAX_NAME_LEN];
    snprintf(title, sizeof(title), "Register Package: %s", id->valuestring);
    
    url_encode(title, encoded_title, MAX_NAME_LEN * 3);
    url_encode(issue_body_raw, encoded_body, (size + 64) * 3);
    
    /* Construct GitHub URL */
    /* Assumes Issue Template handles the rest or general issue */
    char url[8192]; 
    /* Truncate if too long, though modern browsers handle long URLs well */
    snprintf(url, sizeof(url), 
        "https://github.com/nexhq/nex/issues/new?title=%s&body=%s&labels=package-submission",
        encoded_title, encoded_body);
        
    printf("  Opening GitHub to submit package...\n");
    printf("  \033[90m(If browser doesn't open, copy link below)\033[0m\n\n");
    printf("  %s\n\n", url);
    
    open_url(url);
    
    printf("  \033[32m✓ Submission initiated!\033[0m\n");
    printf("  Once approved (merged), your package will be live.\n\n");
    
    free(data);
    free(issue_body_raw);
    free(encoded_title);
    free(encoded_body);
    cJSON_Delete(manifest);
    
    return 0;
}
