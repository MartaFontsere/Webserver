#!/usr/bin/python3
import os
import sys

# Simple session management demo
# It reads the 'session_id' cookie. If not present, it sets it.

cookies = os.environ.get("HTTP_COOKIE", "")
session_id = None

if cookies:
    for cookie in cookies.split(";"):
        cookie = cookie.strip()
        if "=" in cookie and cookie.startswith("session_id="):
            session_id = cookie.split("=")[1]

if not session_id:
    # Generate a dummy session ID
    session_id = "user123"
    print("Set-Cookie: session_id=user123; Path=/; HttpOnly")
    msg = "Hello! This is your first visit. I've set a session cookie for you."
else:
    msg = f"Welcome back! Your session ID is: <b>{session_id}</b>"

print("Content-Type: text/html\r\n\r\n")
print("<html><head><title>Python Session Demo</title></head><body>")
print("<h1>Hello from Python CGI!</h1>")
print("<h2>Session Management Demo</h2>")
print(f"<p>{msg}</p>")
print("<p>Check your browser cookies to see 'session_id'.</p>")
print("<p><a href='/cgi-bin/bonus_session_test.py'>Refresh this page</a></p>")
print("</body></html>")
