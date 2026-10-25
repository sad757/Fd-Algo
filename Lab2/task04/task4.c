/* ============================================================
 * Задание 4 — Резервное копирование (задача о рюкзаке 0/1)
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task4 task4.c
 * Запуск:     ./task4 < input.txt
 *
 * Вход (stdin) :
 *   n capacity
 *   filename1 size1 importance1
 *   ...
 *
 * Выход (stdout) :
 *   TOTAL_SIZE S
 *   TOTAL_IMPORTANCE I
 *   FILES k
 *   filename1
 *   ...
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#define NAME_MAX 256

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_NOMEM,
    ST_ERR_READ,
    ST_ERR_EMPTY
} status_t;

/* ---------- Структура File ---------- */
typedef struct {
    char        *name;
    unsigned int size;
    unsigned int importance;
} File;

/* ---------- Структура результата ---------- */
typedef struct {
    File        **selected;   /* массив указателей на выбранные файлы */
    size_t        count;
    unsigned int  total_size;
    unsigned int  total_importance;
} BackupResult;

/* ============================================================
 *  Собственный strlen
 * ============================================================ */
static size_t my_strlen(const char *s)
{
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

/* ============================================================
 *  Разбор unsigned int без errno
 * ============================================================ */
static status_t parse_uint(const char *s, unsigned int *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    if (*s == '-' || *s == '+') return ST_ERR_PARSE;

    unsigned long long v = 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return ST_ERR_PARSE;
        v = v * 10ULL + (unsigned long long)(*p - '0');
        if (v > UINT_MAX) return ST_ERR_RANGE;
    }
    *out = (unsigned int)v;
    return ST_OK;
}

/* ============================================================
 *  Чтение файла со stdin
 * ============================================================ */
static status_t read_input(File **files, size_t *n, unsigned int *capacity)
{
    char line[1024];

    /* 1. первая строка : n capacity */
    if (!fgets(line, sizeof(line), stdin)) return ST_ERR_READ;

    char n_buf[32], cap_buf[32];
    if (sscanf(line, "%31s %31s", n_buf, cap_buf) != 2) return ST_ERR_PARSE;

    unsigned int n_count = 0;
    status_t st;
    if ((st = parse_uint(n_buf, &n_count)) != ST_OK) return st;
    if (n_count == 0) return ST_ERR_EMPTY;
    if ((st = parse_uint(cap_buf, capacity)) != ST_OK) return st;

    /* 2. n строк с описанием файлов */
    File *arr = malloc((size_t)n_count * sizeof(File));
    if (!arr) return ST_ERR_NOMEM;

    for (unsigned int i = 0; i < n_count; ++i) {
        if (!fgets(line, sizeof(line), stdin)) {
            for (unsigned int j = 0; j < i; ++j) free(arr[j].name);
            free(arr);
            return ST_ERR_READ;
        }
        /* убрать \n */
        size_t len = my_strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        char name_buf[NAME_MAX], size_buf[32], imp_buf[32];
        if (sscanf(line, "%255s %31s %31s", name_buf, size_buf, imp_buf) != 3) {
            for (unsigned int j = 0; j < i; ++j) free(arr[j].name);
            free(arr);
            return ST_ERR_PARSE;
        }

        size_t nlen = my_strlen(name_buf);
        if (nlen == 0 || nlen >= NAME_MAX) {
            for (unsigned int j = 0; j < i; ++j) free(arr[j].name);
            free(arr);
            return ST_ERR_PARSE;
        }
        arr[i].name = malloc(nlen + 1);
        if (!arr[i].name) {
            for (unsigned int j = 0; j < i; ++j) free(arr[j].name);
            free(arr);
            return ST_ERR_NOMEM;
        }
        for (size_t k = 0; k <= nlen; ++k) arr[i].name[k] = name_buf[k];

        unsigned int sz = 0, imp = 0;
        if ((st = parse_uint(size_buf, &sz)) != ST_OK) {
            for (unsigned int j = 0; j <= i; ++j) free(arr[j].name);
            free(arr);
            return st;
        }
        if ((st = parse_uint(imp_buf, &imp)) != ST_OK) {
            for (unsigned int j = 0; j <= i; ++j) free(arr[j].name);
            free(arr);
            return st;
        }
        if (sz == 0) {
            for (unsigned int j = 0; j <= i; ++j) free(arr[j].name);
            free(arr);
            return ST_ERR_RANGE;
        }
        arr[i].size = sz;
        arr[i].importance = imp;
    }

    *files = arr;
    *n = n_count;
    return ST_OK;
}

