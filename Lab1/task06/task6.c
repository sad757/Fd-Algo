/* ============================================================
 * Задание 6 — Функции с переменным числом аргументов
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -lm -o task6 task6.c
 * Запуск: ./task6 <флаг> <параметры...>
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_NOMEM,
    ST_ERR_ZERO,
    ST_ERR_CONV,
    ST_ERR_NOT_KAPREKAR,
    ST_ERR_NO_ROOT,
    ST_ERR_NOT_CONVEX
} status_t;

/* ---------- Разбор ---------- */
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

static status_t parse_long(const char *s, long *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno == ERANGE) return ST_ERR_RANGE;
    if (end == s || *end != '\0') return ST_ERR_PARSE;
    *out = v;
    return ST_OK;
}

/* ============================================================
 *  6.1 — Выпуклый ли многоугольник (вариадическая)
 * ============================================================ */
static status_t is_convex(int n, ...)
{
    if (n < 3) return ST_ERR_RANGE;

    va_list ap;
    va_start(ap, n);
    double *xs = malloc((size_t)n * sizeof(double));
    double *ys = malloc((size_t)n * sizeof(double));
    if (!xs || !ys) { free(xs); free(ys); va_end(ap); return ST_ERR_NOMEM; }

    for (int i = 0; i < n; ++i) {
        xs[i] = va_arg(ap, double);
        ys[i] = va_arg(ap, double);
    }
    va_end(ap);

    int sign = 0;
    status_t st = ST_OK;
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        int k = (i + 2) % n;
        double cross = (xs[j] - xs[i]) * (ys[k] - ys[j])
                     - (ys[j] - ys[i]) * (xs[k] - xs[j]);
        if (fabs(cross) < 1e-12) continue;
        int s = (cross > 0) ? 1 : -1;
        if (sign == 0) sign = s;
        else if (s != sign) { st = ST_ERR_NOT_CONVEX; break; }
    }

    free(xs);
    free(ys);
    return st;
}

/* ============================================================
 *  6.2 — Значение многочлена в точке (вариадическая, схема Горнера)
 * ============================================================ */
static status_t poly_eval(double x, int n, double *out, ...)
{
    if (!out || n < 0) return ST_ERR_RANGE;
    va_list ap;
    va_start(ap, out);
    double r = 0.0;
    for (int i = 0; i <= n; ++i) {
        double c = va_arg(ap, double);
        r = r * x + c;
    }
    va_end(ap);
    *out = r;
    return ST_OK;
}

/* ============================================================
 *  6.3 — Числа Капрекара (вариадическая, строки)
 * ============================================================ */
static long long str_to_base(const char *s, int base)
{
    if (!s || *s == '\0' || base < 2 || base > 36) return -1;
    long long v = 0;
    for (const char *p = s; *p; ++p) {
        int d;
        if (isdigit((unsigned char)*p))      d = *p - '0';
        else if (isalpha((unsigned char)*p)) d = tolower((unsigned char)*p) - 'a' + 10;
        else return -1;
        if (d >= base) return -1;
        v = v * base + d;
        if (v < 0) return -1;
    }
    return v;
}

static int is_kaprekar(unsigned long long n, int base)
{
    if (n == 0) return 0;
    unsigned long long sq = n * n;
    unsigned long long pow_b = 1;
    while (pow_b <= sq) pow_b *= (unsigned long long)base;
    pow_b /= (unsigned long long)base;

    while (pow_b > 0) {
        unsigned long long q = sq / pow_b;
        unsigned long long r = sq % pow_b;
        if (q + r == n && r > 0) return 1;
        if (pow_b < (unsigned long long)base) break;
        pow_b /= (unsigned long long)base;
    }
    return 0;
}

