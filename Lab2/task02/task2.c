/* ============================================================
 * Задание 2 — Сортировка сотрудников (qsort)
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task2 task2.c
 * Запуск:     ./task2 <входной> <флаг> <выходной>
 *   флаг: -a (по возрастанию) или -d (по убыванию)
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#define LINE_BUF 512

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_OPEN_IN,
    ST_ERR_OPEN_OUT,
    ST_ERR_READ,
    ST_ERR_WRITE,
    ST_ERR_PARSE,
    ST_ERR_RANGE,
    ST_ERR_NOMEM,
    ST_ERR_BAD_NAME
} status_t;

/* ---------- Структура Employee ---------- */
typedef struct {
    unsigned int  id;
    char         *name;
    char         *surname;
    double        salary;
} Employee;

/* ---------- Коды для qsort ---------- */
typedef enum {
    SORT_OK = 0,
    SORT_ERR_EMPTY
} sort_status_t;

/* ============================================================
 *  Проверка флага
 * ============================================================ */
static int is_valid_flag(const char *arg, char *out)
{
    if (!arg || !out) return 0;
    if (arg[0] != '-' && arg[0] != '/') return 0;
    if (arg[1] == '\0' || arg[2] != '\0') return 0;
    char c = (char)tolower((unsigned char)arg[1]);
    if (c != 'a' && c != 'd') return 0;
    *out = c;
    return 1;
}

/* ============================================================
 *  Собственный strlen (без string.h)
 * ============================================================ */
static size_t my_strlen(const char *s)
{
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

/* ============================================================
 *  Разбор unsigned int без errno
 * ============================================================ */
static status_t parse_uint(const char *s, unsigned int *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;
    if (*s == '-' || *s == '+') return ST_ERR_PARSE;

    unsigned long long v = 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return ST_ERR_PARSE;
        v = v * 10ULL + (unsigned long long)(*p - '0');
        if (v > UINT_MAX) return ST_ERR_RANGE;
    }
    *out = (unsigned int)v;
    return ST_OK;
}

/* ============================================================
 *  Разбор double без errno
 * ============================================================ */
static status_t parse_double(const char *s, double *out)
{
    if (!s || !out || *s == '\0') return ST_ERR_PARSE;

    /* autorise : chiffres, '.', '-', '+' */
    const char *p = s;
    int neg = 0;
    if (*p == '-' || *p == '+') { neg = (*p == '-'); ++p; }
    if (*p == '\0') return ST_ERR_PARSE;

    int dot_count = 0;
    for (const char *q = p; *q; ++q) {
        if (*q == '.') { ++dot_count; continue; }
        if (*q < '0' || *q > '9') return ST_ERR_PARSE;
    }
    if (dot_count > 1) return ST_ERR_PARSE;
    if (*p == '.' && !isdigit((unsigned char)p[1])) return ST_ERR_PARSE;

    /* utilisation de strtod uniquement sur entrée validée */
    double v = 0.0;
    int int_part = 1;
    double frac = 0.0, scale = 1.0;
    for (const char *q = p; *q; ++q) {
        if (*q == '.') { int_part = 0; continue; }
        int d = *q - '0';
        if (int_part) v = v * 10.0 + (double)d;
        else { scale *= 10.0; frac += (double)d / scale; }
    }
    v += frac;
    if (neg) v = -v;

    /* проверка на NaN/Inf (не должно возникать, но проверяем) */
    if (v != v) return ST_ERR_PARSE;

    *out = v;
    return ST_OK;
}

/* ============================================================
 *  Проверка: строка только из латинских букв, непустая
 * ============================================================ */
static int is_valid_name(const char *s)
{
    if (!s || *s == '\0') return 0;
    for (const char *p = s; *p; ++p) {
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')))
            return 0;
    }
    return 1;
}

/* ============================================================
 *  Копирование строки без string.h
 * ============================================================ */
static char *my_strdup(const char *s)
{
    if (!s) return NULL;
    size_t n = my_strlen(s);
    char *d = malloc(n + 1);
    if (!d) return NULL;
    for (size_t i = 0; i <= n; ++i) d[i] = s[i];
    return d;
}

