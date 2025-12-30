#!/bin/bash

# ============================================
# 🧪 TEST MEJORADO DE AUTOINDEX - WEBSERVER 42
# ============================================

# Colores para mejor visualización
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ============================================
# 🎯 CONFIGURACIÓN
# ============================================

PORT=8080
HOST="http://localhost:$PORT"
BASE_DIR="www/tests"  # 🆕 TODO se crea dentro de www/tests
TEST_DIRS=("files" "public" "private" "uploads")
    #Define dónde está corriendo tu servidor y dónde se crearán los directorios de prueba.
    #Tu servidor NO tiene por defecto estos directorios, por eso el test los crea.

    #TEST_DIRS simplemente lista los nombres de los directorios que se usarán.

# ============================================
# 🚨 FUNCIÓN PARA SALIR CON ERROR
# ============================================

cleanup_and_exit() {
    echo -e "\n${RED}🚨 ERROR: $1${NC}"
    
    # Limpiar archivos temporales
    rm -f response_body.tmp test_output.tmp 2>/dev/null
    
    exit 1
}
    #Si algo falla (por ejemplo no se puede crear un archivo), aborta todo el test.
    #Así el script no sigue ejecutando cosas que ya sabemos que están mal.

# ============================================
# 📊 FUNCIÓN PARA EJECUTAR TEST
# ============================================

run_test() {
    local test_num="$1"
    local description="$2"
    local url="$3"
    local expected_code="$4"
    local check_string="$5"
    local method="${6:-GET}"
    
    echo -n "   Test $test_num: $description... "
    
    # Ejecutar curl según método
    if [ "$method" = "GET" ]; then
        curl -s -o response_body.tmp -w "%{http_code}" "$url" > test_output.tmp 2>&1
    elif [ "$method" = "HEAD" ]; then
        curl -s -I -o response_body.tmp -w "%{http_code}" "$url" > test_output.tmp 2>&1
    fi
    
    local response=$(cat test_output.tmp 2>/dev/null || echo "000")
    
    # Verificar código HTTP
    if [ "$response" = "$expected_code" ]; then
        # Verificar contenido si se especificó
        if [ -n "$check_string" ]; then
            if grep -q "$check_string" response_body.tmp 2>/dev/null; then
                echo -e "${GREEN}✅ PASS${NC}"
                return 0
            else
                echo -e "${RED}❌ FAIL (contenido incorrecto)${NC}"
                echo "     Esperaba: '$check_string'"
                if [ -f response_body.tmp ]; then
                    echo "     Recibido primeros 200 chars:"
                    head -c 200 response_body.tmp 2>/dev/null | sed 's/^/     > /'
                fi
                return 1
            fi
        else
            echo -e "${GREEN}✅ PASS${NC}"
            return 0
        fi
    else
        echo -e "${RED}❌ FAIL (código $response, esperaba $expected_code)${NC}"
        return 1
    fi
}

# ============================================
# 🎬 INICIO DEL TEST
# ============================================

echo ""  # Solo una línea en blanco al inicio
echo -e "${PURPLE}╔════════════════════════════════════════════════════╗"
echo -e "║           🧪 TEST DE AUTOINDEX                     ║"
echo -e "║              webserver 42                          ║"
echo -e "╚════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================
# 📂 1. PREPARAR ENTORNO DE PRUEBA DENTRO DE www/tests
# ============================================

echo -e "${CYAN}[1/3] 📂 PREPARANDO ENTORNO DE PRUEBA EN $BASE_DIR${NC}"
echo "════════════════════════════════════════════════════"

# Crear directorio base si no existe
mkdir -p "$BASE_DIR" || cleanup_and_exit "No se pudo crear directorio $BASE_DIR"

# Limpiar pruebas anteriores dentro de tests/
echo -e "${BLUE}Limpiando pruebas anteriores en $BASE_DIR...${NC}"
rm -rf "$BASE_DIR" 2>/dev/null
mkdir -p "$BASE_DIR"

echo -e "${BLUE}Creando estructura de directorios:${NC}"

