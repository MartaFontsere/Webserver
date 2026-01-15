#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "========================================"
echo "   BONUS: MULTIPLE CGI TYPES (POLYGLOT)"
echo "========================================"

# 1. Test Multiple CGI Types (Bash)
echo -n "-> Requesting .sh script (Interpreted by Bash)... "
RESPONSE=$(curl -s http://localhost:8080/cgi-bin/bonus_multi_cgi_bash.sh)
if echo "$RESPONSE" | grep -q "Hello from Bash CGI!"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

# 2. Test Multiple CGI Types (Python)
echo -n "-> Requesting .py script (Interpreted by Python)... "
RESPONSE=$(curl -s http://localhost:8080/cgi-bin/bonus_multi_cgi_python.py)
if echo "$RESPONSE" | grep -q "Hello from Python CGI!"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

echo ""
echo "========================================"
echo "   BONUS: COOKIES & SESSIONS"
echo "========================================"

# 3. Test Cookie Setting
echo -n "-> Testing Cookie Setting (Set-Cookie)... "
COOKIE=$(curl -s -I http://localhost:8080/cgi-bin/bonus_session_test.py | grep "Set-Cookie")
if echo "$COOKIE" | grep -q "session_id="; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

# 4. Test Session Persistence
echo -n "-> Testing Session Persistence (Cookie header)... "
RESPONSE=$(curl -s -H "Cookie: session_id=user123; visit_count=5" http://localhost:8080/cgi-bin/bonus_session_test.py)
if echo "$RESPONSE" | grep -q "Bienvenido de nuevo"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

echo "========================================"
echo "   VERIFICATION COMPLETED"
echo "========================================"
