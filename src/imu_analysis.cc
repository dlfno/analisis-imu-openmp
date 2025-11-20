#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip> // Para std::setprecision
#include <omp.h>   // Cabecera de OpenMP

// --- Constantes basadas en tu problemática y el CSV ---
const int WINDOW_SIZE = 200; // Nw = 2s * 100 Hz
const int STRIDE = 100;      // 50% de WINDOW_SIZE

/**
 * @brief Estructura para almacenar una sola muestra (fila) del IMU.
 * Solo guardamos los 6 ejes que necesitamos para el cómputo.
 */
struct IMUSample {
    double ax, ay, az, gx, gy, gz;
};

/**
 * @brief Estructura para almacenar todas las métricas calculadas para UNA ventana.
 */
struct WindowMetrics {
    // Media (mu) para los 6 ejes [ax, ay, az, gx, gy, gz]
    double mu[6];
    // Desviación Estándar (sigma) para los 6 ejes
    double sigma[6];
    // Valor RMS para los 6 ejes
    double rms[6];
    // Energía de la magnitud del acelerómetro (E_||a||)
    double energy_accel_mag;
};

/**
 * @brief Calcula todas las métricas requeridas para una sola ventana de datos.
 * @param window_data Puntero al inicio de los datos de la ventana.
 * @param N El número de muestras en la ventana (Nw).
 * @return Un struct WindowMetrics con todos los resultados.
 */
WindowMetrics calculate_metrics(const IMUSample* window_data, int N) {
    WindowMetrics metrics = {0};
    double sums[6] = {0};      // Suma de x[n] para la media
    double sum_sq[6] = {0};    // Suma de x[n]^2 para RMS y sigma
    double sum_accel_mag_sq = 0; // Suma de ||a[n]||^2 para la Energía

    for (int i = 0; i < N; ++i) {
        const IMUSample& s = window_data[i];
        
        // Creamos un arreglo temporal para iterar sobre los 6 ejes
        double data[6] = {s.ax, s.ay, s.az, s.gx, s.gy, s.gz};

        // 1. Cálculo de la magnitud del acelerómetro y su energía
        // ||a||^2 = ax^2 + ay^2 + az^2
        double accel_mag_sq = (s.ax * s.ax) + (s.ay * s.ay) + (s.az * s.az);
        // E = sum( ||a[n]||^2 )
        sum_accel_mag_sq += accel_mag_sq;

        // 2. Cálculos para media, RMS y sigma
        for (int j = 0; j < 6; ++j) {
            sums[j] += data[j];
            sum_sq[j] += (data[j] * data[j]);
        }
    }

    // 3. Finalizar los cálculos (Media, RMS, Sigma, Energía)
    metrics.energy_accel_mag = sum_accel_mag_sq; // Energía total

    for (int j = 0; j < 6; ++j) {
        // Media: mu = (1/N) * sum(x[n])
        metrics.mu[j] = sums[j] / N;
        
        // RMS: RMS = sqrt( (1/N) * sum(x[n]^2) )
        metrics.rms[j] = std::sqrt(sum_sq[j] / N);
        
        // Desviación Estándar: sigma = sqrt( (1/N) * sum(x[n]^2) - mu^2 )
        // Usamos la fórmula computacional E[X^2] - (E[X])^2
        double mean_sq = metrics.mu[j] * metrics.mu[j];
        metrics.sigma[j] = std::sqrt( (sum_sq[j] / N) - mean_sq );
    }
    
    return metrics;
}

/**
 * @brief Función principal
 */
