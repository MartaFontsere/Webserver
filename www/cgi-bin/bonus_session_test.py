#!/usr/bin/python3
import os

# Simple session management demo
# It reads the 'session_id' cookie. If not present, it sets it.

cookies = os.environ.get("HTTP_COOKIE", "")
session_id = None
visit_count = 0

if cookies:
    for cookie in cookies.split(";"):
        cookie = cookie.strip()
        if "=" in cookie:
            key_val = cookie.split("=", 1)
            if key_val[0] == "session_id":
                session_id = key_val[1]
            elif key_val[0] == "visit_count":
                try:
                    visit_count = int(key_val[1])
                except:
                    visit_count = 0

# Increment visit count
visit_count += 1

# Set cookies
if not session_id:
    session_id = "user_" + str(hash(os.environ.get("REMOTE_ADDR", "unknown")) % 10000)
    print(f"Set-Cookie: session_id={session_id}; Path=/; HttpOnly")

print(f"Set-Cookie: visit_count={visit_count}; Path=/")

# Determine if first visit
is_first_visit = (visit_count == 1)

# Generate HTML
print("Content-Type: text/html\r\n\r\n")
print("""<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <title>Sesiones & Cookies Demo</title>
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
            max-width: 500px;
        }
        h1 { color: #38bdf8; font-size: 2rem; margin-bottom: 1rem; }
        .status {
            font-size: 1.5rem;
            padding: 1rem 2rem;
            border-radius: 0.5rem;
            margin: 1.5rem 0;
        }
        .first-visit {
            background: linear-gradient(135deg, #22c55e 0%, #16a34a 100%);
            color: white;
        }
        .returning {
            background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%);
            color: white;
        }
        .info {
            background: #1e293b;
            padding: 1rem;
            border-radius: 0.5rem;
            margin: 1rem 0;
            text-align: left;
        }
        .info p { margin: 0.5rem 0; color: #94a3b8; }
        .info strong { color: #f8fafc; }
        .count { font-size: 3rem; font-weight: bold; color: #38bdf8; }
        .btn {
            display: inline-block;
            padding: 0.75rem 1.5rem;
            background: #38bdf8;
            color: #0f172a;
            text-decoration: none;
            border-radius: 0.5rem;
            font-weight: bold;
            margin: 0.5rem;
        }
        .btn-outline {
            background: transparent;
            border: 1px solid #38bdf8;
            color: #38bdf8;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🍪 Sesiones & Cookies</h1>
""")

if is_first_visit:
    print("""
        <div class="status first-visit">
            ¡Primera visita!
        </div>
        <p>Se ha creado una cookie de sesión para ti.</p>
    """)
else:
    print(f"""
        <div class="status returning">
            ¡Bienvenido de nuevo!
        </div>
        <p>Esta es tu visita número:</p>
        <div class="count">{visit_count}</div>
    """)

print(f"""
        <div class="info">
            <p><strong>Session ID:</strong> {session_id}</p>
            <p><strong>Visitas:</strong> {visit_count}</p>
        </div>
        
        <p style="font-size: 0.85rem; color: #94a3b8;">
            Haz refresh para incrementar el contador.
        </p>
        
        <a href="/cgi-bin/bonus_session_test.py" class="btn">🔄 Refrescar</a>
        <a href="/tests/index.html" class="btn btn-outline">← Volver al Dashboard</a>
        
        <p style="font-size: 0.8rem; color: #64748b; margin-top: 1.5rem;">
            Para borrar las cookies y empezar de nuevo:<br>
            Abre las DevTools (F12) → Application → Cookies → click derecho → Clear.
        </p>
    </div>
</body>
</html>
""")
