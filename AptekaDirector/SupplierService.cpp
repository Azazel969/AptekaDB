#include "SupplierService.h"
#include <iostream>

SupplierService::SupplierService(Database& db) : db(db) {}

void SupplierService::showAll() {
    std::wstring query =
        L"SELECT SupplierId, SupplierName, LegalAddress, Phone, Email, "
        L"ContractNo, ContractDate, ContractExpiry, Reliability "
        L"FROM Suppliers ORDER BY SupplierName";

    auto rows = db.executeQuery(query);
    if (rows.empty()) {
        std::wcout << L"Список поставщиков пуст.\n";
        return;
    }

    std::wcout << L"\n=== Список поставщиков ===\n";
    std::wcout << L"ID | Наименование | Адрес | Телефон | Email | Договор № | Дата договора | Срок | Надёжность\n";
    std::wcout << L"--------------------------------------------------------------------------------------------\n";
    for (const auto& row : rows) {
        int id = std::get<int>(row[0]);
        std::wstring name = std::get<std::wstring>(row[1]);
        std::wstring address = std::holds_alternative<std::nullptr_t>(row[2]) ? L"" : std::get<std::wstring>(row[2]);
        std::wstring phone = std::holds_alternative<std::nullptr_t>(row[3]) ? L"" : std::get<std::wstring>(row[3]);
        std::wstring email = std::holds_alternative<std::nullptr_t>(row[4]) ? L"" : std::get<std::wstring>(row[4]);
        std::wstring contractNo = std::holds_alternative<std::nullptr_t>(row[5]) ? L"" : std::get<std::wstring>(row[5]);
        std::wstring contractDate = std::holds_alternative<std::nullptr_t>(row[6]) ? L"" : std::get<std::wstring>(row[6]);
        std::wstring contractExpiry = std::holds_alternative<std::nullptr_t>(row[7]) ? L"" : std::get<std::wstring>(row[7]);
        std::wstring reliability = std::holds_alternative<std::nullptr_t>(row[8]) ? L"н/д" : std::to_wstring(std::get<double>(row[8]));

        std::wcout << id << L" | " << name << L" | " << address << L" | " << phone
            << L" | " << email << L" | " << contractNo << L" | "
            << contractDate << L" | " << contractExpiry << L" | " << reliability << L"\n";
    }
}

void SupplierService::addSupplier() {
    std::wcout << L"\n=== Добавление нового поставщика ===\n";

    std::wstring name, address, phone, email, contractNo;
    std::wstring contractDate, contractExpiry;
    double reliability;

    std::wcout << L"Наименование организации: ";
    std::wcin.ignore();
    std::getline(std::wcin, name);

    std::wcout << L"Юридический адрес: ";
    std::getline(std::wcin, address);

    std::wcout << L"Телефон: ";
    std::getline(std::wcin, phone);

    std::wcout << L"Email: ";
    std::getline(std::wcin, email);

    std::wcout << L"Номер договора: ";
    std::getline(std::wcin, contractNo);

    std::wcout << L"Дата заключения договора (ГГГГ-ММ-ДД): ";
    std::getline(std::wcin, contractDate);

    std::wcout << L"Срок действия договора (ГГГГ-ММ-ДД): ";
    std::getline(std::wcin, contractExpiry);

    std::wcout << L"Рейтинг надёжности (от 0.00 до 1.00): ";
    std::wcin >> reliability;

    try {
        std::wstring query =
            L"INSERT INTO Suppliers (SupplierName, LegalAddress, Phone, Email, ContractNo, ContractDate, ContractExpiry, Reliability) "
            L"VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        db.executeNonQuery(query, { name, address, phone, email, contractNo, contractDate, contractExpiry, reliability });
        std::wcout << L"\nПоставщик успешно добавлен!\n";
    }
    catch (const std::exception& e) {
        std::string msg(e.what());
        std::wcout << L"Ошибка при добавлении: " << std::wstring(msg.begin(), msg.end()) << L"\n";
    }
}

void SupplierService::searchSupplier() {
    std::wcout << L"\n=== Поиск поставщика ===\n";
    std::wcout << L"1. По наименованию\n";
    std::wcout << L"2. По номеру договора\n";
    std::wcout << L"Ваш выбор: ";

    int choice;
    std::wcin >> choice;

    std::wstring query;
    std::vector<Value> params;

    if (choice == 1) {
        std::wstring name;
        std::wcout << L"Введите часть наименования: ";
        std::wcin.ignore();
        std::getline(std::wcin, name);
        query = L"SELECT SupplierId, SupplierName, LegalAddress, Phone, Email, ContractNo, ContractDate, ContractExpiry, Reliability "
            L"FROM Suppliers WHERE SupplierName LIKE ?";
        params = { L"%" + name + L"%" };
    }
    else if (choice == 2) {
        std::wstring contractNo;
        std::wcout << L"Введите номер договора: ";
        std::wcin >> contractNo;
        query = L"SELECT SupplierId, SupplierName, LegalAddress, Phone, Email, ContractNo, ContractDate, ContractExpiry, Reliability "
            L"FROM Suppliers WHERE ContractNo = ?";
        params = { contractNo };
    }
    else {
        std::wcout << L"Неверный выбор.\n";
        return;
    }

    auto rows = db.executeQuery(query, params);
    if (rows.empty()) {
        std::wcout << L"Поставщики не найдены.\n";
        return;
    }

    std::wcout << L"\nНайдено поставщиков: " << rows.size() << L"\n";
    for (const auto& row : rows) {
        std::wcout << L"ID: " << std::get<int>(row[0]) << L"\n";
        std::wcout << L"Наименование: " << std::get<std::wstring>(row[1]) << L"\n";
        std::wcout << L"Адрес: " << (std::holds_alternative<std::nullptr_t>(row[2]) ? L"" : std::get<std::wstring>(row[2])) << L"\n";
        std::wcout << L"Телефон: " << (std::holds_alternative<std::nullptr_t>(row[3]) ? L"" : std::get<std::wstring>(row[3])) << L"\n";
        std::wcout << L"Email: " << (std::holds_alternative<std::nullptr_t>(row[4]) ? L"" : std::get<std::wstring>(row[4])) << L"\n";
        std::wcout << L"Договор №: " << (std::holds_alternative<std::nullptr_t>(row[5]) ? L"" : std::get<std::wstring>(row[5])) << L"\n";
        std::wcout << L"Дата: " << (std::holds_alternative<std::nullptr_t>(row[6]) ? L"" : std::get<std::wstring>(row[6])) << L"\n";
        std::wcout << L"Срок: " << (std::holds_alternative<std::nullptr_t>(row[7]) ? L"" : std::get<std::wstring>(row[7])) << L"\n";
        std::wcout << L"Надёжность: " << (std::holds_alternative<std::nullptr_t>(row[8]) ? L"н/д" : std::to_wstring(std::get<double>(row[8]))) << L"\n";
        std::wcout << L"----------------------------------------\n";
    }
}