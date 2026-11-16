// ============================================================
// config.h — параметры подключения и общие константы
// ============================================================
#pragma once
#include <string>

namespace Config {
    // Строка подключения ODBC к SQL Server 2022 Express
    const std::wstring CONNECTION_STRING =
        L"Driver={ODBC Driver 17 for SQL Server};"
        L"Server=.\\SQLEXPRESS;"
        L"Database=AptekaDB;"
        L"Trusted_Connection=yes;"
        L"Encrypt=yes;"
        L"TrustServerCertificate=yes;";

    // Настройки безопасности
    const int MAX_LOGIN_ATTEMPTS = 3;
    const int BLOCK_DURATION_MINUTES = 15;

    // Настройки отчётов
    const int DEFAULT_EXPIRY_WARNING_DAYS = 30;
}