# Dynamic parser for Context-Free Grammars

## Компиляция
### С помощью Visual Studio
1. Открыть файл parser.sln в Visual Studio 2019
2. Выбрать режим компиляции Release или StaticRelease, платформу x64 (Win64)
3. Запустить компиляцию (Меню -> Build -> Build solution)

### С помощью GCC
Под Windows необходимо убедиться, что установлена 64-битная версия GCC версии > 8.1.0 (на данный момент доступна только в составе MSYS, GCC версии 9.1.0).
В текущей версии компиляция запускается командой:
   ``` bash
   >  g++ *.cpp -O3 -m64 -mbmi -mavx2 -mpopcnt -o parser.exe -std=c++17
   ```
#### Замечания
- Из C++ 17 используется только модуль filesystem в main.cpp, где парсер запускается на всех  файлах из заданной директории.

- Процессор должен поддерживать расширенный набор инструкций BMI2. (В будущем надо будет сделать также версию с эмуляцией этих инструкций для остальных процессоров).

## Tool usage

    parser.exe [-d dir] file

    arguments:
    file           input file
    dir            директория, из которой читаются все файлы и тестируются в цикле

В текущий момент парсер запускается в режиме измерения скорости работы, поскольку в ближайшее время планируется заниматься оптимизацией текущих алгоритмов и структур данных.

Можно подать несколько файлов, тогда в парсер будет подаваться конкатенация этих файлов. Режим с несколькими файлами удобно использовать, если в грамматика описана в отдельном, и её можно таким образом вставлять перед тем текстом, который нужно разобрать.

### Примеры
1. Запуск разбора одного файла:
   ```
   parser syntax/json.txt test/short.json
   ```

2. При запуске на всех файлах из директории результаты запуска пишутся на экран и в файл **log.txt**, ошибки более подробно записываются в файл **failed.txt**

   Например, команда запуска парсера JavaScript на тестовых файлах из директории **js_tests** выглядит так:
   ```
   parser -d js_tests syntax/js.txt
   ```

## Поддерживаемые грамматики

Разбор осуществляется в 2 этапа:
1. Разбиение на токены при помощи лексера
2. Разбор последовательности токенов восходящим алгоритмом, представляющим собой модифицированную версию LR(1) для динамических грамматик.

Правила, прописанные в коде, могут иметь семантические действия, в том числе расширение грамматики.

### Расширение грамматики
В текущий момент есть несколько правил расширения грамматики. На настоящий момент цель этих правил -- задать минимальный базис, позволяющий задать достаточно широкий класс грамматик посредством расширения базовой грамматики, состоящей лишь из правил её расширения.

Правила сводятся к следующим:
1. Новое правило в грамматике:
   ```
   %syntax: <нетерминал> -> <правая часть> ;
   ```
   Здесь правая часть представляет собой последовательность токенов и нетерминалов. Токены могут задаваться именами, а также константные токены могут задаваться строками в одинарных кавычках. Например, данная само правило для добавления правил имеет вид:
   ```
   new_syntax -> '%' 'syntax' ':' ident '->' rule_rhs ';'
   ```
2. Новый токен:
   ```
   %token: <имя токена> = `<Выражение разбора>` ;
   ```
3. Новое вспомогательное выражение разбора:
   ```
   %pexpr: <идентификатор> = `<Выражение разбора>` ;
   ```
В данной версии парсера было решено использовать выражения разбора вместо более привычных для лексера регулярных выражений. Это позволяет лексеру определять вложенные комментарии, определить токен для самих выражений разбора или регулярных выражений.

Лексер реализован на основе идеи packrat парсера, только packrat парсер читает не весь текст, а один токен.

### Особенности парсера
1. Парсер должен правильно разбирать LR(1) грамматики. Если в грамматике имеются конфликты, то не всегда выдаются ошибки, иногда конфликты разрешаются в соответствии с приоритетами, например:
   - сдвиг имеет всегда приоритет перед свёрткой
   - свёртка производится до ближайшей в верху стека позиции, где можно сделать сдвиг

   Если правила не позволяют разрешить конфликт, то выдаётся ошибка
2. В отличие от обычных алгоритмов семейства LR здесь не строится таблица переходов, а хранятся лишь правила грамматики в специальной структуре данных, что позволяет легко добавлять новые правила, но замедляет разбор. Отчасти это должно компенсироваться специальными структурами данных для хранения правил грамматики, но тесты скорости на сложных грамматиках пока не производились.
3. В текущей реализации лексер отделён от парсера, в частности, могут читаться токены, которые недопутимы в текущем контексте. Это связано с текущим хранением состояний парсера и предполагается исправить.
4. Пока все токены подаются в парсер. В связи с этим не реализованы комментарии, хотя в лексере для них есть заготовка.
5. В связи с использованием в лексере [PEG-грамматик](https://ru.wikipedia.org/wiki/%D0%93%D1%80%D0%B0%D0%BC%D0%BC%D0%B0%D1%82%D0%B8%D0%BA%D0%B0,_%D1%80%D0%B0%D0%B7%D0%B1%D0%B8%D1%80%D0%B0%D1%8E%D1%89%D0%B0%D1%8F_%D0%B2%D1%8B%D1%80%D0%B0%D0%B6%D0%B5%D0%BD%D0%B8%D0%B5), следует очень внимательно писать определения токенов, поскольку при таком определении:
    ```
    %token: number = `int / float`;
    ```
    число 0.5 будет распознано как 2 токена '0' и '.5'.
    Поскольку пользовательские токены предполагается добавлять достаточно редко, то эти сложности не кажутся критичными.
6. Для возможности добавления правил в грамматику имеются следующие встроенные токены:

   - `ident = [_a-zA-Z][_a-zA-Z0-9]*` -- идентификатор
   - `sq_string = '\\'' ('\\\\' [^] / [^\\n'])* '\\''` -- строка в одинарных кавычках
   - `dq_string = (ws '\"' ('\\\\' [^] / [^\\n\"])* '\"')+` -- последовательность строк в двойных кавычках
   - ``peg_expr_def = `<громоздкое определение выражения разбора>` ``

   В синтаксисе выражений разбора пока нет символа `.`, поэтому пока используется выражение `[^]`, обозначающее любой символ.
### Пример задания грамматики JSON
```
%pexpr: pos_int = `[0-9]+`;
%pexpr: int = `[+\-]? pos_int`;
%token: number = `[+\-]? ('.' [0-9]+ / pos_int ('.' [0-9]*)?) ([Ee] int)?`;

%syntax: text -> value;

%syntax: value -> object;
%syntax: value -> array;
%syntax: value -> string;
%syntax: value -> number;
%syntax: value -> 'true';
%syntax: value -> 'false';
%syntax: value -> 'null';

%syntax: object -> '{' '}';
%syntax: object -> '{' members '}';

%syntax: members -> member;
%syntax: members -> member ',' members;

%syntax: member -> string ':' value;

%syntax: array -> '[' ']';
%syntax: array -> '[' elements ']';

%syntax: elements -> value;
%syntax: elements -> value ',' elements;
```
