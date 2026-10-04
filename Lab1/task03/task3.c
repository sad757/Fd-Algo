/* ============================================================
 * Задание 3 — Уравнения и геометрия
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -lm -o task3 task3.c
 * Запуск:
 *   ./task3 -q <эпсилон> <a> <b> <c>
 *   ./task3 -m <n1> <n2>
 *   ./task3 -t <эпсилон> <a> <b> <c>
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
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_ZERO,          /* недопустимый ноль */
    ST_ERR_NOT_TRIANGLE,  /* не треугольник */
    ST_ERR_NO_ROOT,       /* нет перестановок / корней */
    ST_ERR_OVERFLOW
} status_t;

/* ---------- Проверка флага ---------- */
static int is_valid_flag(const char *flag, char *out)
{
    if (!flag || !out) return 0;
    if (flag[0] != '-' && flag[0] != '/') return 0;
    if (flag[1] == '\0' || flag[2] != '\0') return 0;
    char c = (char)tolower((unsigned char)flag[1]);
    if (strchr("qmt", c) == NULL) return 0;
    *out = c;
    return 1;
}

/* ---------- Разбор чисел ---------- */
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

/* ---------- Сравнение вещественных чисел с эпсилон ---------- */
static int dbl_eq(double a, double b, double eps) { return fabs(a - b) < eps; }
/* ============================================================
 *  Флаг -q: квадратное уравнение для уникальных перестановок
 * ============================================================ */

/* Решает a·x² + b·x + c = 0 (или вырожденный случай) */
static void solve_quad(double a, double b, double c, double eps)
{
    if (dbl_eq(a, 0.0, eps)) {
        if (dbl_eq(b, 0.0, eps)) {
            if (dbl_eq(c, 0.0, eps)) printf("    -> бесконечно много решений (0 = 0)\n");
            else                     printf("    -> решений нет\n");
        } else {
            printf("    -> линейное: x = %.6f\n", -c / b);
        }
        return;
    }
    double disc = b * b - 4.0 * a * c;
    if (disc < -eps) {
        printf("    -> нет вещественных корней (дискр. = %.6f)\n", disc);
    } else if (fabs(disc) <= eps) {
        printf("    -> двойной корень: x = %.6f\n", -b / (2.0 * a));
    } else {
        double sq = sqrt(disc);
        double x1 = (-b - sq) / (2.0 * a);
        double x2 = (-b + sq) / (2.0 * a);
        printf("    -> x1 = %.6f, x2 = %.6f\n", x1, x2);
    }
}

/* Проверяет, встречалась ли перестановка ранее */
static int already_seen(double perms[][3], int n,
                        double a, double b, double c, double eps)
{
    for (int i = 0; i < n; ++i) {
        if (dbl_eq(perms[i][0], a, eps) &&
            dbl_eq(perms[i][1], b, eps) &&
            dbl_eq(perms[i][2], c, eps))
            return 1;
    }
    return 0;
}

static status_t do_q(double eps, double a, double b, double c)
{
    double vals[3] = { a, b, c };
    double seen[6][3];
    int seen_n = 0;
    int count = 0;

    printf("Уравнение a·x^2 + b·x + c = 0 (эпсилон=%.1e)\n", eps);
    printf("Уникальные перестановки (a, b, c):\n");

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (j == i) continue;
            for (int k = 0; k < 3; ++k) {
                if (k == i || k == j) continue;
                double pa = vals[i], pb = vals[j], pc = vals[k];
                if (already_seen(seen, seen_n, pa, pb, pc, eps)) continue;
                seen[seen_n][0] = pa;
                seen[seen_n][1] = pb;
                seen[seen_n][2] = pc;
                ++seen_n; ++count;
                printf("  [%d] a=%.4f, b=%.4f, c=%.4f\n", count, pa, pb, pc);
                solve_quad(pa, pb, pc, eps);
            }
        }
    }
    if (count == 0) return ST_ERR_NO_ROOT;
    return ST_OK;
}

/* ============================================================
 *  Флаг -m: делимость двух ненулевых целых
 * ============================================================ */
static status_t do_m(long n1, long n2)
{
    if (n1 == 0 || n2 == 0) return ST_ERR_ZERO;

    int multiple = (n1 % n2 == 0);
    printf("%ld %s числу %ld.\n", n1, multiple ? "кратно" : "не кратно", n2);

    if (n1 != n2) {
        int inv = (n2 % n1 == 0);
        printf("(обратно) %ld %s числу %ld.\n",
               n2, inv ? "кратно" : "не кратно", n1);
    }
    return ST_OK;
}

