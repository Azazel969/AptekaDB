#pragma once
#include <exception>
#include <string>
#include <windows.h>

// Базовый класс исключений приложения с поддержкой Unicode
class AppException : public std::exception {
protected:
    std::wstring wmsg;
    std::string u8msg;

    void init(const std::wstring& message) {
        wmsg = message;
        int size = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), (int)wmsg.size(), NULL, 0, NULL, NULL);
        u8msg.resize(size);
        WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), (int)wmsg.size(), &u8msg[0], size, NULL, NULL);
    }

public:
    AppException(const std::wstring& message) {
        init(message);
    }

    const char* what() const noexcept override {
        return u8msg.c_str();
    }

    const std::wstring& wwhat() const noexcept {
        return wmsg;
    }
};

// Исключение для ошибок базы данных
class DatabaseException : public AppException {
public:
    DatabaseException(const std::wstring& message) : AppException(message) {}
};

// Исключение для недостаточного количества товара на складе
class InsufficientStockException : public AppException {
public:
    InsufficientStockException(const std::wstring& message) : AppException(message) {}
};

// Исключение для ошибок доступа
class AccessDeniedException : public AppException {
public:
    AccessDeniedException(const std::wstring& message) : AppException(message) {}
};