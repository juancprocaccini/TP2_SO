# TP2 - Sistemas Operativos

## Compilación y ejecución

Para compilar (memory manager propio):
```bash
docker run --rm -v $(pwd):/root agodio/itba-so-multiarch:3.1 make
```

Para compilar con buddy system:
```bash
docker run --rm -v $(pwd):/root agodio/itba-so-multiarch:3.1 make buddy
```

Para limpiar artefactos:
```bash
docker run --rm -v $(pwd):/root agodio/itba-so-multiarch:3.1 make clean
```

> **Nota:** al alternar entre `make` y `make buddy` es necesario ejecutar `make clean` primero.

Para correr en QEMU (requiere acceso gráfico):
```bash
qemu-system-x86_64 -enable-kvm -m 512 -hda Image/x64BareBonesImage.img -serial stdio -vga std
```

---

## Instrucciones de replicación

### Comandos disponibles

### Tests provistos por la cátedra

### Pipes y background

### Atajos de teclado

### Ejemplos

---

## Limitaciones

---

## Citas y uso de IA
