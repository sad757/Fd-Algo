/* ============================================================
 * Задание 8 — Сложение больших чисел в произвольной системе
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task8 task8.c
 * Запуск:     ./task8 <base> <n1> <n2> ... <nk>
 *
 * Вариадическая функция суммирует числа, представленные строками
 * в заданной системе счисления [2..36]. Результат — строка без
 * ведущих нулей.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_DIGITS 4096

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
 *  Значение символа (0..35) или -1
 * ============================================================ */
static int char_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

/* ============================================================
 *  Символ из значения (0..35)
 * ============================================================ */
static char value_char(int d, int uppercase)
{
    if (d < 10) return (char)('0' + d);
    return (char)((uppercase ? 'A' : 'a') + d - 10);
}

/* ============================================================
 *  Сложение двух чисел в base как строк
 *  Результат выделяется динамически, без ведущих нулей.
 * ============================================================ */
static status_t add_in_base(const char *a, const char *b, int base,
                            char **out)
{
    if (!a || !b || !out) return ST_ERR_PARSE;
    if (base < 2 || base > 36) return ST_ERR_RANGE;

    size_t la = my_strlen(a);
    size_t lb = my_strlen(b);
    size_t maxlen = (la > lb) ? la : lb;

    /* maxlen + 1 для переноса */
    char *res = malloc(maxlen + 2);
    if (!res) return ST_ERR_NOMEM;

    int carry = 0;
    size_t k = 0;
    for (size_t i = 0; i < maxlen; ++i) {
        int da = (i < la) ? char_value(a[la - 1 - i]) : 0;
        int db = (i < lb) ? char_value(b[lb - 1 - i]) : 0;
        if (da < 0 || db < 0 || da >= base || db >= base) {
            free(res);
            return ST_ERR_PARSE;
        }
        int sum = da + db + carry;
        res[k++] = value_char(sum % base, 0);
        carry = sum / base;
    }
    if (carry > 0) {
        res[k++] = value_char(carry, 0);
    }
    res[k] = '\0';

    /* Инвертировать (получили с младших разрядов) */
    for (size_t i = 0; i < k / 2; ++i) {
        char t = res[i]; res[i] = res[k - 1 - i]; res[k - 1 - i] = t;
    }

    /* Убрать ведущие нули */
    size_t start = 0;
    while (start + 1 < k && res[start] == '0') ++start;
    if (start > 0) {
        size_t new_len = k - start;
        for (size_t i = 0; i < new_len; ++i) res[i] = res[start + i];
        res[new_len] = '\0';
    }

    *out = res;
    return ST_OK;
}

/* ============================================================
 *  Сумма N чисел в заданной системе
 * ============================================================ */
static status_t sum_in_base(int base, int count,
                            char **out, ...)
{
    if (base < 2 || base > 36 || count <= 0 || !out) return ST_ERR_EMPTY;

    va_list ap;
    va_start(ap, out);

    /* Начинаем с "0" */
    char *acc = malloc(2);
    if (!acc) { va_end(ap); return ST_ERR_NOMEM; }
    acc[0] = '0'; acc[1] = '\0';

    status_t st = ST_OK;
    for (int i = 0; i < count; ++i) {
        const char *s = va_arg(ap, const char *);
        char *next = NULL;
        st = add_in_base(acc, s, base, &next);
        free(acc);
        if (st != ST_OK) { va_end(ap); return st; }
        acc = next;
    }
    va_end(ap);

    *out = acc;
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st)
{
    switch (st) {
        case ST_OK:        break;
        case ST_ERR_ARGC:  fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_PARSE: fprintf(stderr, "Ошибка: неверный формат числа.\n"); break;
        case ST_ERR_RANGE: fprintf(stderr, "Ошибка: основание вне [2..36].\n"); break;
        case ST_ERR_NOMEM: fprintf(stderr, "Ошибка: нет памяти.\n"); break;
        case ST_ERR_EMPTY: fprintf(stderr, "Ошибка: пустой ввод.\n"); break;
    }
}

/* ---------- Разбор int без errno ---------- */
static status_t parse_int(const char *s, int *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    if (*s == '-' || *s == '+') return ST_ERR_PARSE;
    long long v = 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return ST_ERR_PARSE;
        v = v * 10 + (*p - '0');
        if (v > 1000000) return ST_ERR_RANGE;
    }
    *out = (int)v;
    return ST_OK;
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Использование: %s <base> <n1> <n2> ... <nk>\n", argv[0]);
        return (int)ST_ERR_ARGC;
    }

    int base = 0;
    status_t st;
    if ((st = parse_int(argv[1], &base)) != ST_OK) { print_status(st); return (int)st; }
    if (base < 2 || base > 36) { print_status(ST_ERR_RANGE); return (int)ST_ERR_RANGE; }

    int count = argc - 2;
    char *result = NULL;

    switch (count) {
        case 1: st = sum_in_base(base, 1, &result, argv[2]); break;
        case 2: st = sum_in_base(base, 2, &result, argv[2], argv[3]); break;
        case 3: st = sum_in_base(base, 3, &result, argv[2], argv[3], argv[4]); break;
        case 4: st = sum_in_base(base, 4, &result, argv[2], argv[3], argv[4], argv[5]); break;
        case 5: st = sum_in_base(base, 5, &result, argv[2], argv[3], argv[4], argv[5], argv[6]); break;
        case 6: st = sum_in_base(base, 6, &result, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]); break;
        case 7: st = sum_in_base(base, 7, &result, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]); break;
        case 8: st = sum_in_base(base, 8, &result, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]); break;
        default:
            fprintf(stderr, "Слишком много чисел (максимум 8).\n");
            return (int)ST_ERR_ARGC;
    }

    if (st != ST_OK) { print_status(st); return (int)st; }

    printf("Сумма в системе %d : %s\n", base, result);
    free(result);
    return (int)ST_OK;
}
