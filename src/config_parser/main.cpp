#include <iostream>
#include <fstream>
#include <string>
#include <vector>

std::string trimLine(const std::string &line)
{
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return "";
    }
    size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

bool isEmptyOrComment(const std::string &trimmedLine)
{
    return trimmedLine.empty() || trimmedLine[0] == '#';
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t pos = str.find(delimiter, start);
    int cont = 0;

    while (pos != std::string::npos)
    {
        tokens.push_back(str.substr(start, pos - start));
        std::cout << "Token [" << cont++ << "] : " << str.substr(start, pos - start) << std::endl;
        start = pos + 1;
        pos = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));
    std::cout << "Token [" << cont << "] : " << str.substr(start) << std::endl;
    return tokens;
}

bool readConfigFile(const std::string &filePath)
{
    std::cout << "Intentando leer archivo: " << filePath << std::endl;

    std::ifstream file;
    file.open(filePath.c_str());

    if (!file.is_open())
    {
        std::cout << "❌ No se pudo abrir el archivo" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::string trimmed = trimLine(line);
        if (isEmptyOrComment(trimmed))
        {
            continue;
        }
        // Ahora clasificar la línea
        if (trimmed[trimmed.length() - 1] == '{')
        {
            std::cout << "🟦 BLOQUE INICIO: " << trimmed << std::endl;
        }
        else if (trimmed == "}")
        {
            std::cout << "🟦 BLOQUE FIN: " << trimmed << std::endl;
        }
        else if (trimmed[trimmed.length() - 1] == ';')
        {
            std::cout << "📝 DIRECTIVA: " << trimmed << std::endl;
            trimmed = trimmed.substr(0, trimmed.length() - 1);
            std::cout << "/*/*/*/SPLITEADO*/*/*/*/*/ " << trimmed << std::endl;
            std::vector<std::string> tokens = split(trimmed, ' ');
        }
        else
        {
            std::cout << "❓ DESCONOCIDO: " << trimmed << std::endl;
        }
    }

    return true;
}

int main()
{
    std::cout << "=== TESTING CONFIG PARSER ===" << std::endl;

    // Por ahora solo probamos con un archivo de ejemplo
    bool result = readConfigFile("test.conf");

    if (result)
    {
        std::cout << "✅ Archivo leído correctamente" << std::endl;
    }
    else
    {
        std::cout << "❌ Error al leer archivo" << std::endl;
    }

    return 0;
}
