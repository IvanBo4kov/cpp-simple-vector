#pragma once

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <initializer_list>

#include "array_ptr.h"

struct ReserveProxyObject {
    size_t capacity_to_reserve;
    explicit ReserveProxyObject(size_t capacity) : capacity_to_reserve(capacity) {}
};

ReserveProxyObject Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObject(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) : size_(size), capacity_(size), data_(size) {
        std::fill(data_.Get(), data_.Get() + size_, Type());
    }

    SimpleVector(size_t size, const Type& value) : size_(size), capacity_(size), data_(size) {
        std::fill(data_.Get(), data_.Get() + size_, value);
    }

    SimpleVector(std::initializer_list<Type> init) : size_(init.size()), capacity_(init.size()), data_(init.size()) {
        std::copy(init.begin(), init.end(), data_.Get());
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
        } else if (new_size <= capacity_) {
            for (size_t i = size_; i < new_size; ++i) {
                data_[i] = Type();
            }
            size_ = new_size;
        } else {  // new_size > capacity
            int new_capacity = std::max(new_size, capacity_ * 2);

            ArrayPtr<Type> new_array(new_capacity);
            for (size_t i = 0; i < size_; ++i) {
                new_array[i] = std::move(data_[i]);
            }
            for (size_t i = size_; i < new_size; ++i) {
                new_array[i] = Type();
            }

            data_.swap(new_array);
            capacity_ = new_capacity;
            size_ = new_size;
        }
    }

    Iterator begin() noexcept {
        return data_.Get();
    }

    Iterator end() noexcept {
        return data_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return data_.Get();
    }

    ConstIterator end() const noexcept {
        return data_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return data_.Get();
    }

    ConstIterator cend() const noexcept {
        return data_.Get() + size_;
    }

    SimpleVector(const SimpleVector& other) {
        SimpleVector<Type> cp(other.size_);
        std::copy(other.begin(), other.end(), cp.begin());

        size_ = cp.size_;
        capacity_ = cp.capacity_;
        data_.swap(cp.data_);
    }

    SimpleVector(SimpleVector&& other) noexcept
        : size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          data_(std::move(other.data_)) {}

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            auto temp(rhs);
            swap(temp);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& other) noexcept {
        if (this != &other) {
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
            data_ = std::move(other.data_);
        }
        return *this;
    }

    explicit SimpleVector(ReserveProxyObject wrapper) {
        capacity_ = wrapper.capacity_to_reserve;
        Reserve(capacity_);
    }

    void PushBack(const Type& value) {
        size_t new_size = size_ + 1;
        if (new_size > capacity_) {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2 * capacity_) : 1;
            ReallocateAndMoveData(new_capacity);
        }
        data_[size_] = value;
        size_ = new_size;
    }

    void PushBack(Type&& value) {
        size_t new_size = size_ + 1;
        if (new_size > capacity_) {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2 * capacity_) : 1;
            ReallocateAndMoveData(new_capacity);
        }
        data_[size_] = std::move(value);
        size_ = new_size;
    }

    Iterator Insert(ConstIterator position, const Type& value) {
        size_t position_offset = position - data_.Get();
        assert(position_offset <= size_);
        Iterator element_position = data_.Get() + position_offset;

        size_t new_size = size_ + 1;
        if (new_size <= capacity_) {
            std::copy_backward(element_position, data_.Get() + size_, data_.Get() + size_ + 1u);
            data_[position_offset] = value;
        } else {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2u * capacity_) : 1;

            ArrayPtr<Type> new_data(new_capacity);
            std::copy(data_.Get(), element_position, new_data.Get());
            new_data[position_offset] = value;
            std::copy(element_position, data_.Get() + size_, new_data.Get() + position_offset + 1u);

            data_.swap(new_data);

            capacity_ = new_capacity;
        }
        size_ = new_size;

        return data_.Get() + position_offset;
    }

    Iterator Insert(ConstIterator position, Type&& value) {
        size_t position_offset = position - data_.Get();
        assert(position_offset <= size_);
        Iterator element_position = data_.Get() + position_offset;

        size_t new_size = size_ + 1;
        if (new_size <= capacity_) {
            std::move_backward(element_position, data_.Get() + size_, data_.Get() + size_ + 1u);
            data_[position_offset] = std::move(value);
        } else {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2u * capacity_) : 1;

            ArrayPtr<Type> new_data(new_capacity);
            std::move(data_.Get(), element_position, new_data.Get());
            new_data[position_offset] = std::move(value);
            std::move(element_position, data_.Get() + size_, new_data.Get() + position_offset + 1u);

            data_.swap(new_data);
            capacity_ = new_capacity;
        }
        size_ = new_size;

        return data_.Get() + position_offset;
    }

    void PopBack() noexcept {
        assert(size_ != 0);
        --size_;
    }
    
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());

        size_t erase_index = pos - cbegin();
        Iterator new_first = &(data_[erase_index]);
        std::move(begin() + erase_index + 1, end(), new_first);
        --size_;
        return begin() + erase_index;
    }

    void swap(SimpleVector& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        data_.swap(other.data_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ReallocateAndMoveData(new_capacity);
        }
    }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> data_;

    void ReallocateAndMoveData(size_t new_capacity) {
        ArrayPtr<Type> new_data(new_capacity);
        size_t elements_count = std::min(size_, new_capacity);
        for (size_t i = 0; i < elements_count; ++i) {
            new_data[i] = std::move(data_[i]);
        }
        data_.swap(new_data);
        capacity_ = new_capacity;
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()); 
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
} 