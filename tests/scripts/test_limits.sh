#!/bin/bash

echo "--- TESTING CLIENT BODY LIMITS ---"
echo "1. Small body (should succeed)..."
curl -s -o /dev/null -w "%{http_code}\n" -X POST -d "small" http://localhost:8080/

echo "2. Large body (should fail with 413)..."
# Creating a 200 byte string (limit is 100 in mega_test.conf)
LARGE_DATA=$(printf 'a%.0s' {1..200})
curl -s -o /dev/null -w "%{http_code}\n" -X POST -d "$LARGE_DATA" http://localhost:8080/
