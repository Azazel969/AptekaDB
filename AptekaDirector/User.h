#pragma once
#include <string>
#include <optional>

namespace Models {
    enum class UserRole {
        Director = 1,
        Manager = 2,
        Pharmacist = 3,
        Storekeeper = 4
    };

    struct User {
        int userId;
        std::wstring login;
        std::wstring fullName;
        UserRole role;
        bool isBlocked;
        std::optional<std::wstring> blockedUntil;
        int failedAttempts;

        bool canAccess(const std::wstring& permission) const {
            if (role == UserRole::Director) return true;
            if (permission == L"RegisterSale" && (role == UserRole::Pharmacist || role == UserRole::Manager)) return true;
            if (permission == L"ViewReports" && (role == UserRole::Director || role == UserRole::Manager)) return true;
            if (permission == L"ManageUsers" && role == UserRole::Director) return true;
            if (permission == L"ManageSupplies" && (role == UserRole::Director || role == UserRole::Manager || role == UserRole::Storekeeper)) return true;
            return false;
        }
    };
}