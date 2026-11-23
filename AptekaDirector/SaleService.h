// ============================================================
// SaleService.h — сервис регистрации продажи
// Реализует списание по принципу FEFO (First Expired — First Out):
// в первую очередь расходуется партия с наиболее ранним сроком годности.

// ============================================================
#pragma once
#include "Database.h"
#include "Exceptions.h"

class SaleService {
public:
    SaleService(Database& db);
    int registerSale(int productId, int quantity, int userId, const std::wstring& receiptNo);
    int registerReturn(int productId, int quantity, int userId, const std::wstring& receiptNo);
private:
    Database& db;
};