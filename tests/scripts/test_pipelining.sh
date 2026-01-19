#!/bin/bash

# Test script for HTTP Pipelining
# Sends two requests in a single TCP connection and expects 2 responses

PORT=8080
if [ ! -z "$1" ]; then
    PORT=$1
fi

echo "Testing Pipelining on port $PORT..."

# Use Python for cross-platform socket handling (Mac nc has issues with -c)
RESPONSES=$(python3 -c "
import socket

request1 = 'GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n'
request2 = 'GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n'
pipelined = request1 + request2

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(5)
try:
    sock.connect(('localhost', $PORT))
    sock.sendall(pipelined.encode())
    
    response = b''
    while True:
        data = sock.recv(4096)
        if not data:
            break
        response += data
except Exception as e:
    pass
finally:
    sock.close()

count = response.count(b'HTTP/1.1 200 OK')
print(count)
" 2>/dev/null)

if [ "$RESPONSES" -eq 2 ]; then
    echo "✅ SUCCESS: Received $RESPONSES responses for 2 pipelined requests."
else
    echo "❌ FAILURE: Received $RESPONSES responses (expected 2)."
fi
