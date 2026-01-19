#!/usr/bin/env python3
"""
Test script for chdir() functionality.
This script reads a file using a RELATIVE path.
If chdir() works correctly, it will find 'chdir_test_data.txt' in the same directory.
If chdir() is NOT working, it will fail because it looks in the wrong directory.
"""
import os

print("Content-Type: text/plain\r")
print("\r")

try:
    # This is a RELATIVE path - depends on CWD being set correctly
    with open("chdir_test_data.txt", "r") as f:
        content = f.read().strip()
    print("SUCCESS: chdir() is working!")
    print("File content: " + content)
    print("CWD: " + os.getcwd())
except FileNotFoundError:
    print("FAILURE: chdir() is NOT working!")
    print("Could not find 'chdir_test_data.txt' using relative path")
    print("CWD: " + os.getcwd())
except Exception as e:
    print("ERROR: " + str(e))
