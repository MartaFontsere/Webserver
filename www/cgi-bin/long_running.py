#!/usr/bin/python3
import time
import sys

# Long running CGI
time.sleep(10)
print("Content-Type: text/plain\r\n\r\n")
print("CGI Finished")
