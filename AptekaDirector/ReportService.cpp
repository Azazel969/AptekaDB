#include "ReportService.h"

ReportService::ReportService(Database& db) : db(db) {}

std::vector<ReportItem> ReportService::getExpiringItems(int warningDays) {
    std::vector<ReportItem> result;

    std::wstring query =
        L"SELECT p.ProductName, m.ManufacturerName, b.BatchNumber, b.Quantity, "
        L"DATEDIFF(day, GETDATE(), b.ExpiryDate) AS DaysUntilExpiry "
        L"FROM Batches b "
        L"INNER JOIN Products p ON b.ProductId = p.ProductId "
        L"INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
        L"WHERE b.ExpiryDate <= DATEADD(day, ?, GETDATE()) AND b.Quantity > 0 "
        L"ORDER BY b.ExpiryDate ASC";

    auto rows = db.executeQuery(query, { warningDays });

    for (const auto& row : rows) {
        ReportItem item;
        item.productName = std::get<std::wstring>(row[0]);
        item.manufacturerName = std::get<std::wstring>(row[1]);
        item.batchNumber = std::get<std::wstring>(row[2]);
        item.quantity = std::get<int>(row[3]);
        item.daysUntilExpiry = std::get<int>(row[4]);
        result.push_back(item);
    }

    return result;
}