/* ============================================================
 *  Флаг -t: прямоугольный треугольник ?
 * ============================================================ */
static status_t do_t(double eps, double a, double b, double c)
{
    if (a <= 0.0 || b <= 0.0 || c <= 0.0) return ST_ERR_RANGE;

    /* неравенство треугольника с допуском */
    if (!(a + b > c - eps && a + c > b - eps && b + c > a - eps))
        return ST_ERR_NOT_TRIANGLE;

    /* три варианта гипотенузы */
    double sides[3] = { a, b, c };
    int found = 0;
    for (int h = 0; h < 3; ++h) {
        double hyp = sides[h];
        double s1 = sides[(h + 1) % 3];
        double s2 = sides[(h + 2) % 3];
        if (dbl_eq(hyp * hyp, s1 * s1 + s2 * s2, eps * (hyp * hyp + 1.0))) {
            printf("Прямоугольный треугольник: гипотенуза = %.4f, катеты = %.4f и %.4f\n",
                   hyp, s1, s2);
            found = 1;
        }
    }
    if (!found) printf("НЕ прямоугольный треугольник.\n");
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st, const char *flag)
{
    switch (st) {
        case ST_OK:               break;
        case ST_ERR_ARGC:         fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_FLAG:         fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", flag ? flag : "?"); break;
        case ST_ERR_PARSE:        fprintf(stderr, "Ошибка: параметр не является числом.\n"); break;
        case ST_ERR_RANGE:        fprintf(stderr, "Ошибка: параметр вне диапазона.\n"); break;
        case ST_ERR_ZERO:         fprintf(stderr, "Ошибка: нулевой параметр недопустим.\n"); break;
        case ST_ERR_NOT_TRIANGLE: fprintf(stderr, "Ошибка: не треугольник.\n"); break;
        case ST_ERR_NO_ROOT:      fprintf(stderr, "Ошибка: нет перестановок для обработки.\n"); break;
        case ST_ERR_OVERFLOW:     fprintf(stderr, "Ошибка: переполнение.\n"); break;
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
            "  %s -q <эпсилон> <a> <b> <c>\n"
            "  %s -m <n1> <n2>\n"
            "  %s -t <эпсилон> <a> <b> <c>\n",
            argv[0], argv[0], argv[0]);
        return (int)ST_ERR_ARGC;
    }

    char flag = '\0';
    if (!is_valid_flag(argv[1], &flag)) {
        print_status(ST_ERR_FLAG, argv[1]);
        return (int)ST_ERR_FLAG;
    }

    status_t st = ST_OK;

    switch (flag) {
        case 'q': {
            if (argc != 6) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
            double eps, a, b, c;
            if ((st = parse_double(argv[2], &eps)) != ST_OK) break;
            if (eps <= 0.0) { st = ST_ERR_RANGE; break; }
            if ((st = parse_double(argv[3], &a)) != ST_OK) break;
            if ((st = parse_double(argv[4], &b)) != ST_OK) break;
            if ((st = parse_double(argv[5], &c)) != ST_OK) break;
            st = do_q(eps, a, b, c);
            break;
        }
        case 'm': {
            if (argc != 4) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
            long n1, n2;
            if ((st = parse_long(argv[2], &n1)) != ST_OK) break;
            if ((st = parse_long(argv[3], &n2)) != ST_OK) break;
            st = do_m(n1, n2);
            break;
        }
        case 't': {
            if (argc != 6) { print_status(ST_ERR_ARGC, NULL); return (int)ST_ERR_ARGC; }
            double eps, a, b, c;
            if ((st = parse_double(argv[2], &eps)) != ST_OK) break;
            if (eps <= 0.0) { st = ST_ERR_RANGE; break; }
            if ((st = parse_double(argv[3], &a)) != ST_OK) break;
            if ((st = parse_double(argv[4], &b)) != ST_OK) break;
            if ((st = parse_double(argv[5], &c)) != ST_OK) break;
            st = do_t(eps, a, b, c);
            break;
        }
        default:
            st = ST_ERR_FLAG;
            break;
    }

    if (st != ST_OK) print_status(st, argv[1]);
    return (int)st;
}
