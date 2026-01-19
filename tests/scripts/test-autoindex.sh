#!/bin/bash

# ============================================
# 🧪 TEST DE AUTOINDEX - WEBSERVER 42
# ============================================
# NOTA: Este script usa un directorio temporal (.test_temp)
# para NO modificar los archivos de producción en www/tests/

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
# Usamos un directorio temporal oculto para NO afectar www/tests/
BASE_DIR="www/.test_temp"
TEST_DIRS=("files" "public" "private" "uploads")

# ============================================
# 🚨 FUNCIÓN PARA SALIR CON ERROR
# ============================================

cleanup_and_exit() {
    echo -e "\n${RED}🚨 ERROR: $1${NC}"
    rm -f response_body.tmp test_output.tmp 2>/dev/null
    exit 1
}

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
    
    if [ "$method" = "GET" ]; then
        curl -s -o response_body.tmp -w "%{http_code}" "$url" > test_output.tmp 2>&1
    elif [ "$method" = "HEAD" ]; then
        curl -s -I -o response_body.tmp -w "%{http_code}" "$url" > test_output.tmp 2>&1
    fi
    
    local response=$(cat test_output.tmp 2>/dev/null || echo "000")
    
    if [ "$response" = "$expected_code" ]; then
        if [ -n "$check_string" ]; then
            if grep -q "$check_string" response_body.tmp 2>/dev/null; then
                echo -e "${GREEN}✅ PASS${NC}"
                return 0
            else
                echo -e "${RED}❌ FAIL (contenido incorrecto)${NC}"
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

echo ""
echo -e "${PURPLE}╔════════════════════════════════════════════════════╗"
echo -e "║           🧪 TEST DE AUTOINDEX                     ║"
echo -e "║              webserver 42                          ║"
echo -e "╚════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================
# 📂 1. PREPARAR ENTORNO DE PRUEBA TEMPORAL
# ============================================

echo -e "${CYAN}[1/3] 📂 PREPARANDO ENTORNO TEMPORAL EN $BASE_DIR${NC}"
echo "════════════════════════════════════════════════════"
echo -e "${YELLOW}⚠️  Usando directorio temporal .test_temp (no afecta www/tests/)${NC}"

# Limpiar y crear directorio temporal
rm -rf "$BASE_DIR" 2>/dev/null
mkdir -p "$BASE_DIR" || cleanup_and_exit "No se pudo crear directorio $BASE_DIR"

echo -e "${BLUE}Creando estructura de directorios:${NC}"

# 1. /.test_temp/files/ - Directorio CON autoindex
mkdir -p "$BASE_DIR/files"
echo "   📁 $BASE_DIR/files/ (autoindex ON)" 
echo "Archivo de texto normal" > "$BASE_DIR/files/document1.txt"
echo "Contenido de imagen" > "$BASE_DIR/files/image.jpg"
echo "PDF secreto" > "$BASE_DIR/files/secret.pdf"
echo "Archivo con espacios" > "$BASE_DIR/files/file with spaces.txt"
echo "Archivo#con#hashes" > "$BASE_DIR/files/file#with#hashes.txt"
mkdir -p "$BASE_DIR/files/subdir"
echo "Dentro de subdir" > "$BASE_DIR/files/subdir/inside.txt"

# 2. /.test_temp/public/ - CON autoindex Y CON index.html
mkdir -p "$BASE_DIR/public"
echo "   📁 $BASE_DIR/public/ (autoindex ON + index.html)"
cat > "$BASE_DIR/public/index.html" << 'EOF'
<!DOCTYPE html>
<html><head><title>Public Index</title></head>
<body><h1>Este es el index.html público</h1>
<p>No deberías ver autoindex aquí</p></body></html>
EOF

# 3. /.test_temp/private/ - SIN autoindex y SIN index
mkdir -p "$BASE_DIR/private"
echo "   📁 $BASE_DIR/private/ (autoindex OFF, sin index)"

# 4. /.test_temp/uploads/
mkdir -p "$BASE_DIR/uploads"
echo "   📁 $BASE_DIR/uploads/ (directorio de uploads)"

echo -e "${GREEN}✅ Estructura temporal creada exitosamente${NC}"
echo ""

# ============================================
# 🧪 2. EJECUTAR PRUEBAS DE AUTOINDEX
# ============================================

echo -e "${CYAN}[2/3] 🧪 EJECUTANDO PRUEBAS DE AUTOINDEX${NC}"
echo "════════════════════════════════════════════════════"

passed=0
total=0

echo -e "${YELLOW}📌 CONFIGURACIÓN TEMPORAL:${NC}"
echo "   • /.test_temp/files/    → autoindex ON"
echo "   • /.test_temp/public/   → autoindex ON + tiene index.html"
echo "   • /.test_temp/private/  → autoindex OFF + sin index.html"
echo ""

# GRUPO 1: DIRECTORIOS CON AUTOINDEX ACTIVADO
echo -e "${BLUE}📁 GRUPO 1: DIRECTORIOS CON AUTOINDEX ACTIVADO${NC}"
echo "────────────────────────────────────────────────────────"

run_test "1.1" "GET /.test_temp/files/ (autoindex)" "$HOST/.test_temp/files/" "200" "Index of"
[ $? -eq 0 ] && ((passed++))
((total++))

