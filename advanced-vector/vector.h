#pragma once 
#include <cassert> 
#include <cstdlib> 
#include <new> 
#include <utility> 
#include <memory> 
#include <iostream> 
#include <algorithm> 
 
template <typename T> 
class RawMemory { 
public: 
    RawMemory() = default; 
    explicit RawMemory(size_t capacity); 
    RawMemory(const RawMemory&) = delete; 
    RawMemory(RawMemory&& other) noexcept; 
    ~RawMemory(); 
 
    RawMemory& operator=(const RawMemory& rhs) = delete; 
    RawMemory& operator=(RawMemory&& rhs) noexcept; 
    T* operator+(size_t offset) noexcept; 
    const T* operator+(size_t offset) const noexcept; 
    const T& operator[](size_t index) const noexcept; 
    T& operator[](size_t index) noexcept; 
 
    void Swap(RawMemory& other) noexcept; 
 
    const T* GetAddress() const noexcept; 
    T* GetAddress() noexcept; 
 
    size_t Capacity() const; 
private: 
    T* buffer_ = nullptr; 
    size_t capacity_ = 0; 
 
    // Выделяет сырую память под n элементов и возвращает указатель на неё 
    static T* Allocate(size_t n); 
    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate 
    static void Deallocate(T* buf) noexcept; 
}; 
 
 
template <typename T> 
class Vector { 
public: 
    Vector() = default; 
 
    explicit Vector(size_t size); 
    Vector(const Vector& other); 
    Vector(Vector&& other) noexcept; 
 
    ~Vector(); 
 
    using iterator = T*; 
    using const_iterator = const T*; 
 
    iterator begin() noexcept; 
    iterator end() noexcept; 
    const_iterator begin() const noexcept; 
    const_iterator end() const noexcept; 
    const_iterator cbegin() const noexcept; 
    const_iterator cend() const noexcept; 
 
    const T& operator[](size_t index) const noexcept ; 
    T& operator[](size_t index) noexcept; 
 
    Vector& operator=(const Vector& rhs); 
    Vector& operator=(Vector&& rhs) noexcept; 
 
    size_t Size() const noexcept; 
    size_t Capacity() const noexcept; 
 
    void Reserve(size_t new_capacity); 
    void Resize(size_t new_size); 
 
    void Swap(Vector& other) noexcept; 
 
    void PushBack(const T& value); 
    void PushBack(T&& value); 
    void PopBack() /* noexcept */; 
 
    template <typename... Args> 
    T& EmplaceBack(Args&&... args); 
    template <typename... Args> 
    iterator Emplace(const_iterator pos, Args&&... args); 
 
    iterator Insert(const_iterator pos, const T& value); 
    iterator Insert(const_iterator pos, T&& value); 
 
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/; 
private: 
    RawMemory<T> data_; 
    size_t size_ = 0; 
 
    void CopyExistingElements(const Vector& rhs); 
    void CopyElements(const Vector& rhs, size_t count); 
    // Конструируем элементы в new_data, перемещая или копируя их из data_ 
    void ConstructElements(RawMemory<T>& new_data); 
    void CopyOrMoveElements(T* from, T* to, size_t count); 
 
    template <typename... Args> 
    void EmplaceInFullVector(const_iterator pos, Args&&... args); 
}; 
 
 
 
template <typename T> 
RawMemory<T>::RawMemory(size_t capacity) 
    : buffer_(Allocate(capacity)) 
    , capacity_(capacity) { 
} 
 
template <typename T> 
RawMemory<T>::RawMemory(RawMemory&& other) noexcept 
    : buffer_(other.buffer_) 
    , capacity_(other.capacity_) { 
    other.buffer_ = nullptr; 
    other.capacity_ = 0; 
} 
 
template <typename T> 
RawMemory<T>& RawMemory<T>::operator=(RawMemory&& rhs) noexcept { 
    Deallocate(buffer_); 
    buffer_ = rhs.buffer_; 
    capacity_ = rhs.capacity_; 
    rhs.buffer_ = nullptr; 
    rhs.capacity_ = 0; 
    return *this; 
} 
 
template <typename T> 
RawMemory<T>::~RawMemory() { 
    Deallocate(buffer_); 
} 
 
template <typename T> 
T* RawMemory<T>::operator+(size_t offset) noexcept { 
    // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива 
    assert(offset <= capacity_); 
    return buffer_ + offset; 
} 
 
template <typename T> 
const T* RawMemory<T>::operator+(size_t offset) const noexcept { 
    return const_cast<RawMemory&>(*this) + offset; 
} 
 
