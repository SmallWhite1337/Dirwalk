/*
DirWalk
 
Рекурсивный обход файловой системы с фильтрацией по типам.
Формат вывода аналогичен утилите find.
 
Поддерживаемые опции:
    -l  только символические ссылки
    -d  только каталоги
    -f  только регулярные файлы
    -s  сортировка вывода по LC_COLLATE
 
 Автор: Родкевич Н.А.
 */

#define _XOPEN_SOURCE 700  // Определяем стандарт POSIX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // POSIX: getopt, и другие системные вызовы
#include <dirent.h>  // Работа с директориями: DIR, opendir, readdir, closedir
#include <sys/stat.h>  // Структура stat и макросы S_ISDIR, S_ISREG, S_ISLNK
#include <errno.h>
#include <locale.h>

/* ---------- структура для накопления путей ---------- */

typedef struct {
    char **items;
    int size;
    int capacity;
} str_array_t;

/* ---------- глобальные флаги ---------- */

static int opt_l = 0;  // Только симлинки
static int opt_d = 0;  // Только каталоги
static int opt_f = 0;  // Только обычные файлы
static int opt_s = 0;  // Сортировать вывод

/* ---------- проверка типа ---------- */

static int type_match(const struct stat* st) {
    if (!opt_l && !opt_d && !opt_f) {
        return 1; // если фильтров нет — выводим всё 
    }
    if (opt_l && S_ISLNK(st->st_mode)) {
        return 1;
    }
    if (opt_d && S_ISDIR(st->st_mode)) {
        return 1;
    }
    if (opt_f && S_ISREG(st->st_mode)) {
        return 1;
    }
    return 0;
}

/* ---------- динамический массив ---------- */

static void arr_init(str_array_t* arr) {
    arr->items = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

static void arr_push(str_array_t* arr, const char* path) {
    if (arr->size == arr->capacity) {
        int new_cap = arr->capacity ? arr->capacity * 2 : 64;
        char** tmp = realloc(arr->items, new_cap * sizeof(char*));
        if (!tmp) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        arr->items = tmp;
        arr->capacity = new_cap;
    }
    arr->items[arr->size] = strdup(path);
    if (!arr->items[arr->size]) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    arr->size++;
}

static void arr_free(str_array_t* arr) {
    for (int i = 0; i < arr->size; ++i)
        free(arr->items[i]);
    free(arr->items);
}

/* ---------- сравнение для сортировки ---------- */

static int path_cmp(const void* a, const void* b) {
    const char* pa = *(const char* const*)a;
    const char* pb = *(const char* const*)b;
    return strcoll(pa, pb);
}

/* ---------- рекурсивный обход ---------- */

static void walk(const char* dirpath, str_array_t* out) {
    DIR* dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "opendir(%s): %s\n", dirpath, strerror(errno));
        return;
    }
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        int len = strlen(dirpath) + strlen(ent->d_name) + 2;
        char* path = malloc(len);
        if (!path) {
            perror("malloc");
            closedir(dir);
            exit(EXIT_FAILURE);
        }
        snprintf(path, len, "%s/%s", dirpath, ent->d_name);
        struct stat st;
        if (lstat(path, &st) == -1) {
            fprintf(stderr, "lstat(%s): %s\n", path, strerror(errno));
            free(path);
            continue;
        }
        if (type_match(&st)) {
            arr_push(out, path);
        }
        if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {  // рекурсия только в реальные каталоги (не симлинки)
            walk(path, out);
        }
        free(path);
    }
    closedir(dir);
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    int opt;
    setlocale(LC_COLLATE, "");
    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
        case 'l': opt_l = 1; break;
        case 'd': opt_d = 1; break;
        case 'f': opt_f = 1; break;
        case 's': opt_s = 1; break;
        default:
            fprintf(stderr, "Usage: %s [dir] [-l] [-d] [-f] [-s]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    const char *start_dir = ".";
    if (optind < argc) {
        start_dir = argv[optind];
    }
    str_array_t results;
    arr_init(&results);
    walk(start_dir, &results);
    if (opt_s && results.size > 1) {
        qsort(results.items, results.size, sizeof(char*), path_cmp);
    }
    for (int i = 0; i < results.size; ++i) {
        printf("%s\n", results.items[i]);
    }
    arr_free(&results);
    return 0;
}
