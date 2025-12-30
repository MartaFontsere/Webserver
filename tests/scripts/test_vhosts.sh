#!/bin/bash

echo "--- TESTING VIRTUAL HOSTS ---"
echo "1. Default host (localhost)..."
curl -s http://localhost:8080/ | grep -q "Hello from Python CGI" && echo "Fail: Got CGI instead of index" || echo "Success: Got default index"

echo "2. Virtual host (marta.com)..."
curl -s -H "Host: marta.com" http://localhost:8080/ | grep -q "Hello from Python CGI" && echo "Success: Got marta.com content" || echo "Fail: Got default index"
