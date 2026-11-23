#include "ProductService.h"
#include <iostream>

ProductService::ProductService(Database& db) : db(db) {}

void ProductService::printProduct(const ProductItem& p) {
    std::wcout << p.productId << L" | " << p.productName
        << L" | " << p.mnn << L" | " << p.dosageForm << L" | " << p.dosage
        << L" | " << p.manufacturerName << L" | " << p.categoryName
        << L" | " << p.barcode << L" | ост.: " << p.totalQuantity
        << L" | цена: " << p.salePrice << L"\n";
}

void ProductService::showAll() {
    std::wstring query =
        L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, p.Dosage, "
        L"m.ManufacturerName, c.CategoryName, p.Barcode, "
        L"ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0) AS TotalQty, "
        L"ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId ORDER BY b.ExpiryDate ASC), 0) AS SalePrice "
        L"FROM Products p "
        L"INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId "
        L"INNER JOIN Categories c ON p.CategoryId = c.CategoryId "
        L"ORDER BY p.ProductName";

    auto rows = db.executeQuery(query);
    if (rows.empty()) { std::wcout << L"Список товаров пуст.\n"; return; }

    std::wcout << L"\n=== Список товаров ===\n";
    std::wcout << L"ID | Наименование | МНН | Форма | Дозировка | Производитель | Категория | Штрихкод | Остаток | Цена\n";
    std::wcout << L"------------------------------------------------------------------------------------------------------\n";
    for (const auto& row : rows) {
        ProductItem p;
        p.productId = std::get<int>(row[0]);
        p.productName = std::get<std::wstring>(row[1]);
        p.mnn = std::holds_alternative<std::nullptr_t>(row[2]) ? L"" : std::get<std::wstring>(row[2]);
        p.dosageForm = std::holds_alternative<std::nullptr_t>(row[3]) ? L"" : std::get<std::wstring>(row[3]);
        p.dosage = std::holds_alternative<std::nullptr_t>(row[4]) ? L"" : std::get<std::wstring>(row[4]);
        p.manufacturerName = std::get<std::wstring>(row[5]);
        p.categoryName = std::get<std::wstring>(row[6]);
        p.barcode = std::holds_alternative<std::nullptr_t>(row[7]) ? L"" : std::get<std::wstring>(row[7]);
        p.totalQuantity = std::get<int>(row[8]);
        p.salePrice = std::get<double>(row[9]);
        printProduct(p);
    }
}

