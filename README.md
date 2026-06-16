# TP2 - Sistemas Operativos

Núcleo de un sistema operativo monolítico de 64 bits: administración de memoria
física (memory manager propio + buddy system), procesos con context switching y
scheduling Round Robin con prioridades, semáforos, pipes y un intérprete de
comandos (`sh`) con sus aplicaciones de usuario.

## Compilación y ejecución

Toda la compilación se hace dentro de la imagen provista por la cátedra
(`agodio/itba-so-multiarch:3.1`).

Para compilar (memory manager propio):
```bash
docker run --rm --user $(id -u):$(id -g) -v "$(pwd):/root" -w /root agodio/itba-so-multiarch:3.1 make
```

Para compilar con buddy system:
```bash
docker run --rm --user $(id -u):$(id -g) -v "$(pwd):/root" -w /root agodio/itba-so-multiarch:3.1 make buddy
```

Para limpiar artefactos:
```bash
docker run --rm --user $(id -u):$(id -g) -v "$(pwd):/root" -w /root agodio/itba-so-multiarch:3.1 make clean
```
> **Nota:** al alternar entre `make` y `make buddy` es necesario ejecutar `make clean`
> primero. `make` solo compara timestamps y no detecta que cambió `EXTRA_FLAGS`,
> por lo que sin un `clean` previo quedarían objetos mezclados de los dos managers.

Para correr en QEMU (requiere acceso gráfico):
```bash
qemu-system-x86_64 -enable-kvm -m 512 -drive format=raw,file=Image/x64BareBonesImage.img -serial stdio -vga std
```
O bien, con la regla del Makefile (fuera de la imagen de Docker):
```bash
make run
```

> Las reglas `make`, `make all` y `make buddy` son exclusivamente de compilación.
> El arranque de QEMU es una regla aparte (`make run`).

---

## Instrucciones de replicación

### Comandos disponibles

Los siguientes comandos se escriben en el prompt `>` de la shell. Salvo `help`,
`clear` y `killall` (que son *built-ins* y corren dentro del proceso shell), todos
se ejecutan como procesos de usuario independientes.

| Comando | Parámetros | Descripción |
|---------|-----------|-------------|
| `help` | — | Lista todos los comandos disponibles y los tests de la cátedra. Built-in. |
| `clear` | — | Limpia la pantalla. Built-in. |
| `mem` | — | Imprime el estado de los dos heaps (kernel y usuario): total, ocupado y libre en bytes. |
| `ps` | — | Tabla de procesos: PID, nombre, prioridad, estado, foreground, fds (in/out/err), RSP y base del stack. |
| `loop` | `<ticks>` | Espera activa de `ticks` iteraciones y luego imprime `hello from pid <id>`. La espera es **busy wait** (consigna). |
| `kill` | `<pid>` | Termina el proceso con el PID dado. |
| `nice` | `<pid> <low\|medium\|high>` | Cambia la prioridad del proceso indicado. |
| `block` | `<pid>` | Alterna el estado del proceso entre bloqueado y listo. |
| `cat` | — | Copia su stdin a stdout tal cual, hasta recibir EOF. |
| `wc` | — | Cuenta las líneas de su stdin y las imprime al recibir EOF. |
| `filter` | — | Filtra (descarta) las vocales de su stdin y escribe el resto en stdout. |
| `mvar` | `<W> <R>` | Problema de múltiples lectores/escritores sobre una variable compartida (tipo MVar). Crea `W` escritores y `R` lectores sincronizados con semáforos; el proceso principal termina de inmediato. Cada lector imprime con un color único. |
| `killall` | `<prefijo>` | Mata todos los procesos cuyo nombre empieza con `<prefijo>` (p. ej. `killall mvar` frena todos los workers de `mvar`). Built-in. Utilidad propia, no exigida por la consigna. |

### Tests provistos por la cátedra

Corren como **procesos de usuario** (no son built-ins) y aceptan `&` para correr
en background. Se interrumpen con `Ctrl+C`.

