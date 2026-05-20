# Lokale Build-Anpassungen

Diese Datei dokumentiert lokale Anpassungen, die gemacht wurden, damit
`make all` in diesem Checkout wieder durchläuft.

## Ausgangslage

Der Build brach zuerst in `test.cpp` ab, weil der Benchmark-Code noch die
alte API verwendet hat:

- `DataStructureWrapper` wurde ohne Argument konstruiert.
- `BTreeCppPerfEventBlock` wurde nur mit `BTreeCppPerfEvent` und `scale`
  konstruiert.

Die aktuellen Header erwarten aber:

- `DataStructureWrapper(bool isInt)`
- `BTreeCppPerfEventBlock(BTreeCppPerfEvent& e, DataStructureWrapper& ds, uint64_t scale = 1)`

Danach kamen Linkerfehler für Wormhole und TLX dazu, weil einige Targets
`wh_adapter.cpp` bzw. den TLX-Adapter mitkompilieren, aber die passenden
Bibliotheken nicht verlinkt haben.

## Was geändert wurde

### `test.cpp`

`DataStructureWrapper` wird jetzt mit einem Integer-Key-Hinweis erzeugt:

```cpp
DataStructureWrapper t(dataName == "int");
```

Alle gemessenen Blöcke übergeben nun auch die Datenstruktur an
`BTreeCppPerfEventBlock`, damit die neue API erfüllt ist und der
Perf-Block Node-Statistiken auslesen kann:

```cpp
BTreeCppPerfEventBlock b(e, t, count);
```

### `Makefile`

Die Targets, die `wh_adapter.cpp` verwenden, linken jetzt gegen die lokal
gebaute Wormhole-Bibliothek:

```make
wh_link_arg = -L. -lwh
```

Die Named-Test- und TPCC-Targets linken außerdem gegen den TLX-Wrapper,
weil Konfigurationen wie `tlx` sonst undefinierte Symbole für
`TlxWrapper` erzeugen.

`make all` baut lokal jetzt:

- `test.elf`
- `optimized.elf`
- alle Named-Test-Binaries
- alle YCSB-Binaries

TPCC wurde aus dem Default-Target herausgenommen, weil es zusätzliche
Systemabhängigkeiten hat. Für eine Umgebung mit TBB gibt es weiterhin:

```bash
make all-with-tpcc
```

Wenn trotzdem ein Target wie `named-build/adapt-d3-tpcc` gebaut wird, wurde
ein TPCC-Target angefordert. Ohne TBB bricht das jetzt absichtlich früh mit
einer klaren Makefile-Meldung ab. Für den lokalen Benchmark-Build ohne TPCC:

```bash
make all
```

Für TPCC muss vorher das TBB-Development-Paket installiert werden, sodass
dieser Header existiert:

```text
tbb/parallel_for.h
```

Der TPCC-Code wurde außerdem an die moderne TBB-API angepasst. Alte TBB
hatte:

```cpp
#include <tbb/task_scheduler_init.h>
tbb::task_scheduler_init init(nthreads);
```

Neue TBB-Versionen stellen diesen Header nicht mehr bereit. Stattdessen wird
jetzt verwendet:

```cpp
#include <tbb/global_control.h>
tbb::global_control tbbThreads(tbb::global_control::max_allowed_parallelism, nthreads);
```

### `tpcc/newbm.cpp`

Der Include von `libaio.h` wurde entfernt. Die Datei wurde zwar inkludiert,
aber keine `libaio`-API wird im TPCC-Code verwendet. Dadurch fällt keine
ungültige Systemabhängigkeit mehr an.

Zusätzlich wurde TPCC auf die neue `DataStructureWrapper`- und
`BTreeCppPerfEventBlock`-API angepasst:

```cpp
tree = new DataStructureWrapper(false);
BTreeCppPerfEventBlock b(e, *warehouse.tree, 0);
```

`false` bedeutet hier: TPCC nutzt gefaltete Byte-Keys, nicht den speziellen
Integer-Key-Pfad. Für den Perf-Block wird eine vorhandene TPCC-Tabelle
übergeben, damit die neue Signatur erfüllt ist und Node-Statistiken
ausgelesen werden können.

## Warum so

Die Änderungen folgen der bereits vorhandenen neueren Nutzung in
`ycsb2.cpp`: Dort wird `DataStructureWrapper` mit `isDataInt(e)` erzeugt
und an `BTreeCppPerfEventBlock` übergeben. `test.cpp` war lediglich noch
auf dem alten Stand.

Die Linker-Anpassungen machen explizit, welche Adapter-Bibliotheken benötigt
werden. Vorher wurden Adapter-Quellen mitkompiliert, aber ihre externen
Symbole nicht aufgelöst.

TPCC bleibt bewusst als separates Ziel erhalten, statt es stillschweigend zu
deaktivieren. Wer TBB installiert hat, kann den vollständigen Build mit
`make all-with-tpcc` ausführen.

## Verifikation

Ausgeführt in:

```bash
/home/student/btree_redis/original_paper_benchmark
```

Kommando:

```bash
make all
```

Ergebnis: Build erfolgreich abgeschlossen.

Nach Installation/Verfügbarkeit von TBB wurde außerdem ausgeführt:

```bash
make all-with-tpcc
```

Ergebnis: Build inklusive TPCC erfolgreich abgeschlossen.

Es bleiben Warnungen zu `random_shuffle`; diese sind Deprecation-Warnungen
und brechen den Build nicht ab.
