#!/usr/bin/env python3
import time
import os

print("Content-Type: text/html")
print("")
print("<html><body>")
print("<h1>Slow CGI Response</h1>")
print("<p>I slept for 3 seconds.</p>")
time.sleep(3)
print("</body></html>")
