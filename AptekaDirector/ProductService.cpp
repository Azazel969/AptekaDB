#include "ProductService.h"
#include <iostream>

ProductService::ProductService(Database& db) : db(db) {}

void ProductService::printProduct(const ProductItem& p) {
    std::wcout << p.productId << L" | " << p.productName
        << L" | " << p.mnn
        << L" | " << p.dosageForm
        << L" | " << p.manufacturerName
        << L" | " << p.categoryName
        << L" | " << p.barcode
        << L" | ост.: " << p.totalQuantity
        << L" | цена: " << p.salePrice << L"\n";
}

void ProductService::showAll() {
    std::wstring query =
        L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, "
        L"m.ManufacturerName, c.CategoryName, p.Barcode, "
        L"ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0) AS TotalQty, "
        L"ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId ORDER BY b.ExpiryDate ASC), 0) AS SalePrice "
        L"FROM Products p "
        L"INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
        L"INNER JOIN Categories c ON p.CategoryId = c.CategoryId "
        L"ORDER BY p.ProductName";

    auto rows = db.executeQuery(query);
    if (rows.empty()) {
        std::wcout << L"Список товаров пуст.\n";
        return;
    }

    std::wcout << L"\n=== Список товаров ===\n";
    std::wcout << L"ID | Наименование | МНН | Форма | Производитель | Категория | Штрихкод | Остаток | Цена\n";
    std::wcout << L"-------------------------------------------------------------------------------------------\n";
    for (const auto& row : rows) {
        ProductItem p;
        p.productId = std::get<int>(row[0]);
        p.productName = std::get<std::wstring>(row[1]);
        p.mnn = std::holds_alternative<std::nullptr_t>(row[2]) ? L"" : std::get<std::wstring>(row[2]);
        p.dosageForm = std::holds_alternative<std::nullptr_t>(row[3]) ? L"" : std::get<std::wstring>(row[3]);
        p.manufacturerName = std::get<std::wstring>(row[4]);
        p.categoryName = std::get<std::wstring>(row[5]);
        p.barcode = std::holds_alternative<std::nullptr_t>(row[6]) ? L"" : std::get<std::wstring>(row[6]);
        p.totalQuantity = std::get<int>(row[7]);
        p.salePrice = std::get<double>(row[8]);
        printProduct(p);
    }
}

