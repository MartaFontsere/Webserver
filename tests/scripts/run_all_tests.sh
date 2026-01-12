#!/bin/bash

# Master script to run all tests
# Usage: ./tests/scripts/run_all_tests.sh

BASE_DIR=$(dirname "$0")

chmod +x "$BASE_DIR"/test_*.sh

echo "========================================"
echo "   RUNNING ALL WEBSERVER TESTS"
echo "========================================"
echo

"$BASE_DIR"/test_cgi.sh
echo
"$BASE_DIR"/test_non_blocking_cgi.sh
echo
"$BASE_DIR"/test_limits.sh
echo
"$BASE_DIR"/test_redirects.sh
echo
"$BASE_DIR"/test_errors.sh
echo
"$BASE_DIR"/test_vhosts.sh
echo
"$BASE_DIR"/test_alias.sh
echo
"$BASE_DIR"/test_ports.sh
echo
"$BASE_DIR"/test_chunked.sh
echo
echo "--- RUNNING LEGACY TESTS ---"
"$BASE_DIR"/test-autoindex.sh
echo
"$BASE_DIR"/test-post-delete.sh
echo
echo "--- RUNNING STRESS TEST ---"
python3 "$BASE_DIR"/number_clients_stress_test.py

echo
echo "========================================"
echo "   TESTS COMPLETED"
echo "========================================"
