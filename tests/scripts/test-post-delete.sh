#!/bin/bash

echo "=== Iniciando pruebas del servidor ==="

# 1. Limpiar y preparar directorio de prueba
echo "Limpiando archivos de pruebas anteriores..."
rm -rf www/tests/post-delete 2>/dev/null
rm -rf www/tests/uploads 2>/dev/null
mkdir -p www/tests/post-delete
mkdir -p www/tests/uploads

echo "Contenido de prueba" > www/tests/post-delete/test.txt
echo "✅ test.txt creado"

# 2. Probar POST con curl
echo -n "Probando POST... "
curl -s -X POST -d "Datos de prueba POST" http://localhost:8080/tests/uploads | grep -q "Upload successful" && echo "✅" || echo "❌"

echo "   📁 Contenido de uploads/ tras el POST:"
ls -1 www/tests/uploads/ | sed 's/^/      - /'

# 3. Probar DELETE con curl
echo -n "Probando DELETE... "
curl -s -X DELETE http://localhost:8080/tests/post-delete/test.txt
if [ ! -f "www/tests/post-delete/test.txt" ]; then
    echo "✅"
else
    echo "❌"
fi

echo "   📁 Contenido de post-delete/ tras el DELETE:"
ls -1 www/tests/post-delete/ | sed 's/^/      - /' || echo "      (vacío)"

# 4. Fin
echo ""
echo "✅ Test post-delete completado."