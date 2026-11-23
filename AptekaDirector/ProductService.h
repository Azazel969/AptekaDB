#pragma once
#include "Database.h"
#include "Product.h"
#include <vector>

class ProductService {
public:
    ProductService(Database& db);
    void showAll();
    void addProduct();
    void editProduct(int productId);
    void deleteProduct(int productId);
    void searchProduct();
    void showLowStock();

private:
    Database& db;
    void printProduct(const ProductItem& p);
};