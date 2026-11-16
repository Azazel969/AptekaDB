// ============================================================
// Product.h — модель товарной позиции
// ============================================================
#pragma once
#include <string>

namespace Models {
    struct Product {
        int productId;
        std::wstring productName;
        std::wstring mnn;
        std::wstring dosageForm;
        std::wstring dosage;
        std::wstring barcode;
        int categoryId;
        int manufacturerId;
        bool isPrescription;
        int minStock;
    };
}