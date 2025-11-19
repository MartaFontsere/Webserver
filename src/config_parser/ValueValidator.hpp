#include "../../includes/config_parser/ValueValidator.hpp"
#include <cctype>
#include <cstdlib>
#include <string>
#include <sstream>

bool isValidNumber(const std::string &value)
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!isdigit(value[i]))
            return false;
    }
    return true;
}

bool isValidPort(const std::string &value)
{
    if (!isValidNumber(value))
        return false;
    int port = atoi(value.c_str());
    if (port < 1 || port > 65535)
        return false;
    return true;
}

bool isValidPath(const std::string &value)
{
    if (value[0] != '/' && value[0] != '.')
        return false;
    return true;
}

bool isValidHost(const std::string &value)
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        char c = value[i];
        if (!isalnum(c) && c != '.' && c != '-' && c != '*' &&
            c != '_' && c != ':' && c != '~')
            return false;
    }

    return true;
}

bool isValidIP(const std::string &value)
{
    int dotCount = 0;
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '.')
            dotCount++;
    }
    if (dotCount != 3)
        return false;

    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!isdigit(value[i]) && value[i] != '.')
            return false;
    }
    std::string oct;
    int octetCount = 0;

    for (size_t i = 0; i < value.size(); ++i)
    {

        if (value[i] == '.')
        {
            // TODO:
            // - Convertir oct a número
            // - Verificar rango 0-255
            // - Incrementar octetCount
            // - Limpiar oct
        }
        else
        {
            oct += value[i]; // Acumular dígito
        }
    }

    // TODO: Procesar el ÚLTIMO octeto (no termina en punto)

    // TODO: Verificar que octetCount == 4

    return true;
}

bool isValidHttpCode(const std::string &value)
{
    if (!isValidNumber(value))
        return false;
    int code = atoi(value.c_str());
    if (code < 100 || code > 599)
        return false;
    return true;
}

bool isValidBool(const std::string &value)
{
    if (value == "on" || value == "off")
        return true;
    return false;
}

bool isValidPattern(const std::string &value)
{
    // TODO: Implementar
    (void)value;
    return false;
}