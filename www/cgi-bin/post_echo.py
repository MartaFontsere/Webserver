#!/usr/bin/python3
import sys
import os
import urllib.parse

# Script para procesar datos POST y mostrarlos de forma elegante
print("Content-Type: text/html; charset=utf-8\r\n\r\n")

print("<!DOCTYPE html>")
print("<html><head><meta charset='UTF-8'><title>CGI POST Result</title>")
print("<style>body{font-family:sans-serif; background:#f1f5f9; padding:2rem;}")
print(".card{background:white; padding:2rem; border-radius:1rem; box-shadow:0 4px 6px -1px rgb(0 0 0 / 0.1); max-width:600px; margin:auto;}")
print("h1{color:#1e293b; border-bottom:2px solid #38bdf8; padding-bottom:0.5rem;}")
print("pre{background:#f8fafc; padding:1rem; border-radius:0.5rem; border:1px solid #e2e8f0; overflow-x:auto;}")
print("</style></head><body>")

print("<div class='card'>")
print("<h1>✅ Datos Recibidos por POST</h1>")

# Leer el cuerpo del POST
try:
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    post_data = sys.stdin.read(content_length)
    
    if post_data:
        print("<p>El servidor ha recibido la siguiente información:</p>")
        print("<pre>")
        # Intentar parsear si es application/x-www-form-urlencoded
        parsed = urllib.parse.parse_qs(post_data)
        if parsed:
            for key, values in parsed.items():
                print(f"<b>{key}</b>: {', '.join(values)}")
        else:
            print(post_data)
        print("</pre>")
    else:
        print("<p style='color:#ef4444;'>No se recibieron datos en el cuerpo del POST.</p>")
except Exception as e:
    print(f"<p style='color:#ef4444;'>Error procesando datos: {str(e)}</p>")

print("<br><a href='/tests/index.html'>← Volver al Dashboard</a>")
print("</div></body></html>")
