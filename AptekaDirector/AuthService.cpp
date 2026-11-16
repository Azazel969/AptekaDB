#include "AuthService.h"

#include <stdexcept>
#include "config.h"

AuthService::AuthService(Database& db) : db(db) {}

std::optional<Models::User> AuthService::login(const std::wstring& login, const std::wstring& password) {
    std::wstring query = L"SELECT UserId, Login, FullName, RoleId, Password, IsBlocked, FailedAttempts FROM Users WHERE Login = ?";
    auto row = db.executeQuerySingle(query, { login });

    if (!row.has_value()) return std::nullopt;

    int userId = std::get<int>((*row)[0]);
    bool isBlocked = std::get<int>((*row)[5]) != 0;
    int failedAttempts = std::get<int>((*row)[6]);
    std::wstring storedPassword = std::get<std::wstring>((*row)[4]);

    if (isBlocked) {
        throw std::runtime_error("Учетная запись заблокирована.");
    }

    if (storedPassword == password) {
        db.executeNonQuery(L"UPDATE Users SET FailedAttempts = 0, LastLogin = GETDATE() WHERE UserId = ?", { userId });
        db.executeNonQuery(L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId) VALUES (?, 'LOGIN_SUCCESS', 'Users', ?)", { userId, userId });

        Models::User user;
        user.userId = userId;
        user.login = std::get<std::wstring>((*row)[1]);
        user.fullName = std::get<std::wstring>((*row)[2]);
        user.role = static_cast<Models::UserRole>(std::get<int>((*row)[3]));
        user.isBlocked = false;
        user.failedAttempts = 0;
        return user;
    }
    else {
        failedAttempts++;
        bool block = (failedAttempts >= Config::MAX_LOGIN_ATTEMPTS);
        db.executeNonQuery(L"UPDATE Users SET FailedAttempts = ?, IsBlocked = ? WHERE UserId = ?", { failedAttempts, block ? 1 : 0, userId });
        db.executeNonQuery(L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId) VALUES (?, 'LOGIN_FAILED', 'Users', ?)", { userId, userId });

        if (block) throw std::runtime_error("Превышено количество попыток. Учетная запись заблокирована.");
        return std::nullopt;
    }
}

bool AuthService::registerUser(const std::wstring& login, const std::wstring& password,
    const std::wstring& fullName, Models::UserRole role) {
    try {
        std::wstring query = L"INSERT INTO Users (Login, Password, RoleId, FullName, IsBlocked, FailedAttempts) VALUES (?, ?, ?, ?, 0, 0)";
        db.executeNonQuery(query, { login, password, static_cast<int>(role), fullName });
        return true;
    }
    catch (...) {
        return false;
    }
}