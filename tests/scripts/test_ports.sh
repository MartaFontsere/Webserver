#!/bin/bash

echo "--- TESTING MULTIPLE PORTS ---"

# 1. Probar puerto 8080
echo -n "1. Probando puerto 8080... "
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/ | grep -q "200" && echo "✅ OK" || echo "❌ FAIL"

# 2. Probar puerto 9090 (configurado en exhaustive_test.conf)
echo -n "2. Probando puerto 9090... "
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:9090/ | grep -q "200" && echo "✅ OK" || echo "❌ FAIL"

# 3. Probar puerto inexistente
echo -n "3. Probando puerto 7070 (no debería responder)... "
curl -s --connect-timeout 2 http://localhost:7070/ > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✅ OK (Conexión rechazada/timeout)"
else
    echo "❌ FAIL (¡Algo respondió en el 7070!)"
fi
