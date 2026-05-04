# Tarea 1: R-trees y Bulk-Loading

Este proyecto implementa la construcción masiva (bulk-loading) de R-trees utilizando dos algoritmos: **Nearest-X** y **Sort-Tile-Recursive (STR)**, diseñados para ser guardados y consultados en memoria externa (disco).

## Estructura del Proyecto
- `node.c`: Contiene las estructuras de datos alineadas a bloques de 4096 bytes, la implementación de R-tree, los algoritmos de construcción (Nearest-X y STR) y las funciones de manipulación y consulta en disco simulado/real.
- `main.c`: Archivo principal que orquesta los experimentos solicitados (medición de tiempos de construcción y métricas de consultas en disco).
- `Datos/`: Directorio (debe ser creado por el usuario) que alojará los archivos de prueba, en particular: europa.bin, random.bin, europa_bonus.bin

## Dependencias y Requisitos Previos
El código está escrito en **C estándar puro** y no requiere la instalación de librerías externas de terceros. Sin embargo, hace uso de la librería matemática estándar de C (`math.h`).

Se requiere:
- Un compilador de C (se recomienda **GCC**).
- Un sistema operativo basado en Linux/Unix (o entorno compatible, como WSL).

## Preparación de los Datos
Antes de ejecutar el programa, debes posicionar los datasets descargados desde github.com/claugaete/tarea1-cc4102-2026-1 

1. Crea una carpeta llamada `Datos` en el mismo directorio donde se encuentran `main.c` y `node.c` (si no existe ya).
2. Asegúrate de que los archivos binarios tengan **exactamente** los siguientes nombres y estén ubicados dentro de la carpeta `Datos/`:
   - `Datos/random.bin` (Dataset Aleatorio)
   - `Datos/europa.bin` (Dataset de Europa)
   - `Datos/europa_bonus.bin` (Dataset de Europa con valores reales)


## Compilación
Abre tu terminal en el directorio raíz del proyecto y ejecuta el siguiente comando. 
*Nota: Es obligatorio incluir el flag `-lm` al final para enlazar dinámicamente la librería matemática de C (`math.h`). El flag `-O3` se utiliza para optimizar la carga masiva en RAM de los millones de datos.*

```bash
gcc main.c -o main -lm -O3
```

## Ejecución
Para ejecutar, en la misma terminal hay que darle permisos de ejecución al main recién compilado usando:

```bash 
chmod +x main
```

luego, para ejecutarlo es cosa de escribir en la terminal:

```bash
./main
```

## Resultados Esperados
El programa requiere procesar arreglos en memoria RAM de hasta $N = 2^{24}$ (16.7 millones de puntos). Esto tomará algunos segundos de procesamiento bruto. 

La salida del programa imprimirá en la consola:
1. **(Sec. 5.1):** Una lista continua con los tiempos (en segundos) que toma construir un árbol al variar $N$ usando STR y Nearest-X.
2. **Exportación:** Guardará un archivo `.bin` en tu directorio local por cada R-tree de $N=2^{24}$ construido (`tree_Rand_NX.bin`, `tree_Rand_STR.bin`, etc.).
3. **(Sec. 5.2):** Imprimirá automáticamente los promedios de operaciones de lectura por bloque (I/Os) y los puntos encontrados (con su respectiva desviación estándar) para 100 rectángulos de consulta generados aleatoriamente por cada tamaño ($s$).
4. **(Sec. 5.3 Bonus):** Utilizando el dataset `europa_bonus.bin`, el sistema generará un R-tree real y ejecutará una búsqueda de delimitación (bounding box) sobre las coordenadas de Barcelona, España. Exportará el resultado al archivo `bonus_barcelona.csv` en el directorio de ejecución, el cual está listo para ser visualizado en un scatterplot.