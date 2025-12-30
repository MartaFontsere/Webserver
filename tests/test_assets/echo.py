#!/usr/bin/env python3
# Simple CGI test script - Echo POST data

import os
import sys

# CGI Header
print("Content-Type: text/html")
print()  # Empty line separates headers from body

# HTML output
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>CGI Python Test</title></head>")
print("<body>")
print("<h1>Python CGI Echo</h1>")

# Display method
method = os.environ.get('REQUEST_METHOD', 'UNKNOWN')
print(f"<p><strong>Method:</strong> {method}</p>")

# Display query string
query = os.environ.get('QUERY_STRING', '')
if query:
    print(f"<p><strong>Query String:</strong> {query}</p>")

# Display POST data if present
if method == 'POST':
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    if content_length and content_length != '0':
        body = sys.stdin.read(int(content_length))
        print(f"<p><strong>POST Data:</strong></p>")
        print(f"<pre>{body}</pre>")

# Display all environment variables
print("<hr>")
print("<h2>CGI Environment Variables:</h2>")
print("<ul>")
for key, value in sorted(os.environ.items()):
    if key.startswith('HTTP_') or key in ['REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH', 'SCRIPT_NAME', 'PATH_INFO']:
        print(f"<li><strong>{key}:</strong> {value}</li>")
print("</ul>")

print("</body>")
print("</html>")