# Лабораторная работа №2 — Фундаментальные алгоритмы

Реализация 10 заданий на языке **C (стандарт C99)**.

## Задания

| № | Тема | Файл |
|---|------|------|
| 1 | Операции над строками (без `string.h`) | [`task01/task1.c`](task01/task1.c) |
| 2 | Сортировка сотрудников (`qsort`) | [`task02/task2.c`](task02/task2.c) |
| 3 | Поиск подстроки в нескольких файлах | [`task03/task3.c`](task03/task3.c) |
| 4 | Резервное копирование (задача о рюкзаке 0/1) | [`task04/task4.c`](task04/task4.c) |
| 5 | `overfprintf` / `oversprintf` | [`task05/task5.c`](task05/task5.c) |
| 6 | `overfscanf` / `overscanf` | [`task06/task6.c`](task06/task6.c) |
| 7 | Поиск векторов с максимальной нормой | [`task07/task7.c`](task07/task7.c) |
| 8 | Сложение больших чисел в произвольной системе | [`task08/task8.c`](task08/task8.c) |
| 9 | Дроби в системе счисления с заданным основанием | [`task09/task9.c`](task09/task9.c) |
| 10 | Переразложение многочлена | [`task10/task10.c`](task10/task10.c) |

## Компиляция

Общий вид:

	gcc -std=c99 -Wall -Wextra -Wpedantic taskNN/taskN.c -o taskN

Для заданий, использующих математические функции (5, 6, 7, 9, 10),
добавьте -lm в конце команды:

	gcc -std=c99 -Wall -Wextra -Wpedantic -o task5 task05/task5.c -lm
	gcc -std=c99 -Wall -Wextra -Wpedantic -o task6 task06/task6.c
	gcc -std=c99 -Wall -Wextra -Wpedantic -o task7 task07/task7.c -lm
	gcc -std=c99 -Wall -Wextra -Wpedantic -o task9 task09/task9.c -lm
	gcc -std=c99 -Wall -Wextra -Wpedantic -o task10 task10/task10.c -lm

Соглашения:

Язык: C99 (-std=c99 -Wall -Wextra -Wpedantic)

Ошибки: коды возврата через enum status_t, без exit() вне main

Память: проверка всех malloc, освобождение во всех ветках

Файлы: fclose для всех потоков, в том числе при ошибках

Разделение: логика ≠ ввод/вывод

Вещественные числа: сравнение с эпсилон, передаваемым как параметр

const: везде, где модификация не предполагается

Без глобальных переменных

Без errno (используется strtod/strtol без errno)

Без goto

Комментарии и сообщения: на русском

Идентификаторы (переменные, функции): на английском
