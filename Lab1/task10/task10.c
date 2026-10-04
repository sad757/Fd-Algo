/* ============================================================
 * Задание 10 — Интерактивный ввод, разные системы счисления
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task10 task10.c
 * Запуск:     ./task10
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#define LINE_BUF 256
#define MAX_NUMS 10000

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_OVERFLOW,
    ST_ERR_NOMEM,
    ST_ERR_READ,
    ST_ERR_BAD_NUM,
    ST_ERR_EMPTY
} status_t;

/* ---------- Разбор основания ---------- */
static status_t parse_base(const char *s, int *base)
{
    if (!s || !base || *s == '\0') return ST_ERR_PARSE;
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno == ERANGE) return ST_ERR_RANGE;
    if (end == s || *end != '\0') return ST_ERR_PARSE;
    if (v < 2 || v > 36) return ST_ERR_RANGE;
    *base = (int)v;
    return ST_OK;
}

/* ---------- Значение символа (0..35) или -1 ---------- */
static int char_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

/* ---------- Разбор числа в базе b ---------- */
static status_t parse_number(const char *s, int base, long long *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;

    int neg = 0;
    const char *p = s;
    if (*p == '+' || *p == '-') {
        neg = (*p == '-');
        ++p;
    }
    if (*p == '\0') return ST_ERR_PARSE;

    unsigned long long v = 0;
    unsigned long long max_pos = neg ? (unsigned long long)LLONG_MAX + 1ULL
                                     : (unsigned long long)LLONG_MAX;
    for (; *p; ++p) {
        int d = char_value(*p);
        if (d < 0 || d >= base) return ST_ERR_BAD_NUM;
        if (v > (max_pos - (unsigned long long)d) / (unsigned long long)base)
            return ST_ERR_OVERFLOW;
        v = v * (unsigned long long)base + (unsigned long long)d;
    }
    if (neg) {
        if (v == (unsigned long long)LLONG_MAX + 1ULL) *out = LLONG_MIN;
        else *out = -(long long)v;
    } else {
        *out = (long long)v;
    }
    return ST_OK;
}

/* ---------- Преобразование long long в строку в базе b ---------- */
static void to_base_str(long long v, int base, char *out, size_t sz)
{
    if (base < 2 || base > 36 || sz < 2) { if (sz) out[0] = '\0'; return; }

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
            tmp[n++] = (d < 10) ? (char)('0' + d) : (char)('A' + d - 10);
            uv /= (unsigned long long)base;
        }
    }
    size_t i = 0;
    if (neg && i + 1 < sz) out[i++] = '-';
    while (n > 0 && i + 1 < sz) out[i++] = tmp[--n];
    out[i] = '\0';
}

/* ---------- Чтение строки ---------- */
static status_t read_line(char *buf, size_t sz)
{
    if (!fgets(buf, (int)sz, stdin)) return ST_ERR_READ;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return ST_OK;
}

