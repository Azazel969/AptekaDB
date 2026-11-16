// ============================================================
// Database.h — класс работы с Microsoft SQL Server 2022
// Использует ADO (ActiveX Data Objects) через #import
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

using Value = std::variant<int, double, std::wstring, std::nullptr_t>;
using Row = std::vector<Value>;
using ResultSet = std::vector<Row>;

class Database {
public:
    Database();
    ~Database();

    void connect(const std::wstring& connString);
    void disconnect();

    ResultSet executeQuery(const std::wstring& sql, const std::vector<Value>& params = {});
    std::optional<Row> executeQuerySingle(const std::wstring& sql, const std::vector<Value>& params = {});
    void executeNonQuery(const std::wstring& sql, const std::vector<Value>& params = {});

    void beginTransaction();
    void commit();
    void rollback();

private:
    SQLHENV hEnv;
    SQLHDBC hDbc;
    bool connected;

    ResultSet executeInternal(const std::wstring& sql, const std::vector<Value>& params);
    std::wstring substituteParams(const std::wstring& sql, const std::vector<Value>& params);
    std::wstring escapeString(const std::wstring& str);
    void checkError(SQLRETURN ret, SQLSMALLINT handleType, SQLHANDLE handle, const std::string& context);
};