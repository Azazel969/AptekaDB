#include "SupplierService.h"
#include <iostream>

SupplierService::SupplierService(Database& db) : db(db) {}

void SupplierService::showAll() {
    std::wstring query = L"SELECT SupplierId, SupplierName, LegalAddress, Phone, Email, ContractNo, ContractDate, ContractExpiry, ContactManager, Reliability FROM Suppliers ORDER BY SupplierName";
    auto rows = db.executeQuery(query);
    if (rows.empty()) { std::wcout << L"Список поставщиков пуст.\n"; return; }
    std::wcout << L"\n=== Список поставщиков ===\nID | Наименование | Адрес | Телефон | Email | Договор № | Срок | Менеджер | Надёжность\n";
    for (const auto& row : rows) {
        std::wcout << std::get<int>(row[0]) << L" | " << std::get<std::wstring>(row[1]) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[2]) ? L"" : std::get<std::wstring>(row[2])) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[3]) ? L"" : std::get<std::wstring>(row[3])) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[4]) ? L"" : std::get<std::wstring>(row[4])) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[5]) ? L"" : std::get<std::wstring>(row[5])) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[7]) ? L"" : std::get<std::wstring>(row[7])) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[8]) ? L"" : std::get<std::wstring>(row[8])) << L" | "
            << (std::holds_alternative<std::nullptr_t>(row[9]) ? L"н/д" : std::to_wstring(std::get<double>(row[9]))) << L"\n";
    }
}

