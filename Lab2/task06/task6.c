/* ============================================================
 * Задание 6 — overfscanf и overscanf
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task6 task6.c
 * Запуск:     ./task6
 *
 * Реализованы функции overfscanf(FILE*, fmt, ...) и
 * overscanf(const char *src, fmt, ...) с дополнительными флагами:
 *   %Ro  — int в римской записи
 *   %Zr  — unsigned int в представлении Зеккендорфа
 *   %Cv  — int в произвольной системе (2..36), строчные
 *   %CV  — int в произвольной системе (2..36), прописные
 *
 * Валидация входных данных не требуется (по условию).
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_EOF,
    ST_ERR_FMT
} status_t;

/* ============================================================
 *  Чтение римского числа
 * ============================================================ */
static int roman_char_value(char c)
{
    switch (c) {
        case 'I': return 1;
        case 'V': return 5;
        case 'X': return 10;
        case 'L': return 50;
        case 'C': return 100;
        case 'D': return 500;
        case 'M': return 1000;
        default:  return 0;
    }
}

/* ============================================================
 *  Чтение int в римской записи
 * ============================================================ */
static status_t read_roman(int (*getc_fn)(void *ctx), void *ctx,
                           int *out, int *first_c)
{
    int c = *first_c;
    if (c == EOF) return ST_ERR_EOF;
    /* skip whitespace */
    while (c != EOF && isspace((unsigned char)c)) c = getc_fn(ctx);
    if (c == EOF) return ST_ERR_EOF;
    int total = 0;
    int prev = 0;
    int count = 0;

    while (c != EOF && roman_char_value(c) > 0) {
        int v = roman_char_value(c);
        if (prev > 0 && prev < v) {
            /* soustraction : IV = 4, IX = 9, etc. */
            total -= prev;
            total += v - prev;
        } else {
            total += v;
        }
        prev = v;
        ++count;
        c = getc_fn(ctx);
    }

    if (count == 0) return ST_ERR_PARSE;

    *out = total;
    *first_c = c;
    return ST_OK;
}

/* ============================================================
 *  Чтение Zeckendorf
 * ============================================================ */
static status_t read_zeckendorf(int (*getc_fn)(void *ctx), void *ctx,
                                unsigned int *out, int *first_c)
{
    int c = *first_c;
    if (c == EOF) return ST_ERR_EOF;
    /* skip whitespace */
    while (c != EOF && isspace((unsigned char)c)) c = getc_fn(ctx);
    if (c == EOF) return ST_ERR_EOF;

    /* Lire les caractères binaires dans un buffer */
    char bits[64];
    int nbits = 0;
    while ((c == '0' || c == '1') && nbits < 63) {
        bits[nbits++] = (char)c;
        c = getc_fn(ctx);
    }
    bits[nbits] = '\0';

    if (nbits == 0) return ST_ERR_PARSE;

    /* Le dernier bit doit être '1' (marqueur de fin) */
    if (bits[nbits - 1] != '1') return ST_ERR_PARSE;

    /* Les bits de Fibonacci sont bits[0..nbits-2] */
    unsigned int total = 0;
    unsigned int f1 = 1, f2 = 2;
    for (int i = 0; i < nbits - 1; ++i) {
        if (bits[i] == '1') total += f1;
        unsigned int f3 = f1 + f2;
        f1 = f2;
        f2 = f3;
    }

    *out = total;
    *first_c = c;
    return ST_OK;
}
/* ============================================================
 *  Чтение int в произвольной системе
 * ============================================================ */
static status_t read_base_int(int (*getc_fn)(void *ctx), void *ctx,
                              int base, int *out, int *first_c)
{
    int c = *first_c;
    if (c == EOF) return ST_ERR_EOF;

    int neg = 0;
    if (c == '-') { neg = 1; c = getc_fn(ctx); }

    long long v = 0;
    int count = 0;
    while (c != EOF) {
        int d = -1;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
        if (d < 0 || d >= base) break;
        v = v * base + d;
        ++count;
        c = getc_fn(ctx);
    }

    if (count == 0) return ST_ERR_PARSE;

    *out = (int)(neg ? -v : v);
    *first_c = c;
    return ST_OK;
}

/* ============================================================
 *  Contexte de lecture : fichier
 * ============================================================ */
static int file_getc(void *ctx) { return fgetc((FILE *)ctx); }

/* ============================================================
 *  Contexte de lecture : chaîne
 * ============================================================ */
typedef struct {
    const char *p;
} str_ctx_t;

static int str_getc(void *ctx)
{
    str_ctx_t *s = (str_ctx_t *)ctx;
    if (*s->p == '\0') return EOF;
    return (unsigned char)*s->p++;
}

/* ============================================================
 *  Moteur commun : lit le format et remplit les pointeurs
 * ============================================================ */
