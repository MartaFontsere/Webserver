#!/bin/bash

# Colores para mejor visualización
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "--- 🧪 TEST UNIFICADO DE CGI ---"

# 1. GET básico con parámetros (Python)
echo -n "1. GET con parámetros (Python)... "
curl -s "http://localhost:8080/cgi-bin/test.py?user=Marta&status=testing" | grep -q "user=Marta" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"

# 2. POST básico con datos (Python)
echo -n "2. POST con datos (Python)... "
curl -s -X POST -d "message=HelloCGI" http://localhost:8080/cgi-bin/test.py | grep -q "message=HelloCGI" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"

# 3. CGI con otro lenguaje (Bash Script .sh)
echo -n "3. Ejecución de script Bash (.sh)... "
curl -s http://localhost:8080/cgi-bin/hello.sh | grep -q "Hola desde Bash!" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"

# 4. Manejo de Errores (Script que falla)
echo -n "4. Script con error (debe dar 500)... "
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/cgi-bin/error.py | grep -q "500" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"

# 5. Headers personalizados
echo -n "5. Headers personalizados desde CGI... "
curl -s -I http://localhost:8080/cgi-bin/header.py | grep -q "X-Custom-Header: MyValue" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"

# 6. Script inexistente
echo -n "6. Script inexistente (debe dar 404)... "
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/cgi-bin/no_existe.py | grep -q "404" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"
