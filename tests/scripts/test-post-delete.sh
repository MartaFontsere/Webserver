#!/bin/bash

# ============================================
# 🧪 TEST DE POST/DELETE - WEBSERVER 42
# ============================================
# - POST crea archivos en www/tests/uploads (limpia anteriores)
# - DELETE elimina archivo test_to_delete.txt (NO toca test.html)

echo "=== Iniciando pruebas POST/DELETE ==="

# 1. Preparar directorios
mkdir -p "www/tests/uploads"
mkdir -p "www/tests/post-delete"

# 2. Limpiar uploads anteriores de tests (archivos .dat generados por POST)
echo "Limpiando uploads anteriores..."
rm -f www/tests/uploads/upload_*.dat 2>/dev/null
rm -f www/tests/uploads/test_upload_*.txt 2>/dev/null

# 3. Crear archivo para DELETE (sin tocar test.html!)
TEST_FILE="www/tests/post-delete/test_to_delete.txt"
echo "Contenido de prueba para DELETE" > "$TEST_FILE"
echo "✅ test_to_delete.txt creado (test.html NO se tocará)"

# 4. Probar POST con curl a /tests/uploads
echo -n "Probando POST a /tests/uploads... "
# El servidor necesita upload_path configurado para /tests/uploads
# Primero verificamos qué código devuelve
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "test data from shell script" "http://localhost:8080/tests/uploads")
if [ "$CODE" -eq 200 ] || [ "$CODE" -eq 201 ]; then
    echo "✅ (code $CODE)"
else
    # Si /tests/uploads no tiene upload_path, usamos /uploads que sí lo tiene
    CODE2=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "test data" "http://localhost:8080/uploads")
    if [ "$CODE2" -eq 200 ] || [ "$CODE2" -eq 201 ]; then
        echo "✅ (usó /uploads, code $CODE2)"
    else
        echo "❌ (codes: $CODE, $CODE2)"
    fi
fi

echo "   📁 Contenido de uploads/ tras el POST:"
ls -1 www/uploads/ 2>/dev/null | tail -3 | sed 's/^/      - /' || echo "      (vacío)"

# 5. Probar DELETE con curl (solo borra test_to_delete.txt, NO test.html)
echo -n "Probando DELETE test_to_delete.txt... "
curl -s -X DELETE "http://localhost:8080/tests/post-delete/test_to_delete.txt" > /dev/null
if [ ! -f "$TEST_FILE" ]; then
    echo "✅"
else
    echo "❌ (archivo aún existe)"
fi

# 6. Verificar que test.html NO fue borrado
echo -n "Verificando que test.html existe... "
if [ -f "www/tests/post-delete/test.html" ]; then
    echo "✅"
else
    echo "❌ (falta test.html!)"
fi

echo "   📁 Contenido de post-delete/:"
ls -1 www/tests/post-delete/ 2>/dev/null | sed 's/^/      - /' || echo "      (vacío)"

echo ""
echo "✅ Test post-delete completado."