#include <iostream>
#include <optional>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <string>

#include "config.h"
#include "User.h"
#include "Database.h"
#include "AuthService.h"
#include "SaleService.h"
#include "ReportService.h"
#include "Exceptions.h"
#include "ProductService.h"
#include "SupplierService.h"
#include "UserService.h"

// Универсальная функция для конвертации узких строк в wide strings
static std::wstring convert_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    if (size_needed > 0) {
        std::wstring wstrTo(size_needed, 0);
        if (MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed) > 0) {
            return wstrTo;
        }
    }
    size_needed = MultiByteToWideChar(1251, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(1251, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void setupConsoleEncoding() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    setlocale(LC_ALL, "Russian");
}

void showMainMenu(const Models::User& user) {
    std::wcout << L"\n========================================\n";
    std::wcout << L" Программная система директора аптеки\n";
    std::wcout << L" Пользователь: " << user.fullName << L"\n";
    std::wcout << L"========================================\n";
    std::wcout << L"1. Товары\n";
    std::wcout << L"2. Поставщики\n";
    std::wcout << L"3. Регистрация продажи\n";
    std::wcout << L"4. Отчёты\n";
    if (user.canAccess(L"ManageUsers")) {
        std::wcout << L"5. Сотрудники\n";
    }
    std::wcout << L"0. Выход\n";
    std::wcout << L"========================================\n";
    std::wcout << L"Ваш выбор: ";
}

int main() {
    try {
        setupConsoleEncoding();

        Database db;
        db.connect(Config::CONNECTION_STRING);
        std::wcout << L"Подключение к серверу СУБД установлено.\n";

        AuthService authService(db);
        std::optional<Models::User> currentUser;

        while (!currentUser.has_value()) {
            std::wcout << L"\n=== Авторизация ===\n";
            std::wcout << L"1. Войти\n";
            std::wcout << L"2. Регистрация\n";
            std::wcout << L"0. Выход\n";
            std::wcout << L"Выбор: ";

            int authChoice;
            std::wcin >> authChoice;

            if (authChoice == 0) {
                std::wcout << L"Завершение работы...\n";
                return 0;
            }
            else if (authChoice == 1) {
                std::wstring login, password;
                std::wcout << L"Логин: ";
                std::wcin >> login;
                std::wcout << L"Пароль: ";
                std::wcin >> password;

                try {
                    currentUser = authService.login(login, password);
                    if (!currentUser.has_value()) {
                        std::wcout << L"Неверный логин или пароль.\n";
                    }
                }
                catch (const AppException& e) {
                    std::wcout << L"Ошибка: " << e.wwhat() << L"\n";
                }
                catch (const std::exception& e) {
                    std::wcout << L"Системная ошибка: " << convert_to_wstring(e.what()) << L"\n";
                }
            }
            else if (authChoice == 2) {
                std::wcout << L"\n=== Регистрация нового пользователя ===\n";
                std::wstring login, password, fullName;
                int roleChoice;
                std::wcin.ignore();

                std::wcout << L"Логин: ";
                std::getline(std::wcin, login);
                std::wcout << L"Пароль: ";
                std::getline(std::wcin, password);
                std::wcout << L"ФИО: ";
                std::getline(std::wcin, fullName);

                std::wcout << L"\nВыберите роль:\n";
                std::wcout << L"1. Директор\n";
                std::wcout << L"2. Заведующий\n";
                std::wcout << L"3. Фармацевт\n";
                std::wcout << L"4. Кладовщик\n";
                std::wcout << L"Ваш выбор: ";
                std::wcin >> roleChoice;

                Models::UserRole role;
                switch (roleChoice) {
                case 1: role = Models::UserRole::Director; break;
                case 2: role = Models::UserRole::Manager; break;
                case 3: role = Models::UserRole::Pharmacist; break;
                case 4: role = Models::UserRole::Storekeeper; break;
                default: role = Models::UserRole::Pharmacist; break;
                }

                if (authService.registerUser(login, password, fullName, role)) {
                    std::wcout << L"\n Пользователь успешно зарегистрирован!\n";
                }
                else {
                    std::wcout << L"\n Ошибка регистрации. Возможно, логин уже существует.\n";
                }
            }
            else {
                std::wcout << L"Неверный выбор. Попробуйте снова.\n";
            }
        }

        std::wcout << L"\nДобро пожаловать, " << currentUser->fullName << L"!\n";

        bool running = true;
        while (running) {
            showMainMenu(*currentUser);
            int choice;
            std::wcin >> choice;

            switch (choice) {
            case 1: {
                std::wcout << L"\n=== Раздел: Товары ===\n";
                std::wcout << L"1. Показать все товары\n";
                std::wcout << L"2. Добавить новый товар\n";
                std::wcout << L"3. Редактировать товар\n";
                std::wcout << L"4. Удалить товар\n";
                std::wcout << L"5. Поиск товара\n";
                std::wcout << L"6. Товары с низким остатком\n";
                std::wcout << L"0. Назад\n";
                std::wcout << L"Ваш выбор: ";

                int subChoice;
                std::wcin >> subChoice;
                ProductService productService(db);

                switch (subChoice) {
                case 1: productService.showAll(); break;
                case 2: productService.addProduct(); break;
                case 3: { int id; std::wcout << L"ID товара: "; std::wcin >> id; productService.editProduct(id); break; }
                case 4: { int id; std::wcout << L"ID товара: "; std::wcin >> id; productService.deleteProduct(id); break; }
                case 5: productService.searchProduct(); break;
                case 6: productService.showLowStock(); break;
                case 0: break;
                default: std::wcout << L"Неверный выбор.\n";
                }
                break;
            }
            case 2: {
                std::wcout << L"\n=== Раздел: Поставщики ===\n";
                std::wcout << L"1. Показать всех поставщиков\n";
                std::wcout << L"2. Добавить нового поставщика\n";
                std::wcout << L"3. Поиск поставщика\n";
                std::wcout << L"0. Назад\n";
                std::wcout << L"Ваш выбор: ";

                int subChoice;
                std::wcin >> subChoice;
                SupplierService supplierService(db);

                switch (subChoice) {
                case 1: supplierService.showAll(); break;
                case 2: supplierService.addSupplier(); break;
                case 3: supplierService.searchSupplier(); break;
                case 0: break;
                default: std::wcout << L"Неверный выбор.\n";
                }
                break;
            }
            case 3: {
                if (!currentUser->canAccess(L"RegisterSale")) {
                    std::wcout << L"Доступ запрещён.\n";
                    break;
                }
                int productId, quantity;
                std::wcout << L"ID товара: ";
                std::wcin >> productId;
                std::wcout << L"Количество: ";
                std::wcin >> quantity;

                try {
                    SaleService saleService(db);
                    int saleId = saleService.registerSale(productId, quantity, currentUser->userId, L"");
                    std::wcout << L"Продажа зарегистрирована. ID: " << saleId << L"\n";
                }
                catch (const InsufficientStockException& e) {
                    std::wcout << L"Ошибка: " << e.wwhat() << L"\n";
                }
                catch (const AppException& e) {
                    std::wcout << L"Ошибка: " << e.wwhat() << L"\n";
                }
                catch (const std::exception& e) {
                    std::wcout << L"Критическая ошибка: " << convert_to_wstring(e.what()) << L"\n";
                }
                break;
            }
            case 4: {
                if (!currentUser->canAccess(L"ViewReports")) {
                    std::wcout << L"Доступ запрещён.\n";
                    break;
                }
                ReportService reportService(db);
                auto items = reportService.getExpiringItems(Config::DEFAULT_EXPIRY_WARNING_DAYS);

                std::wcout << L"\nТовары с истекающим сроком годности:\n";
                std::wcout << L"--------------------------------------\n";
                if (items.empty()) {
                    std::wcout << L"Нет товаров с истекающим сроком годности.\n";
                }
                else {
                    for (const auto& item : items) {
                        std::wcout << item.productName << L" | "
                            << item.manufacturerName << L" | "
                            << L"серия " << item.batchNumber << L" | "
                            << L"ост.: " << item.quantity << L" | "
                            << L"дней: " << item.daysUntilExpiry << L"\n";
                    }
                }
                break;
            }
            case 5: {
                if (!currentUser->canAccess(L"ManageUsers")) {
                    std::wcout << L"Доступ запрещён. Функция доступна только директору.\n";
                    break;
                }

                std::wcout << L"\n=== Раздел: Сотрудники ===\n";
                std::wcout << L"1. Показать всех сотрудников\n";
                std::wcout << L"2. Заблокировать пользователя\n";
                std::wcout << L"3. Разблокировать пользователя\n";
                std::wcout << L"4. Журнал операций\n";
                std::wcout << L"0. Назад\n";
                std::wcout << L"Ваш выбор: ";

                int subChoice;
                std::wcin >> subChoice;
                UserService userService(db);

                switch (subChoice) {
                case 1: userService.showAllUsers(); break;
                case 2: userService.blockUser(); break;
                case 3: userService.unblockUser(); break;
                case 4: userService.showAuditLog(); break;
                case 0: break;
                default: std::wcout << L"Неверный выбор.\n";
                }
                break;
            }
            case 0:
                running = false;
                std::wcout << L"Завершение работы...\n";
                break;
            default:
                std::wcout << L"Неверный выбор.\n";
            }
        }

        db.disconnect();
        std::wcout << L"Соединение с сервером закрыто.\n";
        return 0;
    }
    catch (const DatabaseException& e) {
        std::wcerr << L"Ошибка базы данных: " << e.wwhat() << L"\n";
        return 1;
    }
    catch (const AppException& e) {
        std::wcerr << L"Ошибка: " << e.wwhat() << L"\n";
        return 2;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Критическая ошибка: " << convert_to_wstring(e.what()) << L"\n";
        return 3;
    }
}