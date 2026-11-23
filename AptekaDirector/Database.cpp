#include "Database.h"
#include "Exceptions.h"
#include <sstream>
#include <algorithm>
#include <cctype>

// ============================================================
// Конструктор / деструктор
// ============================================================
Database::Database() : hEnv(SQL_NULL_HENV), hDbc(SQL_NULL_HDBC), connected(false) {
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        throw DatabaseException("Не удалось создать ODBC environment.");
    }
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
}

Database::~Database() {
    if (connected) disconnect();
    if (hDbc != SQL_NULL_HDBC) SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    if (hEnv != SQL_NULL_HENV) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}

// ============================================================
// Подключение / отключение
// ============================================================
void Database::connect(const std::wstring& connString) {
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    checkError(ret, SQL_HANDLE_ENV, hEnv, "SQLAllocHandle(Dbc)");

    SQLSMALLINT outLen;
    wchar_t outConn[1024];
    ret = SQLDriverConnectW(hDbc, NULL,
        (SQLWCHAR*)connString.c_str(), (SQLSMALLINT)connString.length(),
        outConn, 1024, &outLen, SQL_DRIVER_NOPROMPT);
    checkError(ret, SQL_HANDLE_DBC, hDbc, "SQLDriverConnect");

    connected = true;
}

void Database::disconnect() {
    if (connected) {
        SQLDisconnect(hDbc);
        connected = false;
    }
}

// ============================================================
// Экранирование строки для SQL
// ============================================================
std::wstring Database::escapeString(const std::wstring& str) {
    std::wstring res;
    res.reserve(str.length() + 4);
    for (wchar_t c : str) {
        if (c == L'\'') res += L"''";
        else res += c;
    }
    return res;
}

// ============================================================
// Подстановка параметров в SQL-запрос
// ============================================================
std::wstring Database::substituteParams(const std::wstring& sql, const std::vector<Value>& params) {
    std::wstring result;
    result.reserve(sql.length() * 2);
    size_t paramIdx = 0;

    for (size_t i = 0; i < sql.length(); ++i) {
        if (sql[i] == L'?' && paramIdx < params.size()) {
            const auto& val = params[paramIdx++];
            if (std::holds_alternative<std::nullptr_t>(val)) {
                result += L"NULL";
            }
            else if (std::holds_alternative<int>(val)) {
                result += std::to_wstring(std::get<int>(val));
            }
            else if (std::holds_alternative<double>(val)) {
                double d = std::get<double>(val);
                std::wstring s = std::to_wstring(d);
                // Убираем лишние нули после точки
                size_t dotPos = s.find(L'.');
                if (dotPos != std::wstring::npos) {
                    size_t lastNonZero = s.find_last_not_of(L'0');
                    if (lastNonZero != std::wstring::npos && lastNonZero > dotPos) {
                        s = s.substr(0, lastNonZero + 1);
                    }
                    else if (lastNonZero == dotPos) {
                        s = s.substr(0, dotPos);
                    }
                }
                result += s;
            }
            else if (std::holds_alternative<std::wstring>(val)) {
                result += L"N'" + escapeString(std::get<std::wstring>(val)) + L"'";
            }
        }
        else {
            result += sql[i];
        }
    }
    return result;
}

// ============================================================
// Проверка ошибок ODBC
// ============================================================
void Database::checkError(SQLRETURN ret, SQLSMALLINT handleType, SQLHANDLE handle, const std::string& context) {
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return;

    SQLWCHAR sqlState[6], message[512];
    SQLINTEGER nativeError;
    SQLSMALLINT msgLen;
    SQLGetDiagRecW(handleType, handle, 1, sqlState, &nativeError, message, 512, &msgLen);

    std::wstring wmsg(message, msgLen);
    std::string errMsg(context + ": " + std::string(wmsg.begin(), wmsg.end()));
    throw DatabaseException(errMsg);
}

