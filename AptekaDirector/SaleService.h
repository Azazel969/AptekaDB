// ============================================================
// SaleService.h — сервис регистрации продажи
// Реализует списание по принципу FEFO (First Expired — First Out):
// в первую очередь расходуется партия с наиболее ранним сроком годности.
// Все операции выполняются в транзакции для гарантии ACID.
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#pragma once
#include "Database.h"
#include "Batch.h"
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace Services {

    class InsufficientStockException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class SaleService {
    public:
        // Конструктор: сохраняем ссылку на объект базы данных
        explicit SaleService(Db::Database& db) : m_db(db) {}

        // Регистрация продажи товара
        // Возвращает идентификатор созданной записи о продаже
        int registerSale(int productId, int quantity, int pharmacistId) {
            std::wstring sql =
                L"SELECT BatchId, ExpiryDate, Quantity, SalePrice "
                L"FROM Batches "
                L"WHERE ProductId = ? AND Quantity > 0 "
                L"  AND ExpiryDate > GETDATE() "
                L"ORDER BY ExpiryDate ASC";

            std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> params;
            params.emplace_back(L"@", adInteger, _variant_t(productId));

            auto rs = m_db.executeParameterized(sql, params);

            struct BatchAllocation {
                int batchId;
                int quantityToSell;
                double salePrice;
            };
            std::vector<BatchAllocation> allocations;
            int remaining = quantity;

            while (!rs->EndOfFile && remaining > 0) {
                int batchId = rs->Fields->GetItem("BatchId")->Value.intVal;
                int available = rs->Fields->GetItem("Quantity")->Value.intVal;
                double price = rs->Fields->GetItem("SalePrice")->Value.dblVal;

                int toSell = (available < remaining) ? available : remaining;
                allocations.push_back({ batchId, toSell, price });
                remaining -= toSell;

                rs->MoveNext();
            }

            if (remaining > 0) {
                throw InsufficientStockException("Недостаточно товара на складе");
            }

            try {
                m_db.beginTransaction();

                int lastSaleId = 0;
                for (const auto& alloc : allocations) {
                    double totalAmount = alloc.quantityToSell * alloc.salePrice;

                    std::wstring insertSale =
                        L"INSERT INTO Sales (BatchId, Quantity, SalePrice, "
                        L"TotalAmount, SaleDate, UserId, IsReturn) "
                        L"OUTPUT INSERTED.SaleId "
                        L"VALUES (?, ?, ?, ?, GETDATE(), ?, 0)";

                    std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> saleParams;
                    saleParams.emplace_back(L"@", adInteger, _variant_t(alloc.batchId));
                    saleParams.emplace_back(L"@", adInteger, _variant_t(alloc.quantityToSell));
                    saleParams.emplace_back(L"@", adDouble, _variant_t(alloc.salePrice));
                    saleParams.emplace_back(L"@", adDouble, _variant_t(totalAmount));
                    saleParams.emplace_back(L"@", adInteger, _variant_t(pharmacistId));

                    auto saleRs = m_db.executeParameterized(insertSale, saleParams);
                    lastSaleId = saleRs->Fields->GetItem(0)->Value.intVal;

                    std::wstring updateBatch =
                        L"UPDATE Batches SET Quantity = Quantity - ? WHERE BatchId = ?";

                    std::vector<std::tuple<std::wstring, DataTypeEnum, _variant_t>> batchParams;
                    batchParams.emplace_back(L"@", adInteger, _variant_t(alloc.quantityToSell));
                    batchParams.emplace_back(L"@", adInteger, _variant_t(alloc.batchId));
                    m_db.executeParameterized(updateBatch, batchParams);
                }

                m_db.commitTransaction();
                return lastSaleId;
            }
            catch (...) {
                m_db.rollbackTransaction();
                throw;
            }
        }

    private:
        
        Db::Database& m_db;
    };

} // namespace Services
