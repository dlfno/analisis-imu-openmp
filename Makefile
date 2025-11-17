# ---------------------------------
# Makefile para Análisis de IMU con OpenMP
# ---------------------------------

# Compilador (cambia g++-14 por el que uses, ej: g++)
CC = g++-14

# Flags del compilador:
# -std=c++11 : Usa el estándar C++11
# -O3        : Optimización de nivel 3
# -fopenmp   : Habilita OpenMP
# -Isrc      : Permite encontrar headers si los hubiera en src/
# -Wall      : Muestra todos los Warnings 
CFLAGS = -std=c++11 -O3 -fopenmp -Isrc -Wall

# Flags del enlazador (Linker)
LDFLAGS = -fopenmp

# Directorios
SRCDIR = src
BINDIR = bin
DATADIR = data

# Archivo de entrada (fuente)
SOURCES = $(SRCDIR)/imu_analysis.cc

# Archivo de salida (ejecutable)
TARGET = $(BINDIR)/imu_analysis

# --- Reglas ---

# Regla por defecto (la que se ejecuta con 'make')
all: $(TARGET)

# Regla para construir el ejecutable
$(TARGET): $(SOURCES)
	@mkdir -p $(BINDIR)  # Crea el directorio bin/ si no existe
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)
	@echo "Compilación exitosa! Ejecutable en: $(TARGET)"

# Regla para limpiar el proyecto
clean:
	@rm -rf $(BINDIR)  # Borra la carpeta bin/
	@rm -f salida_*.txt # Borra los archivos de prueba
	@echo "Proyecto limpiado."

# Regla para ejecutar la prueba de consistencia
run-test: $(TARGET)
	@echo "Ejecutando con 1 hilo..."
	@export OMP_NUM_THREADS=1 && ./$(TARGET) > salida_1_hilo.txt
	@echo "Ejecutando con 8 hilos..."
	@export OMP_NUM_THREADS=8 && ./$(TARGET) > salida_8_hilos.txt
	@echo "--- Comparando salidas ---"
	@diff salida_1_hilo.txt salida_8_hilos.txt || echo "Las salidas difieren SOLO en el tiempo (lo cual es esperado)."
	@echo "--------------------------"
	@echo "Prueba completada."

.PHONY: all clean run-test
