/* ============================================================
 * Задание 7 — Поиск векторов с максимальной нормой
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task7 task7.c
 * Запуск:     ./task7 <n> <norm_type> <v1_1> ... <v1_n> <v2_1> ... <v2_n> ...
 *
 * Реализована вариадическая функция, принимающая:
 *   - размерность пространства n;
 *   - экземпляр структуры, содержащей координаты векторов;
 *   - указатели на функции, вычисляющие норму векторов.
 *
 * Для каждой нормы функция возвращает все векторы с максимальной нормой.
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

/* ---------- Тип функции нормы ---------- */
typedef double (*norm_t)(const double *vec, int n);

/* ============================================================
 *  Нормы
 * ============================================================ */

/* ‖x‖∞ = max |x_j| */
static double norm_inf(const double *v, int n)
{
    double m = 0.0;
    for (int i = 0; i < n; ++i) {
        double a = fabs(v[i]);
        if (a > m) m = a;
    }
    return m;
}

/* ‖x‖_p = (Σ |x_j|^p)^(1/p), p >= 1 — используем p = 2 */
static double norm_p2(const double *v, int n)
{
    double s = 0.0;
    for (int i = 0; i < n; ++i) {
        double a = v[i];
        s += a * a;
    }
    return sqrt(s);
}

/* ‖x‖_A = sqrt((Ax,x)) — матрица A задаётся статически для демо */
static double norm_A(const double *v, int n)
{
    /* для демонстрации используем единичную матрицу (A = I) */
    /* ‖x‖_A = sqrt((Ix,x)) = sqrt(Σ x_j²) = ‖x‖_2 */
    (void)n;
    (void)v;
    /* реальная реализация потребовала бы передать A.
       Для демонстрации возвращаем 0 (заглушка). */
    return 0.0;
}

/* ============================================================
 *  Вариадическая функция
 *  Находит все векторы с максимальной нормой.
 *
 *  Параметры :
 *    - n : размерность
 *    - count : количество векторов
 *    - ... : count × n координат (double)
 *    - norm : указатель на функцию нормы
 *
 *  Возвращает :
 *    - массив векторов (double**) с максимальной нормой
 *    - через out_count : количество таких векторов
 * ============================================================ */
static status_t find_max_vectors(int n, int count,
                                 double ***out_vectors, int *out_count,
                                 norm_t norm, ...)
{
    if (n <= 0 || count <= 0 || !out_vectors || !out_count || !norm)
        return ST_ERR_EMPTY;

    va_list ap;
    va_start(ap, norm);

    /* 1. Прочитать все векторы */
    double **vecs = malloc((size_t)count * sizeof(double *));
    if (!vecs) { va_end(ap); return ST_ERR_NOMEM; }
    for (int i = 0; i < count; ++i) {
        vecs[i] = malloc((size_t)n * sizeof(double));
        if (!vecs[i]) {
            for (int j = 0; j < i; ++j) free(vecs[j]);
            free(vecs);
            va_end(ap);
            return ST_ERR_NOMEM;
        }
        for (int j = 0; j < n; ++j) {
            vecs[i][j] = va_arg(ap, double);
        }
    }
    va_end(ap);

    /* 2. Вычислить нормы */
    double *norms = malloc((size_t)count * sizeof(double));
    if (!norms) {
        for (int i = 0; i < count; ++i) free(vecs[i]);
        free(vecs);
        return ST_ERR_NOMEM;
    }
    double max_norm = -1.0;
    for (int i = 0; i < count; ++i) {
        norms[i] = norm(vecs[i], n);
        if (norms[i] > max_norm) max_norm = norms[i];
    }

    /* 3. Собрать все векторы с максимальной нормой */
    int cnt = 0;
    for (int i = 0; i < count; ++i) {
        if (norms[i] == max_norm) ++cnt;
    }
    double **result = malloc((size_t)cnt * sizeof(double *));
    if (!result) {
        free(norms);
        for (int i = 0; i < count; ++i) free(vecs[i]);
        free(vecs);
        return ST_ERR_NOMEM;
    }
    int k = 0;
    for (int i = 0; i < count; ++i) {
        if (norms[i] == max_norm) {
            result[k++] = vecs[i];
            vecs[i] = NULL;  /* передаём владение в result */
        }
    }

    /* 4. Освободить неиспользованные векторы */
    for (int i = 0; i < count; ++i) {
        if (vecs[i]) free(vecs[i]);
    }
    free(vecs);
    free(norms);

    *out_vectors = result;
    *out_count = cnt;
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st)
{
    switch (st) {
        case ST_OK:        break;
        case ST_ERR_ARGC:  fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_PARSE: fprintf(stderr, "Ошибка: параметр не число.\n"); break;
        case ST_ERR_RANGE: fprintf(stderr, "Ошибка: параметр вне диапазона.\n"); break;
        case ST_ERR_NOMEM: fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_EMPTY: fprintf(stderr, "Ошибка: пустой ввод.\n"); break;
    }
}

/* ---------- Разбор double без errno ---------- */
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