void ProductService::addProduct() {
    std::wcout << L"\n=== Добавление нового товара ===\n";
    std::wstring productName, mnn, dosageForm, dosage, barcode;
    int categoryId, manufacturerId, minStock;
    bool isPrescription;

    std::wcout << L"Наименование: "; std::wcin.ignore(); std::getline(std::wcin, productName);
    std::wcout << L"МНН: "; std::getline(std::wcin, mnn);
    std::wcout << L"Лекарственная форма: "; std::getline(std::wcin, dosageForm);
    std::wcout << L"Дозировка: "; std::getline(std::wcin, dosage);
    std::wcout << L"Штрих-код: "; std::getline(std::wcin, barcode);

    std::wcout << L"\nКатегории:\n";
    auto cats = db.executeQuery(L"SELECT CategoryId, CategoryName FROM Categories ORDER BY CategoryName");
    for (const auto& row : cats) std::wcout << std::get<int>(row[0]) << L" - " << std::get<std::wstring>(row[1]) << L"\n";
    std::wcout << L"ID категории: "; std::wcin >> categoryId;

    std::wcout << L"\nПроизводители:\n";
    auto mans = db.executeQuery(L"SELECT ManufacturerId, ManufacturerName FROM Manufacturers ORDER BY ManufacturerName");
    for (const auto& row : mans) std::wcout << std::get<int>(row[0]) << L" - " << std::get<std::wstring>(row[1]) << L"\n";
    std::wcout << L"ID производителя: "; std::wcin >> manufacturerId;

    std::wcout << L"Рецептурный (1 - да, 0 - нет): "; int presc; std::wcin >> presc; isPrescription = (presc != 0);
    std::wcout << L"Минимальный неснижаемый остаток: "; std::wcin >> minStock;

    try {
        std::wstring query = L"INSERT INTO Products (ProductName, MNN, DosageForm, Dosage, Barcode, CategoryId, ManufacturerId, IsPrescription, MinStock) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
        db.executeNonQuery(query, { productName, mnn, dosageForm, dosage, barcode, categoryId, manufacturerId, isPrescription ? 1 : 0, minStock });
        std::wcout << L"\nТовар успешно добавлен!\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what()); std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void ProductService::editProduct(int productId) {
    std::wstring query = L"SELECT ProductName, MNN, DosageForm, Dosage, Barcode, MinStock FROM Products WHERE ProductId = ?";
    auto row = db.executeQuerySingle(query, { productId });
    if (!row.has_value()) { std::wcout << L"Товар не найден.\n"; return; }

    std::wcout << L"\n=== Редактирование товара ID: " << productId << L" ===\n";
    std::wstring productName, mnn, dosageForm, dosage, barcode;
    std::wcout << L"Новое наименование (Enter чтобы оставить): "; std::wcin.ignore(); std::getline(std::wcin, productName);
    if (productName.empty()) productName = std::get<std::wstring>((*row)[0]);

    std::wcout << L"Новый МНН: "; std::getline(std::wcin, mnn); if (mnn.empty()) mnn = std::holds_alternative<std::nullptr_t>((*row)[1]) ? L"" : std::get<std::wstring>((*row)[1]);
    std::wcout << L"Новая форма: "; std::getline(std::wcin, dosageForm); if (dosageForm.empty()) dosageForm = std::holds_alternative<std::nullptr_t>((*row)[2]) ? L"" : std::get<std::wstring>((*row)[2]);
    std::wcout << L"Новая дозировка: "; std::getline(std::wcin, dosage); if (dosage.empty()) dosage = std::holds_alternative<std::nullptr_t>((*row)[3]) ? L"" : std::get<std::wstring>((*row)[3]);
    std::wcout << L"Новый штрих-код: "; std::getline(std::wcin, barcode); if (barcode.empty()) barcode = std::holds_alternative<std::nullptr_t>((*row)[4]) ? L"" : std::get<std::wstring>((*row)[4]);

    int minStock; std::wcout << L"Новый мин. остаток: "; std::wcin >> minStock;

    try {
        std::wstring upd = L"UPDATE Products SET ProductName=?, MNN=?, DosageForm=?, Dosage=?, Barcode=?, MinStock=? WHERE ProductId=?";
        db.executeNonQuery(upd, { productName, mnn, dosageForm, dosage, barcode, minStock, productId });
        std::wcout << L"Товар обновлён.\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what()); std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void ProductService::deleteProduct(int productId) {
    try {
        std::wstring query = L"DELETE FROM Products WHERE ProductId = ?";
        db.executeNonQuery(query, { productId });
        std::wcout << L"Товар успешно удалён.\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what());
        std::wcout << L"Ошибка удаления (возможно, есть связанные партии или продажи): " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void ProductService::searchProduct() {
    std::wcout << L"\n=== Поиск товара ===\n";
    std::wcout << L"1. По наименованию\n2. По МНН\n3. По штрих-коду\n4. По производителю\nВаш выбор: ";
    int choice; std::wcin >> choice;
    std::wstring query; std::vector<Value> params;

    if (choice == 1) {
        std::wstring name; std::wcout << L"Часть наименования: "; std::wcin.ignore(); std::getline(std::wcin, name);
        query = L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, p.Dosage, m.ManufacturerName, c.CategoryName, p.Barcode, ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0), ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId), 0) FROM Products p INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId INNER JOIN Categories c ON p.CategoryId = c.CategoryId WHERE p.ProductName LIKE ?";
        params = { L"%" + name + L"%" };
    }
    else if (choice == 3) {
        std::wstring barcode; std::wcout << L"Штрих-код: "; std::wcin >> barcode;
        query = L"SELECT p.ProductId, p.ProductName, p.MNN, p.DosageForm, p.Dosage, m.ManufacturerName, c.CategoryName, p.Barcode, ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0), ISNULL((SELECT TOP 1 b.SalePrice FROM Batches b WHERE b.ProductId = p.ProductId), 0) FROM Products p INNER JOIN Manufacturers m ON p.ManufacturerId = m.ManufacturerId INNER JOIN Categories c ON p.CategoryId = c.CategoryId WHERE p.Barcode = ?";
        params = { barcode };
    }
    else {
        std::wcout << L"Неверный выбор.\n"; return;
    }

    auto rows = db.executeQuery(query, params);
    if (rows.empty()) { std::wcout << L"Товары не найдены.\n"; return; }
    std::wcout << L"\nНайдено: " << rows.size() << L"\n";
    for (const auto& row : rows) {
        ProductItem p;
        p.productId = std::get<int>(row[0]); p.productName = std::get<std::wstring>(row[1]);
        p.mnn = std::holds_alternative<std::nullptr_t>(row[2]) ? L"" : std::get<std::wstring>(row[2]);
        p.dosageForm = std::holds_alternative<std::nullptr_t>(row[3]) ? L"" : std::get<std::wstring>(row[3]);
        p.dosage = std::holds_alternative<std::nullptr_t>(row[4]) ? L"" : std::get<std::wstring>(row[4]);
        p.manufacturerName = std::get<std::wstring>(row[5]); p.categoryName = std::get<std::wstring>(row[6]);
        p.barcode = std::holds_alternative<std::nullptr_t>(row[7]) ? L"" : std::get<std::wstring>(row[7]);
        p.totalQuantity = std::get<int>(row[8]); p.salePrice = std::get<double>(row[9]);
        printProduct(p);
    }
}

void ProductService::showLowStock() {
    std::wstring query = L"SELECT p.ProductId, p.ProductName, ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0) AS TotalQty, p.MinStock FROM Products p WHERE ISNULL((SELECT SUM(b.Quantity) FROM Batches b WHERE b.ProductId = p.ProductId), 0) <= p.MinStock ORDER BY TotalQty ASC";
    auto rows = db.executeQuery(query);
    if (rows.empty()) { std::wcout << L"\nВсе товары имеют достаточный остаток.\n"; return; }
    std::wcout << L"\n=== Товары с низким остатком ===\nID | Наименование | Остаток | Мин. остаток\n";
    for (const auto& row : rows) {
        std::wcout << std::get<int>(row[0]) << L" | " << std::get<std::wstring>(row[1]) << L" | " << std::get<int>(row[2]) << L" | " << std::get<int>(row[3]) << L"\n";
    }
}