static status_t kaprekar_search(int base, int count, ...)
{
    if (base < 2 || base > 36 || count <= 0) return ST_ERR_RANGE;
    va_list ap;
    va_start(ap, count);
    int found = 0;
    printf("Числа Капрекара в основании %d:\n", base);
    for (int i = 0; i < count; ++i) {
        const char *s = va_arg(ap, const char *);
        long long v = str_to_base(s, base);
        if (v < 0) { printf("  [%d] '%s': недопустимо в основании %d\n", i + 1, s, base); continue; }
        if (is_kaprekar((unsigned long long)v, base)) {
            printf("  [%d] %s = %lld -> ДА\n", i + 1, s, v);
            ++found;
        }
    }
    va_end(ap);
    if (found == 0) { printf("  (нет)\n"); return ST_ERR_NOT_KAPREKAR; }
    return ST_OK;
}

/* ============================================================
 *  6.4 — Среднее геометрическое (вариадическая, N — последний)
 * ============================================================ */
static status_t geo_mean(int n, ...)
{
    if (n <= 0) return ST_ERR_RANGE;
    va_list ap;
    va_start(ap, n);
    double prod = 1.0;
    for (int i = 0; i < n; ++i) {
        double v = va_arg(ap, double);
        if (v <= 0.0) { va_end(ap); return ST_ERR_RANGE; }
        prod *= v;
    }
    va_end(ap);
    double g = pow(prod, 1.0 / (double)n);
    printf("Среднее геометрическое %d чисел = %.10f\n", n, g);
    return ST_OK;
}

/* ============================================================
 *  6.5 — Быстрое возведение в степень (рекурсивно)
 * ============================================================ */
static double pow_fast(double x, long n)
{
    if (n == 0) return 1.0;
    if (n < 0)  return 1.0 / pow_fast(x, -n);
    double h = pow_fast(x, n / 2);
    return (n % 2 == 0) ? h * h : h * h * x;
}

/* ============================================================
 *  6.6 — Метод дихотомии (указатель на функцию)
 * ============================================================ */
typedef double (*func_t)(double);

static double f1(double x) { return x * x - 2.0; }
static double f2(double x) { return cos(x) - x; }
static double f3(double x) { return exp(x) - 3.0 * x; }

