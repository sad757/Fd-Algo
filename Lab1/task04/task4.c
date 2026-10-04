/* ============================================================
 * Задание 4 — Обработка файлов (флаги -d, -i, -s, -a)
 * Компиляция: gcc -std=c99 -Wall -Wextra -Wpedantic -o task4 task4.c
 * Запуск:
 *   ./task4 -<флаг> <входной>
 *   ./task4 -n<флаг> <входной> <выходной>
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define LINE_BUF 4096
#define OUT_PREFIX "out_"
#define OUT_PREFIX_LEN 4

/* ---------- Коды возврата ---------- */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_OPEN_IN,
    ST_ERR_OPEN_OUT,
    ST_ERR_READ,
    ST_ERR_WRITE,
    ST_ERR_NOMEM,
    ST_ERR_NAME_LONG
} status_t;

/* ---------- Разбор флага ---------- */
static status_t parse_flag(const char *arg, char *flag, int *has_n)
{
    if (!arg || !flag || !has_n) return ST_ERR_FLAG;
    if (arg[0] != '-' && arg[0] != '/') return ST_ERR_FLAG;
    *has_n = 0;
    int i = 1;
    if (tolower((unsigned char)arg[i]) == 'n') { *has_n = 1; ++i; }
    char c = (char)tolower((unsigned char)arg[i]);
    if (strchr("disa", c) == NULL) return ST_ERR_FLAG;
    if (arg[i + 1] != '\0') return ST_ERR_FLAG;
    *flag = c;
    return ST_OK;
}

/* ---------- Генерация имени выходного файла ---------- */
static status_t make_output_name(const char *input, char *out, size_t out_sz)
{
    size_t need = OUT_PREFIX_LEN + strlen(input) + 1;
    if (need > out_sz) return ST_ERR_NAME_LONG;
    strcpy(out, OUT_PREFIX);
    strcat(out, input);
    return ST_OK;
}

/* ============================================================
 *  Преобразования (разделение логики и ввода/вывода)
 * ============================================================ */

/* -d: удалить арабские цифры */
static status_t transform_d(FILE *in, FILE *out)
{
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (!isdigit((unsigned char)c)) {
            if (fputc(c, out) == EOF) return ST_ERR_WRITE;
        }
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

/* -i: посчитать латинские буквы в каждой строке */
static status_t transform_i(FILE *in, FILE *out)
{
    char line[LINE_BUF];
    while (fgets(line, sizeof(line), in)) {
        int count = 0;
        for (const char *p = line; *p; ++p) {
            if (isalpha((unsigned char)*p)) ++count;
        }
        if (fprintf(out, "%d\n", count) < 0) return ST_ERR_WRITE;
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

/* -s: посчитать символы, отличные от букв, цифр и пробела */
static status_t transform_s(FILE *in, FILE *out)
{
    char line[LINE_BUF];
    while (fgets(line, sizeof(line), in)) {
        int count = 0;
        for (const char *p = line; *p; ++p) {
            unsigned char c = (unsigned char)*p;
            if (c == '\n') continue;
            if (!isalpha(c) && !isdigit(c) && c != ' ') ++count;
        }
        if (fprintf(out, "%d\n", count) < 0) return ST_ERR_WRITE;
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

/* -a: заменить символы, отличные от цифр, их ASCII-кодом в 16-ричной */
static status_t transform_a(FILE *in, FILE *out)
{
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (isdigit((unsigned char)c)) {
            if (fputc(c, out) == EOF) return ST_ERR_WRITE;
        } else {
            if (fprintf(out, "%X", (unsigned char)c) < 0) return ST_ERR_WRITE;
        }
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

/* ---------- Диспетчеризация ---------- */
static status_t run_transform(char flag, FILE *in, FILE *out)
{
    switch (flag) {
        case 'd': return transform_d(in, out);
        case 'i': return transform_i(in, out);
        case 's': return transform_s(in, out);
        case 'a': return transform_a(in, out);
        default:  return ST_ERR_FLAG;
    }
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
        case ST_ERR_READ:      fprintf(stderr, "Ошибка: чтение входного файла.\n"); break;
        case ST_ERR_WRITE:     fprintf(stderr, "Ошибка: запись в выходной файл.\n"); break;
        case ST_ERR_NOMEM:     fprintf(stderr, "Ошибка: недостаточно памяти.\n"); break;
        case ST_ERR_NAME_LONG: fprintf(stderr, "Ошибка: слишком длинное имя файла.\n"); break;
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
            "  %s -<d|i|s|a> <входной>\n"
            "  %s -n<d|i|s|a> <входной> <выходной>\n",
            argv[0], argv[0]);
        return (int)ST_ERR_ARGC;
    }

    char flag = '\0';
    int has_n = 0;
    status_t st = parse_flag(argv[1], &flag, &has_n);
    if (st != ST_OK) { print_status(st, argv[1]); return (int)st; }

    const char *input;
    char output[1024];

    if (has_n) {
        if (argc != 4) {
            fprintf(stderr, "Ошибка: флаг '-n%c' требует <входной> <выходной>.\n", flag);
            return (int)ST_ERR_ARGC;
        }
        input = argv[2];
        strncpy(output, argv[3], sizeof(output) - 1);
        output[sizeof(output) - 1] = '\0';
    } else {
        if (argc != 3) {
            fprintf(stderr, "Ошибка: флаг '-%c' требует ровно <входной>.\n", flag);
            return (int)ST_ERR_ARGC;
        }
        input = argv[2];
        st = make_output_name(input, output, sizeof(output));
        if (st != ST_OK) { print_status(st, NULL); return (int)st; }
    }

    FILE *fin = fopen(input, "r");
    if (!fin) { print_status(ST_ERR_OPEN_IN, input); return (int)ST_ERR_OPEN_IN; }

    FILE *fout = fopen(output, "w");
    if (!fout) {
        fclose(fin);
        print_status(ST_ERR_OPEN_OUT, output);
        return (int)ST_ERR_OPEN_OUT;
    }

    st = run_transform(flag, fin, fout);

    if (fclose(fin) != 0 && st == ST_OK)  st = ST_ERR_READ;
    if (fclose(fout) != 0 && st == ST_OK) st = ST_ERR_WRITE;

    if (st != ST_OK) print_status(st, NULL);
    else printf("Готово: '%s' -> '%s'\n", input, output);
    return (int)st;
}