void ProductService::addProduct() {
    std::wcout << L"\n=== Добавление нового товара ===\n";

    std::wstring productName, mnn, dosageForm, barcode;
    int categoryId, manufacturerId, minStock;
    bool isPrescription;

    std::wcout << L"Наименование: ";
    std::wcin.ignore();
    std::getline(std::wcin, productName);

    std::wcout << L"МНН (международное непатентованное наименование): ";
    std::getline(std::wcin, mnn);

    std::wcout << L"Лекарственная форма (таблетки, капсулы, раствор...): ";
    std::getline(std::wcin, dosageForm);

    std::wcout << L"Штрих-код: ";
    std::getline(std::wcin, barcode);

    // Показать категории
    std::wcout << L"\nДоступные категории:\n";
    auto cats = db.executeQuery(L"SELECT CategoryId, CategoryName FROM Categories ORDER BY CategoryName");
    for (const auto& row : cats) {
        std::wcout << std::get<int>(row[0]) << L" - " << std::get<std::wstring>(row[1]) << L"\n";
    }
    std::wcout << L"ID категории: ";
    std::wcin >> categoryId;

    // Показать производителей
    std::wcout << L"\nДоступные производители:\n";
    auto mans = db.executeQuery(L"SELECT ManufacturerId, ManufacturerName FROM Manufacturers ORDER BY ManufacturerName");
    for (const auto& row : mans) {
        std::wcout << std::get<int>(row[0]) << L" - " << std::get<std::wstring>(row[1]) << L"\n";
    }
    std::wcout << L"ID производителя: ";
    std::wcin >> manufacturerId;

    std::wcout << L"Рецептурный (1 - да, 0 - нет): ";
    int presc;
    std::wcin >> presc;
    isPrescription = (presc != 0);

    std::wcout << L"Минимальный неснижаемый остаток: ";
    std::wcin >> minStock;

    try {
        std::wstring query =
            L"INSERT INTO Products (ProductName, MNN, DosageForm, Barcode, CategoryId, ManufacturerId, IsPrescription, MinStock) "
            L"VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        db.executeNonQuery(query, { productName, mnn, dosageForm, barcode, categoryId, manufacturerId, isPrescription ? 1 : 0, minStock });
        std::wcout << L"\nТовар успешно добавлен!\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what());
        std::wcout << L"Ошибка при добавлении: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void ProductService::searchProduct() {
    std::wcout << L"\n=== Поиск товара ===\n";
    std::wcout << L"1. По наименованию\n";
    std::wcout << L"2. По МНН\n";
    std::wcout << L"3. По штрих-коду\n";
    std::wcout << L"4. По производителю\n";
    std::wcout << L"Ваш выбор: ";

    int choice;
    std::wcin >> choice;

    std::wstring query;
    std::vector<Value> params;

    switch (choice) {
    case 1: {
        std::wstring name;
        std::wcout << L"Введите часть наименования: ";
        std::wcin.ignore();
        std::getline(std::wcin, name);
        query = L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, m.ManufacturerName, c.CategoryName, p.Barcode, "
            L"ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0), "
            L"ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId), 0) "
            L"FROM Products p INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
            L"INNER JOIN Categories c ON p.CategoryId = c.CategoryId "
            L"WHERE p.ProductName LIKE ?";
        params = { L"%" + name + L"%" };
        break;
    }
    case 2: {
        std::wstring mnn;
        std::wcout << L"Введите МНН: ";
        std::wcin.ignore();
        std::getline(std::wcin, mnn);
        query = L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, m.ManufacturerName, c.CategoryName, p.Barcode, "
            L"ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0), "
            L"ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId), 0) "
            L"FROM Products p INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
            L"INNER JOIN Categories c ON p.CategoryId = c.CategoryId "
            L"WHERE p.MNN LIKE ?";
        params = { L"%" + mnn + L"%" };
        break;
    }
    case 3: {
        std::wstring barcode;
        std::wcout << L"Введите штрих-код: ";
        std::wcin >> barcode;
        query = L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, m.ManufacturerName, c.CategoryName, p.Barcode, "
            L"ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0), "
            L"ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId), 0) "
            L"FROM Products p INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
            L"INNER JOIN Categories c ON p.CategoryId = c.CategoryId "
            L"WHERE p.Barcode = ?";
        params = { barcode };
        break;
    }
    case 4: {
        std::wstring manName;
        std::wcout << L"Введите название производителя: ";
        std::wcin.ignore();
        std::getline(std::wcin, manName);
        query = L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, m.ManufacturerName, c.CategoryName, p.Barcode, "
            L"ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0), "
            L"ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId), 0) "
            L"FROM Products p INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
            L"INNER JOIN Categories c ON p.CategoryId = c.CategoryId "
            L"WHERE m.ManufacturerName LIKE ?";
        params = { L"%" + manName + L"%" };
        break;
    }
    default:
        std::wcout << L"Неверный выбор.\n";
        return;
    }

    auto rows = db.executeQuery(query, params);
    if (rows.empty()) {
        std::wcout << L"Товары не найдены.\n";
        return;
    }

    std::wcout << L"\nНайдено товаров: " << rows.size() << L"\n";
    std::wcout << L"ID | Наименование | МНН | Форма | Производитель | Категория | Штрихкод | Остаток | Цена\n";
    std::wcout << L"-------------------------------------------------------------------------------------------\n";
    for (const auto& row : rows) {
        ProductItem p;
        p.productId = std::get<int>(row[0]);
        p.productName = std::get<std::wstring>(row[1]);
        p.mnn = std::holds_alternative<std::nullptr_t>(row[2]) ? L"" : std::get<std::wstring>(row[2]);
        p.dosageForm = std::holds_alternative<std::nullptr_t>(row[3]) ? L"" : std::get<std::wstring>(row[3]);
        p.manufacturerName = std::get<std::wstring>(row[4]);
        p.categoryName = std::get<std::wstring>(row[5]);
        p.barcode = std::holds_alternative<std::nullptr_t>(row[6]) ? L"" : std::get<std::wstring>(row[6]);
        p.totalQuantity = std::get<int>(row[7]);
        p.salePrice = std::get<double>(row[8]);
        printProduct(p);
    }
}

void ProductService::showLowStock() {
    std::wstring query =
        L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, m.ManufacturerName, c.CategoryName, p.Barcode, "
        L"ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0) AS TotalQty, "
        L"p.MinStock "
        L"FROM Products p "
        L"INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
        L"INNER JOIN Categories c ON p.CategoryId = c.CategoryId "
        L"WHERE ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0) <= p.MinStock "
        L"ORDER BY TotalQty ASC";

    auto rows = db.executeQuery(query);
    if (rows.empty()) {
        std::wcout << L"\nВсе товары имеют достаточный остаток.\n";
        return;
    }

    std::wcout << L"\n=== Товары с низким остатком (требуют дозаказа) ===\n";
    std::wcout << L"ID | Наименование | Остаток | Мин. остаток\n";
    std::wcout << L"--------------------------------------------------\n";
    for (const auto& row : rows) {
        std::wcout << std::get<int>(row[0]) << L" | "
            << std::get<std::wstring>(row[1]) << L" | "
            << std::get<int>(row[7]) << L" | "
            << std::get<int>(row[8]) << L"\n";
    }
}