// ============================================================
// Batch.h — модель партии товара (серия со сроком годности)
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#pragma once
#include <string>
#include <ctime>

namespace Models {
    class Batch {
    public:
        int         batchId = 0;
        int         productId = 0;
        int         supplierId = 0;
        std::wstring batchNumber;
        std::time_t manufactureDate = 0;
        std::time_t expiryDate = 0;
        double      purchasePrice = 0.0;
        double      salePrice = 0.0;
        int         quantity = 0;
        std::wstring storageLocation;

        // Проверка, истёк ли срок годности
        bool isExpired() const {
            return std::time(nullptr) > expiryDate;
        }

        // Количество дней до истечения срока годности
        int daysUntilExpiry() const {
            double seconds = std::difftime(expiryDate, std::time(nullptr));
            return static_cast<int>(seconds / 86400.0);
        }
    };
}
