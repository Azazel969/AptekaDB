#pragma once
#include <stdexcept>
#include <string>

class InsufficientStockException : public std::runtime_error {
public:
    explicit InsufficientStockException(const std::string& msg) : std::runtime_error(msg) {}
};

class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& msg) : std::runtime_error(msg) {}
};