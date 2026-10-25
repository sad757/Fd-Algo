/* ============================================================
 * Задание 9 — Дроби в системе счисления с заданным основанием
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task9 task9.c -lm
 * Запуск:     ./task9 <base> <d1> <d2> ... <dk>
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_NOMEM,
    ST_ERR_EMPTY
} status_t;

/* ============================================================
 *  Восстановить p/q из double d ∈ (0,1)
 *  Возвращает 1 si réussi, 0 sinon.
 * ============================================================ */
static int reconstruct_fraction(double d, unsigned long long *p_out,
                                unsigned long long *q_out)
{
    if (d <= 0.0 || d >= 1.0) return 0;

    for (int n = 1; n <= 18; ++n) {
        unsigned long long q = 1;
        for (int i = 0; i < n; ++i) q *= 10;
        double scaled = d * (double)q;
        double rounded = round(scaled);
        if (fabs(scaled - rounded) < 1e-6 && rounded > 0.0) {
            *p_out = (unsigned long long)rounded;
            *q_out = q;
            return 1;
        }
    }
    return 0;
}

/* ============================================================
 *  Проверка : имеет ли p/q (irréductible) конечное представление
 *  в base.
 * ============================================================ */
static int finite_in_base(unsigned long long p, unsigned long long q,
                          int base)
{
    /* 1. Simplifier p/q */
    unsigned long long a = p, b = q;
    while (b != 0) {
        unsigned long long t = b;
        b = a % b;
        a = t;
    }
    if (a == 0) return 0;
    p /= a;
    q /= a;

    /* 2. Retirer les facteurs premiers de base de q */
    for (int f = 2; f <= base; ++f) {
        if (base % f == 0) {
            while (q % (unsigned long long)f == 0)
                q /= (unsigned long long)f;
        }
    }
    return (q == 1);
}

/* ============================================================
 *  Проверка одной дроби
 * ============================================================ */
static status_t check_finite(double d, int base, int *out_finite)
{
    if (!out_finite) return ST_ERR_PARSE;
    if (d <= 0.0 || d >= 1.0) return ST_ERR_RANGE;
    if (base < 2 || base > 36) return ST_ERR_RANGE;

    unsigned long long p = 0, q = 1;
    if (!reconstruct_fraction(d, &p, &q)) {
        /* Fallback : considérer périodique */
        *out_finite = 0;
        return ST_OK;
    }
    *out_finite = finite_in_base(p, q, base);
    return ST_OK;
}

/* ============================================================
 *  Вариадическая функция
 * ============================================================ */
static status_t check_all(int base, int count, ...)
{
    if (base < 2 || base > 36 || count <= 0) return ST_ERR_EMPTY;

    va_list ap;
    va_start(ap, count);
    for (int i = 0; i < count; ++i) {
        double d = va_arg(ap, double);
        if (d <= 0.0 || d >= 1.0) {
            printf("  [%d] %.15g : недопустимое значение\n", i + 1, d);
            continue;
        }
        int finite = 0;
        status_t st = check_finite(d, base, &finite);
        if (st != ST_OK) {
            printf("  [%d] %.15g : ошибка\n", i + 1, d);
            continue;
        }
        printf("  [%d] %.15g : %s\n", i + 1, d,
               finite ? "КОНЕЧНОЕ" : "бесконечное (периодическое)");
    }
    va_end(ap);
    return ST_OK;
}

/* ---------- Разбор int ---------- */
static status_t parse_int(const char *s, int *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    if (*s == '-' || *s == '+') return ST_ERR_PARSE;
    long long v = 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return ST_ERR_PARSE;
        v = v * 10 + (*p - '0');
        if (v > 100) return ST_ERR_RANGE;
    }
    *out = (int)v;
    return ST_OK;
}

/* ---------- Разбор double ---------- */
static status_t parse_double(const char *s, double *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    const char *p = s;
    int neg = 0;
    if (*p == '-' || *p == '+') { neg = (*p == '-'); ++p; }
    if (*p == '\0') return ST_ERR_PARSE;

    int dots = 0;
    for (const char *q = p; *q; ++q) {
        if (*q == '.') { ++dots; continue; }
        if (*q < '0' || *q > '9') return ST_ERR_PARSE;
    }
    if (dots > 1) return ST_ERR_PARSE;

    double v = 0.0, frac = 0.0, scale = 1.0;
    int int_part = 1;
    for (const char *q = p; *q; ++q) {
        if (*q == '.') { int_part = 0; continue; }
        int d = *q - '0';
        if (int_part) v = v * 10.0 + d;
        else { scale *= 10.0; frac += d / scale; }
    }
    v += frac;
    if (neg) v = -v;
    *out = v;
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st)
{
    switch (st) {
        case ST_OK:        break;
        case ST_ERR_ARGC:  fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_PARSE: fprintf(stderr, "Ошибка: параметр не число.\n"); break;
        case ST_ERR_RANGE: fprintf(stderr, "Ошибка: основание вне [2..36].\n"); break;
        case ST_ERR_NOMEM: fprintf(stderr, "Ошибка: нет памяти.\n"); break;
        case ST_ERR_EMPTY: fprintf(stderr, "Ошибка: пустой ввод.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <base> <d1> <d2> ... <dk>\n", argv[0]);
        return (int)ST_ERR_ARGC;
    }

    int base = 0;
    status_t st;
    if ((st = parse_int(argv[1], &base)) != ST_OK) {
        print_status(st); return (int)st;
    }
    if (base < 2 || base > 36) {
        print_status(ST_ERR_RANGE); return (int)ST_ERR_RANGE;
    }

    printf("Проверка конечности представления дробей в системе %d :\n", base);

    int count = argc - 2;
    double vals[64];
    if (count > 64) {
        fprintf(stderr, "Слишком много дробей (макс. 64).\n");
        return (int)ST_ERR_ARGC;
    }
    for (int i = 0; i < count; ++i) {
        if ((st = parse_double(argv[2 + i], &vals[i])) != ST_OK) {
            print_status(st); return (int)st;
        }
    }

    switch (count) {
        case 1: st = check_all(base, 1, vals[0]); break;
        case 2: st = check_all(base, 2, vals[0], vals[1]); break;
        case 3: st = check_all(base, 3, vals[0], vals[1], vals[2]); break;
        case 4: st = check_all(base, 4, vals[0], vals[1], vals[2], vals[3]); break;
        case 5: st = check_all(base, 5, vals[0], vals[1], vals[2], vals[3], vals[4]); break;
        case 6: st = check_all(base, 6, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]); break;
        case 7: st = check_all(base, 7, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6]); break;
        case 8: st = check_all(base, 8, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7]); break;
        default:
            fprintf(stderr, "Слишком много дробей (макс. 8 в диспетчере).\n");
            return (int)ST_ERR_ARGC;
    }

    if (st != ST_OK) print_status(st);
    return (int)st;
}
