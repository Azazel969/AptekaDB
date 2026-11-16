// ============================================================
// Product.h — модель товарной позиции
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#pragma once
#include <string>

namespace Models {
    class Product {
    public:
        int         productId = 0;
        std::wstring productName;
        std::wstring mnn;                  // Международное непатентованное наименование
        std::wstring dosageForm;
        std::wstring dosage;
        std::wstring barcode;
        int         categoryId = 0;
        int         manufacturerId = 0;
        bool        isPrescription = false;
        int         minStock = 10;

        // Валидация обязательных полей
        bool isValid() const {
            return !productName.empty() && !barcode.empty() &&
                categoryId > 0 && manufacturerId > 0;
        }
    };
}