/* ---------- Сравнение строк без учёта регистра ---------- */
static int str_ieq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        ++a; ++b;
    }
    return (*a == '\0' && *b == '\0');
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    char line[LINE_BUF];

    printf("=== Задание 10: перевод в разные системы счисления ===\n");

    /* --- 1. Ввод основания --- */
    int base = 0;
    while (1) {
        printf("Основание [2..36]: ");
        fflush(stdout);
        status_t st = read_line(line, sizeof(line));
        if (st != ST_OK) { fprintf(stderr, "Ошибка чтения.\n"); return (int)ST_ERR_READ; }
        st = parse_base(line, &base);
        if (st == ST_OK) break;
        fprintf(stderr, "  -> неверное основание, попробуйте снова.\n");
    }
    printf("Основание = %d\n", base);

    /* --- 2. Ввод чисел --- */
    long long *nums = malloc(MAX_NUMS * sizeof(long long));
    if (!nums) { fprintf(stderr, "Ошибка памяти.\n"); return (int)ST_ERR_NOMEM; }

    int count = 0;
    while (1) {
        printf("Число (или \"Stop\"): ");
        fflush(stdout);
        status_t st = read_line(line, sizeof(line));
        if (st != ST_OK) {
            fprintf(stderr, "Ошибка чтения.\n");
            free(nums); return (int)ST_ERR_READ;
        }

        if (str_ieq(line, "Stop") == 1) break;

        if (line[0] == '\0') {
            fprintf(stderr, "  -> пустая строка, попробуйте снова.\n");
            continue;
        }

        long long v = 0;
        st = parse_number(line, base, &v);
        if (st != ST_OK) {
            switch (st) {
                case ST_ERR_BAD_NUM:
                    fprintf(stderr, "  -> недопустимая цифра в основании %d.\n", base);
                    break;
                case ST_ERR_OVERFLOW:
                    fprintf(stderr, "  -> переполнение.\n");
                    break;
                default:
                    fprintf(stderr, "  -> неверное число.\n");
                    break;
            }
            continue;
        }
        if (count >= MAX_NUMS) {
            fprintf(stderr, "  -> достигнут лимит в %d чисел.\n", MAX_NUMS);
            continue;
        }
        nums[count++] = v;
    }

    if (count == 0) {
        fprintf(stderr, "Не введено ни одного числа.\n");
        free(nums);
        return (int)ST_ERR_EMPTY;
    }

    /* --- 3. Вычисления --- */
    long long sum = 0;
    long long max_abs = 0;
    int max_abs_idx = -1;
    int overflow_sum = 0;

    for (int i = 0; i < count; ++i) {
        if (!overflow_sum) {
            long long s = sum + nums[i];
            if ((nums[i] > 0 && s < sum) || (nums[i] < 0 && s > sum)) overflow_sum = 1;
            else sum = s;
        }
        unsigned long long uav = (nums[i] < 0)
            ? (unsigned long long)(-(nums[i] + 1)) + 1ULL
            : (unsigned long long)nums[i];
        unsigned long long umax = (max_abs_idx < 0) ? 0ULL :
            ((nums[max_abs_idx] < 0)
                ? (unsigned long long)(-(nums[max_abs_idx] + 1)) + 1ULL
                : (unsigned long long)nums[max_abs_idx]);
        if (max_abs_idx < 0 || uav > umax) max_abs_idx = i;
    }
    max_abs = (max_abs_idx >= 0) ? nums[max_abs_idx] : 0;

    /* --- 4. Вывод --- */
    printf("\n=== Результаты ===\n");
    printf("Количество значений: %d\n", count);
    if (overflow_sum)
        printf("Сумма: ПЕРЕПОЛНЕНИЕ (слишком велика для long long)\n");
    else
        printf("Сумма: %lld\n", sum);
    printf("Максимум по модулю: %lld\n", max_abs);

    printf("\nНормализованные числа (основание %d):\n", base);
    int show = (count < 20) ? count : 20;
    for (int i = 0; i < show; ++i) {
        char buf[80];
        to_base_str(nums[i], base, buf, sizeof(buf));
        printf("  [%d] %s\n", i + 1, buf);
    }
    if (count > show) printf("  ... (всего %d чисел)\n", count);

    printf("\nПредставление суммы в основаниях 9, 18, 27, 36:\n");
    if (!overflow_sum) {
        int bases[] = { 9, 18, 27, 36 };
        for (int b = 0; b < 4; ++b) {
            char buf[80];
            to_base_str(sum, bases[b], buf, sizeof(buf));
            printf("  основание %2d: %s\n", bases[b], buf);
        }
    } else {
        printf("  (недоступно: сумма переполнена)\n");
    }

    printf("\nПредставление максимума по модулю в основаниях 9, 18, 27, 36:\n");
    {
        int bases[] = { 9, 18, 27, 36 };
        for (int b = 0; b < 4; ++b) {
            char buf[80];
            to_base_str(max_abs, bases[b], buf, sizeof(buf));
            printf("  основание %2d: %s\n", bases[b], buf);
        }
    }

    free(nums);
    return (int)ST_OK;
}
