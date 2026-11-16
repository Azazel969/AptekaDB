// ============================================================
// ReportService.h — сервис аналитической отчётности
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#pragma once

// 1. Исправлен путь к заголовочному файлу (если файл лежит в src/services, а Database.h в src/db)
#include "Database.h" 
#include <string>           
#include <vector>
#include <tuple>            
#include <ctime>

namespace Services {

    struct ExpiryReportItem {
        std::wstring productName;
        std::wstring manufacturerName;
        std::wstring batchNumber;
        std::time_t  expiryDate;
        int          quantity;
        double       writeOffValue;
        int          daysUntilExpiry;
    };

    class ReportService {
    public:
        // Конструктор: сохраняем ссылку на объект базы данных
        explicit ReportService(Db::Database& db) : m_db(db) {}

        // Отчёт о товарах с истекающим сроком годности
        std::vector<ExpiryReportItem> getExpiringItems(int daysAhead = 30) {
            std::wstring sql =
                L"SELECT p.ProductName, m.ManufacturerName, b.BatchNumber, "
                L"       b.ExpiryDate, b.Quantity, "
                L"       b.Quantity * b.SalePrice AS WriteOffValue "
                L"FROM Batches b "
                L"INNER JOIN Products p ON b.ProductId = p.ProductId "
                L"INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
                L"WHERE b.ExpiryDate BETWEEN GETDATE() AND "
                L"      DATEADD(day, ?, GETDATE()) "
                L"  AND b.Quantity > 0 "
                L"ORDER BY b.ExpiryDate ASC";

            // Используем std::tuple, как было согласовано ранее
            std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> params;
            params.emplace_back(L"@", adInteger, _variant_t(daysAhead));

            auto rs = m_db.executeParameterized(sql, params);

            std::vector<ExpiryReportItem> result;
            while (!rs->EndOfFile) {
                ExpiryReportItem item;

                // Безопасное получение строковых значений через _bstr_t
                item.productName = (wchar_t*)(_bstr_t)rs->Fields->GetItem("ProductName")->Value;
                item.manufacturerName = (wchar_t*)(_bstr_t)rs->Fields->GetItem("ManufacturerName")->Value;
                item.batchNumber = (wchar_t*)(_bstr_t)rs->Fields->GetItem("BatchNumber")->Value;

                item.quantity = rs->Fields->GetItem("Quantity")->Value.intVal;
                item.writeOffValue = rs->Fields->GetItem("WriteOffValue")->Value.dblVal;

                // Преобразование даты из VARIANT в time_t
                DATE expiryVariant = rs->Fields->GetItem("ExpiryDate")->Value.date;
                SYSTEMTIME st;
                ::VariantTimeToSystemTime(expiryVariant, &st);

                struct tm tm = {};
                tm.tm_year = st.wYear - 1900;
                tm.tm_mon = st.wMonth - 1;
                tm.tm_mday = st.wDay;
                item.expiryDate = mktime(&tm);

                double seconds = std::difftime(item.expiryDate, std::time(nullptr));
                item.daysUntilExpiry = static_cast<int>(seconds / 86400.0);

                result.push_back(item);
                rs->MoveNext();
            }
            return result;
        }

    private:
        Db::Database& m_db;
    };

} // namespace Services