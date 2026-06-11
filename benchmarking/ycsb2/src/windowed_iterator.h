#pragma once

#include <span>

template<typename T>
class windowed_iterator {
public:
    windowed_iterator(const std::span<T>& span, const int windowSize)
        : windowSize(windowSize), span(span) {
    }
    class iterator {
    public:
        iterator(const size_t index, const windowed_iterator& iterable)
            : index(index),
              iterable(iterable) {
        }
        std::span<T> operator*();

        iterator operator++();

        iterator operator++(int);

        bool operator==(const iterator& other) const;
        bool operator!=(const iterator& other) const;
    private:
        size_t index;
        windowed_iterator iterable;
    };

private:
    int windowSize;
    std::span<T> span;
public:
    iterator begin();
    iterator end();
};

template <typename T>
std::span<T> windowed_iterator<T>::iterator::operator*() {
    return iterable.span.subspan(index, iterable.windowSize);
}

template <typename T>
windowed_iterator<T>::iterator windowed_iterator<T>::iterator::operator++() {
    index += iterable.windowSize;
    return *this;
}

template <typename T>
windowed_iterator<T>::iterator windowed_iterator<T>::iterator::operator++(int) {
    auto ret = *this;
    ++*this;
    return ret;
}

template <typename T>
bool windowed_iterator<T>::iterator::operator==(const iterator& other) const {
    return index == other.index;
}

template <typename T>
bool windowed_iterator<T>::iterator::operator!=(const iterator& other) const {
    return !(*this == other);
}

template <typename T>
windowed_iterator<T>::iterator windowed_iterator<T>::begin() {
    return {0, *this};
}

template <typename T>
windowed_iterator<T>::iterator windowed_iterator<T>::end() {
    auto remainder = span.size() % windowSize;
    auto ceilDiff = remainder == 0 ? 0 : windowSize - remainder;
    return {span.size() + span.size() + ceilDiff, *this};
}

