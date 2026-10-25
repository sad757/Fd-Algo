/* ============================================================
 * Задание 10 — Переразложение многочлена
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task10 task10.c -lm
 * Запуск:     ./task10 <eps> <a> <n> <f0> <f1> ... <fn>
 *
 * Найти коэффициенты g0, g1, ..., gn такие, что
 *   f(x) = f0 + f1·x + ... + fn·x^n
 *        = g0 + g1·(x-a) + ... + gn·(x-a)^n
 *
 * Используется схема Горнера + разложение Тейлора.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>

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
 *  Разбор double без errno
 *  Использует strtod (стандартный C99), но не читает errno.
 *  Принимает : 1e-9, 3.14, -2.5, +1.0e10, etc.
 * ============================================================ */
static status_t parse_double(const char *s, double *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;

    char *end = NULL;
    double v = strtod(s, &end);

    if (end == s) return ST_ERR_PARSE;           /* rien de parsé */
    while (*end && isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return ST_ERR_PARSE;       /* caractères résiduels */

    if (v != v) return ST_ERR_PARSE;             /* NaN */
    if (v == 1.0 / 0.0 || v == -1.0 / 0.0)      /* Inf */
        return ST_ERR_PARSE;

    *out = v;
    return ST_OK;
}

/* ============================================================
 *  Разбор int без errno
 * ============================================================ */
static status_t parse_int(const char *s, int *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;

    char *end = NULL;
    long v = strtol(s, &end, 10);

    if (end == s) return ST_ERR_PARSE;
    while (*end && isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return ST_ERR_PARSE;
    if (v < INT_MIN || v > INT_MAX) return ST_ERR_RANGE;

    *out = (int)v;
    return ST_OK;
}

/* ============================================================
 *  Вариадическая функция : переразложение многочлена
 *
 *  Параметры :
 *    - eps : точность (зарезервировано, для совместимости)
 *    - a   : значение, относительно которого раскладываем
 *    - out : указатель, куда записать массив коэффициентов g
 *    - n   : степень многочлена
 *    - ... : n+1 коэффициентов f0, f1, ..., fn (от младшей степени
 *            к старшей) — как в условии
 *
 *  Возвращает : status_t
 * ============================================================ */
static status_t translate_poly(double eps, double a, double **out, int n, ...)
{
    if (!out || n < 0) return ST_ERR_RANGE;
    if (eps <= 0.0) return ST_ERR_RANGE;

    /* 1. Прочитать коэффициенты f0..fn */
    double *f = malloc((size_t)(n + 1) * sizeof(double));
    if (!f) return ST_ERR_NOMEM;

    va_list ap;
    va_start(ap, n);
    for (int i = 0; i <= n; ++i) {
        f[i] = va_arg(ap, double);
    }
    va_end(ap);

    /* 2. Алгоритм : на каждой итерации находим g_i и делим на (x-a) */
    double *g = malloc((size_t)(n + 1) * sizeof(double));
    if (!g) { free(f); return ST_ERR_NOMEM; }

    double *curr = f;  /* coefficients courants (croissants) */
    for (int i = 0; i <= n; ++i) {
        int m = n - i;  /* degré courant */
        /* Horner : évaluer curr en a → g_i */
        double val = curr[m];
        for (int k = m - 1; k >= 0; --k) {
            val = val * a + curr[k];
        }
        g[i] = val;

        /* Division synthétique : curr = (curr - val) / (x - a) */
        if (m > 0) {
            double *next = malloc((size_t)m * sizeof(double));
            if (!next) {
                if (curr != f) free(curr);
                free(f);
                free(g);
                return ST_ERR_NOMEM;
            }
            /* b_{k-1} = curr[k] + a · b_k, avec b_m = 0 */
            double carry = 0.0;
            for (int k = m - 1; k >= 0; --k) {
                next[k] = curr[k + 1] + a * carry;
                carry = next[k];
            }
            if (curr != f) free(curr);
            curr = next;
        }
    }
    if (curr != f) free(curr);

    free(f);
    *out = g;
    return ST_OK;
}

/* ============================================================
 *  Évaluation d'un polynôme par Horner
 *  coefficients en ordre croissant : c[0] + c[1]·x + ...
 * ============================================================ */
static double eval_poly(const double *c, int n, double x)
{
    double r = c[n];
    for (int i = n - 1; i >= 0; --i) {
        r = r * x + c[i];
    }
    return r;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st)
{
    switch (st) {
        case ST_OK:        break;
        case ST_ERR_ARGC:  fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_PARSE: fprintf(stderr, "Ошибка: параметр не число.\n"); break;
        case ST_ERR_RANGE: fprintf(stderr, "Ошибка: параметр вне диапазона.\n"); break;
        case ST_ERR_NOMEM: fprintf(stderr, "Ошибка: нет памяти.\n"); break;
        case ST_ERR_EMPTY: fprintf(stderr, "Ошибка: пустой ввод.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr,
            "Использование: %s <eps> <a> <n> <f0> <f1> ... <fn>\n",
            argv[0]);
        return (int)ST_ERR_ARGC;
    }

    double eps = 0.0, a = 0.0;
    int n = 0;
    status_t st;

    if ((st = parse_double(argv[1], &eps)) != ST_OK) {
        print_status(st); return (int)st;
    }
    if (eps <= 0.0) {
        print_status(ST_ERR_RANGE); return (int)ST_ERR_RANGE;
    }
    if ((st = parse_double(argv[2], &a)) != ST_OK) {
        print_status(st); return (int)st;
    }
    if ((st = parse_int(argv[3], &n)) != ST_OK) {
        print_status(st); return (int)st;
    }
    if (n < 0 || n > 50) {
        print_status(ST_ERR_RANGE); return (int)ST_ERR_RANGE;
    }

    /* Vérifier qu'on a bien n+1 coefficients */
    if (argc != 4 + n + 1) {
        fprintf(stderr, "Ошибка: ожидается %d коэффициентов (n+1), передано %d.\n",
                n + 1, argc - 4);
        return (int)ST_ERR_ARGC;
    }

    /* Lire les coefficients */
    double *f = malloc((size_t)(n + 1) * sizeof(double));
    if (!f) { print_status(ST_ERR_NOMEM); return (int)ST_ERR_NOMEM; }
    for (int i = 0; i <= n; ++i) {
        if ((st = parse_double(argv[4 + i], &f[i])) != ST_OK) {
            free(f);
            print_status(st);
            return (int)st;
        }
    }

    /* Appeler translate_poly via dispatcher (switch sur n) */
    double *g = NULL;
    switch (n) {
        case 0: st = translate_poly(eps, a, &g, 0, f[0]); break;
        case 1: st = translate_poly(eps, a, &g, 1, f[0], f[1]); break;
        case 2: st = translate_poly(eps, a, &g, 2, f[0], f[1], f[2]); break;
        case 3: st = translate_poly(eps, a, &g, 3, f[0], f[1], f[2], f[3]); break;
        case 4: st = translate_poly(eps, a, &g, 4, f[0], f[1], f[2], f[3], f[4]); break;
        case 5: st = translate_poly(eps, a, &g, 5, f[0], f[1], f[2], f[3], f[4], f[5]); break;
        case 6: st = translate_poly(eps, a, &g, 6, f[0], f[1], f[2], f[3], f[4], f[5], f[6]); break;
        case 7: st = translate_poly(eps, a, &g, 7, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]); break;
        case 8: st = translate_poly(eps, a, &g, 8, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]); break;
        default:
            fprintf(stderr, "Степень > 8 не поддерживается диспетчером.\n");
            free(f);
            return (int)ST_ERR_RANGE;
    }

    if (st != ST_OK) {
        free(f);
        print_status(st);
        return (int)st;
    }

    /* Affichage */
    printf("Переразложение относительно a = %g :\n", a);
    printf("f(x) = ");
    for (int i = 0; i <= n; ++i) {
        if (i == 0) {
            printf("%.6g", g[0]);
        } else {
            printf(" + %.6g·(x - %.6g)^%d", g[i], a, i);
        }
    }
    printf("\n");

    /* Vérification : évaluer f et g en quelques points */
    printf("\nПроверка (f(x) vs g(x) для нескольких x) :\n");
    double test_pts[] = { 0.0, 1.0, 2.0, -1.0 };
    for (int i = 0; i < 4; ++i) {
        double x = test_pts[i];
        double fx = eval_poly(f, n, x);
        double gx = eval_poly(g, n, x - a);
        printf("  x=%.2f : f=%.10g, g=%.10g, |diff|=%.2g\n",
               x, fx, gx, fabs(fx - gx));
    }

    free(g);
    free(f);
    return (int)ST_OK;
}