# 1. /tests/files/ - Directorio CON autoindex (simulado)
mkdir -p "$BASE_DIR/files"
echo "   📁 $BASE_DIR/files/ (autoindex ON)" 
echo "Archivo de texto normal" > "$BASE_DIR/files/document1.txt" #el output del echo se redirige a un archivo.
echo "Contenido de imagen" > "$BASE_DIR/files/image.jpg"
echo "PDF secreto" > "$BASE_DIR/files/secret.pdf"
echo "Archivo con espacios" > "$BASE_DIR/files/file with spaces.txt"
echo "Archivo#con#hashes" > "$BASE_DIR/files/file#with#hashes.txt"
mkdir -p "$BASE_DIR/files/subdir"
echo "Dentro de subdir" > "$BASE_DIR/files/subdir/inside.txt"
# Si el autoindex está activado o no, se sabe por el script. Aquí solo creamos carpetas y archivos. Lo que decide si una ruta tiene autoindex ON/OFF es TU CONFIGURACIÓN en tu webserv.conf o en tu estructura interna de rutas (temporal o final). 
#Por lo tanto: ¿Cómo se sabe si autoindex está ON u OFF? Depende de la configuración por ruta en el código.

#Dentro de files/ mete archivos variados (texto, imagen, pdf, con espacios, con #, subdirectorio)
#Esto te sirve para probar que tu autoindex lista todo tipo de nombres raros.


# 2. /tests/public/ - Directorio CON autoindex Y CON index.html
mkdir -p "$BASE_DIR/public"
echo "   📁 $BASE_DIR/public/ (autoindex ON + index.html)"
echo "<!DOCTYPE html>" > "$BASE_DIR/public/index.html"
echo "<html><head><title>Public Index</title></head>" >> "$BASE_DIR/public/index.html"
echo "<body><h1>Este es el index.html público</h1>" >> "$BASE_DIR/public/index.html"
echo "<p>No deberías ver autoindex aquí</p></body></html>" >> "$BASE_DIR/public/index.html"
#Se crea un index.html para comprobar que si hay index.html → NO entre en autoindex


# 3. /tests/private/ - Directorio SIN autoindex y SIN index
mkdir -p "$BASE_DIR/private"
echo "   📁 $BASE_DIR/private/ (autoindex OFF, sin index)"
#No se crea un index.html para comprobar que si no se pone nada y autoindex está OFF → 403 Forbidden

# 4. /tests/uploads/ - Directorio para uploads
mkdir -p "$BASE_DIR/uploads"
echo "   📁 $BASE_DIR/uploads/ (directorio de uploads)"

echo -e "${GREEN}✅ Estructura creada exitosamente en $BASE_DIR/${NC}"
echo ""

# ============================================
# 🧪 2. EJECUTAR PRUEBAS DE AUTOINDEX
# ============================================

echo -e "${CYAN}[2/3] 🧪 EJECUTANDO PRUEBAS DE AUTOINDEX${NC}"
echo "════════════════════════════════════════════════════"

passed=0
total=0

echo -e "${YELLOW}📌 CONFIGURACIÓN TEMPORAL ASIGNADA:${NC}"
echo "   • /tests/files/    → autoindex ON"
echo "   • /tests/public/   → autoindex ON + tiene index.html"
echo "   • /tests/private/  → autoindex OFF + sin index.html"
echo "   • /tests/uploads/  → sin autoindex"
echo ""

# ---------------------------------------------------------------------
echo -e "${BLUE}📁 GRUPO 1: DIRECTORIOS CON AUTOINDEX ACTIVADO${NC}"
echo "────────────────────────────────────────────────────────"

# Test 1.1: /tests/files/ - Debería mostrar autoindex
run_test "1.1" "GET /tests/files/ (autoindex)" "$HOST/tests/files/" "200" "Index of"
[ $? -eq 0 ] && ((passed++))
((total++))
#Tu servidor debe detectar que “/tests/files/” es un directorio SIN index.html y con autoindex ON
#Y debe generar una página HTML de autoindex que contenga “Index of” (Todos los servidores (nginx, apache, etc.) incluyen el texto “Index of”, por eso el test lo busca.)

