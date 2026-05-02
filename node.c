#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define B_CAPACITY 204

// Reemplaza a NUMTYPE double de rtree.c para usar 4 bytes por coordenada
typedef float NUMTYPE_DISK; 

// Representa el MBR (16 bytes en total) según (x1, x2, y1, y2).
// Sirve para delimitar espacialmente un conjunto de puntos o un rectángulo.
struct rect_disk {
    NUMTYPE_DISK x_min;
    NUMTYPE_DISK x_max;
    NUMTYPE_DISK y_min;
    NUMTYPE_DISK y_max;
};

// Reemplaza el uso de struct item y union { nodes, datas } de rtree.c
struct key_value {
    struct rect_disk mbr; // Clave: 16 bytes
    int32_t child_index;  // Valor: 4 bytes (-1 si es hoja, u offset en archivo)
};

// Reemplaza a struct node original de rtree.c
// Estructura de bloque exacto de 4096 bytes que actúa como nodo en el R-Tree.
struct node_disk {
    int32_t k;                                // 4 bytes (cantidad de hijos)
    struct key_value hijos[B_CAPACITY];       // 204 * 20 = 4080 bytes
    char pad[12];                             // 12 bytes de relleno
}; 
// Total: 4 + 4080 + 12 = 4096 bytes exactos

// --- funciones geométricas y utilitarias ---

/**
 * Devuelve el mínimo entre dos valores.
 * Entrada: x, y (floats).
 * Salida: El valor menor entre x e y.
 */
static inline NUMTYPE_DISK min0(NUMTYPE_DISK x, NUMTYPE_DISK y) {
    return x < y ? x : y;
}

/**
 * Devuelve el máximo entre dos valores.
 * Entrada: x, y (floats).
 * Salida: El valor mayor entre x e y.
 */
static inline NUMTYPE_DISK max0(NUMTYPE_DISK x, NUMTYPE_DISK y) {
    return x > y ? x : y;
}

/**
 * Expande un rectángulo 'rect' para que incluya espacialmente al rectángulo 'other'.
 * Entrada: 
 *   - rect: puntero al rectángulo a expandir.
 *   - other: puntero al rectángulo que debe ser contenido.
 * Salida: Ninguna (modifica 'rect' in situ).
 */
static void rect_expand(struct rect_disk *rect, const struct rect_disk *other) {
    rect->x_min = min0(rect->x_min, other->x_min);
    rect->x_max = max0(rect->x_max, other->x_max);
    rect->y_min = min0(rect->y_min, other->y_min);
    rect->y_max = max0(rect->y_max, other->y_max);
}

/**
 * Calcula el área de un MBR.
 * Entrada: puntero a un rectángulo.
 * Salida: Área (float).
 */
static NUMTYPE_DISK rect_area(const struct rect_disk *r) {
    NUMTYPE_DISK dx = r->x_max - r->x_min;
    NUMTYPE_DISK dy = r->y_max - r->y_min;
    if (dx < 0) dx = 0;
    if (dy < 0) dy = 0;
    return dx * dy;
}

/**
 * Verifica si dos rectángulos se cruzan o solapan.
 * Entrada: punteros a dos rectángulos.
 * Salida: true si se intersectan, false en caso contrario.
 */
static bool rect_intersects(const struct rect_disk *rect, const struct rect_disk *other) {
    if (other->x_min > rect->x_max || other->x_max < rect->x_min) return false;
    if (other->y_min > rect->y_max || other->y_max < rect->y_min) return false;
    return true;
}

/**
 * Genera un MBR de dimensiones nulas a partir de una coordenada 2D.
 * Entrada: x, y (coordenadas del punto).
 * Salida: Estructura rect_disk que representa el punto.
 */
static struct rect_disk rect_point(NUMTYPE_DISK x, NUMTYPE_DISK y) {
    struct rect_disk r;
    r.x_min = r.x_max = x;
    r.y_min = r.y_max = y;
    return r;
}

/**
 * Calcula el centro horizontal (eje X) de un MBR.
 * Entrada: puntero a un rectángulo.
 * Salida: Centroide en X (float).
 */