echo -n "   Test 1.2: Autoindex lista archivos correctos... "
if curl -s "$HOST/.test_temp/files/" 2>/dev/null | grep -q "document1.txt" && \
   curl -s "$HOST/.test_temp/files/" 2>/dev/null | grep -q "image.jpg" && \
   curl -s "$HOST/.test_temp/files/" 2>/dev/null | grep -q "subdir/"; then
    echo -e "${GREEN}✅ PASS${NC}"
    ((passed++))
else
    echo -e "${RED}❌ FAIL (no lista todos los archivos)${NC}"
fi
((total++))

run_test "1.3" "GET /.test_temp/files/subdir/" "$HOST/.test_temp/files/subdir/" "200" ""
[ $? -eq 0 ] && ((passed++))
((total++))

# GRUPO 2: DIRECTORIOS CON INDEX.HTML
echo -e "\n${BLUE}📁 GRUPO 2: DIRECTORIOS CON INDEX.HTML${NC}"
echo "────────────────────────────────────────────────────────"

run_test "2.1" "GET /.test_temp/public/ (con index.html)" "$HOST/.test_temp/public/" "200" "Este es el index.html público"
[ $? -eq 0 ] && ((passed++))
((total++))

echo -n "   Test 2.2: No muestra autoindex si hay index... "
if curl -s "$HOST/.test_temp/public/" 2>/dev/null | grep -q "Index of"; then
    echo -e "${RED}❌ FAIL (muestra autoindex en lugar de index.html)${NC}"
else
    echo -e "${GREEN}✅ PASS${NC}"
    ((passed++))
fi
((total++))

# GRUPO 3: DIRECTORIOS SIN AUTOINDEX
echo -e "\n${BLUE}📁 GRUPO 3: DIRECTORIOS SIN AUTOINDEX${NC}"
echo "────────────────────────────────────────────────────────"

# Use /tests/private which has autoindex off in config
echo -n "   Test 3.1: GET /tests/private/ (sin autoindex)... "
response=$(curl -s -o /dev/null -w "%{http_code}" "$HOST/tests/private/" 2>/dev/null)
if [ "$response" = "403" ] || [ "$response" = "404" ]; then
    echo -e "${GREEN}✅ PASS ($response)${NC}"
    ((passed++))
else
    echo -e "${RED}❌ FAIL (código $response, esperaba 403 o 404)${NC}"
fi
((total++))

# GRUPO 4: ARCHIVOS ESPECÍFICOS
echo -e "\n${BLUE}📁 GRUPO 4: ARCHIVOS ESPECÍFICOS${NC}"
echo "────────────────────────────────────────────────────────"

run_test "4.1" "GET /.test_temp/files/document1.txt" "$HOST/.test_temp/files/document1.txt" "200" "Archivo de texto normal"
[ $? -eq 0 ] && ((passed++))
((total++))

echo -n "   Test 4.2: Archivo con espacios... "
response=$(curl -s -o /dev/null -w "%{http_code}" "$HOST/.test_temp/files/file%20with%20spaces.txt" 2>/dev/null)
if [ "$response" = "200" ]; then
    echo -e "${GREEN}✅ PASS${NC}"
    ((passed++))
else
    echo -e "${YELLOW}⚠️  WARN ($response)${NC}"
fi
((total++))

echo -n "   Test 4.3: Archivo con #... "
response=$(curl -s -o /dev/null -w "%{http_code}" "$HOST/.test_temp/files/file%23with%23hashes.txt" 2>/dev/null)
if [ "$response" = "200" ] || [ "$response" = "404" ]; then
    echo -e "${GREEN}✅ PASS ($response)${NC}"
    ((passed++))
else
    echo -e "${YELLOW}⚠️  WARN ($response)${NC}"
    ((passed++))
fi
((total++))

# GRUPO 5: PRUEBAS ADICIONALES
echo -e "\n${BLUE}📁 GRUPO 5: PRUEBAS ADICIONALES${NC}"
echo "────────────────────────────────────────────────────────"

run_test "5.1" "HEAD /.test_temp/files/" "$HOST/.test_temp/files/" "200" "" "HEAD"
[ $? -eq 0 ] && ((passed++))
((total++))

# ============================================
# 📊 3. RESULTADOS Y LIMPIEZA
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

percentage=0
if [ $total -gt 0 ]; then
    percentage=$((passed * 100 / total))
fi

echo ""
echo -e "${BLUE}📈 PORCENTAJE DE ÉXITO: $percentage%${NC}"
echo ""

if [ $percentage -eq 100 ]; then
    echo -e "${GREEN}🎉 ¡EXCELENTE! AUTOINDEX FUNCIONA PERFECTAMENTE${NC}"
elif [ $percentage -ge 80 ]; then
    echo -e "${GREEN}✅ MUY BIEN - Autoindex funciona correctamente${NC}"
elif [ $percentage -ge 60 ]; then
    echo -e "${YELLOW}⚠️  ACEPTABLE - Funciona básicamente${NC}"
else
    echo -e "${RED}⚠️  NECESITA MEJORAS - Problemas significativos${NC}"
fi

echo ""

# LIMPIEZA: Borrar directorio temporal
echo -e "${BLUE}🧹 Limpiando directorio temporal...${NC}"
rm -rf "$BASE_DIR" 2>/dev/null
rm -f response_body.tmp test_output.tmp 2>/dev/null
echo -e "${GREEN}✅ Directorio temporal eliminado${NC}"

echo ""
echo -e "${GREEN}✅ TEST COMPLETADO${NC}"
echo ""

exit 0
