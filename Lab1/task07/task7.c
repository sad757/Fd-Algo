/* ============================================================
 * Задание 7 — Продвинутая обработка файлов (-r и -a)
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task7 task7.c
 * Запуск:
 *   ./task7 -r <файл1> <файл2> <выходной>
 *   ./task7 -a <входной> <выходной>
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define LEX_MAX 1024

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_OPEN_IN,
    ST_ERR_OPEN_OUT,
    ST_ERR_READ,
    ST_ERR_WRITE,
    ST_ERR_LEX_LONG
} status_t;

/* ---------- Разбор флага ---------- */
static status_t parse_flag(const char *arg, char *out)
{
    if (!arg || !out) return ST_ERR_FLAG;
    if (arg[0] != '-' && arg[0] != '/') return ST_ERR_FLAG;
    if (arg[1] == '\0' || arg[2] != '\0') return ST_ERR_FLAG;
    char c = (char)tolower((unsigned char)arg[1]);
    if (strchr("ra", c) == NULL) return ST_ERR_FLAG;
    *out = c;
    return ST_OK;
}

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

/* Возвращает: 1 — лексема прочитана, 0 — конец файла, -1 — слишком длинная */
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

/* ============================================================
 *  Флаг -r: чередование лексем
 * ============================================================ */
static status_t do_r(const char *f1, const char *f2, const char *out)
{
    FILE *in1 = fopen(f1, "r");
    if (!in1) return ST_ERR_OPEN_IN;
    FILE *in2 = fopen(f2, "r");
    if (!in2) { fclose(in1); return ST_ERR_OPEN_IN; }
    FILE *fo = fopen(out, "w");
    if (!fo) { fclose(in1); fclose(in2); return ST_ERR_OPEN_OUT; }

    lexer_t l1, l2;
    lexer_init(&l1, in1);
    lexer_init(&l2, in2);

    char buf[LEX_MAX];
    int first = 1;
    status_t st = ST_OK;

    while (1) {
        int got1 = lexer_next(&l1, buf, sizeof(buf));
        if (got1 == -1) { st = ST_ERR_LEX_LONG; break; }
        if (got1 == 0) break;
        if (!first) { if (fputc(' ', fo) == EOF) { st = ST_ERR_WRITE; break; } }
        if (fputs(buf, fo) == EOF) { st = ST_ERR_WRITE; break; }
        first = 0;

        int got2 = lexer_next(&l2, buf, sizeof(buf));
        if (got2 == -1) { st = ST_ERR_LEX_LONG; break; }
        if (got2 == 0) break;
        if (!first) { if (fputc(' ', fo) == EOF) { st = ST_ERR_WRITE; break; } }
        if (fputs(buf, fo) == EOF) { st = ST_ERR_WRITE; break; }
        first = 0;
    }

    /* дописать остаток первого файла */
    if (st == ST_OK && !l1.eof) {
        int g;
        while ((g = lexer_next(&l1, buf, sizeof(buf))) == 1) {
            if (!first) { if (fputc(' ', fo) == EOF) { st = ST_ERR_WRITE; break; } }
            if (fputs(buf, fo) == EOF) { st = ST_ERR_WRITE; break; }
            first = 0;
        }
        if (g == -1) st = ST_ERR_LEX_LONG;
    }
    /* дописать остаток второго файла */
    if (st == ST_OK && !l2.eof) {
        int g;
        while ((g = lexer_next(&l2, buf, sizeof(buf))) == 1) {
            if (!first) { if (fputc(' ', fo) == EOF) { st = ST_ERR_WRITE; break; } }
            if (fputs(buf, fo) == EOF) { st = ST_ERR_WRITE; break; }
            first = 0;
        }
        if (g == -1) st = ST_ERR_LEX_LONG;
    }

    if (st == ST_OK) { if (fputc('\n', fo) == EOF) st = ST_ERR_WRITE; }
    if (ferror(in1) || ferror(in2)) st = ST_ERR_READ;

    if (fclose(in1) != 0 && st == ST_OK) st = ST_ERR_READ;
    if (fclose(in2) != 0 && st == ST_OK) st = ST_ERR_READ;
    if (fclose(fo)  != 0 && st == ST_OK) st = ST_ERR_WRITE;
    return st;
}

/* ============================================================
 *  Флаг -a: условные преобразования лексем
 * ============================================================ */