static status_t dicho(double a, double b, double eps, func_t f, double *out)
{
    if (!f || !out || eps <= 0.0) return ST_ERR_RANGE;
    double fa = f(a), fb = f(b);
    if (fa * fb > 0.0) return ST_ERR_NO_ROOT;
    double m = a;
    for (int i = 0; i < 1000000; ++i) {
        m = 0.5 * (a + b);
        double fm = f(m);
        if (fabs(b - a) < eps || fabs(fm) < eps) { *out = m; return ST_OK; }
        if (fa * fm < 0.0) { b = m; fb = fm; }
        else               { a = m; fa = fm; }
    }
    *out = m;
    return ST_ERR_CONV;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st, const char *arg)
{
    switch (st) {
        case ST_OK:               break;
        case ST_ERR_ARGC:         fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_FLAG:         fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_PARSE:        fprintf(stderr, "Ошибка: параметр не число.\n"); break;
        case ST_ERR_RANGE:        fprintf(stderr, "Ошибка: параметр вне диапазона.\n"); break;
        case ST_ERR_NOMEM:        fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_ZERO:         fprintf(stderr, "Ошибка: деление на ноль.\n"); break;
        case ST_ERR_CONV:         fprintf(stderr, "Внимание: сходимость не достигнута.\n"); break;
        case ST_ERR_NOT_KAPREKAR: fprintf(stderr, "Инфо: числа Капрекара не найдены.\n"); break;
        case ST_ERR_NO_ROOT:      fprintf(stderr, "Ошибка: нет корня на интервале.\n"); break;
        case ST_ERR_NOT_CONVEX:   fprintf(stderr, "Результат: многоугольник НЕ выпуклый.\n"); break;
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
            "  %s -poly <n> <x> <c0> <c1> ... <cn>\n"
            "  %s -kap <основание> <s1> <s2> ...\n"
            "  %s -geo <v1> <v2> ... <N>\n"
            "  %s -pow <x> <n>\n"
            "  %s -dicho <a> <b> <эпсилон> <id_уравнения>\n"
            "  %s -convex <n> <x1> <y1> ... <xn> <yn>\n",
            argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
        return (int)ST_ERR_ARGC;
    }

    const char *flag = argv[1];
    if (flag[0] != '-' && flag[0] != '/') {
        print_status(ST_ERR_FLAG, flag);
        return (int)ST_ERR_FLAG;
    }

    status_t st = ST_OK;

    /* ---------- -poly ---------- */
    if (strcmp(flag, "-poly") == 0 || strcmp(flag, "/poly") == 0) {
        if (argc < 5) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        long n; double x;
        if ((st = parse_long(argv[2], &n)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if (n < 0 || n > 20) { print_status(ST_ERR_RANGE, NULL); return (int)ST_ERR_RANGE; }
        if ((st = parse_double(argv[3], &x)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if (argc != (int)(4 + n + 1)) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }

        double out = 0.0;
        double coeffs[21];
        for (int i = 0; i <= (int)n; ++i) {
            if ((st = parse_double(argv[4 + i], &coeffs[i])) != ST_OK) {
                print_status(st, NULL); return (int)st;
            }
        }
        switch ((int)n) {
            case 0: st = poly_eval(x, 0, &out, coeffs[0]); break;
            case 1: st = poly_eval(x, 1, &out, coeffs[0], coeffs[1]); break;
            case 2: st = poly_eval(x, 2, &out, coeffs[0], coeffs[1], coeffs[2]); break;
            case 3: st = poly_eval(x, 3, &out, coeffs[0], coeffs[1], coeffs[2], coeffs[3]); break;
            case 4: st = poly_eval(x, 4, &out, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4]); break;
            case 5: st = poly_eval(x, 5, &out, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], coeffs[5]); break;
            default:
                fprintf(stderr, "Степень > 5 не поддерживается этим диспетчером.\n");
                return (int)ST_ERR_RANGE;
        }
        if (st == ST_OK) printf("P(%.6f) = %.10f\n", x, out);
    }
    /* ---------- -kap ---------- */
    else if (strcmp(flag, "-kap") == 0 || strcmp(flag, "/kap") == 0) {
        if (argc < 4) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        long base;
        if ((st = parse_long(argv[2], &base)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if (base < 2 || base > 36) { print_status(ST_ERR_RANGE, NULL); return (int)ST_ERR_RANGE; }
        int count = argc - 3;
                switch (count) {
            case 1: st = kaprekar_search((int)base, 1, argv[3]); break;
            case 2: st = kaprekar_search((int)base, 2, argv[3], argv[4]); break;
            case 3: st = kaprekar_search((int)base, 3, argv[3], argv[4], argv[5]); break;
            case 4: st = kaprekar_search((int)base, 4, argv[3], argv[4], argv[5], argv[6]); break;
            case 5: st = kaprekar_search((int)base, 5, argv[3], argv[4], argv[5], argv[6], argv[7]); break;
            case 6: st = kaprekar_search((int)base, 6, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]); break;
            case 7: st = kaprekar_search((int)base, 7, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]); break;
            case 8: st = kaprekar_search((int)base, 8, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10]); break;
            case 9: st = kaprekar_search((int)base, 9, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11]); break;
            case 10: st = kaprekar_search((int)base, 10, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12]); break;
            default:
                fprintf(stderr, "Слишком много строк (максимум 10).\n");
                return (int)ST_ERR_RANGE;
	        }
    }
    /* ---------- -geo ---------- */
    else if (strcmp(flag, "-geo") == 0 || strcmp(flag, "/geo") == 0) {
        if (argc < 4) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        long n;
        if ((st = parse_long(argv[argc - 1], &n)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if (n <= 0 || argc != (int)(n + 3)) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        double vals[100];
        for (int i = 0; i < (int)n; ++i) {
            if ((st = parse_double(argv[2 + i], &vals[i])) != ST_OK) { print_status(st, NULL); return (int)st; }
        }
        switch ((int)n) {
            case 1: st = geo_mean(1, vals[0]); break;
            case 2: st = geo_mean(2, vals[0], vals[1]); break;
            case 3: st = geo_mean(3, vals[0], vals[1], vals[2]); break;
            case 4: st = geo_mean(4, vals[0], vals[1], vals[2], vals[3]); break;
            case 5: st = geo_mean(5, vals[0], vals[1], vals[2], vals[3], vals[4]); break;
            case 6: st = geo_mean(6, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]); break;
            default:
                fprintf(stderr, "N > 6 не поддерживается.\n");
                return (int)ST_ERR_RANGE;
        }
    }
    /* ---------- -pow ---------- */
    else if (strcmp(flag, "-pow") == 0 || strcmp(flag, "/pow") == 0) {
        if (argc != 4) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        double x; long n;
        if ((st = parse_double(argv[2], &x)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if ((st = parse_long(argv[3], &n)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if (x == 0.0 && n < 0) { print_status(ST_ERR_ZERO, NULL); return (int)ST_ERR_ZERO; }
        double r = pow_fast(x, n);
        printf("%.6f^%ld = %.10f\n", x, n, r);
    }
    /* ---------- -dicho ---------- */
    else if (strcmp(flag, "-dicho") == 0 || strcmp(flag, "/dicho") == 0) {
        if (argc != 6) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        double a, b, eps; long eq;
        if ((st = parse_double(argv[2], &a)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if ((st = parse_double(argv[3], &b)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if ((st = parse_double(argv[4], &eps)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if (eps <= 0.0) { print_status(ST_ERR_RANGE, NULL); return (int)ST_ERR_RANGE; }
        if ((st = parse_long(argv[5], &eq)) != ST_OK) { print_status(st, NULL); return (int)st; }

        func_t f = NULL;
        const char *eqname = NULL;
        switch ((int)eq) {
            case 1: f = f1; eqname = "x^2 - 2 = 0"; break;
            case 2: f = f2; eqname = "cos(x) - x = 0"; break;
            case 3: f = f3; eqname = "e^x - 3x = 0"; break;
            default:
                fprintf(stderr, "Уравнение %ld неизвестно (1, 2 или 3).\n", eq);
                return (int)ST_ERR_RANGE;
        }
        double root = 0.0;
        st = dicho(a, b, eps, f, &root);
        if (st == ST_OK)
            printf("Корень уравнения %s на [%.4f, %.4f]: %.10f\n", eqname, a, b, root);
    }
    /* ---------- -convex ---------- */
    else if (strcmp(flag, "-convex") == 0 || strcmp(flag, "/convex") == 0) {
        if (argc < 5) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        long n;
        if ((st = parse_long(argv[2], &n)) != ST_OK) { print_status(st, NULL); return (int)st; }
        if (n < 3 || n > 10) { print_status(ST_ERR_RANGE, NULL); return (int)ST_ERR_RANGE; }
        if (argc != (int)(3 + 2 * n)) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
        double vals[20];
        for (int i = 0; i < 2 * (int)n; ++i) {
            if ((st = parse_double(argv[3 + i], &vals[i])) != ST_OK) {
                print_status(st, NULL); return (int)st;
            }
        }
        switch ((int)n) {
            case 3: st = is_convex(3, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]); break;
            case 4: st = is_convex(4, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7]); break;
            case 5: st = is_convex(5, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8], vals[9]); break;
            case 6: st = is_convex(6, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8], vals[9], vals[10], vals[11]); break;
            default:
                fprintf(stderr, "n > 6 не поддерживается.\n");
                return (int)ST_ERR_RANGE;
        }
        if (st == ST_OK) printf("Многоугольник ВЫПУКЛЫЙ.\n");
    }
    else {
        st = ST_ERR_FLAG;
    }

    if (st != ST_OK) print_status(st, flag);
    return (int)st;
}