/* ============================================================
 *  Задача о рюкзаке 0/1 — программирование динамическое
 *  dp[i][c] = max importance using first i files and capacity c
 *  Используем 1D-массив для экономии памяти.
 * ============================================================ */
static status_t solve_knapsack(File *files, size_t n, unsigned int capacity,
                               BackupResult *res)
{
    if (!files || !res || n == 0) return ST_ERR_EMPTY;

    unsigned int *dp = malloc((size_t)(capacity + 1) * sizeof(unsigned int));
    if (!dp) return ST_ERR_NOMEM;
    for (unsigned int c = 0; c <= capacity; ++c) dp[c] = 0;

    size_t cells = n * (size_t)(capacity + 1);
    unsigned char *taken = malloc(cells);
    if (!taken) { free(dp); return ST_ERR_NOMEM; }
    for (size_t i = 0; i < cells; ++i) taken[i] = 0;

    for (size_t i = 0; i < n; ++i) {
        unsigned int sz  = files[i].size;
        unsigned int imp = files[i].importance;
        for (unsigned int c = capacity; c >= sz; --c) {
            unsigned int cand = dp[c - sz] + imp;
            if (cand > dp[c]) {
                dp[c] = cand;
                taken[i * (capacity + 1) + c] = 1;
            }
        }
    }

    /* 3. Backtracking */
    File **sel = malloc(n * sizeof(File *));
    if (!sel) { free(dp); free(taken); return ST_ERR_NOMEM; }

    unsigned int remaining = capacity;
    size_t count = 0;
    unsigned int total_sz = 0;
    for (size_t i = n; i-- > 0; ) {
        if (taken[i * (capacity + 1) + remaining]) {
            sel[count++] = &files[i];
            remaining -= files[i].size;
            total_sz += files[i].size;
        }
    }

    for (size_t i = 0; i < count / 2; ++i) {
        File *t = sel[i]; sel[i] = sel[count - 1 - i]; sel[count - 1 - i] = t;
    }

    res->selected = sel;
    res->count = count;
    res->total_size = total_sz;
    res->total_importance = dp[capacity];

    free(dp);
    free(taken);
    return ST_OK;
}
/* ============================================================
 *  Вывод результата
 * ============================================================ */
static void print_result(const BackupResult *res)
{
    printf("TOTAL_SIZE %u\n", res->total_size);
    printf("TOTAL_IMPORTANCE %u\n", res->total_importance);
    printf("FILES %zu\n", res->count);
    for (size_t i = 0; i < res->count; ++i) {
        printf("%s\n", res->selected[i]->name);
    }
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st)
{
    switch (st) {
        case ST_OK:         break;
        case ST_ERR_PARSE:  fprintf(stderr, "Ошибка: неверный формат входа.\n"); break;
        case ST_ERR_RANGE:  fprintf(stderr, "Ошибка: значение вне диапазона.\n"); break;
        case ST_ERR_NOMEM:  fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_READ:   fprintf(stderr, "Ошибка: чтение.\n"); break;
        case ST_ERR_EMPTY:  fprintf(stderr, "Ошибка: пустой ввод.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    File *files = NULL;
    size_t n = 0;
    unsigned int capacity = 0;

    status_t st = read_input(&files, &n, &capacity);
    if (st != ST_OK) { print_status(st); return (int)st; }

    BackupResult res;
    st = solve_knapsack(files, n, capacity, &res);
    if (st != ST_OK) {
        for (size_t i = 0; i < n; ++i) free(files[i].name);
        free(files);
        print_status(st);
        return (int)st;
    }

    print_result(&res);

    free(res.selected);
    for (size_t i = 0; i < n; ++i) free(files[i].name);
    free(files);
    return (int)ST_OK;
}
