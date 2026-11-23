// ============================================================
// «Программная система для директора аптеки»
// ============================================================
#include <iostream>
#include <optional>
#include <windows.h>

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

        // Цикл авторизации
        while (!currentUser.has_value()) {
            std::wcout << L"\n========================================\n";
            std::wcout << L" Программная система директора аптеки\n";
            std::wcout << L"========================================\n";
            std::wcout << L"1. Войти в систему\n";
            std::wcout << L"2. Зарегистрировать нового пользователя\n";
            std::wcout << L"0. Выход\n";
            std::wcout << L"========================================\n";
            std::wcout << L"Ваш выбор: ";

            int authChoice;
            std::wcin >> authChoice;

            if (authChoice == 0) {
                std::wcout << L"Завершение работы...\n";
                return 0;
            }
            else if (authChoice == 1) {
                std::wstring login, password;
                std::wcout << L"\nЛогин: ";
                std::wcin >> login;
                std::wcout << L"Пароль: ";
                std::wcin >> password;

                try {
                    currentUser = authService.login(login, password);
                    if (!currentUser.has_value()) {
                        std::wcout << L"Неверный логин или пароль.\n";
                    }
                }
                catch (const std::exception& e) {
                    std::string msg(e.what());
                    std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
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

        // Основной цикл работы
        bool running = true;
        while (running) {
            showMainMenu(*currentUser);
            int choice;
            std::wcin >> choice;

            switch (choice) {
            case 1: {
                // === ТОВАРЫ ===
                std::wcout << L"\n=== Раздел: Товары ===\n";
                std::wcout << L"1. Показать все товары\n";
                std::wcout << L"2. Добавить новый товар\n";
                std::wcout << L"3. Поиск товара\n";
                std::wcout << L"4. Товары с низким остатком\n";
                std::wcout << L"0. Назад\n";
                std::wcout << L"Ваш выбор: ";

                int subChoice;
                std::wcin >> subChoice;

                ProductService productService(db);

                switch (subChoice) {
                case 1: productService.showAll(); break;
                case 2: productService.addProduct(); break;
                case 3: productService.searchProduct(); break;
                case 4: productService.showLowStock(); break;
                case 0: break;
                default: std::wcout << L"Неверный выбор.\n";
                }
                break;
            }
            case 2: {
                // === ПОСТАВЩИКИ ===
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
                // Регистрация продажи (уже работает)
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
                    int saleId = saleService.registerSale(productId, quantity, currentUser->userId);
                    std::wcout << L"Продажа зарегистрирована. ID: " << saleId << L"\n";
                }
                catch (const InsufficientStockException& e) {
                    std::string msg(e.what());
                    std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
                }
                catch (const std::exception& e) {
                    std::string msg(e.what());
                    std::wcout << L"Критическая ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
                }
                break;
            }
            case 4: {
                // Отчёты (уже работает)
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
                // === СОТРУДНИКИ (только для директора) ===
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
        std::string msg(e.what());
        std::wcerr << L"Ошибка базы данных: " << std::wstring(msg.begin(), msg.end()) << L"\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::string msg(e.what());
        std::wcerr << L"Критическая ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
        return 2;
    }
}