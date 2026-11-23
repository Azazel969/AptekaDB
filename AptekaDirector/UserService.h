#pragma once
#include "Database.h"
#include "User.h"

class UserService {
public:
    explicit UserService(Database& db);

    void showAllUsers();
    void blockUser();
    void unblockUser();
    void showAuditLog();

private:
    Database& db;
};
