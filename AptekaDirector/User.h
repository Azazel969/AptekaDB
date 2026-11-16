// ============================================================
// User.h — модель учётной записи пользователя
// ============================================================
#pragma once
#include <string>

namespace Models {
    enum class UserRole {
        Director = 1,
        Manager = 2,      // Заведующий
        Pharmacist = 3,   // Фармацевт
        Storekeeper = 4   // Кладовщик
    };

    struct User {
        int userId;
        std::wstring login;
        std::wstring fullName;
        UserRole role;
        bool isBlocked;
        int failedAttempts;

        bool canAccess(const std::wstring& permission) const {
            if (role == UserRole::Director) return true;
            if (permission == L"RegisterSale" &&
                (role == UserRole::Pharmacist || role == UserRole::Manager)) return true;
            if (permission == L"ViewReports" &&
                (role == UserRole::Director || role == UserRole::Manager)) return true;
            if (permission == L"ManageUsers" && role == UserRole::Director) return true;
            return false;
        }
    };
}