template <typename T> 
const T& RawMemory<T>::operator[](size_t index) const noexcept { 
    return const_cast<RawMemory&>(*this)[index]; 
} 
 
template <typename T> 
T& RawMemory<T>::operator[](size_t index) noexcept { 
    assert(index < capacity_); 
    return buffer_[index]; 
} 
 
template <typename T> 
void RawMemory<T>::Swap(RawMemory& other) noexcept { 
    std::swap(buffer_, other.buffer_); 
    std::swap(capacity_, other.capacity_); 
} 
 
template <typename T> 
const T* RawMemory<T>::GetAddress() const noexcept { 
    return buffer_; 
} 
 
template <typename T> 
T* RawMemory<T>::GetAddress() noexcept { 
    return buffer_; 
} 
 
template <typename T> 
size_t RawMemory<T>::Capacity() const { 
    return capacity_; 
} 
 
template <typename T> 
T* RawMemory<T>::Allocate(size_t n) { 
    return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr; 
} 
 
template <typename T> 
void RawMemory<T>::Deallocate(T* buf) noexcept { 
    operator delete(buf); 
} 
 
template <typename T> 
void Vector<T>::CopyOrMoveElements(T* from, T* to, size_t count) { 
    // constexpr оператор if будет вычислен во время компиляции 
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) { 
        std::uninitialized_move_n(from, count, to); 
    } else { 
        std::uninitialized_copy_n(from, count, to); 
    } 
} 
 
 
template <typename T> 
Vector<T>::Vector(size_t size) 
    : data_(size) 
    , size_(size) { 
    std::uninitialized_value_construct_n(data_.GetAddress(), size); 
} 
 
template <typename T> 
Vector<T>::Vector(const Vector& other) 
        : data_(other.size_) 
        , size_(other.size_) { 
    // Конструируем элементы в data_, копируя их из other.data_ 
    std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress()); 
} 
 
template <typename T> 
Vector<T>::~Vector() { 
    std::destroy_n(data_.GetAddress(), size_); 
} 
 
template <typename T> 
void Vector<T>::Reserve(size_t new_capacity) { 
 
    if (new_capacity <= data_.Capacity()) { 
        return; 
    } 
 
    RawMemory<T> new_data(new_capacity); 
    ConstructElements(new_data); 
} 
 
template <typename T> 
size_t Vector<T>::Size() const noexcept { 
    return size_; 
} 
 
template <typename T> 
size_t Vector<T>::Capacity() const noexcept { 
    return data_.Capacity(); 
} 
 
template <typename T> 
const T& Vector<T>::operator[](size_t index) const noexcept { 
    return const_cast<Vector&>(*this)[index]; 
} 
 
template <typename T> 
T& Vector<T>::operator[](size_t index) noexcept { 
    assert(index < size_); 
    return data_[index]; 
} 
 
template <typename T> 
Vector<T>::Vector(Vector&& other) noexcept 
    : data_(std::move(other.data_)) 
    , size_(std::move(other.size_)) 
{ 
    other.size_ = 0; 
} 
 
template <typename T> 
Vector<T>& Vector<T>::operator=(const Vector& rhs) { 
    if (this != &rhs) { 
        if (rhs.size_ > data_.Capacity()) { 
            Vector rhs_copy(rhs); 
            Swap(rhs_copy); 
        } else { 
            CopyExistingElements(rhs); 
        } 
    } 
    return *this; 
} 
 
template <typename T> 
Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept { 
    data_ = std::move(rhs.data_); 
    size_ = std::move(rhs.size_); 
    rhs.size_ = 0; 
    return *this; 
} 
 
template <typename T> 
void Vector<T>::Swap(Vector& other) noexcept { 
    std::swap(data_, other.data_); 
    std::swap(size_, other.size_); 
} 
 
template <typename T> 
void Vector<T>::Resize(size_t new_size) { 
    if (new_size == size_) { 
        return; 
    } 
 
    if (new_size < size_) { 
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size); 
    } 
    else { 
        Reserve(new_size); 
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_); 
    } 
    size_ = new_size; 
} 
 
template <typename T> 
void Vector<T>::PushBack(const T& value) { 
    EmplaceBack(value); 
} 
 
template <typename T> 
void Vector<T>::PushBack(T&& value) { 
    EmplaceBack(std::move(value)); 
} 
 
template <typename T> 
void Vector<T>::PopBack() /* noexcept */ { 
    std::destroy_at(data_.GetAddress() + (--size_)); 
} 
 
