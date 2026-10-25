/* ============================================================
 * Задание 5 — overfprintf и oversprintf
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task5 task5.c
 * Запуск:     ./task5
 *
 * Реализованы функции overfprintf(FILE*, fmt, ...) и
 * oversprintf(char*, fmt, ...) с дополнительными флагами:
 *   %Ro  — int в римской записи
 *   %Zr  — unsigned int в представлении Зеккендорфа
 *   %Cv  — int в произвольной системе (2..36), строчные
 *   %CV  — int в произвольной системе (2..36), прописные
 *   %to  — char* в системе base → int (база 10)
 *   %TO  — то же, но буквы в верхнем регистре
 *   %mi  — дамп памяти int (4 байта, бинарно)
 *   %mu  — дамп памяти unsigned int
 *   %md  — дамп памяти double (8 байт)
 *   %mf  — дамп памяти float (4 байта)
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#define BUF_MAX 4096

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_NOMEM,
    ST_ERR_FMT,
    ST_ERR_ROMAN
} status_t;

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
 *  %Ro — римская запись
 * ============================================================ */
static status_t to_roman(int v, char *out, size_t sz)
{
    if (!out || sz == 0) return ST_ERR_PARSE;
    if (v <= 0 || v >= 4000) return ST_ERR_ROMAN;

    static const struct { int val; const char *sym; } table[] = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
        {100, "C"},  {90, "XC"},  {50, "L"},  {40, "XL"},
        {10, "X"},   {9, "IX"},   {5, "V"},   {4, "IV"},
        {1, "I"}
    };
    size_t k = 0;
    for (size_t i = 0; i < sizeof(table)/sizeof(table[0]); ++i) {
        while (v >= table[i].val) {
            const char *sym = table[i].sym;
            while (*sym) {
                if (k + 1 >= sz) return ST_ERR_RANGE;
                out[k++] = *sym++;
            }
            v -= table[i].val;
        }
    }
    out[k] = '\0';
    return ST_OK;
}

/* ============================================================
 *  %Zr — представление Зеккендорфа
 *  Биты Фибоначчи от младшего к старшему + "1" в конце.
 * ============================================================ */
static status_t to_zeckendorf(unsigned int v, char *out, size_t sz)
{
    if (!out || sz == 0) return ST_ERR_PARSE;

    /* 1. Построить числа Фибоначчи до v */
    unsigned int fib[64];
    int n = 0;
    fib[0] = 1; fib[1] = 2;
    n = 2;
    while (fib[n-1] <= v && n < 64) {
        fib[n] = fib[n-1] + fib[n-2];
        if (fib[n] < fib[n-1]) break;  /* overflow */
        ++n;
    }

    /* 2. Жадный алгоритм: от большего к меньшему */
    char bits[64];
    int nbits = 0;
    int idx = n - 1;
    while (idx >= 0 && fib[idx] > v) --idx;
    if (idx < 0) {
        /* v == 0 */
        if (sz < 2) return ST_ERR_RANGE;
        out[0] = '1';
        out[1] = '\0';
        return ST_OK;
    }

    /* on va du plus grand au plus petit */
    unsigned int rem = v;
    for (int i = idx; i >= 0; --i) {
        if (fib[i] <= rem) {
            bits[nbits++] = '1';
            rem -= fib[i];
        } else {
            bits[nbits++] = '0';
        }
    }

    /* 3. Инвертировать порядок (младший → старший) + "1" в конце */
    size_t k = 0;
    for (int i = nbits - 1; i >= 0; --i) {
        if (k + 1 >= sz) return ST_ERR_RANGE;
        out[k++] = bits[i];
    }
    if (k + 1 >= sz) return ST_ERR_RANGE;
    out[k++] = '1';
    out[k] = '\0';
    return ST_OK;
}

/* ============================================================
 *  %Cv / %CV — int в произвольной системе (2..36)
 * ============================================================ */
