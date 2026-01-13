#!/bin/bash
echo "Content-Type: text/html"
echo ""
echo "<html><head><title>Bash CGI</title></head><body>"
echo "<h1>Hello from Bash CGI!</h1>"
echo "<p>This demonstrates that the server can handle multiple CGI types (Python, Bash, PHP, etc.)</p>"
echo "<h3>Environment Variables:</h3>"
echo "<ul>"
env | sort | while read line; do
    echo "<li>$line</li>"
done
echo "</ul>"
echo "</body></html>"
