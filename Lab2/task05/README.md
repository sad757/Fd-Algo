# Задание 5 — `overfprintf` / `oversprintf`

## Условие

Реализовать функции `overfprintf` и `oversprintf`, поведение которых
совпадает с поведением стандартных `fprintf` и `sprintf`, но с
дополнительными флагами.

## Дополнительные флаги

| Флаг | Аргументы | Описание |
|------|-----------|----------|
| `%Ro` | `int` | Римская запись (1..3999) |
| `%Zr` | `unsigned int` | Представление Зеккендорфа (Fibonacci + `1`) |
| `%Cv` | `int, int(base)` | Произвольная система, строчные |
| `%CV` | `int, int(base)` | Произвольная система, прописные |
| `%to` | `char*, int(base)` | Из base в base 10 |
| `%TO` | `char*, int(base)` | То же (буквы — верхний регистр) |
| `%mi` | `int` | Дамп памяти 4 байта (бинарно) |
| `%mu` | `unsigned int` | Дамп памяти 4 байта |
| `%md` | `double` | Дамп памяти 8 байт |
| `%mf` | `float` (через `double`) | Дамп памяти 4 байта |

## Компиляция

	gcc -std=c99 -Wall -Wextra -Wpedantic -o task5 task5.c

Использование:

	./task5

Пример:

overfprintf(stdout, "%Ro\n", 1994);     → MCMXCIV
overfprintf(stdout, "%Zr\n", 10);       → 010011
overfprintf(stdout, "%Cv\n", 255, 16);  → ff
overfprintf(stdout, "%CV\n", 255, 16);  → FF
overfprintf(stdout, "%to\n", "FF", 16); → 255
overfprintf(stdout, "%mi\n", 42);       → 00000000 00000000 00000000 00101010

Ограничения:

Никакого errno, никакого goto, никакого exit() вне main.

const для параметров.

Управление памятью (никакого malloc в overfprintf).

%mf : float продвигается до double в va_arg, потом кастуется.