static status_t to_base(long long v, int base, int uppercase,
                        char *out, size_t sz)
{
    if (!out || sz == 0) return ST_ERR_PARSE;
    if (base < 2 || base > 36) base = 10;

    char tmp[80];
    int n = 0;
    unsigned long long uv;
    int neg = (v < 0);
    if (neg) uv = (unsigned long long)(-(v + 1)) + 1ULL;
    else     uv = (unsigned long long)v;

    if (uv == 0) { tmp[n++] = '0'; }
    else {
        while (uv > 0) {
            int d = (int)(uv % (unsigned long long)base);
            char c;
            if (d < 10) c = (char)('0' + d);
            else        c = (char)((uppercase ? 'A' : 'a') + d - 10);
            tmp[n++] = c;
            uv /= (unsigned long long)base;
        }
    }

    size_t k = 0;
    if (neg && k + 1 < sz) out[k++] = '-';
    while (n > 0 && k + 1 < sz) out[k++] = tmp[--n];
    out[k] = '\0';
    return ST_OK;
}

/* ============================================================
 *  %to / %TO — char* en base → int en base 10
 * ============================================================ */
static status_t from_base_to_dec(const char *s, int base,
                                 char *out, size_t sz)
{
    if (!s || !out || sz == 0) return ST_ERR_PARSE;
    if (base < 2 || base > 36) base = 10;

    long long v = 0;
    int neg = 0;
    const char *p = s;
    if (*p == '-') { neg = 1; ++p; }

    for (; *p; ++p) {
        int d;
        if (*p >= '0' && *p <= '9')      d = *p - '0';
        else if (*p >= 'a' && *p <= 'z') d = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'Z') d = *p - 'A' + 10;
        else return ST_ERR_PARSE;
        if (d >= base) return ST_ERR_PARSE;
        v = v * base + d;
    }
    if (neg) v = -v;

    /* Записать в out */
    char tmp[80];
    int n = 0;
    unsigned long long uv;
    int neg2 = (v < 0);
    if (neg2) uv = (unsigned long long)(-(v + 1)) + 1ULL;
    else      uv = (unsigned long long)v;
    if (uv == 0) tmp[n++] = '0';
    else {
        while (uv > 0) {
            tmp[n++] = (char)('0' + (uv % 10));
            uv /= 10;
        }
    }
    size_t k = 0;
    if (neg2 && k + 1 < sz) out[k++] = '-';
    while (n > 0 && k + 1 < sz) out[k++] = tmp[--n];
    out[k] = '\0';
    return ST_OK;
}

/* ============================================================
 *  %mi, %mu, %md, %mf — дамп памяти
 * ============================================================ */
static void dump_bytes(const unsigned char *bytes, size_t n,
                       char *out, size_t sz)
{
    size_t k = 0;
    for (size_t i = 0; i < n; ++i) {
        for (int b = 7; b >= 0; --b) {
            if (k + 1 >= sz) { out[k] = '\0'; return; }
            out[k++] = ((bytes[i] >> b) & 1) ? '1' : '0';
        }
        if (i + 1 < n && k + 1 < sz) out[k++] = ' ';
    }
    out[k] = '\0';
}

/* ============================================================
 *  overfprintf
 * ============================================================ */
