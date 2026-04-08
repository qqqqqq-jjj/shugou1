#include <stdio.h>
#include <dirent.h>
#include <string.h>

int main() {
    char keyword[100];
    printf("输入搜索关键词：");
    scanf("%s", keyword);

    DIR *dir = opendir(".");
    if (!dir) { perror("打开目录失败"); return 1; }

    struct dirent *entry;
    printf("\n搜索结果：\n");
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, keyword)) {
            printf("? %s\n", entry->d_name);
        }
    }
    closedir(dir);
    return 0;
}
