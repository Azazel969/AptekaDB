#include "SaleService.h"
#include "Exceptions.h"
#include <algorithm>

SaleService::SaleService(Database& db) : db(db) {}

int SaleService::registerSale(int productId, int quantity, int userId, const std::wstring& receiptNo) {
    db.beginTransaction();
    try {
        std::wstring query = L"SELECT BatchId, Quantity, SalePrice FROM Batches WHERE ProductId = ? AND Quantity > 0 ORDER BY ExpiryDate ASC";
        auto batches = db.executeQuery(query, { productId });
        if (batches.empty()) {
            db.rollback();
            throw InsufficientStockException(L"Товар отсутствует на складе.");
        }

        int remainingQty = quantity;
        double totalAmount = 0;
        std::vector<std::tuple<int, int, double>> toDeduct;

        for (const auto& row : batches) {
            int batchId = std::get<int>(row[0]);
            int availableQty = std::get<int>(row[1]);
            double price = std::get<double>(row[2]);
            int deduct = (std::min)(remainingQty, availableQty);
            toDeduct.push_back({ batchId, deduct, price });
            totalAmount += deduct * price;
            remainingQty -= deduct;
            if (remainingQty == 0) break;
        }

        if (remainingQty > 0) {
            db.rollback();
            throw InsufficientStockException(L"Недостаточно товара на складе.");
        }

        int saleId = 0;
        for (const auto& item : toDeduct) {
            int batchId = std::get<0>(item);
            int qty = std::get<1>(item);
            double price = std::get<2>(item);

            db.executeNonQuery(L"UPDATE Batches SET Quantity = Quantity - ? WHERE BatchId = ?", { qty, batchId });

            auto res = db.executeQuerySingle(
                L"INSERT INTO Sales (BatchId, Quantity, SalePrice, TotalAmount, UserId, IsReturn, ReceiptNo) OUTPUT INSERTED.SaleId VALUES (?, ?, ?, ?, ?, 0, ?)",
                { batchId, qty, price, qty * price, userId, receiptNo }
            );
            if (res.has_value() && saleId == 0) saleId = std::get<int>((*res)[0]);
        }

        db.executeNonQuery(L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId, NewValues) VALUES (?, 'SALE', 'Sales', ?, ?)",
            { userId, saleId, L"Продажа товара ID=" + std::to_wstring(productId) + L", кол-во=" + std::to_wstring(quantity) });

        db.commit();
        return saleId;
    }
    catch (...) {
        db.rollback();
        throw;
    }
}

int SaleService::registerReturn(int productId, int quantity, int userId, const std::wstring& receiptNo) {
    db.beginTransaction();
    try {
        std::wstring query = L"SELECT TOP 1 BatchId, PurchasePrice FROM Batches WHERE ProductId = ? ORDER BY ExpiryDate DESC";
        auto row = db.executeQuerySingle(query, { productId });
        if (!row.has_value()) {
            db.rollback();
            throw AppException(L"Нет партий данного товара для возврата.");
        }

        int batchId = std::get<int>((*row)[0]);
        double price = std::get<double>((*row)[1]);
        std::wstring getPriceQ = L"SELECT SalePrice FROM Batches WHERE BatchId = ?";
        auto priceRow = db.executeQuerySingle(getPriceQ, { batchId });
        if (priceRow.has_value()) price = std::get<double>((*priceRow)[0]);

        db.executeNonQuery(L"UPDATE Batches SET Quantity = Quantity + ? WHERE BatchId = ?", { quantity, batchId });

        auto res = db.executeQuerySingle(
            L"INSERT INTO Sales (BatchId, Quantity, SalePrice, TotalAmount, UserId, IsReturn, ReceiptNo) OUTPUT INSERTED.SaleId VALUES (?, ?, ?, ?, ?, 1, ?)",
            { batchId, quantity, price, quantity * price, userId, receiptNo }
        );

        int saleId = res.has_value() ? std::get<int>((*res)[0]) : 0;
        db.executeNonQuery(L"INSERT INTO AuditLog (UserId, OperationType, TableName, RecordId, NewValues) VALUES (?, 'RETURN', 'Sales', ?, ?)",
            { userId, saleId, L"Возврат товара ID=" + std::to_wstring(productId) + L", кол-во=" + std::to_wstring(quantity) });

        db.commit();
        return saleId;
    }
    catch (...) {
        db.rollback();
        throw;
    }
}