// ============================================================
// AuthService.h — сервис аутентификации пользователей
// Простая проверка пароля (без хэширования)
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <iostream>

#include "Database.h"
#include "User.h"
#include "config.h"

namespace Services {

    class AuthService {
    public:
        explicit AuthService(Db::Database& db) : m_db(db) {}

        // Попытка входа в систему
        std::optional<Models::User> login(
            const std::wstring& login,
            const std::wstring& password)
        {
            std::wstring sql =
                L"SELECT UserId, Login, Password, RoleId, "
                L"FullName, IsBlocked, FailedAttempts "
                L"FROM Users WHERE Login = ?";

            std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> params;
            params.emplace_back(L"", adVarWChar, _variant_t(login.c_str()));

            auto rs = m_db.executeParameterized(sql, params);

            if (rs->EndOfFile) {
                return std::nullopt;
            }

            bool isBlocked = rs->Fields->GetItem("IsBlocked")->Value.boolVal;
            if (isBlocked) {
                return std::nullopt;
            }

            Models::User user;
            user.userId = rs->Fields->GetItem("UserId")->Value.intVal;
            user.login = (wchar_t*)(_bstr_t)rs->Fields->GetItem("Login")->Value;
            user.fullName = (wchar_t*)(_bstr_t)rs->Fields->GetItem("FullName")->Value;
            user.role = static_cast<Models::UserRole>(
                rs->Fields->GetItem("RoleId")->Value.intVal);
            user.isBlocked = isBlocked;
            user.failedAttempts = rs->Fields->GetItem("FailedAttempts")->Value.intVal;

            std::wstring storedPassword = (wchar_t*)(_bstr_t)
                rs->Fields->GetItem("Password")->Value;

            if (password == storedPassword) {
                std::wstring updateSql =
                    L"UPDATE Users SET FailedAttempts = 0, "
                    L"LastLogin = GETDATE() WHERE UserId = ?";

                std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> updParams;
                updParams.emplace_back(L"", adInteger, _variant_t(user.userId));
                m_db.executeParameterized(updateSql, updParams);

                logAudit(user.userId, L"LOGIN", L"Users", user.userId,
                    L"", L"Успешный вход");
                return user;
            } 
            else {
                int newAttempts = user.failedAttempts + 1;
                bool shouldBlock = (newAttempts >= Config::MAX_LOGIN_ATTEMPTS);

                std::wstring updateSql =
                    L"UPDATE Users SET FailedAttempts = ?, "
                    L"IsBlocked = ? WHERE UserId = ?";

                std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> updParams;
                updParams.emplace_back(L"", adInteger, _variant_t(newAttempts));
                updParams.emplace_back(L"", adBoolean, _variant_t(shouldBlock));
                updParams.emplace_back(L"", adInteger, _variant_t(user.userId));
                m_db.executeParameterized(updateSql, updParams);

                logAudit(user.userId, L"LOGIN_FAILED", L"Users",
                    user.userId, L"", L"Неверный пароль");
                return std::nullopt;
            }
        }

        // Регистрация нового пользователя
        bool registerUser(
            const std::wstring& login,
            const std::wstring& password,
            const std::wstring& fullName,
            Models::UserRole role)
        {
            // 1. Проверка уникальности логина
            std::wstring checkSql = L"SELECT COUNT(*) AS UserCount FROM Users WHERE Login = ?";
            std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> checkParams;
            checkParams.emplace_back(L"", adVarWChar, _variant_t(login.c_str()));

            auto checkRs = m_db.executeParameterized(checkSql, checkParams);
            int userCount = checkRs->Fields->GetItem("UserCount")->Value.intVal;

            if (userCount > 0) {
                return false;
            }

            // 2. Вставка нового пользователя
            std::wstring insertSql =
                L"INSERT INTO Users (Login, Password, RoleId, FullName, IsBlocked, FailedAttempts) "
                L"VALUES (?, ?, ?, ?, 0, 0)";

            std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> insertParams;
            insertParams.emplace_back(L"", adVarWChar, _variant_t(login.c_str()));        
            insertParams.emplace_back(L"", adVarWChar, _variant_t(password.c_str()));     
            insertParams.emplace_back(L"", adInteger, _variant_t(static_cast<int>(role))); 
            insertParams.emplace_back(L"", adVarWChar, _variant_t(fullName.c_str()));     

            try {
                m_db.executeParameterized(insertSql, insertParams);

                // Получение ID нового пользователя
                std::wstring getUserIdSql = L"SELECT UserId FROM Users WHERE Login = ?";
                auto userIdRs = m_db.executeParameterized(getUserIdSql, checkParams);
                int newUserId = userIdRs->Fields->GetItem("UserId")->Value.intVal;

                // Фиксация в журнале аудита
                logAudit(newUserId, L"REGISTER", L"Users", newUserId, L"", L"Регистрация: " + fullName);

                return true;
            }
            catch (const Db::DatabaseException& e) {
                std::wcerr << L"Ошибка регистрации: " << e.wwhat() << L"\n";
                return false;
            }
            catch (...) {
                std::wcerr << L"Неизвестная ошибка при регистрации\n";
                return false;
            }
        }

    private:
        Db::Database& m_db;

        void logAudit(int userId, const std::wstring& opType,
            const std::wstring& table, int recordId,
            const std::wstring& oldVal, const std::wstring& newVal)
        {
            ///*
            std::wstring sql =
                L"INSERT INTO AuditLog (UserId, OperationType, TableName, "
                L"RecordId, OldValues, NewValues) VALUES (?, ?, ?, ?, ?, ?)";
            
            std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> params;
            params.emplace_back(L"", adInteger, _variant_t(userId));
            params.emplace_back(L"", adVarWChar, _variant_t(opType.c_str()));
            params.emplace_back(L"", adVarWChar, _variant_t(table.c_str()));
            params.emplace_back(L"", adInteger, _variant_t(recordId));
            params.emplace_back(L"", adVarWChar, _variant_t(oldVal.c_str()));
            params.emplace_back(L"", adVarWChar, _variant_t(newVal.c_str()));
            m_db.executeParameterized(sql, params);
           // */
        }
    };

} // namespace Services