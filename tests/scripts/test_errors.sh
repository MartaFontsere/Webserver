#!/bin/bash

echo "--- TESTING CUSTOM ERROR PAGES ---"
echo "1. Requesting non-existent page..."
curl -s http://localhost:8080/non_existent_page_xyz | grep "Lo siento chati"
