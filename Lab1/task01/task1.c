/* ============================================================
 * Задание 1 — Вычисления через аргументы командной строки
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task1 task1.c
 * Запуск:     ./task1 -<флаг> <x>
 *
 * Флаги:
 *   -h  числа, кратные x, в диапазоне [1..100]
 *   -p  простое ли число x
 *   -s  шестнадцатеричные цифры числа x
 *   -e  таблица степеней (x <= 10)
 *   -a  сумма 1..x
 *   -f  факториал x
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,       /* неверное число аргументов */
    ST_ERR_FLAG,       /* неизвестный флаг */
    ST_ERR_PARSE,      /* x не является числом */
    ST_ERR_RANGE,      /* x вне диапазона */
    ST_ERR_NOMEM,      /* не удалось выделить память */
    ST_ERR_DIVZERO,    /* деление на ноль */
    ST_ERR_NO_MULT,    /* нет кратных */
    ST_ERR_OVERFLOW    /* переполнение */
} status_t;

/* ---------- Проверка флага ---------- */
static int is_valid_flag(const char *flag, char *out)
{
    if (!flag || !out) return 0;
    if (flag[0] != '-' && flag[0] != '/') return 0;
    if (flag[1] == '\0' || flag[2] != '\0') return 0;  /* ровно 1 буква */
    char c = (char)tolower((unsigned char)flag[1]);
    if (strchr("hpseaf", c) == NULL) return 0;
    *out = c;
    return 1;
}

/* ---------- Строгий разбор целого числа ---------- */
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

/* ---------- Флаг -h: кратные x в [1..100] ---------- */
static status_t do_h(long x, int *found_count)
{
    if (found_count) *found_count = 0;
    if (x == 0) return ST_ERR_DIVZERO;
    long ax = (x < 0) ? -x : x;

    int count = 0;
    printf("Числа, кратные %ld, в диапазоне [1..100]:\n", x);
    for (long n = 1; n <= 100; ++n) {
        if (n % ax == 0) {
            printf("%ld ", n);
            ++count;
        }
    }
    printf("\n");
    if (found_count) *found_count = count;
    return (count == 0) ? ST_ERR_NO_MULT : ST_OK;
}

/* ---------- Флаг -p: простое / составное ---------- */
static status_t do_p(long x, int *is_prime)
{
    if (is_prime) *is_prime = 0;
    if (x < 2) return ST_ERR_RANGE;
    if (x == 2) { if (is_prime) *is_prime = 1; return ST_OK; }
    if (x % 2 == 0) return ST_OK;
    for (long d = 3; d * d <= x; d += 2) {
        if (x % d == 0) return ST_OK;
    }
    if (is_prime) *is_prime = 1;
    return ST_OK;
}

/* ---------- Флаг -s: цифры в 16-ричной системе ---------- */
static status_t do_s(long x)
{
    if (x < 0) return ST_ERR_RANGE;
    if (x == 0) { printf("0\n"); return ST_OK; }

    char buf[32];
    int n = 0;
    unsigned long ux = (unsigned long)x;
    while (ux > 0 && n < (int)sizeof(buf)) {
        int d = (int)(ux & 0xF);
        buf[n++] = (d < 10) ? (char)('0' + d) : (char)('A' + d - 10);
        ux >>= 4;
    }
    /* вывод от старших разрядов к младшим */
    for (int i = n - 1; i >= 0; --i) {
        putchar(buf[i]);
        if (i > 0) putchar(' ');
    }
    putchar('\n');
    return ST_OK;
}

/* ---------- Флаг -e: таблица степеней (x <= 10) ---------- */
static status_t do_e(long x)
{
    if (x < 1 || x > 10) return ST_ERR_RANGE;

    printf("Таблица степеней (показатели 1..%ld, основания 1..10):\n", x);
    printf("Осн\\Пок");
    for (long e = 1; e <= x; ++e) printf("%12ld", e);
    printf("\n");

    for (int b = 1; b <= 10; ++b) {
        printf("%7d", b);
        for (long e = 1; e <= x; ++e) {
            double val = 1.0;
            for (long k = 0; k < e; ++k) val *= (double)b;
            printf("%12.4g", val);
        }
        printf("\n");
    }
    return ST_OK;
}

