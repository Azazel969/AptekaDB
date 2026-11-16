// ============================================================
// User.h — модель учётной записи пользователя
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#pragma once
#include <string>

namespace Models {
    enum class UserRole {
        Director = 1,
        Manager = 2,
        Pharmacist = 3,
        Storekeeper = 4
    };

    class User {
    public:
        int         userId = 0;
        std::wstring login;
        std::wstring fullName;
        UserRole    role = UserRole::Pharmacist;
        bool        isBlocked = false;
        int         failedAttempts = 0;

        // Проверка доступа к функции по роли
        bool canAccess(const std::wstring& functionName) const {
            if (role == UserRole::Director) return true;
            if (role == UserRole::Manager) {
                return functionName != L"ManageUsers" &&
                    functionName != L"ViewAuditLog";
            }
            if (role == UserRole::Pharmacist) {
                return functionName == L"RegisterSale" ||
                    functionName == L"ViewStock";
            }
            if (role == UserRole::Storekeeper) {
                return functionName == L"RegisterSupply" ||
                    functionName == L"ViewStock" ||
                    functionName == L"Inventory";
            }
            return false;
        }
    };
}
