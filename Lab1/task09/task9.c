/* ============================================================
 * Задание 9 — Массивы фиксированные и динамические
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task9 task9.c
 * Запуск:
 *   ./task9 -1 <a> <b>
 *   ./task9 -2
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#define FIXED_N 100

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_NOMEM
} status_t;

/* ---------- Разбор ---------- */
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

/* ---------- Разбор флага ---------- */
static int parse_flag(const char *arg, char *out)
{
    if (!arg || !out) return 0;
    if (arg[0] != '-' && arg[0] != '/') return 0;
    if (arg[1] == '\0' || arg[2] != '\0') return 0;
    if (arg[1] != '1' && arg[1] != '2') return 0;
    *out = arg[1];
    return 1;
}

/* ---------- Генерация целого в [lo, hi] ---------- */
static int rand_range_int(int lo, int hi)
{
    if (lo > hi) { int t = lo; lo = hi; hi = t; }
    long range = (long)hi - (long)lo + 1;
    return lo + (int)(rand() % range);
}

/* ============================================================
 *  9.1 — Фиксированный массив: min/max за один проход
 * ============================================================ */
static void find_and_swap_extremes(int *arr, int n)
{
    if (n < 2) return;
    int min_idx = 0, max_idx = 0;
    for (int i = 1; i < n; ++i) {
        if (arr[i] < arr[min_idx]) min_idx = i;
        if (arr[i] > arr[max_idx]) max_idx = i;
    }
    if (min_idx != max_idx) {
        int t = arr[min_idx];
        arr[min_idx] = arr[max_idx];
        arr[max_idx] = t;
    }
}

static status_t task_9_1(long a, long b)
{
    if (a > b) return ST_ERR_RANGE;
    if (a < INT_MIN || b > INT_MAX) return ST_ERR_RANGE;

    int arr[FIXED_N];
    for (int i = 0; i < FIXED_N; ++i)
        arr[i] = rand_range_int((int)a, (int)b);

    printf("Исходный массив (N=%d, [%ld..%ld]):\n", FIXED_N, a, b);
    for (int i = 0; i < FIXED_N; ++i) {
        printf("%5d", arr[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");

    int min_before = arr[0], max_before = arr[0];
    for (int i = 1; i < FIXED_N; ++i) {
        if (arr[i] < min_before) min_before = arr[i];
        if (arr[i] > max_before) max_before = arr[i];
    }
    printf("Минимум = %d, Максимум = %d\n", min_before, max_before);

    find_and_swap_extremes(arr, FIXED_N);

    printf("Массив после обмена min<->max:\n");
    for (int i = 0; i < FIXED_N; ++i) {
        printf("%5d", arr[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");
    return ST_OK;
}

/* ============================================================
 *  9.2 — Динамические массивы
 * ============================================================ */
static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static int closest_in_sorted(const int *sorted, int n, int key)
{
    if (n == 0) return 0;
    int lo = 0, hi = n - 1;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (sorted[mid] < key) lo = mid + 1;
        else                   hi = mid;
    }
    int best = sorted[lo];
    int best_dist = abs(best - key);
    if (lo > 0) {
        int d2 = abs(sorted[lo - 1] - key);
        if (d2 < best_dist) { best = sorted[lo - 1]; best_dist = d2; }
    }
    return best;
}

static status_t task_9_2(void)
{
    int n = rand_range_int(10, 10000);
    int m = rand_range_int(10, 10000);

    printf("Размеры: n=%d, m=%d\n", n, m);

    int *A = malloc((size_t)n * sizeof(int));
    int *B = malloc((size_t)m * sizeof(int));
    int *C = malloc((size_t)n * sizeof(int));
    int *Bsorted = malloc((size_t)m * sizeof(int));
    if (!A || !B || !C || !Bsorted) {
        free(A); free(B); free(C); free(Bsorted);
        return ST_ERR_NOMEM;
    }

    for (int i = 0; i < n; ++i) A[i] = rand_range_int(-1000, 1000);
    for (int i = 0; i < m; ++i) B[i] = rand_range_int(-1000, 1000);

    memcpy(Bsorted, B, (size_t)m * sizeof(int));
    qsort(Bsorted, (size_t)m, sizeof(int), cmp_int);

    for (int i = 0; i < n; ++i) {
        int closest = closest_in_sorted(Bsorted, m, A[i]);
        C[i] = A[i] + closest;
    }

    int show = (n < 20) ? n : 20;
    printf("Первые элементы (A[i], ближайший_B, C[i]):\n");
    for (int i = 0; i < show; ++i) {
        int closest = closest_in_sorted(Bsorted, m, A[i]);
        printf("  A[%d]=%5d  ближайший(B)=%5d  C[%d]=%5d\n",
               i, A[i], closest, i, C[i]);
    }
    if (n > show) printf("  ... (всего %d элементов)\n", n);

    free(A); free(B); free(C); free(Bsorted);
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st)
{
    switch (st) {
        case ST_OK:        break;
        case ST_ERR_ARGC:  fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_FLAG:  fprintf(stderr, "Ошибка: неверный флаг (нужен -1 или -2).\n"); break;
        case ST_ERR_PARSE: fprintf(stderr, "Ошибка: параметр не число.\n"); break;
        case ST_ERR_RANGE: fprintf(stderr, "Ошибка: параметр вне диапазона.\n"); break;
        case ST_ERR_NOMEM: fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,
            "Использование:\n"
            "  %s -1 <a> <b>   (фиксированный массив, N=%d)\n"
            "  %s -2           (динамические массивы)\n",
            argv[0], FIXED_N, argv[0]);
        return (int)ST_ERR_ARGC;
    }

    char flag = '\0';
    if (!parse_flag(argv[1], &flag)) { print_status(ST_ERR_FLAG); return (int)ST_ERR_FLAG; }

    srand((unsigned)time(NULL) ^ (unsigned)(size_t)&flag);

    status_t st = ST_OK;

    if (flag == '1') {
        if (argc != 4) { print_status(ST_ERR_ARGC); return (int)ST_ERR_ARGC; }
        long a, b;
        if ((st = parse_long(argv[2], &a)) != ST_OK) { print_status(st); return (int)st; }
        if ((st = parse_long(argv[3], &b)) != ST_OK) { print_status(st); return (int)st; }
        st = task_9_1(a, b);
    } else {
        if (argc != 2) { print_status(ST_ERR_ARGC); return (int)ST_ERR_ARGC; }
        st = task_9_2();
    }

    if (st != ST_OK) print_status(st);
    return (int)st;
}