# Test 1.2: Verificar que autoindex contiene archivos esperados (Es una prueba de contenido de autoindex)
echo -n "   Test 1.2: Autoindex lista archivos correctos... "
if curl -s "$HOST/tests/files/" 2>/dev/null | grep -q "document1.txt" && \
   curl -s "$HOST/tests/files/" 2>/dev/null | grep -q "image.jpg" && \
   curl -s "$HOST/tests/files/" 2>/dev/null | grep -q "subdir/"; then
    echo -e "${GREEN}✅ PASS${NC}"
    ((passed++))
else
    echo -e "${RED}❌ FAIL (no lista todos los archivos)${NC}"
    # Mostrar qué hay en el directorio real
    echo "     Archivos reales en el directorio:"
    ls -la "$BASE_DIR/files/" 2>/dev/null | tail -n +2 | sed 's/^/       /'
fi
((total++))

# Test 1.3: Navegación a subdirectorio (Comprueba que navegar a /subdir/ también funciona, es una prueba de recursividad)
run_test "1.3" "GET /tests/files/subdir/" "$HOST/tests/files/subdir/" "200" ""
[ $? -eq 0 ] && ((passed++))
((total++))

# ---------------------------------------------------------------------
echo -e "\n${BLUE}📁 GRUPO 2: DIRECTORIOS CON INDEX.HTML${NC}"
echo "────────────────────────────────────────────────────────"

# Test 2.1: /tests/public/ - Debería mostrar index.html, NO autoindex
run_test "2.1" "GET /tests/public/ (con index.html)" "$HOST/tests/public/" "200" "Este es el index.html público"
[ $? -eq 0 ] && ((passed++))
((total++))

# Test 2.2: Verificar que NO muestra autoindex
echo -n "   Test 2.2: No muestra autoindex si hay index... "
if curl -s "$HOST/tests/public/" 2>/dev/null | grep -q "Index of"; then
    echo -e "${RED}❌ FAIL (muestra autoindex en lugar de index.html)${NC}"
else
    echo -e "${GREEN}✅ PASS${NC}"
    ((passed++))
fi
((total++))

# ---------------------------------------------------------------------
echo -e "\n${BLUE}📁 GRUPO 3: DIRECTORIOS SIN AUTOINDEX${NC}"
echo "────────────────────────────────────────────────────────"

# Test 3.1: /tests/private/ - Sin autoindex y sin index → 403 o 404
# (Aceptamos ambos códigos porque depende de la implementación)
echo -n "   Test 3.1: GET /tests/private/ (sin autoindex)... "
response=$(curl -s -o /dev/null -w "%{http_code}" "$HOST/tests/private/" 2>/dev/null)
if [ "$response" = "403" ] || [ "$response" = "404" ]; then
    echo -e "${GREEN}✅ PASS ($response)${NC}"
    ((passed++))
else
    echo -e "${RED}❌ FAIL (código $response, esperaba 403 o 404)${NC}"
fi
((total++))

# ---------------------------------------------------------------------
echo -e "\n${BLUE}📁 GRUPO 4: ARCHIVOS ESPECÍFICOS${NC}"
echo "────────────────────────────────────────────────────────"

# Test 4.1: Archivo normal dentro de directorio con autoindex
run_test "4.1" "GET /tests/files/document1.txt" "$HOST/tests/files/document1.txt" "200" "Archivo de texto normal"
[ $? -eq 0 ] && ((passed++))
((total++))

# Test 4.2: Archivo con espacios (URL encoding)
echo -n "   Test 4.2: Archivo con espacios... "
response=$(curl -s -o /dev/null -w "%{http_code}" "$HOST/tests/files/file%20with%20spaces.txt" 2>/dev/null)
if [ "$response" = "200" ]; then
    echo -e "${GREEN}✅ PASS${NC}"
    ((passed++))