/* ============================================================
 *  Разбор одной строки в Employee
 *  Формат : id name surname salary
 * ============================================================ */
static status_t parse_employee(const char *line, Employee *emp)
{
    if (!line || !emp) return ST_ERR_PARSE;

    char id_buf[32], name_buf[128], surname_buf[128], sal_buf[64];
    int field = 0;
    const char *p = line;

    while (*p && field < 4) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;

        const char *start = p;
        while (*p && *p != ' ' && *p != '\t') ++p;
        size_t len = (size_t)(p - start);

        char *dst = NULL;
        size_t cap = 0;
        switch (field) {
            case 0: dst = id_buf;      cap = sizeof(id_buf);      break;
            case 1: dst = name_buf;    cap = sizeof(name_buf);    break;
            case 2: dst = surname_buf; cap = sizeof(surname_buf); break;
            case 3: dst = sal_buf;     cap = sizeof(sal_buf);     break;
        }
        if (len + 1 > cap) return ST_ERR_PARSE;
        for (size_t i = 0; i < len; ++i) dst[i] = start[i];
        dst[len] = '\0';
        ++field;
    }

    if (field != 4) return ST_ERR_PARSE;

    status_t st;
    unsigned int id = 0;
    double sal = 0.0;
    if ((st = parse_uint(id_buf, &id)) != ST_OK) return st;
    if ((st = parse_double(sal_buf, &sal)) != ST_OK) return st;
    if (sal < 0.0) return ST_ERR_RANGE;
    if (!is_valid_name(name_buf)) return ST_ERR_BAD_NAME;
    if (!is_valid_name(surname_buf)) return ST_ERR_BAD_NAME;

    emp->name = my_strdup(name_buf);
    if (!emp->name) return ST_ERR_NOMEM;
    emp->surname = my_strdup(surname_buf);
    if (!emp->surname) { free(emp->name); return ST_ERR_NOMEM; }
    emp->id = id;
    emp->salary = sal;
    return ST_OK;
}

/* ============================================================
 *  Сравнение строк (лексикографическое)
 * ============================================================ */
static int my_strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return (unsigned char)*a - (unsigned char)*b;
}

/* ============================================================
 *  Глобальные указатели для qsort (НЕ глобальные переменные,
 *  а static внутри функции — но проще передать через контекст)
 *  → on utilise des fonctions statiques et un flag static via
 *    un struct global interdit... on va plutôt définir
 *    2 comparateurs (ascendant et descendant).
 * ============================================================ */

static int cmp_asc(const void *a, const void *b)
{
    const Employee *ea = (const Employee *)a;
    const Employee *eb = (const Employee *)b;

    /* 1. зарплата */
    if (ea->salary < eb->salary) return -1;
    if (ea->salary > eb->salary) return  1;

    /* 2. фамилия */
    int c = my_strcmp(ea->surname, eb->surname);
    if (c != 0) return c;

    /* 3. имя */
    c = my_strcmp(ea->name, eb->name);
    if (c != 0) return c;

    /* 4. id */
    if (ea->id < eb->id) return -1;
    if (ea->id > eb->id) return  1;
    return 0;
}

static int cmp_desc(const void *a, const void *b)
{
    return -cmp_asc(a, b);
}

/* ============================================================
 *  Чтение файла → массив Employee
 * ============================================================ */