static float rect_center_x(const struct rect_disk *r) {
    return r->x_min + (r->x_max - r->x_min) / 2.0f;
}

/**
 * Función de comparación para qsort basada en el centroide en X.
 * Entrada: punteros a dos elementos key_value.
 * Salida: -1 si a < b, 1 si a > b, 0 si son iguales.
 */
static int compare_x(const void *a, const void *b) {
    const struct key_value *kv_a = (const struct key_value *)a;
    const struct key_value *kv_b = (const struct key_value *)b;
    float cx_a = rect_center_x(&kv_a->mbr);
    float cx_b = rect_center_x(&kv_b->mbr);
    if (cx_a < cx_b) return -1;
    if (cx_a > cx_b) return 1;
    return 0;
}

/**
 * Calcula el centro vertical (eje Y) de un MBR.
 * Entrada: puntero a un rectángulo.
 * Salida: Centroide en Y (float).
 */
static float rect_center_y(const struct rect_disk *r) {
    return r->y_min + (r->y_max - r->y_min) / 2.0f;
}

/**
 * Función de comparación para qsort basada en el centroide en Y.
 * Entrada: punteros a dos elementos key_value.
 * Salida: -1 si a < b, 1 si a > b, 0 si son iguales.
 */
static int compare_y(const void *a, const void *b) {
    const struct key_value *kv_a = (const struct key_value *)a;
    const struct key_value *kv_b = (const struct key_value *)b;
    float cy_a = rect_center_y(&kv_a->mbr);
    float cy_b = rect_center_y(&kv_b->mbr);
    if (cy_a < cy_b) return -1;
    if (cy_a > cy_b) return 1;
    return 0;
}

// Estructura para simular el almacenamiento en disco en RAM
struct rtree_ram {
    struct node_disk *nodes;
    int capacity;
    int count; // nodes[0] será siempre la raíz final
};

// Reserva de memoria para el simulador de disco
/**
 * Inicializa la estructura temporal RAM simulando el disco.
 * Entrada: tree (puntero a la estructura), initial_capacity (capacidad base en nodos).
 * Salida: Ninguna (reserva memoria in situ).
 */
void init_rtree_ram(struct rtree_ram *tree, int initial_capacity) {
    tree->nodes = malloc(initial_capacity * sizeof(struct node_disk));
    tree->capacity = initial_capacity;
    tree->count = 0;
}

/**
 * Libera la memoria utilizada por la representación del árbol en RAM.
 * Entrada: tree (puntero a la estructura temporal).
 * Salida: Ninguna.
 */
void free_rtree_ram(struct rtree_ram *tree) {
    free(tree->nodes);
    tree->nodes = NULL;
    tree->capacity = 0;
    tree->count = 0;
}

/**
 * Añade secuencialmente un nodo al disco simulado en RAM y devuelve su ID posicional.
 * De la posición 0 siempre se reserva para el nodo raíz final.
 * Entrada: tree (puntero a la ram), node (nodo a copiar).
 * Salida: Índice en el que quedó registrado (int32_t).
 */
static int32_t append_node(struct rtree_ram *tree, const struct node_disk *node) {
    if (tree->count >= tree->capacity) {
        tree->capacity = tree->capacity == 0 ? 1024 : tree->capacity * 2;
        tree->nodes = realloc(tree->nodes, tree->capacity * sizeof(struct node_disk));
    }
    // Dejamos la posición 0 reservada para la raíz final de forma perezosa
    if (tree->count == 0) {
        tree->count = 1; // Hacer espacio 0 (raíz eventual)
    }
    int32_t idx = tree->count++;
    tree->nodes[idx] = *node;
    return idx;
}

/**
 * Función recursiva que ejecuta la carga por lotes usando la técnica Nearest-X.
 * Construye dinámicamente de abajo hacia arriba empaquetando en bloques de B_CAPACITY.
 * Entrada: tree (árbol donde se irán guardando), items (array de tuplas MBR-Index), n (cantidad actual).
 * Salida: Ninguna. Termina guardando la raíz en tree->nodes[0].
 */