void SupplierService::addSupplier() {
    std::wcout << L"\n=== Добавление поставщика ===\n";
    std::wstring name, address, phone, email, contractNo, contractDate, contractExpiry, manager;
    double reliability;
    std::wcout << L"Наименование: "; std::wcin.ignore(); std::getline(std::wcin, name);
    std::wcout << L"Адрес: "; std::getline(std::wcin, address);
    std::wcout << L"Телефон: "; std::getline(std::wcin, phone);
    std::wcout << L"Email: "; std::getline(std::wcin, email);
    std::wcout << L"Номер договора: "; std::getline(std::wcin, contractNo);
    std::wcout << L"Дата договора (ГГГГ-ММ-ДД): "; std::getline(std::wcin, contractDate);
    std::wcout << L"Срок договора (ГГГГ-ММ-ДД): "; std::getline(std::wcin, contractExpiry);
    std::wcout << L"Менеджер: "; std::getline(std::wcin, manager);
    std::wcout << L"Надёжность (0.00-1.00): "; std::wcin >> reliability;

    try {
        std::wstring q = L"INSERT INTO Suppliers (SupplierName, LegalAddress, Phone, Email, ContractNo, ContractDate, ContractExpiry, ContactManager, Reliability) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
        db.executeNonQuery(q, { name, address, phone, email, contractNo, contractDate, contractExpiry, manager, reliability });
        std::wcout << L"Поставщик добавлен!\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what()); std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void SupplierService::editSupplier(int supplierId) {
    std::wstring query = L"SELECT SupplierName, LegalAddress, Phone, Email, ContractNo, ContractDate, ContractExpiry, ContactManager, Reliability FROM Suppliers WHERE SupplierId = ?";
    auto row = db.executeQuerySingle(query, { supplierId });
    if (!row.has_value()) { std::wcout << L"Поставщик не найден.\n"; return; }

    std::wcout << L"\n=== Редактирование поставщика ID: " << supplierId << L" ===\n";
    std::wstring name, address, phone, email, contractNo, contractDate, contractExpiry, manager;
    double reliability;
    std::wcout << L"Новое наименование (Enter чтобы оставить): "; std::wcin.ignore(); std::getline(std::wcin, name);
    if (name.empty()) name = std::get<std::wstring>((*row)[0]);
    std::wcout << L"Новый адрес: "; std::getline(std::wcin, address); if (address.empty()) address = std::holds_alternative<std::nullptr_t>((*row)[1]) ? L"" : std::get<std::wstring>((*row)[1]);
    std::wcout << L"Новый телефон: "; std::getline(std::wcin, phone); if (phone.empty()) phone = std::holds_alternative<std::nullptr_t>((*row)[2]) ? L"" : std::get<std::wstring>((*row)[2]);
    std::wcout << L"Новый Email: "; std::getline(std::wcin, email); if (email.empty()) email = std::holds_alternative<std::nullptr_t>((*row)[3]) ? L"" : std::get<std::wstring>((*row)[3]);
    std::wcout << L"Новый номер договора: "; std::getline(std::wcin, contractNo); if (contractNo.empty()) contractNo = std::holds_alternative<std::nullptr_t>((*row)[4]) ? L"" : std::get<std::wstring>((*row)[4]);
    std::wcout << L"Новая дата договора: "; std::getline(std::wcin, contractDate); if (contractDate.empty()) contractDate = std::holds_alternative<std::nullptr_t>((*row)[5]) ? L"" : std::get<std::wstring>((*row)[5]);
    std::wcout << L"Новый срок договора: "; std::getline(std::wcin, contractExpiry); if (contractExpiry.empty()) contractExpiry = std::holds_alternative<std::nullptr_t>((*row)[6]) ? L"" : std::get<std::wstring>((*row)[6]);
    std::wcout << L"Новый менеджер: "; std::getline(std::wcin, manager); if (manager.empty()) manager = std::holds_alternative<std::nullptr_t>((*row)[7]) ? L"" : std::get<std::wstring>((*row)[7]);
    std::wcout << L"Новая надёжность: "; std::wcin >> reliability;

    try {
        std::wstring upd = L"UPDATE Suppliers SET SupplierName=?, LegalAddress=?, Phone=?, Email=?, ContractNo=?, ContractDate=?, ContractExpiry=?, ContactManager=?, Reliability=? WHERE SupplierId=?";
        db.executeNonQuery(upd, { name, address, phone, email, contractNo, contractDate, contractExpiry, manager, reliability, supplierId });
        std::wcout << L"Поставщик обновлён.\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what()); std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void SupplierService::deleteSupplier(int supplierId) {
    try {
        db.executeNonQuery(L"DELETE FROM Suppliers WHERE SupplierId = ?", { supplierId });
        std::wcout << L"Поставщик удалён.\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what()); std::wcout << L"Ошибка удаления (есть связанные поставки или партии): " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void SupplierService::searchSupplier() {
    std::wcout << L"\n=== Поиск поставщика ===\n1. По наименованию\n2. По номеру договора\nВаш выбор: ";
    int choice; std::wcin >> choice;
    std::wstring query; std::vector<Value> params;
    if (choice == 1) {
        std::wstring name; std::wcout << L"Часть наименования: "; std::wcin.ignore(); std::getline(std::wcin, name);
        query = L"SELECT SupplierId, SupplierName, LegalAddress, Phone, Email, ContractNo, ContractExpiry FROM Suppliers WHERE SupplierName LIKE ?";
        params = { L"%" + name + L"%" };
    }
    else if (choice == 2) {
        std::wstring contractNo; std::wcout << L"Номер договора: "; std::wcin >> contractNo;
        query = L"SELECT SupplierId, SupplierName, LegalAddress, Phone, Email, ContractNo, ContractExpiry FROM Suppliers WHERE ContractNo = ?";
        params = { contractNo };
    }
    else { std::wcout << L"Неверный выбор.\n"; return; }

    auto rows = db.executeQuery(query, params);
    if (rows.empty()) { std::wcout << L"Не найдено.\n"; return; }
    for (const auto& row : rows) {
        std::wcout << L"ID: " << std::get<int>(row[0]) << L", Название: " << std::get<std::wstring>(row[1])
            << L", Договор: " << (std::holds_alternative<std::nullptr_t>(row[5]) ? L"" : std::get<std::wstring>(row[5])) << L"\n";
    }
}