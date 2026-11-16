// ============================================================
// ReportService.h — сервис аналитической отчётности
// ============================================================
#pragma once

#pragma once
#include "Database.h"
#include <string>
#include <vector>

struct ReportItem {
    std::wstring productName;
    std::wstring manufacturerName;
    std::wstring batchNumber;
    int quantity;
    int daysUntilExpiry;
};

class ReportService {
public:
    explicit ReportService(Database& db);
    std::vector<ReportItem> getExpiringItems(int warningDays);

private:
    Database& db;
};