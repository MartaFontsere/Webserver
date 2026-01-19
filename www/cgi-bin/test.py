#!/usr/bin/python3
import os
import sys

print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>Hello from Python CGI!</h1>")
print("<ul>")
for k, v in os.environ.items():
    print(f"<li>{k}: {v}</li>")
print("</ul>")
if os.environ.get("REQUEST_METHOD") == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    if content_length > 0:
        body = sys.stdin.read(content_length)
        print(f"<h2>Body:</h2><p>{body}</p>")
print("</body></html>")
