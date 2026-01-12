#!/bin/bash

# Test script for HTTP Pipelining
# Sends two requests in a single TCP packet/buffer

PORT=8080
if [ ! -z "$1" ]; then
    PORT=$1
fi

echo "Testing Pipelining on port $PORT..."

# Create a file with two concatenated requests
printf "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\nGET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n" > pipelining_req.txt

# Send them using netcat
cat pipelining_req.txt | nc -c localhost $PORT > pipelining_res.txt

# Count how many responses we got (should be 2)
RESPONSES=$(grep -c "HTTP/1.1 200 OK" pipelining_res.txt)

if [ "$RESPONSES" -eq 2 ]; then
    echo "✅ SUCCESS: Received $RESPONSES responses for 2 pipelined requests."
else
    echo "❌ FAILURE: Received $RESPONSES responses (expected 2)."
    echo "Check pipelining_res.txt for details."
fi

rm pipelining_req.txt pipelining_res.txt
