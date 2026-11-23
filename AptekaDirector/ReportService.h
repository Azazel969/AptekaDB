// ============================================================
// ReportService.h — сервис аналитической отчётности
// ============================================================
#pragma once
#include "Database.h"
#include <vector>
#include <string>

struct ReportItem {
    std::wstring productName;
    std::wstring manufacturerName;
    std::wstring batchNumber;
    int quantity = 0;
    int daysUntilExpiry = 0;
};

struct SalesReportItem {
    std::wstring productName;
    int totalQuantity = 0;
    double totalRevenue = 0.0;
};

struct ProfitReportItem {
    std::wstring productName;
    double totalPurchaseCost = 0.0;
    double totalRevenue = 0.0;
    double profit = 0.0;
};

class ReportService {
public:
    ReportService(Database& db);
    std::vector<ReportItem> getExpiringItems(int warningDays);
    std::vector<SalesReportItem> getSalesReport(const std::wstring& startDate, const std::wstring& endDate);
    std::vector<ProfitReportItem> getProfitabilityReport(const std::wstring& startDate, const std::wstring& endDate);
private:
    Database& db;
};