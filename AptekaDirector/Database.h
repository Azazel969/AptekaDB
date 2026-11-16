// ============================================================
// Database.h — класс работы с Microsoft SQL Server 2022
// Использует ADO (ActiveX Data Objects) через #import
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
// Database.h — класс подключения к Microsoft SQL Server 2022
#pragma once
#import "C:\Program Files\Common Files\System\ado\msado15.dll" \
        rename("EOF", "EndOfFile") no_namespace

#include <string>
#include <memory>
#include <vector>
#include <tuple>
#include <stdexcept>

namespace Db {

    class DatabaseException : public std::exception {
    private:
        std::wstring m_message;
    public:
        explicit DatabaseException(const std::wstring& msg) : m_message(msg) {}

        const char* what() const noexcept override {
            static std::string narrow_msg;
            narrow_msg.clear();
            for (wchar_t c : m_message) {
                if (c < 128) narrow_msg += static_cast<char>(c);
                else narrow_msg += '?';
            }
            return narrow_msg.c_str();
        }

        const std::wstring& wwhat() const noexcept {
            return m_message;
        }
    };

    class Database {
    public:
        Database();
        ~Database();

        void connect(const std::wstring& connectionString);
        void disconnect();
        bool isConnected() const;

        void beginTransaction();
        void commitTransaction();
        void rollbackTransaction();

        _RecordsetPtr executeQuery(const std::wstring& sql);
        int executeNonQuery(const std::wstring& sql);

        _RecordsetPtr executeParameterized(
            const std::wstring& sql,
            const std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>>& params);

    private:
        _ConnectionPtr m_connection;
        bool m_inTransaction;
    };
}