#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "--- Running Non-Blocking CGI Test ---"

# Check if server is running on 8080
if ! lsof -i :8080 > /dev/null; then
    echo -e "${RED}Error: Server is not running on port 8080. Skipping test.${NC}"
    exit 1
fi

# 1. Start the slow CGI in the background
echo "Requesting slow CGI (3s delay) in background..."
curl -s "http://localhost:8080/cgi-bin/slow.py" > /tmp/slow_out &
SLOW_PID=$!

# 2. Immediately request a static file
echo "Requesting static file immediately..."
START_TIME=$(date +%s)
curl -s "http://localhost:8080/index.html" > /tmp/fast_out
END_TIME=$(date +%s)

DIFF=$((END_TIME - START_TIME))

if [ $DIFF -lt 2 ]; then
    echo -e "${GREEN}SUCCESS: Static file was served in ${DIFF}s while CGI was still running.${NC}"
else
    echo -e "${RED}FAILURE: Static file took ${DIFF}s (it might have been blocked by CGI).${NC}"
    rm /tmp/slow_out /tmp/fast_out
    exit 1
fi

# 3. Wait for slow CGI to finish
echo "Waiting for slow CGI to finish..."
wait $SLOW_PID
echo -e "${GREEN}Slow CGI finished.${NC}"

# Cleanup
rm /tmp/slow_out /tmp/fast_out
echo "Non-Blocking CGI Test complete."
