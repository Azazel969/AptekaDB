#include "SupplyService.h"
#include <iostream>

SupplyService::SupplyService(Database& db) : db(db) {}

void SupplyService::registerSupply(int userId) {
    std::wcout << L"\n=== Регистрация новой поставки ===\n";

    std::wcout << L"ID поставщика: ";
    int supplierId; std::wcin >> supplierId;

    std::wstring invoiceNo;
    std::wcout << L"Номер накладной: "; std::wcin.ignore(); std::getline(std::wcin, invoiceNo);

    std::vector<SupplyItem> items;
    bool addMore = true;
    double totalAmount = 0;

    while (addMore) {
        SupplyItem item;
        std::wcout << L"\n--- Новая позиция ---\n";
        std::wcout << L"ID товара: "; std::wcin >> item.productId;
        std::wcout << L"Серия партии: "; std::wcin.ignore(); std::getline(std::wcin, item.batchNumber);
        std::wcout << L"Дата изготовления (ГГГГ-ММ-ДД): "; std::getline(std::wcin, item.manufactureDate);
        std::wcout << L"Срок годности (ГГГГ-ММ-ДД): "; std::getline(std::wcin, item.expiryDate);
        std::wcout << L"Закупочная цена: "; std::wcin >> item.purchasePrice;
        std::wcout << L"Розничная цена: "; std::wcin >> item.salePrice;
        std::wcout << L"Количество: "; std::wcin >> item.quantity;
        std::wcout << L"Место хранения: "; std::wcin.ignore(); std::getline(std::wcin, item.storageLocation);

        items.push_back(item);
        totalAmount += item.purchasePrice * item.quantity;

        std::wcout << L"Добавить ещё позицию? (1 - Да, 0 - Нет): ";
        int choice; std::wcin >> choice;
        addMore = (choice == 1);
    }

    db.beginTransaction();
    try {
        auto res = db.executeQuerySingle(
            L"INSERT INTO Supplies (SupplierId, InvoiceNumber, TotalAmount, UserId) OUTPUT INSERTED.SupplyId VALUES (?, ?, ?, ?)",
            { supplierId, invoiceNo, totalAmount, userId }
        );
        int supplyId = res.has_value() ? std::get<int>((*res)[0]) : 0;

        for (const auto& item : items) {
            std::wstring checkBatch = L"SELECT BatchId FROM Batches WHERE ProductId = ? AND BatchNumber = ?";
            auto existing = db.executeQuerySingle(checkBatch, { item.productId, item.batchNumber });

            if (existing.has_value()) {
                int batchId = std::get<int>((*existing)[0]);
                db.executeNonQuery(L"UPDATE Batches SET Quantity = Quantity + ? WHERE BatchId = ?", { item.quantity, batchId });
            }
            else {
                db.executeNonQuery(
                    L"INSERT INTO Batches (ProductId, SupplierId, BatchNumber, ManufactureDate, ExpiryDate, PurchasePrice, SalePrice, Quantity, StorageLocation) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                    { item.productId, supplierId, item.batchNumber, item.manufactureDate, item.expiryDate, item.purchasePrice, item.salePrice, item.quantity, item.storageLocation }
                );
            }
        }

        db.executeNonQuery(L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId, NewValues) VALUES (?, 'SUPPLY', 'Supplies', ?, ?)",
            { userId, supplyId, L"Поставка от поставщика ID=" + std::to_wstring(supplierId) });

        db.commit();
        std::wcout << L"\nПоставка успешно зарегистрирована!\n";
    }
    catch (const std::exception& e) {
        db.rollback();
        std::string msg(e.what()); std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}