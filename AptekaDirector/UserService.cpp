#include "UserService.h"
#include <iostream>

UserService::UserService(Database& db) : db(db) {}

void UserService::showAllUsers() {
    std::wstring query =
        L"SELECT u.UserId, u.Login, u.FullName, r.RoleName, u.IsBlocked, u.FailedAttempts, u.LastLogin "
        L"FROM Users u INNER JOIN Roles r ON u.RoleId = r.RoleId ORDER BY u.Login";

    auto rows = db.executeQuery(query);
    if (rows.empty()) {
        std::wcout << L"Список пользователей пуст.\n";
        return;
    }

    std::wcout << L"\n=== Список сотрудников ===\n";
    std::wcout << L"ID | Логин | ФИО | Роль | Заблокирован | Неуд. попыток | Последний вход\n";
    std::wcout << L"------------------------------------------------------------------------\n";
    for (const auto& row : rows) {
        std::wcout << std::get<int>(row[0]) << L" | "
            << std::get<std::wstring>(row[1]) << L" | "
            << std::get<std::wstring>(row[2]) << L" | "
            << std::get<std::wstring>(row[3]) << L" | "
            << (std::get<int>(row[4]) != 0 ? L"Да" : L"Нет") << L" | "
            << std::get<int>(row[5]) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[6]) ? L"никогда" : std::get<std::wstring>(row[6]))
            << L"\n";
    }
}

void UserService::blockUser() {
    int userId;
    std::wcout << L"\nВведите ID пользователя для блокировки: ";
    std::wcin >> userId;

    try {
        db.executeNonQuery(L"UPDATE Users SET IsBlocked = 1, FailedAttempts = 0 WHERE UserId = ?", { userId });
        std::wcout << L"Пользователь заблокирован.\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what());
        std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void UserService::unblockUser() {
    int userId;
    std::wcout << L"\nВведите ID пользователя для разблокировки: ";
    std::wcin >> userId;

    try {
        db.executeNonQuery(L"UPDATE Users SET IsBlocked = 0, FailedAttempts = 0 WHERE UserId = ?", { userId });
        std::wcout << L"Пользователь разблокирован.\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what());
        std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void UserService::showAuditLog() {
    std::wstring query =
        L"SELECT TOP 50 l.OperationDate, u.Login, l.OperationType, l.TableName, l.RecordId, l.NewValues "
        L"FROM AuditLog l INNER JOIN Users u ON l.UserId = u.UserId "
        L"ORDER BY l.OperationDate DESC";

    auto rows = db.executeQuery(query);
    if (rows.empty()) {
        std::wcout << L"Журнал операций пуст.\n";
        return;
    }

    std::wcout << L"\n=== Журнал операций (последние 50) ===\n";
    std::wcout << L"Дата | Пользователь | Операция | Таблица | ID записи | Данные\n";
    std::wcout << L"------------------------------------------------------------------------\n";
    for (const auto& row : rows) {
        std::wcout << std::get<std::wstring>(row[0]) << L" | "
            << std::get<std::wstring>(row[1]) << L" | "
            << std::get<std::wstring>(row[2]) << L" | "
            << std::get<std::wstring>(row[3]) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[4]) ? L"-" : std::to_wstring(std::get<int>(row[4]))) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[5]) ? L"" : std::get<std::wstring>(row[5]))
            << L"\n";
    }
}