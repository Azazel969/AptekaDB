// ============================================================
// SaleService.h — сервис регистрации продажи
// Реализует списание по принципу FEFO (First Expired — First Out):
// в первую очередь расходуется партия с наиболее ранним сроком годности.

// ============================================================
#pragma once
#include "Database.h"

class SaleService {
public:
    explicit SaleService(Database& db);
    int registerSale(int productId, int quantity, int userId);

private:
    Database& db;
};