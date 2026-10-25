/* ============================================================
 * Задание 1 — Операции над строками (без string.h в функциях)
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task1 task1.c
 * Запуск:
 *   ./task1 -l <строка>
 *   ./task1 -r <строка>
 *   ./task1 -u <строка>
 *   ./task1 -n <строка>
 *   ./task1 -c <строка> <seed> <строка1> <строка2> ...
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_NOMEM,
    ST_ERR_EMPTY
} status_t;

/* ============================================================
 *  Собственные строковые функции (string.h запрещён в функциях)
 * ============================================================ */
static size_t my_strlen(const char *s)
{
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

/* ============================================================
 *  Проверка флага
 * ============================================================ */
static int is_valid_flag(const char *arg, char *out)
{
    if (!arg || !out) return 0;
    if (arg[0] != '-' && arg[0] != '/') return 0;
    if (arg[1] == '\0' || arg[2] != '\0') return 0;
    char c = (char)tolower((unsigned char)arg[1]);
    if (c != 'l' && c != 'r' && c != 'u' && c != 'n' && c != 'c') return 0;
    *out = c;
    return 1;
}

/* ============================================================
 *  Флаг -l : длина строки
 * ============================================================ */
static status_t do_l(const char *s, size_t *out_len)
{
    if (!s || !out_len) return ST_ERR_EMPTY;
    *out_len = my_strlen(s);
    return ST_OK;
}

/* ============================================================
 *  Флаг -r : перевёрнутая строка
 * ============================================================ */
static status_t do_r(const char *s, char **out)
{
    if (!s || !out) return ST_ERR_EMPTY;
    size_t n = my_strlen(s);
    char *r = malloc(n + 1);
    if (!r) return ST_ERR_NOMEM;
    for (size_t i = 0; i < n; ++i) r[i] = s[n - 1 - i];
    r[n] = '\0';
    *out = r;
    return ST_OK;
}

/* ============================================================
 *  Флаг -u : нечётные позиции → верхний регистр
 * ============================================================ */
static status_t do_u(const char *s, char **out)
{
    if (!s || !out) return ST_ERR_EMPTY;
    size_t n = my_strlen(s);
    char *r = malloc(n + 1);
    if (!r) return ST_ERR_NOMEM;
    for (size_t i = 0; i < n; ++i) {
        r[i] = (i % 2 == 1) ? (char)toupper((unsigned char)s[i]) : s[i];
    }
    r[n] = '\0';
    *out = r;
    return ST_OK;
}

/* ============================================================
 *  Флаг -n : цифры, затем буквы, затем остальные
 * ============================================================ */
static status_t do_n(const char *s, char **out)
{
    if (!s || !out) return ST_ERR_EMPTY;
    size_t n = my_strlen(s);
    char *r = malloc(n + 1);
    if (!r) return ST_ERR_NOMEM;

    size_t k = 0;
    /* 1. цифры */
    for (size_t i = 0; i < n; ++i)
        if (isdigit((unsigned char)s[i])) r[k++] = s[i];
    /* 2. буквы */
    for (size_t i = 0; i < n; ++i)
        if (isalpha((unsigned char)s[i])) r[k++] = s[i];
    /* 3. всё остальное */
    for (size_t i = 0; i < n; ++i)
        if (!isdigit((unsigned char)s[i]) && !isalpha((unsigned char)s[i]))
            r[k++] = s[i];
    r[k] = '\0';
    *out = r;
    return ST_OK;
}

/* ============================================================
 *  Флаг -c : конкатенация строк в псевдослучайном порядке
 * ============================================================ */
static status_t do_c(int n, char **strings, unsigned int seed, char **out)
{
    if (!strings || !out || n < 1) return ST_ERR_EMPTY;

    /* 1. вычислить общую длину */
    size_t total = 0;
    for (int i = 0; i < n; ++i) total += my_strlen(strings[i]);

    /* 2. выделить результирующий буфер */
    char *res = malloc(total + 1);
    if (!res) return ST_ERR_NOMEM;

    /* 3. создать массив индексов и перемешать (Fisher-Yates) */
    int *order = malloc((size_t)n * sizeof(int));
    if (!order) { free(res); return ST_ERR_NOMEM; }
    for (int i = 0; i < n; ++i) order[i] = i;

    srand(seed);
    for (int i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int t = order[i]; order[i] = order[j]; order[j] = t;
    }

    /* 4. конкатенация */
    size_t k = 0;
    for (int i = 0; i < n; ++i) {
        const char *s = strings[order[i]];
        size_t len = my_strlen(s);
        for (size_t j = 0; j < len; ++j) res[k++] = s[j];
    }
    res[k] = '\0';

    free(order);
    *out = res;
    return ST_OK;
}

/* ============================================================
 *  Разбор unsigned int без errno
 * ============================================================ */
static status_t parse_uint(const char *s, unsigned int *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    if (*s == '-' || *s == '+') return ST_ERR_PARSE;  /* pas de signe */

    unsigned long long v = 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return ST_ERR_PARSE;
        v = v * 10ULL + (unsigned long long)(*p - '0');
        if (v > UINT_MAX) return ST_ERR_RANGE;
    }
    *out = (unsigned int)v;
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st, const char *arg)
{
    switch (st) {
        case ST_OK:        break;
        case ST_ERR_ARGC:  fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_FLAG:  fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_PARSE: fprintf(stderr, "Ошибка: параметр не является числом.\n"); break;
        case ST_ERR_RANGE: fprintf(stderr, "Ошибка: параметр вне диапазона.\n"); break;
        case ST_ERR_NOMEM: fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_EMPTY: fprintf(stderr, "Ошибка: пустая строка.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr,
            "Использование:\n"
            "  %s -l <строка>\n"
            "  %s -r <строка>\n"
            "  %s -u <строка>\n"
            "  %s -n <строка>\n"
            "  %s -c <строка> <seed> <строка1> ...\n",
            argv[0], argv[0], argv[0], argv[0], argv[0]);
        return (int)ST_ERR_ARGC;
    }

    char flag = '\0';
    if (!is_valid_flag(argv[1], &flag)) {
        print_status(ST_ERR_FLAG, argv[1]);
        return (int)ST_ERR_FLAG;
    }

    status_t st = ST_OK;

    /* ---------- -l ---------- */
    if (flag == 'l') {
        if (argc != 3) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        size_t len = 0;
        st = do_l(argv[2], &len);
        if (st == ST_OK) printf("Длина строки: %zu\n", len);
    }
    /* ---------- -r ---------- */
    else if (flag == 'r') {
        if (argc != 3) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        char *res = NULL;
        st = do_r(argv[2], &res);
        if (st == ST_OK) { printf("%s\n", res); free(res); }
    }
    /* ---------- -u ---------- */
    else if (flag == 'u') {
        if (argc != 3) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        char *res = NULL;
        st = do_u(argv[2], &res);
        if (st == ST_OK) { printf("%s\n", res); free(res); }
    }
    /* ---------- -n ---------- */
    else if (flag == 'n') {
        if (argc != 3) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        char *res = NULL;
        st = do_n(argv[2], &res);
        if (st == ST_OK) { printf("%s\n", res); free(res); }
    }
    /* ---------- -c ---------- */
    else {
        if (argc < 5) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        unsigned int seed = 0;
        if ((st = parse_uint(argv[3], &seed)) != ST_OK) { print_status(st, NULL); return (int)st; }
        int n = argc - 4;
        char **strings = &argv[4];
        char *res = NULL;
        st = do_c(n, strings, seed, &res);
        if (st == ST_OK) { printf("%s\n", res); free(res); }
    }

    if (st != ST_OK) print_status(st, argv[1]);
    return (int)st;
}
