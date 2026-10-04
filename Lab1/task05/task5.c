/* ============================================================
 * Задание 5 — Суммы рядов и интегралы с точностью ε
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -lm -o task5 task5.c
 * Запуск:
 *   ./task5 -s<a|b|c|d> <эпсилон> <x>
 *   ./task5 -i<a|b|c|d> <эпсилон>
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

#define MAX_ITER 100000000L

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_CONV,
    ST_ERR_DIVZERO,
    ST_ERR_OVERFLOW
} status_t;

/* ---------- Разбор эпсилон ---------- */
static status_t parse_eps(const char *s, double *eps)
{
    if (!s || !eps || *s == '\0') return ST_ERR_PARSE;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (errno == ERANGE) return ST_ERR_RANGE;
    if (end == s || *end != '\0') return ST_ERR_PARSE;
    if (v <= 0.0 || v >= 1.0) return ST_ERR_RANGE;
    *eps = v;
    return ST_OK;
}

static status_t parse_double(const char *s, double *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (errno == ERANGE) return ST_ERR_RANGE;
    if (end == s || *end != '\0') return ST_ERR_PARSE;
    if (!isfinite(v)) return ST_ERR_RANGE;
    *out = v;
    return ST_OK;
}

/* ---------- Разбор флага ---------- */
static int parse_flag(const char *arg, char *type, char *sub)
{
    if (!arg || !type || !sub) return 0;
    if (arg[0] != '-' && arg[0] != '/') return 0;
    if (arg[1] == '\0' || arg[2] == '\0' || arg[3] != '\0') return 0;
    char t = (char)tolower((unsigned char)arg[1]);
    char s = (char)tolower((unsigned char)arg[2]);
    if (t != 's' && t != 'i') return 0;
    if (strchr("abcd", s) == NULL) return 0;
    *type = t;
    *sub = s;
    return 1;
}

/* ============================================================
 *  РЯДЫ
 * ============================================================ */

