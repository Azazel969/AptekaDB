#pragma once
#include "Database.h"

class SupplierService {
public:
    explicit SupplierService(Database& db);

    void showAll();
    void addSupplier();
    void searchSupplier();

private:
    Database& db;
};
