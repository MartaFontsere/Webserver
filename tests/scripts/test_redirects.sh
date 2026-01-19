#!/bin/bash

echo "--- TESTING REDIRECTS ---"
echo "1. Testing /google redirect..."
curl -s -I http://localhost:8080/google | grep -E "HTTP/1.1 301|Location"