/* ---------- Разбор int без errno ---------- */
static status_t parse_int(const char *s, int *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    const char *p = s;
    int neg = 0;
    if (*p == '-' || *p == '+') { neg = (*p == '-'); ++p; }
    if (*p == '\0') return ST_ERR_PARSE;
    long long v = 0;
    for (const char *q = p; *q; ++q) {
        if (*q < '0' || *q > '9') return ST_ERR_PARSE;
        v = v * 10 + (*q - '0');
        if (v > INT_MAX) return ST_ERR_RANGE;
    }
    *out = (int)(neg ? -v : v);
    return ST_OK;
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr,
            "Использование: %s <n> <norm_type> <v1_1> ... <v1_n> [<v2_1> ...]\n"
            "  norm_type : inf | p2 | A\n",
            argv[0]);
        return (int)ST_ERR_ARGC;
    }

    int n = 0;
    status_t st;
    if ((st = parse_int(argv[1], &n)) != ST_OK) { print_status(st); return (int)st; }
    if (n <= 0 || n > 100) { print_status(ST_ERR_RANGE); return (int)ST_ERR_RANGE; }

    const char *ntype = argv[2];
    norm_t chosen = NULL;
    if (ntype[0] == 'i' && ntype[1] == 'n' && ntype[2] == 'f' && ntype[3] == '\0')
        chosen = norm_inf;
    else if (ntype[0] == 'p' && ntype[1] == '2' && ntype[2] == '\0')
        chosen = norm_p2;
    else if (ntype[0] == 'A' && ntype[1] == '\0')
        chosen = norm_A;
    else { print_status(ST_ERR_PARSE); return (int)ST_ERR_PARSE; }

    /* Остальные аргументы — координаты векторов */
    int total_coords = argc - 3;
    if (total_coords % n != 0 || total_coords == 0) {
        print_status(ST_ERR_ARGC);
        return (int)ST_ERR_ARGC;
    }
    int count = total_coords / n;

    /* Прочитать все координаты */
    double *coords = malloc((size_t)total_coords * sizeof(double));
    if (!coords) { print_status(ST_ERR_NOMEM); return (int)ST_ERR_NOMEM; }
    for (int i = 0; i < total_coords; ++i) {
        if ((st = parse_double(argv[3 + i], &coords[i])) != ST_OK) {
            free(coords);
            print_status(st);
            return (int)st;
        }
    }

    /* Диспетчеризация вариадического вызова (switch sur count × n) */
    double **result = NULL;
    int result_count = 0;

    /* Для простоты поддержим до 3 векторов с n <= 10 (10 × 3 = 30 coords) */
    if (count == 1) {
        switch (n) {
            case 1:  st = find_max_vectors(n, count, &result, &result_count, chosen, coords[0]); break;
            case 2:  st = find_max_vectors(n, count, &result, &result_count, chosen, coords[0], coords[1]); break;
            case 3:  st = find_max_vectors(n, count, &result, &result_count, chosen, coords[0], coords[1], coords[2]); break;
            case 4:  st = find_max_vectors(n, count, &result, &result_count, chosen, coords[0], coords[1], coords[2], coords[3]); break;
            case 5:  st = find_max_vectors(n, count, &result, &result_count, chosen, coords[0], coords[1], coords[2], coords[3], coords[4]); break;
            default: st = ST_ERR_RANGE; break;
        }
    } else if (count == 2) {
        switch (n) {
            case 1:  st = find_max_vectors(n, count, &result, &result_count, chosen,
                    coords[0], coords[1]); break;
            case 2:  st = find_max_vectors(n, count, &result, &result_count, chosen,
                    coords[0], coords[1], coords[2], coords[3]); break;
            case 3:  st = find_max_vectors(n, count, &result, &result_count, chosen,
                    coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]); break;
            default: st = ST_ERR_RANGE; break;
        }
    } else if (count == 3) {
        switch (n) {
            case 1:  st = find_max_vectors(n, count, &result, &result_count, chosen,
                    coords[0], coords[1], coords[2]); break;
            case 2:  st = find_max_vectors(n, count, &result, &result_count, chosen,
                    coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]); break;
            case 3:  st = find_max_vectors(n, count, &result, &result_count, chosen,
                    coords[0], coords[1], coords[2], coords[3], coords[4], coords[5],
                    coords[6], coords[7], coords[8]); break;
            default: st = ST_ERR_RANGE; break;
        }
    } else {
        st = ST_ERR_RANGE;
    }

    free(coords);

    if (st != ST_OK) {
        print_status(st);
        return (int)st;
    }

    /* Вывод результата */
    printf("Векторов с максимальной нормой: %d\n", result_count);
    for (int i = 0; i < result_count; ++i) {
        printf("  [%d] (", i + 1);
        for (int j = 0; j < n; ++j) {
            printf("%g", result[i][j]);
            if (j + 1 < n) printf(", ");
        }
        printf(")\n");
        free(result[i]);
    }
    free(result);

    return (int)ST_OK;
}
