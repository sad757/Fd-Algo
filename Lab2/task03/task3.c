/* ============================================================
 * Задание 3 — Поиск подстроки в нескольких файлах
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task3 task3.c
 * Запуск:     ./task3 <подстрока> <файл1> <файл2> ...
 *
 * Вариадическая функция ищет все вхождения подстроки в каждом файле.
 * Запрещено использовать strstr и strncmp.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define LINE_BUF 4096

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_OPEN_IN,
    ST_ERR_READ,
    ST_ERR_NOMEM,
    ST_ERR_EMPTY
} status_t;

/* ============================================================
 *  Собственный strlen (без string.h)
 * ============================================================ */
static size_t my_strlen(const char *s)
{
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

/* ============================================================
 *  Поиск подстроки в буфере без strstr
 *  Возвращает количество вхождений.
 * ============================================================ */
static size_t count_occurrences(const char *haystack, size_t hlen,
                                const char *needle, size_t nlen)
{
    if (!haystack || !needle || nlen == 0 || nlen > hlen) return 0;
    size_t count = 0;
    for (size_t i = 0; i + nlen <= hlen; ++i) {
        size_t j = 0;
        while (j < nlen && haystack[i + j] == needle[j]) ++j;
        if (j == nlen) ++count;
    }
    return count;
}

/* ============================================================
 *  Поиск подстроки в файле
 *  Возвращает количество вхождений или код ошибки.
 * ============================================================ */
static status_t search_in_file(const char *path, const char *needle,
                               size_t *out_count)
{
    if (!path || !needle || !out_count) return ST_ERR_EMPTY;

    FILE *f = fopen(path, "r");
    if (!f) return ST_ERR_OPEN_IN;

    size_t nlen = my_strlen(needle);
    if (nlen == 0) { fclose(f); return ST_ERR_EMPTY; }

    /* lecture par blocs pour gérer les gros fichiers */
    char buf[LINE_BUF];
    size_t total = 0;
    size_t read_total;

    while ((read_total = fread(buf, 1, sizeof(buf), f)) > 0) {
        total += count_occurrences(buf, read_total, needle, nlen);
    }

    if (ferror(f)) { fclose(f); return ST_ERR_READ; }
    fclose(f);

    *out_count = total;
    return ST_OK;
}

/* ============================================================
 *  Вариадическая функция
 *  Поиск подстроки во всех переданных файлах.
 * ============================================================ */
static status_t search_all(const char *needle, int nfiles, ...)
{
    if (!needle || nfiles <= 0) return ST_ERR_EMPTY;

    va_list ap;
    va_start(ap, nfiles);

    status_t st = ST_OK;
    for (int i = 0; i < nfiles; ++i) {
        const char *path = va_arg(ap, const char *);
        size_t count = 0;
        status_t st1 = search_in_file(path, needle, &count);
        if (st1 != ST_OK) {
            fprintf(stderr, "  [%d] '%s': ошибка\n", i + 1, path);
            if (st1 == ST_ERR_OPEN_IN)
                fprintf(stderr, "       -> не удалось открыть файл\n");
            else if (st1 == ST_ERR_READ)
                fprintf(stderr, "       -> ошибка чтения\n");
            st = st1;
            continue;
        }
        printf("  [%d] '%s': %zu вхождений\n", i + 1, path, count);
    }

    va_end(ap);
    return st;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st)
{
    switch (st) {
        case ST_OK:          break;
        case ST_ERR_ARGC:    fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_OPEN_IN: fprintf(stderr, "Ошибка: не удалось открыть файл.\n"); break;
        case ST_ERR_READ:    fprintf(stderr, "Ошибка: чтение файла.\n"); break;
        case ST_ERR_NOMEM:   fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_EMPTY:   fprintf(stderr, "Ошибка: пустая подстрока.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <подстрока> <файл1> [файл2 ...]\n", argv[0]);
        return (int)ST_ERR_ARGC;
    }

    const char *needle = argv[1];
    int nfiles = argc - 2;

    /* validation de la sous-chaîne */
    if (my_strlen(needle) == 0) {
        print_status(ST_ERR_EMPTY);
        return (int)ST_ERR_EMPTY;
    }

    printf("Поиск подстроки \"%s\" в %d файле(ах):\n", needle, nfiles);

    /* dispatch variadique manuel (C ne permet pas de construire va_list
       dynamiquement depuis argv) */
    status_t st = ST_OK;
    switch (nfiles) {
        case 1: st = search_all(needle, 1, argv[2]); break;
        case 2: st = search_all(needle, 2, argv[2], argv[3]); break;
        case 3: st = search_all(needle, 3, argv[2], argv[3], argv[4]); break;
        case 4: st = search_all(needle, 4, argv[2], argv[3], argv[4], argv[5]); break;
        case 5: st = search_all(needle, 5, argv[2], argv[3], argv[4], argv[5], argv[6]); break;
        case 6: st = search_all(needle, 6, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]); break;
        case 7: st = search_all(needle, 7, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]); break;
        case 8: st = search_all(needle, 8, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]); break;
        default:
            fprintf(stderr, "Слишком много файлов (максимум 8).\n");
            return (int)ST_ERR_ARGC;
    }

    if (st != ST_OK) print_status(st);
    return (int)st;
}
