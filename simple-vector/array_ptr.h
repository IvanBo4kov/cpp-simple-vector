#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <utility>

template <typename Type>
class ArrayPtr {
public:
    ArrayPtr() = default;

    explicit ArrayPtr(size_t size) {
        if (size == 0) {
            raw_ptr_ = nullptr;
        } else {
            raw_ptr_ = new Type[size];
        }
    }

    explicit ArrayPtr(Type* raw_ptr) noexcept : raw_ptr_{raw_ptr} {}

    explicit ArrayPtr(ArrayPtr&& other) noexcept : raw_ptr_(std::exchange(other.raw_ptr_, nullptr)) {}

    ArrayPtr& operator=(ArrayPtr&& other) noexcept {
        if (this != &other)
            raw_ptr_ = std::exchange(other.raw_ptr_, nullptr);
        return *this;
    }

    ArrayPtr(const ArrayPtr&) = delete;

    ~ArrayPtr() {
        delete[] raw_ptr_;
    }

    ArrayPtr& operator=(const ArrayPtr&) = delete;

    [[nodiscard]] Type* Release() noexcept {
        auto delete_array = raw_ptr_;
        raw_ptr_ = nullptr;
        return delete_array;
    }

    Type& operator[](size_t index) noexcept {
        return *(raw_ptr_ + index);
    }

    const Type& operator[](size_t index) const noexcept {
        return *(raw_ptr_ + index);
    }

    explicit operator bool() const {
        return raw_ptr_;
    }

    Type* Get() const noexcept {
        return raw_ptr_;
    }

    void swap(ArrayPtr& other) noexcept {
        auto temp = raw_ptr_;
        raw_ptr_ = other.raw_ptr_;
        other.raw_ptr_ = temp;
    }

private:
    Type* raw_ptr_ = nullptr;
};