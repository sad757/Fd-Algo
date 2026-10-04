/* ============================================================
 * Задание 2 — Константы e, π, ln 2, √2, γ (по 3 метода каждая)
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -lm -o task2 task2.c
 * Запуск:     ./task2 <e|pi|ln2|sqrt2|gamma> <эпсилон>
 *
 * Для каждой константы реализованы три метода:
 *   1) как сумма ряда
 *   2) как решение специального уравнения
 *   3) как значение предела
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,       /* неверное число аргументов */
    ST_ERR_NAME,       /* неизвестное имя константы */
    ST_ERR_PARSE,      /* не удалось разобрать число */
    ST_ERR_RANGE,      /* значение вне диапазона */
    ST_ERR_NOMEM,      /* не удалось выделить память */
    ST_ERR_CONV,       /* сходимость не достигнута */
    ST_ERR_DIVZERO     /* деление на ноль */
} status_t;

/* ---------- Проверка эпсилон ---------- */
static status_t parse_eps(const char *s, double *eps)
{
    if (!s || !eps || *s == '\0') return ST_ERR_PARSE;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (errno == ERANGE) return ST_ERR_RANGE;
    if (end == s || *end != '\0') return ST_ERR_PARSE;
    if (v <= 0.0 || v >= 1.0) return ST_ERR_RANGE;  /* эпсилон ∈ ]0, 1[ */
    *eps = v;
    return ST_OK;
}

/* ---------- Проверка имени константы ---------- */
static int valid_name(const char *n)
{
    if (!n) return 0;
    return strcmp(n, "e") == 0 || strcmp(n, "pi") == 0 ||
           strcmp(n, "ln2") == 0 || strcmp(n, "sqrt2") == 0 ||
           strcmp(n, "gamma") == 0;
}

/* ============================================================
 *  e — 3 метода
 * ============================================================ */
