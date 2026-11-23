#pragma once
#include "Database.h"

class SupplierService {
public:
    SupplierService(Database& db);
    void showAll();
    void addSupplier();
    void editSupplier(int supplierId);
    void deleteSupplier(int supplierId);
    void searchSupplier();
private:
    Database& db;
};