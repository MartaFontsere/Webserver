#!/usr/bin/python3
import os
from datetime import datetime

print("Content-Type: text/html")
print("")
print("""<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <title>Python CGI Demo</title>
    <style>
        body {
            font-family: 'Inter', sans-serif;
            background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%);
            color: #f8fafc;
            display: flex;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            margin: 0;
        }
        .container {
            text-align: center;
            padding: 3rem;
            background: rgba(255,255,255,0.05);
            border-radius: 1rem;
            border: 1px solid rgba(255,255,255,0.1);
            max-width: 600px;
        }
        .lang { 
            font-size: 3rem; 
            font-weight: bold;
            background: linear-gradient(90deg, #4a90c2, #6ab0de, #e8c547);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            margin-bottom: 1rem;
        }
        .badge {
            display: inline-block;
            background: linear-gradient(135deg, #4a90c2, #e8c547);
            color: #0f172a;
            padding: 0.5rem 1rem;
            border-radius: 2rem;
            font-size: 0.9rem;
            font-weight: bold;
            margin: 1rem 0;
        }
        .info {
            background: #1e293b;
            padding: 1rem;
            border-radius: 0.5rem;
            margin: 1rem 0;
            text-align: left;
            font-size: 0.85rem;
        }
        .info p { margin: 0.3rem 0; color: #94a3b8; }
        .info strong { color: #f8fafc; }
        .btn {
            display: inline-block;
            padding: 0.75rem 1.5rem;
            background: linear-gradient(135deg, #4a90c2, #e8c547);
            color: #0f172a;
            text-decoration: none;
            border-radius: 0.5rem;
            font-weight: bold;
            margin-top: 1rem;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="lang">🐍 Python CGI</div>
        <div class="badge">✓ Extensión .py detectada correctamente</div>
        
        <div class="info">
            <p><strong>Intérprete:</strong> /usr/bin/python3</p>""")

print(f"            <p><strong>Fecha/Hora:</strong> {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>")
print(f"            <p><strong>Request Method:</strong> {os.environ.get('REQUEST_METHOD', 'N/A')}</p>")
print(f"            <p><strong>Script:</strong> {os.environ.get('SCRIPT_NAME', 'N/A')}</p>")
print(f"            <p><strong>Server:</strong> {os.environ.get('SERVER_SOFTWARE', 'N/A')}</p>")

print("""        </div>
        
        <p style="font-size: 0.85rem; color: #64748b;">
            Este CGI demuestra que el servidor detecta la extensión .py y usa Python.
        </p>
        
        <a href="/tests/index.html" class="btn">← Volver al Dashboard</a>
    </div>
</body>
</html>
""")