/* a) Σ x^n / n!  = e^x */
static status_t serie_a(double eps, double x, double *out)
{
    double s = 1.0, term = 1.0;
    for (long n = 1; n < MAX_ITER; ++n) {
        term *= x / (double)n;
        double prev = s;
        s += term;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

/* b) Σ (-1)^n x^{2n} / (2n)!  = cos(x) */
static status_t serie_b(double eps, double x, double *out)
{
    double s = 1.0, term = 1.0;
    double x2 = x * x;
    for (long n = 1; n < MAX_ITER; ++n) {
        term *= -x2 / ((double)(2 * n - 1) * (double)(2 * n));
        double prev = s;
        s += term;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

/* c) Σ 3 · 3^n · (n!)^3 · x^{2n} / (3n)! */
static status_t serie_c(double eps, double x, double *out)
{
    double s = 3.0, term = 3.0;
    double x2 = x * x;
    for (long n = 1; n < MAX_ITER; ++n) {
        double n3 = (double)n * n * n;
        double den = (double)(3 * n) * (double)(3 * n - 1) * (double)(3 * n - 2);
        term *= 3.0 * n3 * x2 / den;
        double prev = s;
        s += term;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

/* d) Σ (-1)^n (2n-1)!! x^{2n} / (2n)!! */
static status_t serie_d(double eps, double x, double *out)
{
    double x2 = x * x;
    double term = -x2 / 2.0;
    double s = term;
    for (long n = 2; n < MAX_ITER; ++n) {
        term *= -((double)(2 * n - 1) / (double)(2 * n)) * x2;
        double prev = s;
        s += term;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

/* ============================================================
 *  ИНТЕГРАЛЫ — метод Симпсона
 * ============================================================ */

typedef double (*func_t)(double);

static double simpson_rule(func_t f, double a, double b, long n)
{
    double h = (b - a) / (double)n;
    double s = f(a) + f(b);
    for (long i = 1; i < n; ++i) {
        double x = a + i * h;
        s += ((i & 1) ? 4.0 : 2.0) * f(x);
    }
    return s * h / 3.0;
}

static status_t integrate_simpson(func_t f, double a, double b,
                                  double eps, double *out)
{
    long n = 2;
    double prev = simpson_rule(f, a, b, n);
    for (int it = 0; it < 30; ++it) {
        n *= 2;
        double cur = simpson_rule(f, a, b, n);
        if (fabs(cur - prev) < eps * (fabs(cur) + 1.0)) { *out = cur; return ST_OK; }
        prev = cur;
        if (n > 100000000L) break;
    }
    *out = prev;
    return ST_ERR_CONV;
}

/* ---- Подынтегральные функции ---- */

/* a) ln(1+x)/x, предел при x→0 равен 1 */
static double f_ia(double x)
{
    if (x < 1e-15) return 1.0;
    return log1p(x) / x;
}

/* b) exp(-x²/2) */
static double f_ib(double x) { return exp(-x * x / 2.0); }

/* c) ln(1/(1-x)) = -ln(1-x), при x→1 стремится к +∞ */
static double f_ic(double x)
{
    if (x > 1.0 - 1e-12) x = 1.0 - 1e-12;
    return -log1p(-x);
}

/* d) x^x, предел при x→0 равен 1 */
static double f_id(double x)
{
    if (x < 1e-15) return 1.0;
    return pow(x, x);
}

/* ---------- Обёртки интегралов ---------- */
static status_t integral_a(double eps, double *out) { return integrate_simpson(f_ia, 0.0, 1.0, eps, out); }
static status_t integral_b(double eps, double *out) { return integrate_simpson(f_ib, 0.0, 1.0, eps, out); }
static status_t integral_c(double eps, double *out) { return integrate_simpson(f_ic, 0.0, 1.0, eps, out); }
static status_t integral_d(double eps, double *out) { return integrate_simpson(f_id, 0.0, 1.0, eps, out); }

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st, const char *arg)
{
    switch (st) {
        case ST_OK:            break;
        case ST_ERR_ARGC:      fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_FLAG:      fprintf(stderr, "Ошибка: неверный флаг '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_PARSE:     fprintf(stderr, "Ошибка: параметр не является числом.\n"); break;
        case ST_ERR_RANGE:     fprintf(stderr, "Ошибка: параметр вне диапазона.\n"); break;
        case ST_ERR_CONV:      fprintf(stderr, "Внимание: сходимость не достигнута.\n"); break;
        case ST_ERR_DIVZERO:   fprintf(stderr, "Ошибка: деление на ноль.\n"); break;
        case ST_ERR_OVERFLOW:  fprintf(stderr, "Ошибка: переполнение.\n"); break;
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
            "  %s -s<a|b|c|d> <эпсилон> <x>   (ряды)\n"
            "  %s -i<a|b|c|d> <эпсилон>       (интегралы)\n",
            argv[0], argv[0]);
        return (int)ST_ERR_ARGC;
    }

    char type = '\0', sub = '\0';
    if (!parse_flag(argv[1], &type, &sub)) {
        print_status(ST_ERR_FLAG, argv[1]);
        return (int)ST_ERR_FLAG;
    }

    double eps = 0.0;
    status_t st = parse_eps(argv[2], &eps);
    if (st != ST_OK) { print_status(st, NULL); return (int)st; }

    double result = 0.0;

    if (type == 's') {
        if (argc != 4) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        double x = 0.0;
        if ((st = parse_double(argv[3], &x)) != ST_OK) { print_status(st, NULL); return (int)st; }

        switch (sub) {
            case 'a': st = serie_a(eps, x, &result); break;
            case 'b': st = serie_b(eps, x, &result); break;
            case 'c': st = serie_c(eps, x, &result); break;
            case 'd': st = serie_d(eps, x, &result); break;
        }
        if (st == ST_OK)
            printf("Ряд -s%c (x=%.4f, эпсилон=%.1e) = %.15f\n", sub, x, eps, result);
    } else {
        if (argc != 3) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }

        switch (sub) {
            case 'a': st = integral_a(eps, &result); break;
            case 'b': st = integral_b(eps, &result); break;
            case 'c': st = integral_c(eps, &result); break;
            case 'd': st = integral_d(eps, &result); break;
        }
        if (st == ST_OK)
            printf("Интеграл -i%c (эпсилон=%.1e) = %.15f\n", sub, eps, result);
    }

    if (st != ST_OK) print_status(st, NULL);
    return (int)st;
}