/* ---------- Флаг -a: сумма 1..x ---------- */
static status_t do_a(long x, long long *sum)
{
    if (sum) *sum = 0;
    if (x < 1) return ST_ERR_RANGE;
    /* проверка переполнения: n(n+1)/2 */
    if (x > 2000000000L) return ST_ERR_OVERFLOW;
    long long s = (long long)x * (long long)(x + 1) / 2;
    if (sum) *sum = s;
    return ST_OK;
}

/* ---------- Флаг -f: факториал ---------- */
static status_t do_f(long x, unsigned long long *fact)
{
    if (fact) *fact = 1ULL;
    if (x < 0) return ST_ERR_RANGE;
    if (x > 20) return ST_ERR_OVERFLOW;  /* 20! помещается в ULL */

    unsigned long long r = 1ULL;
    for (long i = 2; i <= x; ++i) r *= (unsigned long long)i;
    if (fact) *fact = r;
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st, const char *flag)
{
    switch (st) {
        case ST_OK:            break;
        case ST_ERR_ARGC:      fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_FLAG:      fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", flag ? flag : "?"); break;
        case ST_ERR_PARSE:     fprintf(stderr, "Ошибка: x не является целым числом.\n"); break;
        case ST_ERR_RANGE:     fprintf(stderr, "Ошибка: x вне допустимого диапазона.\n"); break;
        case ST_ERR_NOMEM:     fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_DIVZERO:   fprintf(stderr, "Ошибка: деление на ноль (x = 0).\n"); break;
        case ST_ERR_NO_MULT:   printf("Чисел, кратных x, в диапазоне [1..100] нет.\n"); break;
        case ST_ERR_OVERFLOW:  fprintf(stderr, "Ошибка: переполнение.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Использование: %s -<h|p|s|e|a|f> <x>\n", argv[0]);
        return (int)ST_ERR_ARGC;
    }

    /* 1. Проверка флага */
    char flag = '\0';
    if (!is_valid_flag(argv[1], &flag)) {
        print_status(ST_ERR_FLAG, argv[1]);
        return (int)ST_ERR_FLAG;
    }

    /* 2. Разбор x */
    long x = 0;
    status_t st = parse_long(argv[2], &x);
    if (st != ST_OK) {
        print_status(st, NULL);
        return (int)st;
    }

    /* 3. Диспетчеризация */
    switch (flag) {
        case 'h': {
            int found = 0;
            st = do_h(x, &found);
            if (st != ST_OK && st != ST_ERR_NO_MULT) print_status(st, NULL);
            break;
        }
        case 'p': {
            int is_prime = 0;
            st = do_p(x, &is_prime);
            if (st == ST_OK)
                printf("%ld — %s.\n", x, is_prime ? "простое" : "составное");
            else
                print_status(st, NULL);
            break;
        }
        case 's': {
            st = do_s(x);
            if (st != ST_OK) print_status(st, NULL);
            break;
        }
        case 'e': {
            st = do_e(x);
            if (st != ST_OK) print_status(st, NULL);
            break;
        }
        case 'a': {
            long long s = 0;
            st = do_a(x, &s);
            if (st == ST_OK) printf("Сумма 1..%ld = %lld\n", x, s);
            else             print_status(st, NULL);
            break;
        }
        case 'f': {
            unsigned long long f = 1ULL;
            st = do_f(x, &f);
            if (st == ST_OK) printf("%ld! = %llu\n", x, f);
            else             print_status(st, NULL);
            break;
        }
        default:
            st = ST_ERR_FLAG;
            print_status(st, argv[1]);
            break;
    }

    return (int)st;
}