// ============================================================
// Внутренний метод выполнения SQL
// КЛЮЧЕВОЕ: читаем через SQL_C_WCHAR — получаем UTF-16 напрямую
// ============================================================
ResultSet Database::executeInternal(const std::wstring& sql, const std::vector<Value>& params) {
    if (!connected) throw DatabaseException("Нет соединения с БД.");

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    checkError(ret, SQL_HANDLE_DBC, hDbc, "SQLAllocHandle(Stmt)");

    std::wstring finalSql = substituteParams(sql, params);

    ret = SQLExecDirectW(hStmt, (SQLWCHAR*)finalSql.c_str(), (SQLINTEGER)finalSql.length());
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        checkError(ret, SQL_HANDLE_STMT, hStmt, "SQLExecDirect");
    }

    ResultSet result;

    SQLSMALLINT colCount = 0;
    SQLNumResultCols(hStmt, &colCount);

    if (colCount > 0) {
        // Получаем типы колонок для правильного чтения
        std::vector<SQLSMALLINT> colTypes(colCount + 1);
        for (SQLSMALLINT col = 1; col <= colCount; ++col) {
            SQLSMALLINT dataType, decimalDigits, nullable;
            SQLULEN colSize;
            SQLDescribeColW(hStmt, col, NULL, 0, NULL, &dataType, &colSize, &decimalDigits, &nullable);
            colTypes[col] = dataType;
        }

        while (SQLFetch(hStmt) == SQL_SUCCESS) {
            Row row;
            row.reserve(colCount);

            for (SQLSMALLINT col = 1; col <= colCount; ++col) {
                SQLLEN indicator = 0;
                SQLSMALLINT dataType = colTypes[col];

                // Числовые типы — читаем напрямую
                if (dataType == SQL_INTEGER || dataType == SQL_SMALLINT ||
                    dataType == SQL_TINYINT || dataType == SQL_BIT) {
                    int buf = 0;
                    ret = SQLGetData(hStmt, col, SQL_C_LONG, &buf, sizeof(buf), &indicator);
                    if (indicator == SQL_NULL_DATA) {
                        row.push_back(std::nullptr_t{});
                    }
                    else {
                        row.push_back(buf);
                    }
                }
                else if (dataType == SQL_BIGINT) {
                    long long buf = 0;
                    ret = SQLGetData(hStmt, col, SQL_C_SBIGINT, &buf, sizeof(buf), &indicator);
                    if (indicator == SQL_NULL_DATA) {
                        row.push_back(std::nullptr_t{});
                    }
                    else {
                        row.push_back(static_cast<int>(buf));
                    }
                }
                else if (dataType == SQL_FLOAT || dataType == SQL_REAL ||
                    dataType == SQL_DOUBLE || dataType == SQL_DECIMAL ||
                    dataType == SQL_NUMERIC) {
                    double buf = 0;
                    ret = SQLGetData(hStmt, col, SQL_C_DOUBLE, &buf, sizeof(buf), &indicator);
                    if (indicator == SQL_NULL_DATA) {
                        row.push_back(std::nullptr_t{});
                    }
                    else {
                        row.push_back(buf);
                    }
                }
                else {
                    // ВСЁ ОСТАЛЬНОЕ (NVARCHAR, VARCHAR, DATE, DATETIME и т.д.)
                    // читаем как UTF-16 — БЕЗ КОНВЕРТАЦИЙ!
                    wchar_t buffer[2048] = { 0 };
                    ret = SQLGetData(hStmt, col, SQL_C_WCHAR, buffer, sizeof(buffer), &indicator);

                    if (indicator == SQL_NULL_DATA) {
                        row.push_back(std::nullptr_t{});
                    }
                    else {
                        // buffer уже содержит UTF-16 строку — просто кладём в wstring
                        std::wstring val(buffer);
                        row.push_back(val);
                    }
                }
            }
            result.push_back(row);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

ResultSet Database::executeQuery(const std::wstring& sql, const std::vector<Value>& params) {
    return executeInternal(sql, params);
}

std::optional<Row> Database::executeQuerySingle(const std::wstring& sql, const std::vector<Value>& params) {
    ResultSet rs = executeInternal(sql, params);
    if (rs.empty()) return std::nullopt;
    return rs[0];
}

void Database::executeNonQuery(const std::wstring& sql, const std::vector<Value>& params) {
    executeInternal(sql, params);
}

void Database::beginTransaction() {
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
}

void Database::commit() {
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}

void Database::rollback() {
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_ROLLBACK);
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}