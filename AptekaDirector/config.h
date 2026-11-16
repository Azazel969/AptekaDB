// ============================================================
// config.h — параметры подключения и общие константы
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#pragma once
#include <string>

namespace Config {
    // Строка подключения к SQL Server 2022 Express
    // Используется аутентификация Windows (Trusted_Connection)
    const std::wstring CONNECTION_STRING =
        L"Provider=SQLOLEDB;"
        L"Data Source=.\\SQLEXPRESS;"
        L"Initial Catalog=AptekaDB;"
        L"Integrated Security=SSPI;"
        L"Encrypt=YES;"
        L"TrustServerCertificate=NO;"
        L"Connection Timeout=30;";

    // Настройки безопасности
    const int MAX_LOGIN_ATTEMPTS = 3;
    const int BLOCK_DURATION_MINUTES = 15;
    const int SALT_LENGTH = 16;

    // Настройки отчётов
    const int DEFAULT_EXPIRY_WARNING_DAYS = 30;
}