static status_t scan_engine(int (*getc_fn)(void *ctx), void *ctx,
                            const char *fmt, va_list *ap, int *out_count)
{
    if (!fmt) return ST_ERR_FMT;

    status_t st = ST_OK;
    const char *p = fmt;
    int assigned = 0;
    int first_c = getc_fn(ctx);

    while (*p) {
        if (*p != '%') {
            /* littéral : on attend le même caractère (avec skip ws) */
            if (isspace((unsigned char)*p)) {
                while (*p && isspace((unsigned char)*p)) ++p;
                continue;
            }
            if (first_c == EOF) { st = ST_ERR_EOF; break; }
            if (first_c != (unsigned char)*p) { st = ST_ERR_FMT; break; }
            first_c = getc_fn(ctx);
            ++p;
            continue;
        }

        ++p;  /* passer le '%' */

        if (*p == 'R' && p[1] == 'o') {
            int *out = va_arg(*ap, int *);
            if (!out) { st = ST_ERR_FMT; break; }
            st = read_roman(getc_fn, ctx, out, &first_c);
            if (st != ST_OK) break;
            ++assigned;
            p += 2;
        }
        else if (*p == 'Z' && p[1] == 'r') {
            unsigned int *out = va_arg(*ap, unsigned int *);
            if (!out) { st = ST_ERR_FMT; break; }
            st = read_zeckendorf(getc_fn, ctx, out, &first_c);
            if (st != ST_OK) break;
            ++assigned;
            p += 2;
        }
        else if (*p == 'C' && p[1] == 'v') {
            int *out = va_arg(*ap, int *);
            int base = va_arg(*ap, int);
            if (!out) { st = ST_ERR_FMT; break; }
            if (base < 2 || base > 36) base = 10;
            st = read_base_int(getc_fn, ctx, base, out, &first_c);
            if (st != ST_OK) break;
            ++assigned;
            p += 2;
        }
        else if (*p == 'C' && p[1] == 'V') {
            int *out = va_arg(*ap, int *);
            int base = va_arg(*ap, int);
            if (!out) { st = ST_ERR_FMT; break; }
            if (base < 2 || base > 36) base = 10;
            st = read_base_int(getc_fn, ctx, base, out, &first_c);
            if (st != ST_OK) break;
            ++assigned;
            p += 2;
        }
        else if (*p == '%') {
            if (first_c != '%') { st = ST_ERR_FMT; break; }
            first_c = getc_fn(ctx);
            ++p;
        }
        else {
            st = ST_ERR_FMT;
            break;
        }
    }

    if (out_count) *out_count = assigned;
    return st;
}

/* ============================================================
 *  overfscanf
 * ============================================================ */
static int overfscanf(FILE *f, const char *fmt, ...)
{
    if (!f || !fmt) return -1;
    va_list ap;
    va_start(ap, fmt);
    int count = 0;
    status_t st = scan_engine(file_getc, f, fmt, &ap, &count);
    va_end(ap);
    if (st == ST_OK || count > 0) return count;
    return (st == ST_ERR_EOF) ? EOF : 0;
}

/* ============================================================
 *  overscanf
 * ============================================================ */
static int overscanf(const char *src, const char *fmt, ...)
{
    if (!src || !fmt) return -1;
    str_ctx_t ctx = { src };
    va_list ap;
    va_start(ap, fmt);
    int count = 0;
    status_t st = scan_engine(str_getc, &ctx, fmt, &ap, &count);
    va_end(ap);
    if (st == ST_OK || count > 0) return count;
    return (st == ST_ERR_EOF) ? EOF : 0;
}

/* ============================================================
 *  Демонстрация
 * ============================================================ */
int main(void)
{
    printf("=== Демонстрация overfscanf ===\n");

    /* Test 1 : lire depuis un fichier temporaire */
    {
        FILE *f = tmpfile();
        if (!f) { perror("tmpfile"); return 1; }
        fputs("MCMXCIV\n010011\nff\nFF\n", f);
        rewind(f);

        int v1 = 0;
        int n1 = overfscanf(f, "%Ro", &v1);
        printf("Roman MCMXCIV = %d (n=%d)\n", v1, n1);

        unsigned int v2 = 0;
        int n2 = overfscanf(f, "%Zr", &v2);
        printf("Zeckendorf 010011 = %u (n=%d)\n", v2, n2);

        int v3 = 0;
        int n3 = overfscanf(f, "%Cv", &v3, 16);
        printf("Hex 'ff' (Cv) = %d (n=%d)\n", v3, n3);

        int v4 = 0;
        int n4 = overfscanf(f, "%CV", &v4, 16);
        printf("Hex 'FF' (CV) = %d (n=%d)\n", v4, n4);

        fclose(f);
    }

    printf("\n=== Демонстрация overscanf ===\n");

    {
        int v = 0;
        int n = overscanf("MCMXCIV", "%Ro", &v);
        printf("Roman MCMXCIV = %d (n=%d)\n", v, n);
    }
    {
        unsigned int v = 0;
        int n = overscanf("010011", "%Zr", &v);
        printf("Zeckendorf 010011 = %u (n=%d)\n", v, n);
    }
    {
        int v = 0;
        int n = overscanf("ff", "%Cv", &v, 16);
        printf("Hex 'ff' (Cv) = %d (n=%d)\n", v, n);
    }

    return 0;
}