int main() {
    std::string filename = "Data 04c2.csv";
    std::vector<IMUSample> all_samples;

    // =====================================================================
    // 1. LECTURA SECUENCIAL
    // =====================================================================
    std::cout << "Iniciando lectura secuencial del archivo: " << filename << "..." << std::endl;
    std::ifstream file(filename);
    std::string line;

    // Omitir la cabecera (header)
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        IMUSample sample;
        
        // Omitimos las primeras 2 columnas (t_ms, clip_id)
        std::getline(ss, cell, ','); 
        std::getline(ss, cell, ','); 
        
        // Leemos las 6 columnas de datos
        std::getline(ss, cell, ','); sample.ax = std::stod(cell);
        std::getline(ss, cell, ','); sample.ay = std::stod(cell);
        std::getline(ss, cell, ','); sample.az = std::stod(cell);
        std::getline(ss, cell, ','); sample.gx = std::stod(cell);
        std::getline(ss, cell, ','); sample.gy = std::stod(cell);
        std::getline(ss, cell, ','); sample.gz = std::stod(cell);
        
        // (Omitimos la columna 'label')
        
        all_samples.push_back(sample);
    }
    file.close();

    long long total_samples = all_samples.size();
    if (total_samples < WINDOW_SIZE) {
        std::cerr << "Error: No hay suficientes muestras (" << total_samples 
                  << ") para una ventana de " << WINDOW_SIZE << std::endl;
        return 1;
    }
    
    // Calcular número total de ventanas
    long long num_windows = static_cast<long long>(std::floor(
                            static_cast<double>(total_samples - WINDOW_SIZE) / STRIDE)) + 1;

    std::cout << "Lectura completada. Muestras totales: " << total_samples << std::endl;
    std::cout << "Parámetros: Tamaño de Ventana (Nw) = " << WINDOW_SIZE 
              << ", Stride = " << STRIDE << std::endl;
    std::cout << "Número total de ventanas a procesar: " << num_windows << std::endl;

    // Pre-asignar el buffer de salida (requisito de la problemática)
    std::vector<WindowMetrics> results(num_windows);

    std::cout << "\nIniciando cómputo paralelo con OpenMP..." << std::endl;

    // Tomar tiempo de inicio
    double start_time = omp_get_wtime();

    // =====================================================================
    // 2. CÓMPUTO PARALELO
    // =====================================================================
    // Usamos #pragma omp parallel for para repartir las iteraciones (ventanas)
    // entre los hilos de ejecución disponibles.
    #pragma omp parallel for
    for (long long i = 0; i < num_windows; ++i) {
        // Cada hilo calcula el índice de inicio de su ventana
        long long start_index = i * STRIDE;
        
        // Obtenemos un puntero al inicio de los datos de esta ventana
        // Esto evita copiar datos y es muy eficiente
        const IMUSample* window_data_start = &all_samples[start_index];
        
        // Calculamos las métricas para esta ventana
        WindowMetrics computed_metrics = calculate_metrics(window_data_start, WINDOW_SIZE);
        
        // Guardamos los resultados en el buffer pre-asignado en la posición 'i'
        results[i] = computed_metrics;
    }
    // Barrera implícita: El programa espera a que todos los hilos terminen
    // antes de continuar.

    // Tomar tiempo de finalización
    double end_time = omp_get_wtime();
    double time_elapsed = end_time - start_time;

    std::cout << "Cómputo paralelo finalizado." << std::endl;

    // =====================================================================
    // 3. VERIFICACIÓN Y DESEMPEÑO
    // =====================================================================
    std::cout << "\n--- Resumen y Verificación ---" << std::endl;
    std::cout << "Archivo procesado: " << filename << std::endl;
    std::cout << "Total de ventanas procesadas: " << results.size() << " (esperadas: " << num_windows << ")" << std::endl;
    std::cout << "Tamaño de ventana (Nw): " << WINDOW_SIZE << " muestras" << std::endl;
    std::cout << "Stride: " << STRIDE << " muestras" << std::endl;

    std::cout << "\n--- Desempeño (OpenMP) ---" << std::endl;
    std::cout << "Tiempo de cómputo paralelo (wall clock): " 
              << std::fixed << std::setprecision(6) << time_elapsed << " segundos" << std::endl;
    
    // Imprimir resumen de la PRIMERA ventana (índice 0) como muestra
    if (num_windows > 0) {
        std::cout << "\n--- Métricas de la Ventana 0 (para verificación) ---" << std::endl;
        WindowMetrics& r = results[0];
        std::cout << std::fixed << std::setprecision(5);
        std::cout << "  Ejes:         [     ax   ,     ay   ,     az   ,     gx   ,     gy   ,     gz   ]" << std::endl;
        std::cout << "  Media (mu):   [";
        for (int j=0; j<6; ++j) std::cout << std::setw(9) << r.mu[j] << (j<5 ? "," : "");
        std::cout << " ]" << std::endl;
        std::cout << "  Sigma (std):  [";
        for (int j=0; j<6; ++j) std::cout << std::setw(9) << r.sigma[j] << (j<5 ? "," : "");
        std::cout << " ]" << std::endl;
        std::cout << "  RMS:          [";
        for (int j=0; j<6; ++j) std::cout << std::setw(9) << r.rms[j] << (j<5 ? "," : "");
        std::cout << " ]" << std::endl;
        std::cout << "  Energía ||a||: " << r.energy_accel_mag << std::endl;
    }

    return 0;
}
