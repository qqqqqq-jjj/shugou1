#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

void search(const char *path, const char *kw) {
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *entry;
    char fullpath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(fullpath, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            search(fullpath, kw);
        } else if (strstr(entry->d_name, kw)) {
            count++;
            printf("[%d] %s\n大小：%ld B | 修改：%s",
                   count, fullpath, st.st_size, ctime(&st.st_mtime));
        }
    }
    closedir(dir);
}

int main() {
     char path[256], kw[100];
    printf("===== 本地文件搜索引擎 V6 =====\n");
    printf("搜索目录（默认当前目录 .）：");
    fgets(path, sizeof(path), stdin);
    path[strcspn(path, "\n")] = 0;
    if (strlen(path) == 0) strcpy(path, ".");
    printf("关键词：");
    scanf("%s", kw);
    printf("\n开始搜索...\n\n");
    search(path, kw);
    printf("\n共找到 %d 个结果\n", count);
    return 0;

}
