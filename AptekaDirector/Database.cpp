// ============================================================
// Database.cpp — реализация класса Database
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#include "Database.h"
#include <comdef.h>
#include <string>

namespace Db {

    Database::Database() : m_inTransaction(false) {
        if (FAILED(::CoInitialize(nullptr))) {
            throw DatabaseException(L"Не удалось инициализировать COM");
        }
        m_connection.CreateInstance(__uuidof(Connection));
    }

    Database::~Database() {
        try {
            if (m_inTransaction) rollbackTransaction();
            disconnect();
        }
        catch (...) {}
        ::CoUninitialize();
    }

    void Database::connect(const std::wstring& connectionString) {
        try {
            _bstr_t connStr(connectionString.c_str());
            m_connection->Open(connStr, L"", L"", adConnectUnspecified);
        }
        catch (const _com_error& e) {
            // Получаем описание ошибки напрямую как wstring
            std::wstring msg = L"Ошибка подключения к SQL Server: ";
            msg += (const wchar_t*)_bstr_t(e.Description());
            throw DatabaseException(msg);
        }
    }

    void Database::disconnect() {
        if (m_connection && m_connection->State == adStateOpen) {
            m_connection->Close();
        }
    }

    bool Database::isConnected() const {
        return m_connection && m_connection->State == adStateOpen;
    }

    void Database::beginTransaction() {
        if (!isConnected())
            throw DatabaseException(L"Соединение не установлено");
        m_connection->BeginTrans();
        m_inTransaction = true;
    }

    void Database::commitTransaction() {
        if (!m_inTransaction) return;
        m_connection->CommitTrans();
        m_inTransaction = false;
    }

    void Database::rollbackTransaction() {
        if (!m_inTransaction) return;
        m_connection->RollbackTrans();
        m_inTransaction = false;
    }

    _RecordsetPtr Database::executeQuery(const std::wstring& sql) {
        if (!isConnected())
            throw DatabaseException(L"Соединение не установлено");

        _RecordsetPtr rs;
        rs.CreateInstance(__uuidof(Recordset));
        _bstr_t query(sql.c_str());

        try {
            rs->Open(query, m_connection.GetInterfacePtr(),
                adOpenStatic, adLockOptimistic, adCmdText);
        }
        catch (const _com_error& e) {
            std::wstring msg = L"Ошибка выполнения запроса: ";
            msg += (const wchar_t*)_bstr_t(e.Description());
            throw DatabaseException(msg);
        }
        return rs;
    }

    int Database::executeNonQuery(const std::wstring& sql) {
        _variant_t recordsAffected;
        _bstr_t query(sql.c_str());
        m_connection->Execute(query, &recordsAffected, adExecuteNoRecords);
        return recordsAffected.intVal;
    }

    _RecordsetPtr Database::executeParameterized(
        const std::wstring& sql,
        const std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>>& params)
    {
        _CommandPtr cmd;
        if (FAILED(cmd.CreateInstance(__uuidof(Command)))) {
            throw DatabaseException(L"Не удалось создать объект Command");
        }
        if (!isConnected()) {
            throw DatabaseException(L"Соединение с базой данных не установлено");
        }

        cmd->ActiveConnection = m_connection;
        cmd->CommandText = _bstr_t(sql.c_str());
        cmd->CommandType = adCmdText;

        try {
            int paramIndex = 1;
            for (const auto& p : params) {
                const std::wstring& paramName = std::get<0>(p);
                DataTypeEnum dataType = std::get<1>(p);
                _variant_t value = std::get<2>(p);

                long size = 0;
                if (dataType == adVarWChar || dataType == adVarChar) {
                    if (value.vt == VT_BSTR) {
                        size = SysStringLen(value.bstrVal);
                    }
                    else if (value.vt == (VT_BSTR | VT_BYREF)) {
                        size = SysStringLen(*value.pbstrVal);
                    }
                    else {
                        size = 255;
                    }
                    // ЗАЩИТА ОТ НУЛЕВОГО РАЗМЕРА
                    if (size == 0) size = 1;
                }
                else if (dataType == adInteger) size = 4;
                else if (dataType == adDouble) size = 8;
                else if (dataType == adBoolean) size = 1;

                // ИСПРАВЛЕНИЕ: Используем непустое имя параметра (например, @p1, @p2)
                std::wstring safeParamName = (paramName.empty()) ? L"@p" + std::to_wstring(paramIndex++) : paramName;

                _ParameterPtr param = cmd->CreateParameter(
                    _bstr_t(safeParamName.c_str()),
                    dataType,
                    adParamInput,
                    size,
                    value);

                if (param == nullptr) {
                    throw DatabaseException(L"Не удалось создать параметр запроса");
                }
                cmd->Parameters->Append(param);
            }

            _RecordsetPtr rs = cmd->Execute(nullptr, nullptr, adCmdText);
            return rs;
        }
        catch (const _com_error& e) {
            std::wstring msg = L"Ошибка выполнения SQL-запроса:\n";
            msg += L"SQL: " + sql + L"\n";
            msg += L"Описание: ";
            msg += (const wchar_t*)_bstr_t(e.Description());
            throw DatabaseException(msg);
        }
    }

} // namespace Db