| Test | Parámetros | Descripción |
|------|-----------|-------------|
| `test_mm` | `<max_memory>` | Ciclo infinito que reserva bloques de tamaño aleatorio hasta `max_memory` bytes, los llena con un patrón y verifica que no se solapen, luego los libera. Solo imprime si hay error. |
| `test_processes` | `<max_procesos>` | Crea, bloquea, desbloquea y mata procesos dummy aleatoriamente (máximo 25). Solo imprime ante errores o eventos. |
| `test_prio` | `<max_value>` | Tres procesos cuentan hasta `max_value`: primero con la misma prioridad, luego con prioridades distintas (LOW/MEDIUM/HIGH) para visualizar la diferencia de tiempos. |
| `test_sync` | `<iteraciones> <use_sem>` | Crea 2 pares de procesos que incrementan/decrementan una variable global `iteraciones` veces. Con `use_sem=1` el resultado final es 0; con `use_sem=0` varía por las race conditions. |

Ejemplos:
```
test_mm 1048576
test_processes 10
test_prio 1000000
test_sync 1000 1
test_sync 1000 0
test_mm 1048576 &
```

### Pipes y comandos en background

- **Background (`&`):** agregando `&` al final de un comando, la shell no espera a
  que termine y devuelve el prompt de inmediato, imprimiendo el/los PID lanzados.
  El proceso en background recibe `stdin = -1` (no compite por el teclado). Los
  built-ins (`help`, `clear`, `killall`) no admiten `&`.
  ```
  loop 50000000 &
  test_mm 1048576 &
  ```

- **Pipe (`|`):** conecta el stdout del comando izquierdo con el stdin del derecho.
  Se soporta **exactamente un** pipe (`p1 | p2`); `p1 | p2 | p3` no está soportado.
  Los built-ins no pueden formar parte de un pipe.
  ```
  cat | wc
  cat | filter
  ```

### Atajos de teclado

| Atajo | Acción |
|-------|--------|
| `Ctrl+C` | Mata el proceso en foreground (y, si éste lee de un pipe, también a su escritor). |
| `Ctrl+D` | Envía EOF al stdin del proceso en foreground. |
| `Backspace` | Borra el último carácter de la línea en edición (sin pasar el prompt). |
| `Enter` | Envía la línea completa (modo canónico). |

### Ejemplos por requerimiento (fuera de los tests)

**Administración de memoria física**
```
mem                     # estado de ambos heaps
loop 10000000           # reserva implícita de stack; ver cambios en 'mem' con procesos vivos
```

**Procesos, context switching y scheduling**
```
loop 80000000 &         # lanza un proceso de fondo, imprime su PID
ps                      # se lo ve READY/RUNNING en la tabla
nice <pid> high         # sube su prioridad (más quantums por turno)
block <pid>             # lo bloquea; 'ps' lo muestra BLOCKED
block <pid>             # lo vuelve a poner READY
kill <pid>              # lo termina
```

**Sincronización (semáforos)**
```
mvar 2 2                # salida alternada: ABABABAB...
mvar 3 2                # ABCABCABC...
mvar 2 2 &              # en background; luego:
ps                      # ver los PIDs de mvar_w0 / mvar_w1 / mvar_r0...
kill <pid de mvar_w1>   # al matar al escritor B la salida muta (ABAB -> ...AAAA)
mvar 2 1 &
nice <pid de mvar_w1> high   # al subirle la prioridad al escritor B: ABABABBBABBB...
killall mvar            # frena todos los workers de mvar
```

**IPC (pipes)**
```
cat | wc                # escribí líneas, Ctrl+D para EOF: imprime la cuenta
cat | filter            # escribí texto, Ctrl+D: devuelve el texto sin vocales
```

---

## Limitaciones



---

## Citas y uso de IA

- El bootloader (Pure64), BMFS y la base del kernel provienen del TP de Arquitectura
  de Computadoras (base: `nicorfalcon/TPE_ARQUI`), usado como punto de partida según
  lo permite la consigna.
- Se utilizó IA (Claude) como asistente durante el desarrollo: revisión de código,
  discusión de diseño (forjado de stack, alineación ABI, orden de locks), y redacción
  de documentación. Todas las decisiones de diseño e implementación fueron revisadas y 
  validadas por el grupo. No se incorporó código generado sin comprensión y prueba previa.
