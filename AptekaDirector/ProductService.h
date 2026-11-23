#pragma once
#include "Database.h"
#include <vector>
#include <string>

struct ProductItem {
    int productId;
    std::wstring productName;
    std::wstring mnn;
    std::wstring dosageForm;
    std::wstring manufacturerName;
    std::wstring categoryName;
    std::wstring barcode;
    int totalQuantity;
    double salePrice;
};

class ProductService {
public:
    explicit ProductService(Database& db);

    void showAll();
    void addProduct();
    void searchProduct();
    void showLowStock();

private:
    Database& db;
    void printProduct(const ProductItem& p);
};