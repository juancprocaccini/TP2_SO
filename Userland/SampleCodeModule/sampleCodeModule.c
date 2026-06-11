#include <stdint.h>
#include "usrlib.h"
#include <stddef.h>
#include "test_util.h"

// Declaramos las firmas de los tests
int test_sync(char *argv[], int argc);
int test_prio(char *argv[], int argc);
int test_processes(char *argv[], int argc);
int test_mm(char *argv[], int argc);

int main(void)
{
    // Limpiamos pantalla y preparamos el entorno
    clearScreen();
    kprintf("================================================\n");
    kprintf("       INICIANDO BATERIA DE TESTS DEL SO        \n");
    kprintf("================================================\n\n");

    int ret;

    // ---------------------------------------------------------
    // 1. TEST DE SINCRONIZACIÓN (Semáforos)
    // ---------------------------------------------------------
    kprintf("[INFO] Iniciando test_sync...\n");
    // Argumentos: "100" iteraciones, "1" (usar semáforos)
    char *argv_sync[] = {"100", "1"};
    ret = test_sync(argv_sync, 2);

    if (ret == 0)
    {
        kprintf("[OK] test_sync finalizo con EXITO.\n\n");
    }
    else
    {
        kprintf("[FAIL] test_sync fallo (Retorno: %d).\n\n", ret);
    }

    // ---------------------------------------------------------
    // 2. TEST DE PRIORIDADES
    // ---------------------------------------------------------
    kprintf("[INFO] Iniciando test_prio...\n");
    // Argumentos: un número grande para que la CPU trabaje y se note la prioridad
    char *argv_prio[] = {"10000000"};
    ret = test_prio(argv_prio, 1);

    if (ret == 0)
    {
        kprintf("[OK] test_prio finalizo con EXITO.\n\n");
    }
    else
    {
        kprintf("[FAIL] test_prio fallo (Retorno: %d).\n\n", ret);
    }

    // ---------------------------------------------------------
    // 3. TEST DE PROCESOS (Creación, kill, block)
    // ---------------------------------------------------------
    kprintf("[INFO] Iniciando test_processes...\n");
    // Argumentos: "10" (cantidad máxima de procesos simultáneos permitidos)
    char *argv_proc[] = {"10"};
    ret = test_processes(argv_proc, 1);

    if (ret == 0)
    {
        kprintf("[OK] test_processes finalizo con EXITO.\n\n");
    }
    else
    {
        kprintf("[FAIL] test_processes fallo (Retorno: %d).\n\n", ret);
    }

    // ---------------------------------------------------------
    // 4. TEST DE MEMORY MANAGER (Malloc y Free)
    // ---------------------------------------------------------
    kprintf("[INFO] Iniciando test_mm...\n");
    // Argumentos: Memoria máxima a pedir (ej. 1MB = "1000000")
    char *argv_mm[] = {"1000000"};
    ret = test_mm(argv_mm, 1);

    if (ret == 0)
    {
        kprintf("[OK] test_mm finalizo con EXITO.\n\n");
    }
    else
    {
        kprintf("[FAIL] test_mm fallo (Retorno: %d).\n\n", ret);
    }

    kprintf("================================================\n");
    kprintf("       BATERIA DE TESTS FINALIZADA              \n");
    kprintf("================================================\n");
    kprintf("Entrando a modo terminal. Presiona teclas:\n\n");

    // ---------------------------------------------------------
    // Ciclo de teclado (Terminal Dummy)
    // ---------------------------------------------------------
    while (1)
    {
        char c = kbdGetChar();
        if (c == 0)
            continue;
        if (c == '\n')
        {
            newLine();
        }
        else if (c == '\b')
        {
            deleteChar();
        }
        else
        {
            drawChar((uint64_t)c, 0xFFFFFF); // Blanco por defecto
        }
    }

    return 0;
}