template <typename T> 
template <typename... Args> 
T& Vector<T>::EmplaceBack(Args&&... args) { 
    if (size_ == Capacity()) { 
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2); 
        new (new_data + size_) T(std::forward<decltype(args)>(args)...); 
        ConstructElements(new_data); 
    } else { 
        new (data_ + size_) T(std::forward<decltype(args)>(args)...); 
    } 
    return data_[size_++]; 
} 
 
template <typename T> 
T* Vector<T>::begin() noexcept { 
    return data_.GetAddress(); 
} 
 
template <typename T> 
T* Vector<T>::end() noexcept { 
    return std::next(data_.GetAddress(), size_); 
} 
 
template <typename T> 
const T* Vector<T>::begin() const noexcept { 
    return const_iterator{data_.GetAddress()}; 
} 
 
template <typename T> 
const T* Vector<T>::end() const noexcept { 
    return const_iterator{std::next(data_.GetAddress(), size_)}; 
} 
 
template <typename T> 
const T* Vector<T>::cbegin() const noexcept { 
    return begin(); 
} 
 
template <typename T> 
const T* Vector<T>::cend() const noexcept { 
    return end(); 
} 
 
template <typename T> 
template <typename... Args> 
T*Vector<T>::Emplace(const_iterator pos, Args&&... args) { 
    size_t pos_index = pos - begin(); 
    if (size_ == Capacity()) { 
        EmplaceInFullVector(pos, std::forward<decltype(args)>(args)...); 
    } else if (size_ == 0) { 
        new (data_.GetAddress()) T(std::forward<decltype(args)>(args)...); 
    } 
    else { 
        T* value = new T(std::forward<decltype(args)>(args)...); 
        new (end()) T(std::forward<T>(data_[size_ - 1])); 
        std::move_backward(begin() + pos_index, end() - 1, end()); 
        data_[pos_index] = std::move(*value); 
        delete value; 
    } 
    ++size_; 
    return std::next(begin(), pos_index) ; 
} 
 
template <typename T> 
T* Vector<T>::Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ { 
    size_t pos_index = pos - begin(); 
    std::move(begin() + pos_index + 1, end(), begin() + pos_index); 
    std::destroy_n(end() - 1, 1); 
    --size_; 
    return begin() + pos_index; 
} 
 
template <typename T> 
T* Vector<T>::Insert(const_iterator pos, const T& value) { 
    return Emplace(pos, value); 
} 
 
template <typename T> 
T* Vector<T>::Insert(const_iterator pos, T&& value) { 
    return Emplace(pos, std::forward<T>(value)); 
} 
 
template <typename T> 
void Vector<T>::CopyExistingElements(const Vector& rhs) { 
    if (rhs.size_ < size_) { 
        CopyElements(rhs, rhs.size_); 
        std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_); 
    } else { 
        CopyElements(rhs, size_); 
        std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_); 
    } 
    size_ = rhs.size_; 
} 
 
template <typename T> 
void Vector<T>::CopyElements(const Vector& rhs, size_t count) { 
    for (size_t index = 0; index < count; ++index) { 
        (*this)[index] = rhs[index]; 
    } 
} 
 
template <typename T> 
void Vector<T>::ConstructElements(RawMemory<T>& new_data) { 
    CopyOrMoveElements(data_.GetAddress(), new_data.GetAddress(), size_); 
    // Разрушаем элементы в data_ 
    std::destroy_n(data_.GetAddress(), size_); 
    // Избавляемся от старой сырой памяти, обменивая её на новую 
    data_.Swap(new_data); 
    // При выходе из метода старая память будет возвращена в кучу 
} 
 
template <typename T> 
template <typename... Args> 
void Vector<T>::EmplaceInFullVector(const_iterator pos, Args&&... args) { 
    RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2); 
    size_t pos_index = pos - begin(); 
    new (&new_data[pos_index]) T(std::forward<decltype(args)>(args)...); 
 
    try { 
        CopyOrMoveElements(data_.GetAddress(), new_data.GetAddress(), pos_index); 
    }  catch (...) { 
        std::destroy_n(new_data.GetAddress() + pos_index, 1); 
        throw; 
    } 
 
    try { 
        CopyOrMoveElements(data_.GetAddress() + pos_index, new_data.GetAddress() + pos_index + 1, size_ - pos_index); 
    }  catch (...) { 
        std::destroy_n(new_data.GetAddress(), pos_index + 1); 
        throw; 
    } 
    std::destroy_n(data_.GetAddress(), size_); 
    data_.Swap(new_data); 
} 