static status_t overfprintf(FILE *f, const char *fmt, ...)
{
    if (!f || !fmt) return ST_ERR_PARSE;

    va_list ap;
    va_start(ap, fmt);

    status_t st = ST_OK;
    const char *p = fmt;
    while (*p) {
        if (*p != '%') {
            if (fputc(*p, f) == EOF) { st = ST_ERR_PARSE; break; }
            ++p;
            continue;
        }

        ++p;  /* passer le '%' */

        /* ---- %Ro ---- */
        if (p[0] == 'R' && p[1] == 'o') {
            int v = va_arg(ap, int);
            char buf[64];
            if ((st = to_roman(v, buf, sizeof(buf))) != ST_OK) break;
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %Zr ---- */
        else if (p[0] == 'Z' && p[1] == 'r') {
            unsigned int v = va_arg(ap, unsigned int);
            char buf[128];
            if ((st = to_zeckendorf(v, buf, sizeof(buf))) != ST_OK) break;
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %Cv ---- */
        else if (p[0] == 'C' && p[1] == 'v') {
            int v = va_arg(ap, int);
            int base = va_arg(ap, int);
            char buf[80];
            if ((st = to_base(v, base, 0, buf, sizeof(buf))) != ST_OK) break;
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %CV ---- */
        else if (p[0] == 'C' && p[1] == 'V') {
            int v = va_arg(ap, int);
            int base = va_arg(ap, int);
            char buf[80];
            if ((st = to_base(v, base, 1, buf, sizeof(buf))) != ST_OK) break;
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %to ---- */
        else if (p[0] == 't' && p[1] == 'o') {
            const char *s = va_arg(ap, const char *);
            int base = va_arg(ap, int);
            char buf[80];
            if ((st = from_base_to_dec(s, base, buf, sizeof(buf))) != ST_OK) break;
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %TO ---- */
        else if (p[0] == 'T' && p[1] == 'O') {
            const char *s = va_arg(ap, const char *);
            int base = va_arg(ap, int);
            char buf[80];
            if ((st = from_base_to_dec(s, base, buf, sizeof(buf))) != ST_OK) break;
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %mi ---- */
        else if (p[0] == 'm' && p[1] == 'i') {
            int v = va_arg(ap, int);
            char buf[128];
            dump_bytes((const unsigned char *)&v, sizeof(v), buf, sizeof(buf));
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %mu ---- */
        else if (p[0] == 'm' && p[1] == 'u') {
            unsigned int v = va_arg(ap, unsigned int);
            char buf[128];
            dump_bytes((const unsigned char *)&v, sizeof(v), buf, sizeof(buf));
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %md ---- */
        else if (p[0] == 'm' && p[1] == 'd') {
            double v = va_arg(ap, double);
            char buf[256];
            dump_bytes((const unsigned char *)&v, sizeof(v), buf, sizeof(buf));
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %mf ---- */
        else if (p[0] == 'm' && p[1] == 'f') {
            /* En C, float est promu en double dans va_arg.
               On récupère un double, on le cast en float, puis dump. */
            double dv = va_arg(ap, double);
            float fv = (float)dv;
            char buf[128];
            dump_bytes((const unsigned char *)&fv, sizeof(fv), buf, sizeof(buf));
            if (fputs(buf, f) == EOF) { st = ST_ERR_PARSE; break; }
            p += 2;
        }
        /* ---- %% (échappement) ---- */
        else if (p[0] == '%') {
            if (fputc('%', f) == EOF) { st = ST_ERR_PARSE; break; }
            ++p;
        }
        /* ---- inconnu ---- */
        else {
            st = ST_ERR_FMT;
            break;
        }
    }

    va_end(ap);
    return st;
}

/* ============================================================
 *  oversprintf (buffer fixe)
 * ============================================================ */
static status_t oversprintf(char *buf, size_t sz, const char *fmt, ...)
{
    if (!buf || !fmt || sz == 0) return ST_ERR_PARSE;

    /* On utilise un FILE* temporaire via fmemopen (POSIX) — mais on
       ne peut pas l'utiliser sous -std=c99 strict. On réimplémente
       overfprintf avec un buffer. */
    /* Simplification : on utilise un buffer intermédiaire via snprintf
       ne marche pas car nos flags ne sont pas standards.
       On va donc dupliquer le code de overfprintf mais écrire dans buf. */

    va_list ap;
    va_start(ap, fmt);

    status_t st = ST_OK;
    const char *p = fmt;
    size_t k = 0;
    while (*p) {
        if (*p != '%') {
            if (k + 1 >= sz) { st = ST_ERR_RANGE; break; }
            buf[k++] = *p++;
            continue;
        }
        ++p;

        char tmp[256];
        const char *arg_str = NULL;

        if (p[0] == 'R' && p[1] == 'o') {
            int v = va_arg(ap, int);
            st = to_roman(v, tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'Z' && p[1] == 'r') {
            unsigned int v = va_arg(ap, unsigned int);
            st = to_zeckendorf(v, tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'C' && p[1] == 'v') {
            int v = va_arg(ap, int);
            int base = va_arg(ap, int);
            st = to_base(v, base, 0, tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'C' && p[1] == 'V') {
            int v = va_arg(ap, int);
            int base = va_arg(ap, int);
            st = to_base(v, base, 1, tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 't' && p[1] == 'o') {
            const char *s = va_arg(ap, const char *);
            int base = va_arg(ap, int);
            st = from_base_to_dec(s, base, tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'T' && p[1] == 'O') {
            const char *s = va_arg(ap, const char *);
            int base = va_arg(ap, int);
            st = from_base_to_dec(s, base, tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'm' && p[1] == 'i') {
            int v = va_arg(ap, int);
            dump_bytes((const unsigned char *)&v, sizeof(v), tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'm' && p[1] == 'u') {
            unsigned int v = va_arg(ap, unsigned int);
            dump_bytes((const unsigned char *)&v, sizeof(v), tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'm' && p[1] == 'd') {
            double v = va_arg(ap, double);
            dump_bytes((const unsigned char *)&v, sizeof(v), tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == 'm' && p[1] == 'f') {
            double dv = va_arg(ap, double);
            float fv = (float)dv;
            dump_bytes((const unsigned char *)&fv, sizeof(fv), tmp, sizeof(tmp));
            arg_str = tmp;
            p += 2;
        } else if (p[0] == '%') {
            if (k + 1 >= sz) { st = ST_ERR_RANGE; break; }
            buf[k++] = '%';
            ++p;
            continue;
        } else {
            st = ST_ERR_FMT;
            break;
        }

        if (st != ST_OK) break;
        size_t len = my_strlen(arg_str);
        for (size_t i = 0; i < len; ++i) {
            if (k + 1 >= sz) { st = ST_ERR_RANGE; break; }
            buf[k++] = arg_str[i];
        }
        if (st != ST_OK) break;
    }

    if (st == ST_OK || st == ST_ERR_RANGE) {
        buf[k] = '\0';
    }
    va_end(ap);
    return st;
}

/* ============================================================
 *  Демонстрация
 * ============================================================ */
int main(void)
{
    char buf[BUF_MAX];

    printf("=== Демонстрация overfprintf ===\n");
    printf("Roman 1994 : ");
    overfprintf(stdout, "%Ro\n", 1994);
    printf("Roman 2024 : ");
    overfprintf(stdout, "%Ro\n", 2024);

    printf("Zeckendorf 10 : ");
    overfprintf(stdout, "%Zr\n", 10);
    printf("Zeckendorf 100 : ");
    overfprintf(stdout, "%Zr\n", 100);

    printf("Hex 255 (Cv) : ");
    overfprintf(stdout, "%Cv\n", 255, 16);
    printf("Hex 255 (CV) : ");
    overfprintf(stdout, "%CV\n", 255, 16);
    printf("Bin 42 (Cv) : ");
    overfprintf(stdout, "%Cv\n", 42, 2);

    printf("From hex 'FF' to dec : ");
    overfprintf(stdout, "%to\n", "FF", 16);
    printf("From bin '1010' to dec : ");
    overfprintf(stdout, "%to\n", "1010", 2);

    printf("Dump int 42 (mi) : ");
    overfprintf(stdout, "%mi\n", 42);
    printf("Dump unsigned 42 (mu) : ");
    overfprintf(stdout, "%mu\n", (unsigned)42);
    printf("Dump double 3.14 (md) : ");
    overfprintf(stdout, "%md\n", 3.14);
    printf("Dump float 3.14 (mf) : ");
    overfprintf(stdout, "%mf\n", (double)3.14f);

    printf("\n=== Демонстрация oversprintf ===\n");
    oversprintf(buf, sizeof(buf), "Roman 1994 = %Ro, hex 255 = %CV", 1994, 255, 16);
    printf("buf = \"%s\"\n", buf);

    return 0;
}
