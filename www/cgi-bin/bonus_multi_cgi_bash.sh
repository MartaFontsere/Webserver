#!/bin/bash
echo "Content-Type: text/html"
echo ""

DATETIME=$(date '+%Y-%m-%d %H:%M:%S')

cat << 'EOF'
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <title>Bash CGI Demo</title>
    <style>
        body {
            font-family: 'Inter', sans-serif;
            background: linear-gradient(135deg, #1e3a2f 0%, #0f172a 100%);
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
            color: #4ade80;
            margin-bottom: 1rem;
        }
        .badge {
            display: inline-block;
            background: linear-gradient(135deg, #22c55e, #16a34a);
            color: white;
            padding: 0.5rem 1rem;
            border-radius: 2rem;
            font-size: 0.9rem;
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
            background: #22c55e;
            color: white;
            text-decoration: none;
            border-radius: 0.5rem;
            font-weight: bold;
            margin-top: 1rem;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="lang">🐚 Bash CGI</div>
        <div class="badge">✓ Extensión .sh detectada correctamente</div>
        
        <div class="info">
            <p><strong>Intérprete:</strong> /bin/bash</p>
EOF

echo "            <p><strong>Fecha/Hora:</strong> $DATETIME</p>"
echo "            <p><strong>Request Method:</strong> $REQUEST_METHOD</p>"
echo "            <p><strong>Script:</strong> $SCRIPT_NAME</p>"
echo "            <p><strong>Server:</strong> $SERVER_SOFTWARE</p>"

cat << 'EOF'
        </div>
        
        <p style="font-size: 0.85rem; color: #64748b;">
            Este CGI demuestra que el servidor detecta la extensión .sh y usa Bash.
        </p>
        
        <a href="/tests/index.html" class="btn">← Volver al Dashboard</a>
    </div>
</body>
</html>
EOF
