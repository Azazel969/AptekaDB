#include "AuthService.h"
#include "Exceptions.h"
#include "config.h"
#include <iostream>
#include <string>

AuthService::AuthService(Database& db) : db(db) {}

std::optional<Models::User> AuthService::login(const std::wstring& login, const std::wstring& password) {
    std::wstring query = L"SELECT UserId, Login, FullName, RoleId, IsBlocked, FailedAttempts "
        L"FROM Users WHERE Login = ?";
    auto row = db.executeQuerySingle(query, { login });

    if (!row.has_value()) {
        throw AppException(L"Неверный логин или пароль.");
    }

    Models::User user;
    user.userId = std::get<int>((*row)[0]);
    user.login = std::get<std::wstring>((*row)[1]);
    user.fullName = std::get<std::wstring>((*row)[2]);
    user.role = static_cast<Models::UserRole>(std::get<int>((*row)[3]));
    user.isBlocked = std::get<int>((*row)[4]) != 0;
    user.failedAttempts = std::get<int>((*row)[5]);

    if (user.isBlocked) {
        throw AppException(L"Учётная запись заблокирована. Обратитесь к администратору.");
    }

    std::wstring passwordQuery = L"SELECT Password FROM Users WHERE UserId = ?";
    auto pwdRow = db.executeQuerySingle(passwordQuery, { user.userId });
    if (!pwdRow.has_value()) {
        throw AppException(L"Ошибка проверки пароля.");
    }

    std::wstring storedPassword = std::get<std::wstring>((*pwdRow)[0]);

    if (password == storedPassword) {
        db.executeNonQuery(L"UPDATE Users SET FailedAttempts = 0, LastLogin = GETDATE() WHERE UserId = ?",
            { user.userId });
        db.executeNonQuery(
            L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId, NewValues) "
            L"VALUES (?, 'LOGIN', 'Users', ?, ?)",
            { user.userId, user.userId, L"Успешный вход в систему" });
        return user;
    }
    else {
        int newAttempts = user.failedAttempts + 1;

        if (newAttempts >= Config::MAX_LOGIN_ATTEMPTS) {
            db.executeNonQuery(
                L"UPDATE Users SET FailedAttempts = ?, IsBlocked = 1 WHERE UserId = ?",
                { newAttempts, user.userId });
            db.executeNonQuery(
                L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId, NewValues) "
                L"VALUES (?, 'LOGIN_FAIL', 'Users', ?, ?)",
                { user.userId, user.userId, L"Превышено количество попыток входа. Учётная запись заблокирована." });
            throw AppException(L"Превышено количество попыток. Учётная запись заблокирована.");
        }
        else {
            db.executeNonQuery(L"UPDATE Users SET FailedAttempts = ? WHERE UserId = ?",
                { newAttempts, user.userId });
            db.executeNonQuery(
                L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId, NewValues) "
                L"VALUES (?, 'LOGIN_FAIL', 'Users', ?, ?)",
                { user.userId, user.userId, L"Неверный пароль. Попытка " + std::to_wstring(newAttempts) });
            throw AppException(L"Неверный логин или пароль.");
        }
    }
}

bool AuthService::registerUser(const std::wstring& login, const std::wstring& password,
    const std::wstring& fullName, Models::UserRole role) {
    try {
        std::wstring query = L"INSERT INTO Users (Login, Password, RoleId, FullName) VALUES (?, ?, ?, ?)";
        db.executeNonQuery(query, { login, password, static_cast<int>(role), fullName });
        return true;
    }
    catch (...) {
        return false;
    }
}