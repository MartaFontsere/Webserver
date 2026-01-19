#!/bin/bash
# Simple CGI test script - Display all environment variables

echo "Content-Type: text/plain"
echo ""
echo "=== CGI ENVIRONMENT VARIABLES ==="
echo ""
env | sort
echo ""
echo "=== END ==="