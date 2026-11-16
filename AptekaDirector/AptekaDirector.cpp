// ============================================================
// AptekaDirector.cpp — точка входа в программную систему
// «Программная система для директора аптеки»
// Орловский М.А., группа 582572, БГУИР, 2026
// ============================================================
#include <iostream>
#include <optional>
#include <windows.h>

// Подключение заголовочных файлов проекта
// ВАЖНО: Если файлы лежат в подпапках, укажите пути, например: "models/User.h"
#include "config.h"
#include "User.h"
#include "Database.h"
#include "AuthService.h"
#include "SaleService.h"
#include "ReportService.h"

// Настройка корректного отображения кириллицы в консоли Windows
void setupConsoleEncoding() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    setlocale(LC_ALL, "Russian");
}

// Главное меню системы
void showMainMenu(const Models::User& user) {
    std::wcout << L"\n========================================\n";
    std::wcout << L"  Программная система директора аптеки\n";
    std::wcout << L"  Пользователь: " << user.fullName << L"\n";
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

        // Инициализация подключения к SQL Server 2022
        Db::Database db;
        db.connect(Config::CONNECTION_STRING);
        std::wcout << L"Подключение к серверу СУБД установлено.\n";

        Services::AuthService authService(db);
        std::optional<Models::User> currentUser;

        // Главный цикл авторизации/регистрации
        while (!currentUser.has_value()) {
            std::wcout << L"\n========================================\n";
            std::wcout << L"  Программная система директора аптеки\n";
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
                // АВТОРИЗАЦИЯ
                std::wstring login, password;
                std::wcout << L"\nЛогин: ";
                std::wcin >> login;
                std::wcout << L"Пароль: ";
                std::wcin >> password;

                currentUser = authService.login(login, password);
                if (!currentUser.has_value()) {
                    std::wcout << L"Неверный логин или пароль, или учётная запись заблокирована.\n";
                }
            }
            else if (authChoice == 2) {
                // РЕГИСТРАЦИЯ
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

                bool success = authService.registerUser(login, password, fullName, role);

                if (success) {
                    std::wcout << L"\n Пользователь успешно зарегистрирован!\n";
                    std::wcout << L"Теперь вы можете войти в систему.\n";
                }
                else {
                    std::wcout << L"\n Ошибка регистрации. Возможно, пользователь с таким логином уже существует.\n";
                }
            }
            else {
                std::wcout << L"Неверный выбор. Попробуйте снова.\n";
            }
        }

        std::wcout << L"\nДобро пожаловать, " << currentUser->fullName << L"!\n";

        // Основной цикл работы приложения
        bool running = true;
        while (running) {
            showMainMenu(*currentUser);
            int choice;
            std::wcin >> choice;

            switch (choice) {
            case 3: {
                // Регистрация продажи с FEFO-списанием
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
                    Services::SaleService saleService(db);
                    int saleId = saleService.registerSale(
                        productId, quantity, currentUser->userId);
                    std::wcout << L"Продажа зарегистрирована. ID: "
                        << saleId << L"\n";
                }
                catch (const Services::InsufficientStockException& e) {
                    std::wcout << L"Ошибка: " << e.what() << L"\n";
                }
                catch (const std::exception& e) {
                    std::wcout << L"Критическая ошибка: " << e.what() << L"\n";
                }
                break;
            }
            case 4: {
                // Формирование отчёта о истекающих сроках годности
                if (!currentUser->canAccess(L"ViewReports")) {
                    std::wcout << L"Доступ запрещён.\n";
                    break;
                }
                Services::ReportService reportService(db);
                auto items = reportService.getExpiringItems(
                    Config::DEFAULT_EXPIRY_WARNING_DAYS);

                std::wcout << L"\nТовары с истекающим сроком годности:\n";
                std::wcout << L"----------------------------------------\n";

                if (items.empty()) {
                    std::wcout << L"Нет товаров с истекающим сроком годности.\n";
                }
                else {
                    for (const auto& item : items) {
                        std::wcout << item.productName << L" | "
                            << item.manufacturerName << L" | "
                            << L"серия " << item.batchNumber << L" | "
                            << L"ост.: " << item.quantity << L" | "
                            << L"дней: " << item.daysUntilExpiry
                            << L"\n";
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
    catch (const Db::DatabaseException& e) {
        std::wcerr << L"Ошибка базы данных: " << e.wwhat() << L"\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Критическая ошибка: " << e.what() << L"\n";
        return 2;
    }
}