void build_nearest_x_recursive(struct rtree_ram *tree, struct key_value *items, int n) {
    // 1. Ordenar por centro X
    qsort(items, n, sizeof(struct key_value), compare_x);

    // 2 y 3. Calcular cuantos nodos de padre necesitamos
    int num_nodos = (n + B_CAPACITY - 1) / B_CAPACITY; // Techo de n / B_CAPACITY
    
    // Si cabe en un solo nodo, hemos llegado a la futura raíz
    if (num_nodos == 1) {
        struct node_disk root = {0};
        root.k = n;
        for (int i = 0; i < n; i++) {
            root.hijos[i] = items[i];
        }
        // Guardar la raíz siempre en la posición 0 del disco/RAM
        tree->nodes[0] = root;
        // Si el tree->count era 0 (solo había un nivel), se hace 1
        if (tree->count == 0) tree->count = 1;
        return;
    }

    // Nivel superior necesita guardar los 'num_nodos' MBRs recién generados
    struct key_value *next_level_items = malloc(num_nodos * sizeof(struct key_value));

    // Llenar nodos
    for (int i = 0; i < num_nodos; i++) {
        struct node_disk nd = {0};
        
        int start_idx = i * B_CAPACITY;
        int elements_in_node = min0(B_CAPACITY, n - start_idx);
        nd.k = elements_in_node;

        struct rect_disk node_mbr = items[start_idx].mbr;
        for (int j = 0; j < elements_in_node; j++) {
            nd.hijos[j] = items[start_idx + j];
            rect_expand(&node_mbr, &nd.hijos[j].mbr);
        }

        // 4. Guardar nodo en memoria y generar el entry para el padre
        int32_t idx_in_ram = append_node(tree, &nd);
        
        next_level_items[i].mbr = node_mbr;
        next_level_items[i].child_index = idx_in_ram;
    }

    // 5. Recursión hacia arriba con el nivel que acabamos de generar
    build_nearest_x_recursive(tree, next_level_items, num_nodos);

    free(next_level_items);
}

/**
 * Función de entrada para construir un R-tree mediante Nearest-X desde un array de puntos hoja.
 * Entrada: tree (árbol vacío), puntos (datos iniciales), n (nro total de puntos).
 * Salida: Ninguna (estructura del arbol modificada in situ).
 */
void rtree_build_nearest_x(struct rtree_ram *tree, struct key_value *puntos, int n) {
    if (n == 0) return;
    init_rtree_ram(tree, (n / B_CAPACITY) + 2); 
    build_nearest_x_recursive(tree, puntos, n);
}

/**
 * Función recursiva que ejecuta la carga por lotes usando Sort-Tile-Recursive (STR).
 * Intercala ordenamientos por X y particiones por Y para un empaquetado ultra compacto.
 * Entrada: tree (árbol donde se irá subiendo), items (tuplas MBR-Index), n (cantidad).
 * Salida: Ninguna. Guarda el padre definitivo en tree->nodes[0].
 */
