#!/bin/bash

echo "--- TESTING ALIAS ---"
echo "1. Testing /test-alias/ (should serve form.html from tests/test_assets)..."
curl -s http://localhost:8080/test-alias/ | grep -q "CGI Form" && echo "Success: Alias working" || echo "Fail: Alias not working"
