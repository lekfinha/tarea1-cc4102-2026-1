#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "node.c" 

// Carga hasta N puntos desde un archivo binario
/**
 * Extrae N coordenadas secuenciales desde el dataset binario no normalizado a un arreglo temporal.
 * Entrada: filename (ruta dataset .bin con floats seriales), n_points (límite de elementos).
 * Salida: Puntero al arreglo key_value generado dinámicamente con los puntos como hojas (MBR colapsado, index -1).
 */
struct key_value* load_points(const char *filename, int n_points) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    struct key_value *puntos = malloc(n_points * sizeof(struct key_value));
    float coords[2];
    int count = 0;
    while (count < n_points && fread(coords, sizeof(float), 2, f) == 2) {
        puntos[count].mbr = rect_point(coords[0], coords[1]);
        puntos[count].child_index = -1;
        count++;
    }
    fclose(f);
    return puntos;
}

/**
 * Ejecuta el pipeline de prueba de construcción de la parte 5.1.
 * Carga los puntos, emite un clock inicial y genera un árbol RAM aplicando STR o Nearest-X.
 * Entrada: dataset_name (identificador visual), file_path (origen), n (cantidad de puntos), use_str (booleano 1=STR, 0=NX).
 * Salida: Ninguna, imprime los tiempos por stdout.
 */
void test_construction(const char *dataset_name, const char *file_path, int n, int use_str) {
    struct key_value *puntos = load_points(file_path, n);
    if (!puntos) {
        printf("Error cargando %s\n", file_path);
        return;
    }

    struct rtree_ram tree;
    clock_t start = clock();
    
    if (use_str) rtree_build_str(&tree, puntos, n);
    else         rtree_build_nearest_x(&tree, puntos, n);
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Construcción %s | %s | N=%d | Tiempo: %f seg\n", 
           dataset_name, use_str ? "STR" : "NX", n, time_spent);

    // Solo para N=2^24 guardamos el árbol para la etapa de consultas
    if (n == (1 << 24)) {
        char db_name[64];
        sprintf(db_name, "tree_%s_%s.bin", dataset_name, use_str ? "STR" : "NX");
        rtree_save_to_disk(&tree, db_name);
    }

    free_rtree_ram(&tree);
    free(puntos);
}

/**
 * Ejecuta el pipeline de consultas 5.2 de la tarea sobre un árbol guardado binariamente.
 * Genera coordenadas sintéticas base "s" y realiza 100 pruebas para establecer media/desviación de puntos e IO's.
 * Entrada: tree_file (ruta base de datos procesada previamente), s (flotante longitud de lados de los box de query).
 * Salida: Ninguna, los resultados consolidados de sus variaciones se imprimen por stdout.
 */
void test_queries(const char *tree_file, float s) {
    FILE *f = fopen(tree_file, "rb");
    if (!f) return;

    int total_io = 0;
    int total_points = 0;
    int points_per_query[100];

    for (int i = 0; i < 100; i++) {
        // Cuadrado aleatorio en [0, 1-s] x [0, 1-s]
        float cx = (float)rand() / (float)RAND_MAX;
        float cy = (float)rand() / (float)RAND_MAX;
        if (cx > 1.0f - s) cx = 1.0f - s;
        if (cy > 1.0f - s) cy = 1.0f - s;

        struct rect_disk query = {cx, cx + s, cy, cy + s};
        int io_count = 0;
        int points_found = 0;

        rtree_query_disk_recursive(f, 0, &query, &io_count, &points_found);
        
        total_io += io_count;
        total_points += points_found;
        points_per_query[i] = points_found;
    }
    fclose(f);

    float avg_io = total_io / 100.0f;
    float avg_points = total_points / 100.0f;
    
    // Calcular desviación estándar
    float var_sum = 0;
    for (int i = 0; i < 100; i++) {
        float diff = points_per_query[i] - avg_points;
        var_sum += diff * diff;
    }
    float std_dev = sqrtf(var_sum / 100.0f);

    printf("Consulta s=%.4f | Árbol: %s | I/O Promedio: %.2f | Puntos: %.2f (+- %.2f)\n", 
           s, tree_file, avg_io, avg_points, std_dev);
}

int main() {
    srand(42); // Semilla fija para reproducibilidad

    const char *d_rand = "Datos/random.bin";
    const char *d_euro = "Datos/europa.bin";

    // --- 5.1 Construcción ---
    printf("\n--- 5.1 TIEMPOS DE CONSTRUCCION ---\n");
    // Aviso: El bucle llega hasta 24. Si estás probando, puedes bajarlo a 18 temporalmente.
    for (int exp = 15; exp <= 24; exp++) {
        int N = 1 << exp; 
        test_construction("Rand", d_rand, N, 0); // Nearest-X
        test_construction("Rand", d_rand, N, 1); // STR
        test_construction("Euro", d_euro, N, 0); // Nearest-X
        test_construction("Euro", d_euro, N, 1); // STR
        printf("-----------------------------------\n");
    }

    // --- 5.2 Consultas ---
    printf("\n--- 5.2 CONSULTAS EN DISCO (Usando arboles de N=2^24) ---\n");
    float s_values[] = {0.0025f, 0.005f, 0.01f, 0.025f, 0.05f};
    
    const char *trees[] = {
        "tree_Rand_NX.bin", "tree_Rand_STR.bin", 
        "tree_Euro_NX.bin", "tree_Euro_STR.bin"
    };

    for (int t = 0; t < 4; t++) {
        printf("\nResultados para %s:\n", trees[t]);
        for (int i = 0; i < 5; i++) {
            test_queries(trees[t], s_values[i]);
        }
    }

    return 0;
}