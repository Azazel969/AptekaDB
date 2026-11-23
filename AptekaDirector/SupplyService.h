#pragma once
#include "Database.h"

struct SupplyItem {
    int productId = 0;
    std::wstring batchNumber;
    std::wstring manufactureDate;
    std::wstring expiryDate;
    double purchasePrice = 0.0;
    double salePrice = 0.0;
    int quantity = 0;
    std::wstring storageLocation;
};

class SupplyService {
public:
    SupplyService(Database& db);
    void registerSupply(int userId);
private:
    Database& db;
};