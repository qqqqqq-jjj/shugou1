#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

void search(const char *path, const char *kw, const char *ext) {
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *entry;
    char fullpath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st; stat(fullpath, &st);
        if (S_ISDIR(st.st_mode)) {
            search(fullpath, kw, ext);
        } else {
            if (strstr(entry->d_name, kw) && strstr(entry->d_name, ext))
                printf("? %s\n", fullpath);
        }
    }
    closedir(dir);
}

int main() {
    char kw[100], ext[20];
    printf("关键词："); scanf("%s", kw);
    printf("后缀（如 .txt）："); scanf("%s", ext);
    printf("\n结果：\n");
    search(".", kw, ext);
    return 0;
}
