﻿
#pragma once

#include "base/array.hpp"

#include "glog/logging.h"

namespace principia {
namespace base {

template<typename Element>
Array<Element>::Array() : data(nullptr), size(0) {}

template<typename Element>
template<typename OtherElement, typename>
Array<Element>::Array(Array<OtherElement> const& other)
    : data(other.data), size(other.size) {}

template<typename Element>
template<typename Size, typename>
Array<Element>::Array(Element* const data, Size const size)
    : data(data), size(static_cast<std::int64_t>(size)) {}

template<typename Element>
UniqueArray<Element>::UniqueArray() : size(0) {}

template<typename Element>
template<typename Size, typename>
UniqueArray<Element>::UniqueArray(Size const size)
    : data(new std::uint8_t[static_cast<std::size_t>(size)]),
      size(static_cast<std::int64_t>(size)) {}

template<typename Element>
template<typename Size, typename>
UniqueArray<Element>::UniqueArray(std::unique_ptr<Element[]> data,
                                  Size const size)
    : data(data.release()),
      size(static_cast<std::int64_t>(size)) {}

template<typename Element>
Array<Element> UniqueArray<Element>::get() const {
  return Array<Element>(data.get(), size);
}

template<typename Element, std::int32_t size_>
typename BoundedArray<Element, size_>::const_iterator
BoundedArray<Element, size_>::begin() const {
  return data.begin();
}

template<typename Element, std::int32_t size_>
typename BoundedArray<Element, size_>::const_iterator
BoundedArray<Element, size_>::end() const {
  return data.begin() + actual_size;
}

template<typename Element, std::int32_t size_>
bool BoundedArray<Element, size_>::empty() const {
  return actual_size == 0;
}

template<typename Element, std::int32_t size_>
std::size_t BoundedArray<Element, size_>::size() const {
  return actual_size;
}

template<typename Element>
bool operator==(Array<Element> left, Array<Element> right) {
  if (left.size != right.size) {
    return false;
  }
  return std::memcmp(left.data,
                     right.data,
                     static_cast<std::size_t>(right.size)) == 0;
}

template<typename Element>
bool operator==(Array<Element> left, UniqueArray<Element> const& right) {
  return left == right.get();
}

template<typename Element>
bool operator==(UniqueArray<Element> const& left, Array<Element> right) {
  return left.get() == right;
}

template<typename Element>
bool operator==(UniqueArray<Element> const& left,
                UniqueArray<Element> const& right) {
  return left.get() == right.get();
}

}  // namespace base
}  // namespace principia