static status_t e_serie(double eps, double *out)
{
    double s = 1.0, term = 1.0;
    for (int n = 1; n < 1000000; ++n) {
        term /= (double)n;
        double prev = s;
        s += term;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

static status_t e_equation(double eps, double *out)
{
    /* решаем ln x = 1 методом Ньютона: x_{n+1} = x_n - (ln x_n - 1)*x_n */
    double x = 2.0;
    for (int i = 0; i < 1000000; ++i) {
        double nx = x - (log(x) - 1.0) * x;
        if (nx <= 0.0) return ST_ERR_CONV;
        if (fabs(nx - x) < eps) { *out = nx; return ST_OK; }
        x = nx;
    }
    return ST_ERR_CONV;
}

static status_t e_limite(double eps, double *out)
{
    /* lim (1 + 1/n)^n — медленная сходимость, используем удвоение n */
    double prev = 0.0;
    for (long n = 1; n < 100000000L; n *= 2) {
        double v = pow(1.0 + 1.0 / (double)n, (double)n);
        if (fabs(v - prev) < eps) { *out = v; return ST_OK; }
        prev = v;
    }
    return ST_ERR_CONV;
}

/* ============================================================
 *  π — 3 метода
 * ============================================================ */
static status_t pi_serie(double eps, double *out)
{
    /* Лейбниц: π = 4 Σ (-1)^n / (2n+1) — очень медленная сходимость */
    double s = 0.0;
    for (long n = 0; n < 100000000L; ++n) {
        double t = 4.0 / (double)(2 * n + 1);
        double prev = s;
        s += (n % 2 == 0) ? t : -t;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

static status_t pi_equation(double eps, double *out)
{
    /* решаем cos x = -1 методом Ньютона: x_{n+1} = x_n + sin(x_n)/(1+cos(x_n)) */
    double x = 3.0;
    for (int i = 0; i < 1000000; ++i) {
        double c = cos(x);
        if (fabs(1.0 + c) < 1e-15) { *out = x; return ST_OK; }
        double nx = x + sin(x) / (1.0 + c);
        if (fabs(nx - x) < eps) { *out = nx; return ST_OK; }
        x = nx;
    }
    return ST_ERR_CONV;
}

static status_t pi_limite(double eps, double *out)
{
    /* Формула Валлиса: π/2 = lim ∏ (2k)² / ((2k-1)(2k+1)) */
    double prod = 1.0;
    double prev = 0.0;
    for (long n = 1; n < 100000000L; ++n) {
        double num = (2.0 * n) * (2.0 * n);
        double den = (2.0 * n - 1.0) * (2.0 * n + 1.0);
        prod *= num / den;
        double v = 2.0 * prod;
        if (fabs(v - prev) < eps) { *out = v; return ST_OK; }
        prev = v;
    }
    return ST_ERR_CONV;
}

/* ============================================================
 *  ln 2 — 3 метода
 * ============================================================ */
static status_t ln2_serie(double eps, double *out)
{
    /* ln 2 = Σ (-1)^{n-1} / n */
    double s = 0.0;
    for (long n = 1; n < 100000000L; ++n) {
        double prev = s;
        s += ((n % 2 == 1) ? 1.0 : -1.0) / (double)n;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

static status_t ln2_equation(double eps, double *out)
{
    /* решаем e^x = 2 методом Ньютона: x_{n+1} = x_n - (e^x_n - 2)/e^x_n */
    double x = 0.5;
    for (int i = 0; i < 1000000; ++i) {
        double ex = exp(x);
        double nx = x - (ex - 2.0) / ex;
        if (fabs(nx - x) < eps) { *out = nx; return ST_OK; }
        x = nx;
    }
    return ST_ERR_CONV;
}

static status_t ln2_limite(double eps, double *out)
{
    /* ln 2 = lim n · (2^{1/n} - 1) */
    double prev = 0.0;
    for (long n = 1; n < 100000000L; n *= 2) {
        double v = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
        if (fabs(v - prev) < eps) { *out = v; return ST_OK; }
        prev = v;
    }
    return ST_ERR_CONV;
}

/* ============================================================
 *  √2 — 3 метода
 * ============================================================ */
static status_t sqrt2_serie(double eps, double *out)
{
    /* √2 = Σ C(1/2, n) — биномиальное разложение */
    double s = 1.0;
    double c = 1.0;
    for (int n = 1; n < 1000000; ++n) {
        c *= (0.5 - (double)(n - 1)) / (double)n;
        double prev = s;
        s += c;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

static status_t sqrt2_equation(double eps, double *out)
{
    /* решаем x² = 2 методом Ньютона: x_{n+1} = (x_n + 2/x_n)/2 */
    double x = 1.0;
    for (int i = 0; i < 1000000; ++i) {
        double nx = 0.5 * (x + 2.0 / x);
        if (fabs(nx - x) < eps) { *out = nx; return ST_OK; }
        x = nx;
    }
    return ST_ERR_CONV;
}

static status_t sqrt2_limite(double eps, double *out)
{
    /* x_{n+1} = x_n - x_n²/2 + 1, x_0 = -0.5 */
    double x = -0.5;
    for (int i = 0; i < 1000000; ++i) {
        double nx = x - x * x / 2.0 + 1.0;
        if (fabs(nx - x) < eps) { *out = nx; return ST_OK; }
        x = nx;
    }
    return ST_ERR_CONV;
}

/* ============================================================
 *  γ (постоянная Эйлера — Маскерони) — 3 метода
 * ============================================================ */
static status_t gamma_serie(double eps, double *out)
{
    /* γ = Σ_{k=1}^∞ [1/k - ln(1 + 1/k)] */
    double s = 0.0;
    for (long k = 1; k < 100000000L; ++k) {
        double t = 1.0 / (double)k - log1p(1.0 / (double)k);
        double prev = s;
        s += t;
        if (fabs(s - prev) < eps) { *out = s; return ST_OK; }
    }
    return ST_ERR_CONV;
}

static status_t gamma_equation(double eps, double *out)
{
    /* Γ'(1) = -γ, приближаем производную конечной разностью */
    double h = 1e-4;
    double g1p = exp(lgamma(1.0 + h));  /* Γ(1+h) */
    double g1m = exp(lgamma(1.0 - h));  /* Γ(1-h) */
    double deriv = (g1p - g1m) / (2.0 * h);
    double g = -deriv;
    /* уточнение методом Ричардсона */
    for (int i = 0; i < 20; ++i) {
        double h2 = h / 2.0;
        double a = (exp(lgamma(1.0 + h2)) - exp(lgamma(1.0 - h2))) / (2.0 * h2);
        double g2 = -a;
        if (fabs(g2 - g) < eps) { *out = g2; return ST_OK; }
        g = g2; h = h2;
    }
    *out = g;
    return ST_OK;
}

static status_t gamma_limite(double eps, double *out)
{
    /* γ = lim (H_n - ln n), где H_n — частичная сумма гармонического ряда */
    double H = 0.0;
    double prev = 0.0;
    for (long n = 1; n < 100000000L; n *= 2) {
        long start = (n == 1) ? 1 : n / 2 + 1;
        for (long k = start; k <= n; ++k) H += 1.0 / (double)k;
        double v = H - log((double)n);
        if (n > 1 && fabs(v - prev) < eps) { *out = v; return ST_OK; }
        prev = v;
    }
    return ST_ERR_CONV;
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <e|pi|ln2|sqrt2|gamma> <эпсилон>\n", argv[0]);
        return (int)ST_ERR_ARGC;
    }
    if (!valid_name(argv[1])) {
        fprintf(stderr, "Ошибка: неизвестная константа '%s'.\n", argv[1]);
        return (int)ST_ERR_NAME;
    }
    double eps = 0.0;
    status_t st = parse_eps(argv[2], &eps);
    if (st != ST_OK) {
        fprintf(stderr, "Ошибка: неверный эпсилон (должен быть в ]0, 1[).\n");
        return (int)st;
    }

    double v1 = 0.0, v2 = 0.0, v3 = 0.0;
    status_t s1, s2, s3;
    const char *name = argv[1];

    if (strcmp(name, "e") == 0) {
        s1 = e_serie(eps, &v1);
        s2 = e_equation(eps, &v2);
        s3 = e_limite(eps, &v3);
        printf("e     : ряд=%.15f уравн=%.15f предел=%.15f (эталон=%.15f)\n",
               v1, v2, v3, 2.718281828459045);
    } else if (strcmp(name, "pi") == 0) {
        s1 = pi_serie(eps, &v1);
        s2 = pi_equation(eps, &v2);
        s3 = pi_limite(eps, &v3);
        printf("pi    : ряд=%.15f уравн=%.15f предел=%.15f (эталон=%.15f)\n",
               v1, v2, v3, 3.141592653589793);
    } else if (strcmp(name, "ln2") == 0) {
        s1 = ln2_serie(eps, &v1);
        s2 = ln2_equation(eps, &v2);
        s3 = ln2_limite(eps, &v3);
        printf("ln2   : ряд=%.15f уравн=%.15f предел=%.15f (эталон=%.15f)\n",
               v1, v2, v3, log(2.0));
    } else if (strcmp(name, "sqrt2") == 0) {
        s1 = sqrt2_serie(eps, &v1);
        s2 = sqrt2_equation(eps, &v2);
        s3 = sqrt2_limite(eps, &v3);
        printf("sqrt2 : ряд=%.15f уравн=%.15f предел=%.15f (эталон=%.15f)\n",
               v1, v2, v3, sqrt(2.0));
    } else { /* gamma */
        s1 = gamma_serie(eps, &v1);
        s2 = gamma_equation(eps, &v2);
        s3 = gamma_limite(eps, &v3);
        printf("gamma : ряд=%.15f уравн=%.15f предел=%.15f (эталон=%.15f)\n",
               v1, v2, v3, 0.5772156649015329);
    }

    /* Сообщения о несходимости */
    if (s1 != ST_OK) fprintf(stderr, "Внимание: ряд не сошёлся.\n");
    if (s2 != ST_OK) fprintf(stderr, "Внимание: уравнение не сошлось.\n");
    if (s3 != ST_OK) fprintf(stderr, "Внимание: предел не достигнут.\n");

    return (int)(s1 == ST_OK && s2 == ST_OK && s3 == ST_OK ? ST_OK : ST_ERR_CONV);
}
