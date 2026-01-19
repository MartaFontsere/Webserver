#!/usr/bin/python3
import time
import sys

# Slow CGI for non-blocking test (3s delay)
time.sleep(3)
print("Content-Type: text/plain\r\n\r\n")
print("Slow CGI Finished")
