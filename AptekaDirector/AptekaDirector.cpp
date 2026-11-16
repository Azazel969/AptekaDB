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
            case 0:
                running = false;
                std::wcout << L"Завершение работы...\n";
                break;
            default:
                std::wcout << L"Функция в разработке.\n";
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