static status_t read_file(const char *path, Employee **arr, size_t *n_out)
{
    FILE *f = fopen(path, "r");
    if (!f) return ST_ERR_OPEN_IN;

    size_t cap = 16, n = 0;
    Employee *a = malloc(cap * sizeof(Employee));
    if (!a) { fclose(f); return ST_ERR_NOMEM; }

    char line[LINE_BUF];
    while (fgets(line, sizeof(line), f)) {
        /* убрать \n */
        size_t len = my_strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (line[0] == '\0') continue;

        if (n == cap) {
            size_t ncap = cap * 2;
            Employee *tmp = realloc(a, ncap * sizeof(Employee));
            if (!tmp) {
                for (size_t i = 0; i < n; ++i) { free(a[i].name); free(a[i].surname); }
                free(a); fclose(f);
                return ST_ERR_NOMEM;
            }
            a = tmp; cap = ncap;
        }

        status_t st = parse_employee(line, &a[n]);
        if (st != ST_OK) {
            for (size_t i = 0; i < n; ++i) { free(a[i].name); free(a[i].surname); }
            free(a); fclose(f);
            return st;
        }
        ++n;
    }

    if (ferror(f)) {
        for (size_t i = 0; i < n; ++i) { free(a[i].name); free(a[i].surname); }
        free(a); fclose(f);
        return ST_ERR_READ;
    }
    fclose(f);

    if (n == 0) {
        free(a);
        *arr = NULL;
        *n_out = 0;
        return ST_OK;
    }
    *arr = a;
    *n_out = n;
    return ST_OK;
}

/* ============================================================
 *  Запись массива в файл
 * ============================================================ */
static status_t write_file(const char *path, const Employee *arr, size_t n)
{
    FILE *f = fopen(path, "w");
    if (!f) return ST_ERR_OPEN_OUT;

    for (size_t i = 0; i < n; ++i) {
        if (fprintf(f, "%u %s %s %.6f\n",
                    arr[i].id, arr[i].name, arr[i].surname, arr[i].salary) < 0) {
            fclose(f);
            return ST_ERR_WRITE;
        }
    }
    if (fclose(f) != 0) return ST_ERR_WRITE;
    return ST_OK;
}

/* ---------- Вывод статуса ---------- */
static void print_status(status_t st, const char *arg)
{
    switch (st) {
        case ST_OK:            break;
        case ST_ERR_ARGC:      fprintf(stderr, "Ошибка: неверное число аргументов.\n"); break;
        case ST_ERR_FLAG:      fprintf(stderr, "Ошибка: неверный флаг '%s'.\n", arg ? arg : "?"); break;
        case ST_ERR_OPEN_IN:   fprintf(stderr, "Ошибка: не удалось открыть '%s' для чтения.\n", arg); break;
        case ST_ERR_OPEN_OUT:  fprintf(stderr, "Ошибка: не удалось открыть '%s' для записи.\n", arg); break;
        case ST_ERR_READ:      fprintf(stderr, "Ошибка: чтение файла.\n"); break;
        case ST_ERR_WRITE:     fprintf(stderr, "Ошибка: запись файла.\n"); break;
        case ST_ERR_PARSE:     fprintf(stderr, "Ошибка: неверный формат записи.\n"); break;
        case ST_ERR_RANGE:     fprintf(stderr, "Ошибка: значение вне диапазона.\n"); break;
        case ST_ERR_NOMEM:     fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_BAD_NAME:  fprintf(stderr, "Ошибка: имя/фамилия должны быть только из латинских букв.\n"); break;
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <входной> <-a|-d> <выходной>\n", argv[0]);
        return (int)ST_ERR_ARGC;
    }

    char flag = '\0';
    if (!is_valid_flag(argv[2], &flag)) {
        print_status(ST_ERR_FLAG, argv[2]);
        return (int)ST_ERR_FLAG;
    }

    Employee *arr = NULL;
    size_t n = 0;
    status_t st = read_file(argv[1], &arr, &n);
    if (st != ST_OK) {
        print_status(st, argv[1]);
        return (int)st;
    }

    if (n > 0) {
        qsort(arr, n, sizeof(Employee), flag == 'a' ? cmp_asc : cmp_desc);
    }

    st = write_file(argv[3], arr, n);
    if (st != ST_OK) {
        print_status(st, argv[3]);
        for (size_t i = 0; i < n; ++i) { free(arr[i].name); free(arr[i].surname); }
        free(arr);
        return (int)st;
    }

    for (size_t i = 0; i < n; ++i) { free(arr[i].name); free(arr[i].surname); }
    free(arr);

    printf("Готово: '%s' -> '%s' (%zu записей)\n", argv[1], argv[3], n);
    return (int)ST_OK;
}
