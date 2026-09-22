# Точка-точка: Hello, кольцо, deadlock

Практика к семинару 2. Код: `codes/deadlock.c`, `deadlock_fix.c`, `deadlock_fix_fast.c`.

## Парадигмы

|      | Single Data | Multiple Data |
| ---- | ----------- | ------------- |
| **S**ingle **P**rogram | SPSD (обычная последовательная программа) | **SPMD** |
| **M**ultiple **P**rogram | MPSD | MPMD |

- **SPMD** — один бинарник на всех процессах, роли по `rank`. Это MPI в курсе: `if (rank == 0) … else …`.
- **MPMD** — разные программы на разных процессах (`mpirun ./a.out : ./b.out`). В MPI можно, в курсе не нужно.
- **MPSD** — разные программы над одними данными. В MPI почти не встречается.

Один исходник + ветки по rank — всё ещё SPMD, не MPMD.

## Точка-точка

Участвуют **ровно два** процесса одного коммуникатора: один шлёт, другой принимает. Явно.

Общий вид:

```
send(buf, count, datatype, dest,   tag, comm)
recv(buf, count, datatype, source, tag, comm, status)
```

Сообщение ищется по **конверту** `(source, tag, comm)`.

```c
int MPI_Send (void *buf, int count, MPI_Datatype type,
              int dest,   int tag, MPI_Comm comm);
int MPI_Ssend(void *buf, int count, MPI_Datatype type,
              int dest,   int tag, MPI_Comm comm);
int MPI_Recv (void *buf, int count, MPI_Datatype type,
              int source, int tag, MPI_Comm comm, MPI_Status *status);
```

- `count` — число **элементов** типа `type`, не байт (`MPI_CHAR` → байты совпадают)
- `tag` — свой ярлык потока сообщений между той же парой
- статус не нужен → `MPI_STATUS_IGNORE`

Из `lec1`: после завершения send буфер **безопасен** (можно сразу писать / переиспользовать). Recv завершается, когда данные лежат в `buf`.

## Hello: 0 шлёт, 1 принимает

```c
char buf[100];
if (rank == 0) {
    sprintf(buf, "Hello from %d", rank);
    MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
} else if (rank == 1) {
    MPI_Recv(buf, 100, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("#%d: %s\n", rank, buf);
}
```

`strlen + 1` — ещё байт на `'\0'`, иначе на приёме строка без конца.

Один и тот же `buf` на send и recv можно: send уже закончился, буфер свободен. Recv его перезапишет.

## Кольцо

Каждый шлёт **вправо**, принимает **слева**:

```c
int right = (rank + 1) % size;
int left  = (rank - 1 + size) % size;
```

`+ size` перед `%`, чтобы у rank 0 не получить отрицательный индекс.

Все процессы — один и тот же код (SPMD). Дальше важен **порядок** Send/Recv.

## Deadlock (`codes/deadlock.c`)

Все делают одно и то же:

```c
MPI_Ssend(buf, …, right, …);   // ждёт, пока сосед начнёт Recv
MPI_Recv (buf, …, left,  …);
```

`MPI_Ssend` (synchronous) завершается **только когда Recv на той стороне уже начался**. Все стоят на Ssend → никто не дошёл до Recv → цикл ожидания → **deadlock**.

Поменять порядок **у всех** (`Recv`, потом `Ssend`) не спасает: все ждут сообщение, которое никто не шлёт. Симметрия осталась.

`MPI_Send` на малых сообщениях может «случайно» пройти (см. ниже). Писать надо так, будто Send = Ssend.

## Фикс: сломать симметрию (`deadlock_fix.c`)

Один процесс **не такой, как все** — сначала принимает:

```c
if (rank == 0) {
    MPI_Recv(buf, …, left,  …);
    MPI_Send(buf, …, right, …);
} else {
    MPI_Send(buf, …, right, …);
    MPI_Recv(buf, …, left,  …);
}
```

rank 0 слушает. Кто шлёт в 0 (это `size-1`) может закончить Send → доходит до своего Recv → разблокирует `size-2` → цепочка до 1.

Время ~ **O(N)**: развязка идёт по кругу от одного особого процесса.

## Быстрее: каждый второй (`deadlock_fix_fast.c`)

Особые — все чётные: они Recv→Send, нечётные Send→Recv.

```c
if (rank % 2 == 0) {   /* Recv, затем Send */
} else {               /* Send, затем Recv */
}
```

Пары работают **параллельно**, не цепочкой. Два шага вместо N.

В файле сейчас `rank / 2 == 0` — это только ranks 0 и 1, не «каждый второй». Нужно `rank % 2 == 0`.

## Под капотом: eager / rendezvous

Реализация `MPI_Send` сама выбирает протокол по **размеру** сообщения:

| протокол | когда | поведение Send |
| -------- | ----- | -------------- |
| **eager** | маленькие | копия в системный буфер, Send может вернуться **до** Recv |
| **rendezvous** | большие | handshake, ждёт готовности приёма ≈ `Ssend` |

Один и тот же кольцевой `Send→Recv` на 8 байтах живёт, на 8 МБ виснет. Потому в `deadlock.c` стоит именно `Ssend` — зависание гарантировано.

Писать приложения так, будто обычный Send синхронный (как сказано на семинаре).

## Прогрев

Первые обмены дороже «боевых»: MPI поднимает соединения, выделяет внутренние буферы, холодные кэш/TLB.

Для замеров (ДЗ: ping-pong 0 ↔ 1, латентность / пропускная способность): несколько холостых кругов **до** таймера, потом много повторов, на график — среднее или медиана. Размеры — степени двойки, ось X логарифмическая.