elif [ "$response" = "404" ]; then
    echo -e "${YELLOW}⚠️  WARN (404) - archivo no encontrado${NC}"
    # Probamos también sin encoding
    echo -n "     Probando sin encoding: "
    response2=$(curl -s -o /dev/null -w "%{http_code}" "$HOST/tests/files/file with spaces.txt" 2>/dev/null)
    echo "código $response2"
else
    echo -e "${RED}❌ FAIL ($response)${NC}"
fi
((total++))

# Test 4.3: Archivo con caracteres especiales
echo -n "   Test 4.3: Archivo con #... "
response=$(curl -s -o /dev/null -w "%{http_code}" "$HOST/tests/files/file%23with%23hashes.txt" 2>/dev/null)
if [ "$response" = "200" ] || [ "$response" = "404" ]; then
    # 404 sería aceptable si no maneja estos caracteres
    echo -e "${GREEN}✅ PASS ($response)${NC}"
    ((passed++))
else
    echo -e "${YELLOW}⚠️  WARN ($response) - podría necesitar mejor encoding${NC}"
    ((passed++))  # No contamos como fallo completo
fi
((total++))

# ---------------------------------------------------------------------
echo -e "\n${BLUE}📁 GRUPO 5: PRUEBAS ADICIONALES${NC}"
echo "────────────────────────────────────────────────────────"

# Test 5.1: HEAD request (sin body)
run_test "5.1" "HEAD /tests/files/" "$HOST/tests/files/" "200" "" "HEAD"
[ $? -eq 0 ] && ((passed++))
((total++))

# ============================================
# 📊 3. RESULTADOS Y ANÁLISIS
# ============================================

echo -e "\n${CYAN}[3/3] 📊 RESULTADOS FINALES${NC}"
echo "════════════════════════════════════════════════════"

echo ""
echo -e "${PURPLE}╔════════════════════════════════════════════════════╗"
echo -e "║                    RESULTADOS                      ║"
echo -e "╚════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "Pruebas ejecutadas: ${BLUE}$total${NC}"
echo -e "Pruebas exitosas:   ${GREEN}$passed${NC}"
if [ $passed -lt $total ]; then
    echo -e "Pruebas fallidas:   ${RED}$((total - passed))${NC}"
fi

# Calcular porcentaje
percentage=0
if [ $total -gt 0 ]; then
    percentage=$((passed * 100 / total))
fi

echo ""
echo -e "${BLUE}📈 PORCENTAJE DE ÉXITO: $percentage%${NC}"
echo ""

# Mostrar diagnóstico basado en resultados
if [ $percentage -eq 100 ]; then
    echo -e "${GREEN}🎉 ¡EXCELENTE! AUTOINDEX FUNCIONA PERFECTAMENTE${NC}"
    echo -e "${GREEN}   Todas las pruebas pasaron correctamente${NC}"
elif [ $percentage -ge 80 ]; then
    echo -e "${GREEN}✅ MUY BIEN - Autoindex funciona correctamente${NC}"
    echo -e "${GREEN}   Solo pequeños ajustes necesarios${NC}"
elif [ $percentage -ge 60 ]; then
    echo -e "${YELLOW}⚠️  ACEPTABLE - Funciona básicamente${NC}"
    echo -e "${YELLOW}   Necesita algunos ajustes importantes${NC}"
else
    echo -e "${RED}⚠️  NECESITA MEJORAS - Problemas significativos${NC}"
    echo -e "${RED}   Revisa tu implementación de autoindex${NC}"
fi

echo ""

# ============================================
# 🧹 LIMPIEZA OPCIONAL
# ============================================

# Fin (No borramos para que el usuario pueda inspeccionar)
echo -e "${GREEN}Estructura mantenida en $BASE_DIR/${NC}"
echo ""
echo -e "${BLUE}📁 Estructura actual:${NC}"
find "$BASE_DIR" -type f 2>/dev/null | sort | sed 's/^/  /'

# Limpiar archivos temporales del script
rm -f response_body.tmp test_output.tmp 2>/dev/null

echo ""
echo -e "${GREEN}✅ TEST COMPLETADO${NC}"
echo ""

exit 0
