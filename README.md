# Análisis de Registros Inerciales (IMU) con OpenMP

Este proyecto es una implementación en C++ y OpenMP para la problemática "OpenMP para análisis de registros inerciales". El objetivo es procesar eficientemente grandes volúmenes de datos de IMU mediante la paralelización del cómputo de métricas en ventanas deslizantes.

## Características

* **Lectura Secuencial:** Carga de datos desde un archivo CSV.
* **Ventanas Deslizantes:** Construcción de un arreglo de ventanas de 2s con 50% de stride (solapamiento).
* **Cómputo Paralelo:** Uso de `#pragma omp parallel for` para distribuir el cálculo de métricas entre múltiples hilos.
* **Métricas Calculadas:** Para cada ventana y por cada eje (ax, ay, az, gx, gy, gz):
    * Media ($\mu$)
    * Desviación Estándar ($\sigma$)
    * Valor RMS
* **Métrica Adicional:** Energía de la magnitud del acelerómetro ($E_{\|\mathbf{a}\|}$).

## Requisitos Previos

Para compilar y ejecutar este proyecto, necesitas:

1.  Un compilador de C++ con soporte para OpenMP:
    * **Linux:** `g++` (generalmente incluido en `build-essential`).
    * **macOS:** `g++` (instalado vía Homebrew: `brew install gcc`) o `clang++` (instalado vía Homebrew: `brew install libomp`).
2.  `make` para automatizar la compilación.
3.  `git` para clonar el repositorio.

## Compilación

Este proyecto incluye un `Makefile` que simplifica la compilación.

1.  **Clona el repositorio:**
    ```bash
    git clone https://github.com/dlfno/analisis-imu-openmp
    cd analisis-imu-openmp
    ```

2.  **Compila el proyecto:**
    ```bash
    make
    ```
    Esto creará el ejecutable en la carpeta `bin/`. Si usas un compilador diferente a `g++-14`, edita la variable `CC` en el `Makefile`.

## Uso

El ejecutable `imu_analysis` buscará el archivo `Datos Q1.csv` en la carpeta `data/`.

1.  **Ejecuta el programa:**
    ```bash
    ./bin/imu_analysis
    ```

2.  **Controlar el número de hilos:**
    Puedes controlar el número de hilos de OpenMP usando la variable de entorno `OMP_NUM_THREADS`.
    ```bash
    export OMP_NUM_THREADS=8
    ./bin/imu_analysis
    ```

## Verificación de Consistencia y Desempeño

El `Makefile` incluye un objetivo para verificar la consistencia de los cálculos (que las métricas no dependen del número de hilos) y comparar el desempeño.

1.  **Ejecuta la prueba:**
    ```bash
    make run-test
    ```

2.  **Resultados Esperados:**
    El comando `diff` mostrará que la única diferencia entre la salida de 1 hilo y la de 8 hilos es la línea que reporta el tiempo de cómputo, demostrando la consistencia.

    ```bash
    16c16
    < Tiempo de cómputo paralelo (wall clock): 0.000202 segundos
    ---
    > Tiempo de cómputo paralelo (wall clock): 0.000208 segundos
    ```

### Análisis de Desempeño
Con el conjunto de datos `Datos Q1.csv` (30,000 muestras), el tiempo total de cómputo es extremadamente bajo (~200 microsegundos). A esta escala, el costo (overhead) de crear y gestionar los hilos de OpenMP es mayor que el trabajo a realizar, por lo que no se observa un *speedup*. Para observar una ganancia de desempeño, se necesitaría un conjunto de datos mucho más grande o cálculos por ventana computacionalmente más costosos.

