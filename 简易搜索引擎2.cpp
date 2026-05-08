#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

void search(const char *path, const char *kw) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char fullpath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st;
        stat(fullpath, &st);

        if (S_ISDIR(st.st_mode)) {
            search(fullpath, kw);
        } else if (strstr(entry->d_name, kw)) {
            printf("? %s\n", fullpath);
        }
    }
    closedir(dir);
}

int main() {
    char kw[100];
    printf("¹Ø¼ü´Ê£º");
    scanf("%s", kw);
    printf("\n½á¹û£º\n");
    search(".", kw);
    return 0;
}
