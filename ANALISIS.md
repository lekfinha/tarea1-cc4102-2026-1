
El segundo es el documento analizando la teoría y el comportamiento de los resultados obtenidos (`ANALISIS.md`), que te servirá directamente para la sección 6.2 de tu informe final.

```markdown
# Análisis Teórico de los Resultados

Al observar las métricas generadas por el experimento, podemos validar de forma empírica el "trade-off" (compromiso) clásico en las estructuras de bases de datos espaciales.

## 1. Tiempos de Construcción
Los resultados reflejan un crecimiento lineal esperado a medida que los datos de entrada ($N$) se duplican. 
- **Nearest-X es consistentemente más rápido que STR.** 
- **Razón teórica:** Nearest-X requiere ordenar el conjunto completo de datos únicamente usando la coordenada X en cada nivel recursivo ($O(N \log N)$). Por otro lado, STR requiere ordenar primero por la coordenada X, dividir en sub-arreglos y luego volver a aplicar ordenamientos locales por la coordenada Y. Este trabajo computacional extra penaliza de forma visible el tiempo total de procesado.

## 2. Consultas y Eficiencia de I/O
A pesar de su lentitud inicial, el experimento de consultas demuestra drásticamente por qué STR es preferido sobre Nearest-X.
- **Nearest-X genera solapamientos masivos:** Al particionar solo en un eje (X), los MBRs (Mínimum Bounding Rectangles) resultantes terminan siendo franjas muy largas y delgadas en el eje Y. Una búsqueda pequeña tiene altas probabilidades de intersectar múltiples franjas, obligando al sistema a cargar desde disco muchos nodos internos que finalmente no contienen puntos útiles.
- **STR minimiza el solapamiento:** Al particionar cuadriculando el espacio en X e Y, STR genera MBRs compactos y cuadrados. Esto preserva mucho mejor la localidad espacial. Como se observó en los datos de prueba masivos ($N=2^{24}$), STR requirió fracciones ínfimas de lecturas a disco frente a su equivalente Nearest-X para encontrar exactamente la misma cantidad de puntos.

## 3. Dispersión en la Distribución de Datos
La experimentación con un dataset sintético (aleatorio uniforme) versus uno orgánico (edificios de Europa) presenta disparidades notables en la desviación estándar:
- **Aleatorio:** La desviación es muy baja. Como los puntos están esparcidos uniformemente en el plano, cualquier ventana aleatoria de tamaño $s$ atrapará estadísticamente una cantidad de datos predecible ($\approx Á rea \times N$).
- **Europa:** La desviación estándar es altísima, a menudo superando a la media de puntos encontrados. Esto confirma la naturaleza aglomerada (clusters) de la civilización humana. Un rectángulo aleatorio tiene altas probabilidades de caer en zonas vacías (océanos, montañas), encontrando 0 puntos; pero cuando intersecta una gran urbe metropolitana, recuperará miles de registros de forma abrupta, distorsionando severamente el promedio.