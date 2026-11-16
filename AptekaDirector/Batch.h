// ============================================================
// Batch.h — модель партии товара (серия со сроком годности)
// ============================================================
#pragma once
#include <string>

namespace Models {
    struct Batch {
        int batchId;
        int productId;
        int supplierId;
        std::wstring batchNumber;
        std::wstring manufactureDate;
        std::wstring expiryDate;
        double purchasePrice;
        double salePrice;
        int quantity;
        std::wstring storageLocation;
    };
}