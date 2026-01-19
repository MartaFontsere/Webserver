#!/bin/bash

# Test for chunked transfer-encoding
# This is a general HTTP feature test, not CGI-specific

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "--- 🧪 TEST CHUNKED TRANSFER-ENCODING ---"

# Test: Send chunked POST request and verify body is correctly un-chunked
echo -n "1. Chunked POST (HelloWorld!)... "
python3 -c "
import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8080))
req = 'POST /cgi-bin/test.py HTTP/1.1\r\nHost: localhost:8080\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n6\r\nWorld!\r\n0\r\n\r\n'
sock.sendall(req.encode())
sock.settimeout(3.0)
data = sock.recv(4096).decode()
sock.close()
exit(0 if 'HelloWorld!' in data else 1)
" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"

# Test 2: Single chunk
echo -n "2. Single chunk (Test123)... "
python3 -c "
import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8080))
req = 'POST /cgi-bin/test.py HTTP/1.1\r\nHost: localhost:8080\r\nTransfer-Encoding: chunked\r\n\r\n7\r\nTest123\r\n0\r\n\r\n'
sock.sendall(req.encode())
sock.settimeout(3.0)
data = sock.recv(4096).decode()
sock.close()
exit(0 if 'Test123' in data else 1)
" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"

# Test 3: Empty body (just 0\r\n\r\n)
echo -n "3. Empty chunked body... "
python3 -c "
import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8080))
req = 'POST /cgi-bin/test.py HTTP/1.1\r\nHost: localhost:8080\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n'
sock.sendall(req.encode())
sock.settimeout(3.0)
data = sock.recv(4096).decode()
sock.close()
exit(0 if '200 OK' in data else 1)
" && echo -e "${GREEN}✅ OK${NC}" || echo -e "${RED}❌ FAIL${NC}"
