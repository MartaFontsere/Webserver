#!/bin/bash

# Comprehensive Parser Robustness Test
# This script tests the HTTP parser with various malformed and edge-case requests.

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "========================================"
echo "   PARSER ROBUSTNESS TESTS"
echo "========================================"

# 1. Missing Host header in HTTP/1.1
echo -n "1. Missing Host header (HTTP/1.1)... "
RES=$(curl -s -v --http1.1 -H "Host:" http://localhost:8080/ 2>&1 | grep "HTTP/1.1 400")
if [[ "$RES" == *"400"* ]]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL (Response: $RES)${NC}"
fi

# 2. Malformed Request Line (Too many parts)
echo -n "2. Malformed Request Line (Too many parts)... "
RES=$( (echo -ne "GET / HTTP/1.1 extra\r\nHost: localhost\r\n\r\n"; sleep 0.5) | nc -w 1 localhost 8080 | head -n 1)
if [[ "$RES" == *"400"* ]]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL (Response: $RES)${NC}"
fi

# 3. Malformed Request Line (Too few parts)
echo -n "3. Malformed Request Line (Too few parts)... "
RES=$( (echo -ne "GET /\r\nHost: localhost\r\n\r\n"; sleep 0.5) | nc -w 1 localhost 8080 | head -n 1)
if [[ "$RES" == *"400"* ]]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL (Response: $RES)${NC}"
fi

# 4. URL Decoding in Path
echo -n "4. URL Decoding in Path (%20)... "
echo "test" > "www/space test.txt"
RES=$(curl -s -o /dev/null -w "%{http_code}" "http://localhost:8080/space%20test.txt")
if [ "$RES" == "200" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL (Code: $RES)${NC}"
fi
rm "www/space test.txt"

# 5. Case-insensitive Headers
echo -n "5. Case-insensitive Headers (hOsT)... "
RES=$(curl -s -o /dev/null -w "%{http_code}" -H "hOsT: localhost" "http://localhost:8080/")
if [ "$RES" == "200" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL (Code: $RES)${NC}"
fi

# 6. Large Header Line (Buffer limit check)
echo -n "6. Very long header line... "
LONG_HEADER=$(printf 'a%.0s' {1..2000})
RES=$(curl -s -o /dev/null -w "%{http_code}" -H "X-Long: $LONG_HEADER" "http://localhost:8080/")
if [ "$RES" == "200" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL (Code: $RES)${NC}"
fi

# 7. Multiple slashes in path
echo -n "7. Multiple slashes in path (///)... "
RES=$(curl -s -o /dev/null -w "%{http_code}" "http://localhost:8080////index.html")
if [ "$RES" == "200" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL (Code: $RES)${NC}"
fi

echo "========================================"
echo "   TESTS COMPLETED"
echo "========================================"
