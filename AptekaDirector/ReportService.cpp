#include "ReportService.h"

ReportService::ReportService(Database& db) : db(db) {}

std::vector<ReportItem> ReportService::getExpiringItems(int warningDays) {
    std::vector<ReportItem> result;
    std::wstring query =
        L"SELECT p.ProductName, m.ManufacturerName, b.BatchNumber, b.Quantity, DATEDIFF(day, GETDATE(), b.ExpiryDate) AS DaysUntilExpiry "
        L"FROM Batches b INNER JOIN Products p ON b.ProductId = p.ProductId INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
        L"WHERE b.ExpiryDate <= DATEADD(day, ?, GETDATE()) AND b.Quantity > 0 ORDER BY b.ExpiryDate ASC";
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

std::vector<SalesReportItem> ReportService::getSalesReport(const std::wstring& startDate, const std::wstring& endDate) {
    std::vector<SalesReportItem> result;
    std::wstring query =
        L"SELECT p.ProductName, SUM(s.Quantity) AS TotalQty, SUM(s.TotalAmount) AS TotalRev "
        L"FROM Sales s INNER JOIN Batches b ON s.BatchId = b.BatchId INNER JOIN Products p ON b.ProductId = p.ProductId "
        L"WHERE s.SaleDate BETWEEN ? AND ? AND s.IsReturn = 0 "
        L"GROUP BY p.ProductName ORDER BY TotalRev DESC";
    auto rows = db.executeQuery(query, { startDate, endDate });
    for (const auto& row : rows) {
        SalesReportItem item;
        item.productName = std::get<std::wstring>(row[0]);
        item.totalQuantity = std::get<int>(row[1]);
        item.totalRevenue = std::get<double>(row[2]);
        result.push_back(item);
    }
    return result;
}

std::vector<ProfitReportItem> ReportService::getProfitabilityReport(const std::wstring& startDate, const std::wstring& endDate) {
    std::vector<ProfitReportItem> result;
    std::wstring query =
        L"SELECT p.ProductName, SUM(s.Quantity * b.PurchasePrice) AS Cost, SUM(s.TotalAmount) AS Rev "
        L"FROM Sales s INNER JOIN Batches b ON s.BatchId = b.BatchId INNER JOIN Products p ON b.ProductId = p.ProductId "
        L"WHERE s.SaleDate BETWEEN ? AND ? AND s.IsReturn = 0 "
        L"GROUP BY p.ProductName ORDER BY Rev DESC";
    auto rows = db.executeQuery(query, { startDate, endDate });
    for (const auto& row : rows) {
        ProfitReportItem item;
        item.productName = std::get<std::wstring>(row[0]);
        item.totalPurchaseCost = std::get<double>(row[1]);
        item.totalRevenue = std::get<double>(row[2]);
        item.profit = item.totalRevenue - item.totalPurchaseCost;
        result.push_back(item);
    }
    return result;
}