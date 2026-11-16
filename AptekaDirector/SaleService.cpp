#include "SaleService.h"
#include "Exceptions.h"

//#define NOMINMAX

#include <algorithm>
#include <stdexcept>

SaleService::SaleService(Database& db) : db(db) {}

int SaleService::registerSale(int productId, int quantity, int userId) {
    db.beginTransaction();
    try {
        // Получаем партии, отсортированные по сроку годности (FEFO)
        std::wstring query = L"SELECT BatchId, Quantity, SalePrice FROM Batches WHERE ProductId = ? AND Quantity > 0 ORDER BY ExpiryDate ASC";
        auto batches = db.executeQuery(query, { productId });

        if (batches.empty()) {
            db.rollback();
            throw InsufficientStockException("Товар отсутствует на складе.");
        }

        int remainingQty = quantity;
        double totalAmount = 0;
        std::vector<std::pair<int, int>> toDeduct; // {BatchId, QtyToDeduct}

        // Распределяем количество по партиям (от самых старых к новым)
        for (const auto& row : batches) {
            int batchId = std::get<int>(row[0]);
            int availableQty = std::get<int>(row[1]);
            double price = std::get<double>(row[2]);

            int deduct = (std::min)(remainingQty, availableQty);
            toDeduct.push_back({ batchId, deduct });
            totalAmount += deduct * price;
            remainingQty -= deduct;

            if (remainingQty == 0) break;
        }

        if (remainingQty > 0) {
            db.rollback();
            throw InsufficientStockException("Недостаточно товара на складе.");
        }

        // Списываем остатки и создаём запись о продаже
        int saleId = 0;
        for (const auto& item : toDeduct) {
            db.executeNonQuery(L"UPDATE Batches SET Quantity = Quantity - ? WHERE BatchId = ?", { item.second, item.first });

            if (saleId == 0) {
                auto res = db.executeQuerySingle(
                    L"INSERT INTO Sales (BatchId, Quantity, SalePrice, TotalAmount, UserId, IsReturn) OUTPUT INSERTED.SaleId VALUES (?, ?, ?, ?, ?, 0)",
                    { item.first, quantity, 0.0, totalAmount, userId }
                );
                if (res.has_value()) saleId = std::get<int>((*res)[0]);
            }
        }

        // Аудит и фиксация транзакции
        db.executeNonQuery(
            L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId, NewValues) VALUES (?, 'SALE', 'Sales', ?, ?)",
            { userId, saleId, L"Продажа товара ID=" + std::to_wstring(productId) + L", кол-во=" + std::to_wstring(quantity) }
        );

        db.commit();
        return saleId;

    }
    catch (...) {
        db.rollback();
        throw;
    }
}