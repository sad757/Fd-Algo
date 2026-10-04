/* ============================================================
 * Задание 8 — Восстановление основания системы счисления
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task8 task8.c
 * Запуск:     ./task8 <входной> <выходной>
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define LEX_MAX 256

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_OPEN_IN,
    ST_ERR_OPEN_OUT,
    ST_ERR_READ,
    ST_ERR_WRITE,
    ST_ERR_LEX_LONG,
    ST_ERR_BAD_CHAR,
    ST_ERR_OVERFLOW
} status_t;

/* ============================================================
 *  Лексер
 * ============================================================ */
typedef struct {
    FILE *fp;
    int   current;
    int   eof;
} lexer_t;

static void lexer_init(lexer_t *lx, FILE *fp)
{
    lx->fp = fp;
    lx->current = fgetc(fp);
    lx->eof = (lx->current == EOF);
}

static int lexer_next(lexer_t *lx, char *buf, size_t sz)
{
    if (lx->eof) return 0;
    while (!lx->eof && isspace(lx->current)) {
        lx->current = fgetc(lx->fp);
        if (lx->current == EOF) { lx->eof = 1; return 0; }
    }
    size_t i = 0;
    while (!lx->eof && !isspace(lx->current)) {
        if (i + 1 < sz) buf[i++] = (char)lx->current;
        else { buf[sz - 1] = '\0'; return -1; }
        lx->current = fgetc(lx->fp);
        if (lx->current == EOF) { lx->eof = 1; break; }
    }
    buf[i] = '\0';
    return (i > 0) ? 1 : 0;
}

/* ---------- Значение символа (0..35) или -1 ---------- */
static int char_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

/* ---------- Минимальное основание ---------- */
static status_t minimal_base(const char *s, int *out_base)
{
    if (!s || !*s || !out_base) return ST_ERR_BAD_CHAR;
    int max_digit = -1;
    for (const char *p = s; *p; ++p) {
        int v = char_value(*p);
        if (v < 0) return ST_ERR_BAD_CHAR;
        if (v > max_digit) max_digit = v;
    }
    if (max_digit < 0) return ST_ERR_BAD_CHAR;
    int base = max_digit + 1;
    if (base < 2) base = 2;
    if (base > 36) return ST_ERR_BAD_CHAR;
    *out_base = base;
    return ST_OK;
}

/* ---------- Нормализация: убрать ведущие нули, привести к верхнему регистру ---------- */
static void normalize(const char *src, char *dst, size_t sz)
{
    const char *p = src;
    while (*p == '0' && *(p + 1) != '\0') ++p;
    size_t i = 0;
    for (; *p && i + 1 < sz; ++p) {
        dst[i++] = (char)toupper((unsigned char)*p);
    }
    dst[i] = '\0';
    if (i == 0) { dst[0] = '0'; dst[1] = '\0'; }
}

/* ---------- Преобразование строки в unsigned long long ---------- */
static status_t to_ull(const char *s, int base, unsigned long long *out)
{
    unsigned long long v = 0;
    for (const char *p = s; *p; ++p) {
        int d = char_value(*p);
        if (d < 0 || d >= base) return ST_ERR_BAD_CHAR;
        unsigned long long nv = v * (unsigned long long)base + (unsigned long long)d;
        if (nv < v) return ST_ERR_OVERFLOW;
        v = nv;
    }
    *out = v;
    return ST_OK;
}

/* ---------- Основная обработка ---------- */
static status_t process(const char *input, const char *output)
{
    FILE *fi = fopen(input, "r");
    if (!fi) return ST_ERR_OPEN_IN;
    FILE *fo = fopen(output, "w");
    if (!fo) { fclose(fi); return ST_ERR_OPEN_OUT; }

    lexer_t lx;
    lexer_init(&lx, fi);

    char buf[LEX_MAX];
    status_t st = ST_OK;
    int g;

    while ((g = lexer_next(&lx, buf, sizeof(buf))) == 1) {
        int base = 0;
        status_t st1 = minimal_base(buf, &base);
        if (st1 != ST_OK) {
            fprintf(fo, "%s\tОШИБКА\n", buf);
            continue;
        }
        char norm[LEX_MAX];
        normalize(buf, norm, sizeof(norm));

        unsigned long long val = 0;
        status_t st2 = to_ull(norm, base, &val);
        if (st2 != ST_OK) {
            fprintf(fo, "%s\t%d\tОШИБКА\n", norm, base);
            continue;
        }
        if (fprintf(fo, "%s\t%d\t%llu\n", norm, base, val) < 0) {
            st = ST_ERR_WRITE; break;
        }
    }
    if (g == -1) st = ST_ERR_LEX_LONG;
    if (ferror(fi) && st == ST_OK) st = ST_ERR_READ;

    if (fclose(fi) != 0 && st == ST_OK) st = ST_ERR_READ;
    if (fclose(fo) != 0 && st == ST_OK) st = ST_ERR_WRITE;
    return st;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st, const char *arg)
{
    switch (st) {
        case ST_OK:            break;
        case ST_ERR_ARGC:      fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_OPEN_IN:   fprintf(stderr, "Ошибка: не удалось открыть '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_OPEN_OUT:  fprintf(stderr, "Ошибка: не удалось открыть '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_READ:      fprintf(stderr, "Ошибка: чтение.\n"); break;
        case ST_ERR_WRITE:     fprintf(stderr, "Ошибка: запись.\n"); break;
        case ST_ERR_LEX_LONG:  fprintf(stderr, "Ошибка: лексема слишком длинная.\n"); break;
        case ST_ERR_BAD_CHAR:  fprintf(stderr, "Ошибка: недопустимый символ.\n"); break;
        case ST_ERR_OVERFLOW:  fprintf(stderr, "Ошибка: переполнение.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <входной> <выходной>\n", argv[0]);
        return (int)ST_ERR_ARGC;
    }
    status_t st = process(argv[1], argv[2]);
    if (st != ST_OK) print_status(st, NULL);
    else printf("Готово: '%s' -> '%s'\n", argv[1], argv[2]);
    return (int)st;
}