/* Перевод строки в base 4, символ за символом */
static status_t to_base4(const char *src, FILE *fo)
{
    for (const char *p = src; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        char tmp[9];
        int n = 0;
        if (c == 0) { tmp[n++] = '0'; }
        else {
            char rev[8]; int rn = 0;
            unsigned char v = c;
            while (v > 0) { rev[rn++] = (char)('0' + (v & 0x3)); v >>= 2; }
            while (rn > 0) tmp[n++] = rev[--rn];
        }
        tmp[n] = '\0';
        if (fputs(tmp, fo) == EOF) return ST_ERR_WRITE;
    }
    return ST_OK;
}

/* Перевод строки в base 8, символ за символом */
static status_t to_base8(const char *src, FILE *fo)
{
    for (const char *p = src; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        char tmp[5];
        int n = 0;
        if (c == 0) { tmp[n++] = '0'; }
        else {
            char rev[4]; int rn = 0;
            unsigned char v = c;
            while (v > 0) { rev[rn++] = (char)('0' + (v & 0x7)); v >>= 3; }
            while (rn > 0) tmp[n++] = rev[--rn];
        }
        tmp[n] = '\0';
        if (fputs(tmp, fo) == EOF) return ST_ERR_WRITE;
    }
    return ST_OK;
}

static void to_lower(char *s)
{
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

static status_t do_a(const char *input, const char *out)
{
    FILE *fi = fopen(input, "r");
    if (!fi) return ST_ERR_OPEN_IN;
    FILE *fo = fopen(out, "w");
    if (!fo) { fclose(fi); return ST_ERR_OPEN_OUT; }

    lexer_t lx;
    lexer_init(&lx, fi);

    char buf[LEX_MAX];
    int pos = 0;
    int first = 1;
    status_t st = ST_OK;

    while (1) {
        int g = lexer_next(&lx, buf, sizeof(buf));
        if (g == -1) { st = ST_ERR_LEX_LONG; break; }
        if (g == 0) break;
        ++pos;

        if (!first) { if (fputc(' ', fo) == EOF) { st = ST_ERR_WRITE; break; } }
        first = 0;

        if (pos % 10 == 0) {
            /* приоритет: каждая 10-я */
            to_lower(buf);
            if ((st = to_base4(buf, fo)) != ST_OK) break;
        } else if (pos % 2 == 0) {
            to_lower(buf);
            if (fputs(buf, fo) == EOF) { st = ST_ERR_WRITE; break; }
        } else if (pos % 5 == 0) {
            if ((st = to_base8(buf, fo)) != ST_OK) break;
        } else {
            if (fputs(buf, fo) == EOF) { st = ST_ERR_WRITE; break; }
        }
    }

    if (st == ST_OK) { if (fputc('\n', fo) == EOF) st = ST_ERR_WRITE; }
    if (ferror(fi)) st = ST_ERR_READ;

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
        case ST_ERR_FLAG:      fprintf(stderr, "Ошибка: неверный флаг '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_OPEN_IN:   fprintf(stderr, "Ошибка: не удалось открыть '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_OPEN_OUT:  fprintf(stderr, "Ошибка: не удалось открыть '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_READ:      fprintf(stderr, "Ошибка: чтение.\n"); break;
        case ST_ERR_WRITE:     fprintf(stderr, "Ошибка: запись.\n"); break;
        case ST_ERR_LEX_LONG:  fprintf(stderr, "Ошибка: лексема слишком длинная (>%d).\n", LEX_MAX - 1); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr,
            "Использование:\n"
            "  %s -r <файл1> <файл2> <выходной>\n"
            "  %s -a <входной> <выходной>\n",
            argv[0], argv[0]);
        return (int)ST_ERR_ARGC;
    }

    char flag = '\0';
    status_t st = parse_flag(argv[1], &flag);
    if (st != ST_OK) { print_status(st, argv[1]); return (int)st; }

    if (flag == 'r') {
        if (argc != 5) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        st = do_r(argv[2], argv[3], argv[4]);
        if (st != ST_OK) print_status(st, NULL);
        else printf("Готово: '%s' + '%s' -> '%s'\n", argv[2], argv[3], argv[4]);
    } else {
        if (argc != 4) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        st = do_a(argv[2], argv[3]);
        if (st != ST_OK) print_status(st, NULL);
        else printf("Готово: '%s' -> '%s'\n", argv[2], argv[3]);
    }

    return (int)st;
}
