#!/usr/bin/env python3

"""
🚀 Mini CGI Test Script
Script esencial para testing básico de CGI
"""

import os

# Headers HTTP obligatorios
print("Content-Type: text/plain; charset=utf-8")
print("")  # Línea vacía obligatoria

# Info básica
print("=== MINI CGI TEST ===")
print("✅ Script ejecutado correctamente")
print()

# Datos del request
method = os.environ.get('REQUEST_METHOD', 'UNKNOWN')
query = os.environ.get('QUERY_STRING', '')
server = os.environ.get('SERVER_NAME', 'localhost')

print(f"Método: {method}")
print(f"Query: {query if query else '(vacío)'}")
print(f"Servidor: {server}")
print()

# Si hay parámetros, mostrarlos
if query:
    print("Parámetros:")
    for param in query.split('&'):
        if '=' in param:
            key, value = param.split('=', 1)
            print(f"  {key} = {value}")
        else:
            print(f"  {param}")
    print()

print("=== FIN TEST ===")
print("🎯 CGI funciona perfectamente!")