void build_str_recursive(struct rtree_ram *tree, struct key_value *items, int n) {
    int P = (n + B_CAPACITY - 1) / B_CAPACITY; // Cantidad de nodos necesarios
    if (P <= 1) {
        // Mismo caso base que Nearest-X
        struct node_disk root = {0};
        root.k = n;
        for (int i = 0; i < n; i++) root.hijos[i] = items[i];
        tree->nodes[0] = root;
        if (tree->count == 0) tree->count = 1;
        return;
    }

    int S = (int)ceil(sqrt(P)); // Cantidad de cortes verticales

    // 1. Ordenar todo por X
    qsort(items, n, sizeof(struct key_value), compare_x);

    struct key_value *next_level_items = malloc(P * sizeof(struct key_value));
    int node_idx = 0;

    // 2. Procesar por franjas (slices)
    int slice_capacity = S * B_CAPACITY;
    
    for (int i = 0; i < S; i++) {
        int slice_start = i * slice_capacity;
        if (slice_start >= n) break;
        int slice_end = min0(slice_start + slice_capacity, n);
        int slice_count = slice_end - slice_start;

        // 3. Ordenar la franja por Y
        qsort(items + slice_start, slice_count, sizeof(struct key_value), compare_y);

        // 4. Empaquetar en nodos
        for (int j = 0; j < slice_count; j += B_CAPACITY) {
            int node_start = slice_start + j;
            int elements_in_node = min0(B_CAPACITY, slice_count - j);

            struct node_disk nd = {0};
            nd.k = elements_in_node;

            struct rect_disk node_mbr = items[node_start].mbr;
            for (int k = 0; k < elements_in_node; k++) {
                nd.hijos[k] = items[node_start + k];
                rect_expand(&node_mbr, &nd.hijos[k].mbr);
            }

            int32_t idx_in_ram = append_node(tree, &nd);
            next_level_items[node_idx].mbr = node_mbr;
            next_level_items[node_idx].child_index = idx_in_ram;
            node_idx++;
        }
    }

    // 5. Recursión nivel superior
    build_str_recursive(tree, next_level_items, node_idx);
    free(next_level_items);
}

/**
 * Función de entrada para construir un R-tree mediante STR desde un array de puntos.
 * Entrada: tree (árbol vacío), puntos (datos iniciales), n (nro total de puntos).
 * Salida: Ninguna.
 */
void rtree_build_str(struct rtree_ram *tree, struct key_value *puntos, int n) {
    if (n == 0) return;
    init_rtree_ram(tree, (n / B_CAPACITY) + 2); 
    build_str_recursive(tree, puntos, n);
}

// --- I/O y Búsquedas en disco ---

/**
 * Realiza un volcado a disco volcando bit a bit los nodos almacenados secuencialmente en RAM.
 * Entrada: tree (estructura RAM finalizada), filename (ruta del archivo de salida).
 * Salida: Ninguna.
 */
void rtree_save_to_disk(struct rtree_ram *tree, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Error al abrir archivo para guardar");
        return;
    }
    fwrite(tree->nodes, sizeof(struct node_disk), tree->count, f);
    fclose(f);
}

/**
 * Lee exactamente un nodo usando un offset simulando lectura por bloque del SO.
 * Suma al contador de entrada I/O de acuerdo a la rúbrica.
 * Entrada: f (puntero de archivo abierto), node_index (ID nodo), node (buffer destino), io_count (puntero as contador externo).
 * Salida: Ninguna (los datos se depositan en `node` y el contador es incrementado).
 */
void readNode(FILE *f, int32_t node_index, struct node_disk *node, int *io_count) {
    fseek(f, (long)node_index * sizeof(struct node_disk), SEEK_SET);
    fread(node, sizeof(struct node_disk), 1, f);
    if (io_count) (*io_count)++;
}

/**
 * Transita el árbol directamente leyendo del archivo binario, aplicando intersecciones recursivamente.
 * Al llegar a un nivel de hojas contabiliza los puntos que caen dentro.
 * Entrada: f (archivo de datos binarios tree), node_index (iniciar por 0 para raíz), query (cuadrado de búsqueda MBR).
 * Entrada/Salida: io_count (contador lecturas acumuladas), points_found (contador salidas halladas).
 * Salida: Ninguna. (Los resultados viajan modificando las entradas en puntero).
 */
void rtree_query_disk_recursive(FILE *f, int32_t node_index, const struct rect_disk *query, int *io_count, int *points_found) {
    struct node_disk node;
    readNode(f, node_index, &node, io_count);

    for (int i = 0; i < node.k; i++) {
        if (rect_intersects(&node.hijos[i].mbr, query)) {
            if (node.hijos[i].child_index == -1) {
                // Es un punto (nodo hoja) dentro del R-tree
                (*points_found)++;
            } else {
                // Es un nodo interno (recursión)
                rtree_query_disk_recursive(f, node.hijos[i].child_index, query, io_count, points_found);
            }
        }
    }
}