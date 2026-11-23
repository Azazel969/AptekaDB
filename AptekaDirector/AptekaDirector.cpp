#include <iostream>
#include <optional>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

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
#include "SupplyService.h"

static std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void setupConsoleEncoding() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
    SetConsoleOutputCP(1200);
    SetConsoleCP(1200);
    setlocale(LC_ALL, "Russian");
}

void showMainMenu(const Models::User& user) {
    std::wcout << L"\n========================================\n";
    std::wcout << L" Программная система директора аптеки\n";
    std::wcout << L" Пользователь: " << user.fullName << L" (" << (int)user.role << L")\n";
    std::wcout << L"========================================\n";
    std::wcout << L"1. Товары\n";
    std::wcout << L"2. Поставщики\n";
    if (user.canAccess(L"ManageSupplies")) std::wcout << L"3. Поставки (Приёмка)\n";
    std::wcout << L"4. Регистрация продажи / Возврата\n";
    std::wcout << L"5. Отчёты\n";
    if (user.canAccess(L"ManageUsers")) std::wcout << L"6. Сотрудники\n";
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
                catch (const std::exception& e) {
                    // Конвертация UTF-8 сообщения в UTF-16 для корректного вывода
                    std::wstring errorMsg = utf8_to_wstring(e.what());
                    std::wcout << L"Ошибка: " << errorMsg << L"\n";
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
            int choice; std::wcin >> choice;

            switch (choice) {
            case 1: {
                std::wcout << L"\n=== Товары ===\n1. Показать все\n2. Добавить\n3. Редактировать\n4. Удалить\n5. Поиск\n6. Низкий остаток\n0. Назад\nВыбор: ";
                int sub; std::wcin >> sub;
                ProductService ps(db);
                if (sub == 1) ps.showAll();
                else if (sub == 2) ps.addProduct();
                else if (sub == 3) { int id; std::wcout << L"ID товара: "; std::wcin >> id; ps.editProduct(id); }
                else if (sub == 4) { int id; std::wcout << L"ID товара: "; std::wcin >> id; ps.deleteProduct(id); }
                else if (sub == 5) ps.searchProduct();
                else if (sub == 6) ps.showLowStock();
                break;
            }
            case 2: {
                std::wcout << L"\n=== Поставщики ===\n1. Показать все\n2. Добавить\n3. Редактировать\n4. Удалить\n5. Поиск\n0. Назад\nВыбор: ";
                int sub; std::wcin >> sub;
                SupplierService ss(db);
                if (sub == 1) ss.showAll();
                else if (sub == 2) ss.addSupplier();
                else if (sub == 3) { int id; std::wcout << L"ID поставщика: "; std::wcin >> id; ss.editSupplier(id); }
                else if (sub == 4) { int id; std::wcout << L"ID поставщика: "; std::wcin >> id; ss.deleteSupplier(id); }
                else if (sub == 5) ss.searchSupplier();
                break;
            }
            case 3: {
                if (!currentUser->canAccess(L"ManageSupplies")) { std::wcout << L"Доступ запрещён.\n"; break; }
                SupplyService sups(db);
                sups.registerSupply(currentUser->userId);
                break;
            }
            case 4: {
                if (!currentUser->canAccess(L"RegisterSale")) { std::wcout << L"Доступ запрещён.\n"; break; }
                std::wcout << L"\n1. Регистрация продажи\n2. Регистрация возврата\n0. Назад\nВыбор: ";
                int sub; std::wcin >> sub;
                if (sub == 1 || sub == 2) {
                    int productId, quantity; std::wstring receiptNo;
                    std::wcout << L"ID товара: "; std::wcin >> productId;
                    std::wcout << L"Количество: "; std::wcin >> quantity;
                    std::wcout << L"Номер чека (Enter для авто): "; std::wcin.ignore(); std::getline(std::wcin, receiptNo);
                    if (receiptNo.empty()) receiptNo = L"CHK-" + std::to_wstring(GetTickCount());

                    try {
                        SaleService ss(db);
                        if (sub == 1) {
                            int id = ss.registerSale(productId, quantity, currentUser->userId, receiptNo);
                            std::wcout << L"Продажа зарегистрирована. ID: " << id << L"\n";
                        }
                        else {
                            int id = ss.registerReturn(productId, quantity, currentUser->userId, receiptNo);
                            std::wcout << L"Возврат зарегистрирован. ID: " << id << L"\n";
                        }
                    }
                    catch (const InsufficientStockException& e) {
                        std::string msg(e.what()); std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
                    }
                    catch (const std::exception& e) {
                        std::string msg(e.what()); std::wcout << L"Ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
                    }
                }
                break;
            }
            case 5: {
                if (!currentUser->canAccess(L"ViewReports")) { std::wcout << L"Доступ запрещён.\n"; break; }
                std::wcout << L"\n=== Отчёты ===\n1. Истекающие сроки\n2. Продажи за период\n3. Рентабельность\n0. Назад\nВыбор: ";
                int sub; std::wcin >> sub;
                ReportService rs(db);
                if (sub == 1) {
                    auto items = rs.getExpiringItems(Config::DEFAULT_EXPIRY_WARNING_DAYS);
                    std::wcout << L"\nТовары с истекающим сроком:\n";
                    for (const auto& item : items) std::wcout << item.productName << L" | " << item.batchNumber << L" | ост.: " << item.quantity << L" | дней: " << item.daysUntilExpiry << L"\n";
                }
                else if (sub == 2 || sub == 3) {
                    std::wstring start, end;
                    std::wcout << L"Дата начала (ГГГГ-ММ-ДД): "; std::wcin >> start;
                    std::wcout << L"Дата конца (ГГГГ-ММ-ДД): "; std::wcin >> end;
                    if (sub == 2) {
                        auto rep = rs.getSalesReport(start, end);
                        for (const auto& r : rep) std::wcout << r.productName << L" | Кол-во: " << r.totalQuantity << L" | Выручка: " << r.totalRevenue << L"\n";
                    }
                    else {
                        auto rep = rs.getProfitabilityReport(start, end);
                        for (const auto& r : rep) std::wcout << r.productName << L" | Затраты: " << r.totalPurchaseCost << L" | Выручка: " << r.totalRevenue << L" | Прибыль: " << r.profit << L"\n";
                    }
                }
                break;
            }
            case 6: {
                if (!currentUser->canAccess(L"ManageUsers")) { std::wcout << L"Доступ запрещён.\n"; break; }
                std::wcout << L"\n=== Сотрудники ===\n1. Показать всех\n2. Заблокировать\n3. Разблокировать\n4. Журнал операций\n0. Назад\nВыбор: ";
                int sub; std::wcin >> sub;
                UserService us(db);
                if (sub == 1) us.showAllUsers();
                else if (sub == 2) us.blockUser();
                else if (sub == 3) us.unblockUser();
                else if (sub == 4) us.showAuditLog();
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
        return 0;
    }
    catch (const std::exception& e) {
        std::string msg(e.what()); std::wcerr << L"Критическая ошибка: " << std::wstring(msg.begin(), msg.end()) << L"\n";
        return 2;
    }
}