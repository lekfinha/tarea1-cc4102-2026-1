# Tarea 1: R-trees y Bulk-Loading

Este proyecto implementa la construcción masiva (bulk-loading) de R-trees utilizando dos algoritmos: **Nearest-X** y **Sort-Tile-Recursive (STR)**, diseñados para ser guardados y consultados en memoria externa (disco).

## Estructura del proyecto
- `node.c`: Contiene las estructuras de datos alineadas a bloques de 4096 bytes, la lógica geométrica, los algoritmos de construcción (Nearest-X y STR) y las funciones de manipulación/consulta en disco.
- `main.c`: Archivo principal que orquesta los experimentos solicitados (medición de tiempos de construcción y métricas de consultas).
- `Datos/`: Directorio donde deben estar ubicados los datasets binarios requeridos (`random.bin`, `europa.bin`, `europa_bonus.bin`).

## Requisitos previos
- Compilador de C (GCC recomendado).
- Datasets en formato binario ubicados en la carpeta `Datos/` relativos al ejecutable.

## Compilación
Para compilar el programa, abre una terminal en el directorio del proyecto y ejecuta el siguiente comando. Se recomienda usar la bandera `-O3` y es obligatorio incluir `-lm` para activar la librería matemática:

```bash
gcc main.c -o main -lm -O3