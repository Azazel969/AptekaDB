// ============================================================
// AuthService.h — сервис аутентификации пользователей
// ============================================================
#pragma once
#include "Database.h"
#include "User.h"
#include <optional>

class AuthService {
public:
    explicit AuthService(Database& db);

    std::optional<Models::User> login(const std::wstring& login, const std::wstring& password);
    bool registerUser(const std::wstring& login, const std::wstring& password,
        const std::wstring& fullName, Models::UserRole role);

private:
    Database& db;
};