#!/bin/bash

echo "=== Iniciando pruebas del servidor ==="

# 1. Crear archivo de prueba
echo "Contenido de prueba" > www/test.txt
echo "✅ test.txt creado"

# 2. Probar POST con curl
echo -n "Probando POST... "
curl -s -X POST -d "Datos de prueba POST" http://localhost:8080/upload | grep -q "Upload successful" && echo "✅" || echo "❌"

# 3. Probar DELETE con curl
echo -n "Probando DELETE... "
curl -s -X DELETE http://localhost:8080/test.txt
if [ ! -f "www/test.txt" ]; then
    echo "✅"
else
    echo "❌"
fi

# 4. Verificar archivos creados
echo "=== Archivos en uploads/: ==="
ls -la www/uploads/ 2>/dev/null || echo "No hay uploads/"

# 5. Limpiar
rm -f www/test.txt
echo "✅ Test completado"