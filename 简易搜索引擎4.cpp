#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

int contains_ignorecase(const char *str, const char *kw) {
    char s[512], k[512];
    int i;
    for (i = 0; str[i]; i++) s[i] = tolower(str[i]); s[i] = 0;
    for (i = 0; kw[i]; i++) k[i] = tolower(kw[i]); k[i] = 0;
    return strstr(s, k) != NULL;
}

void search(const char *path, const char *kw) {
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *entry;
    char fullpath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st; stat(fullpath, &st);
        if (S_ISDIR(st.st_mode)) search(fullpath, kw);
        else if (contains_ignorecase(entry->d_name, kw)) printf("? %s\n", fullpath);
    }
    closedir(dir);
}

int main() {
    char kw[100];
    printf("关键词（大小写不敏感）：");
    scanf("%s", kw);
    printf("\n结果：\n");
    search(".", kw);
    return 0;
}
