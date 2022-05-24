//===- Container.cpp ------------------------------------------------------===//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
//
// This file implements the container descriptor.
//
//===----------------------------------------------------------------------===//

#ifndef CORE_CONTAINER_DEF
#define CORE_CONTAINER_DEF

#include <algorithm>
#include <memory>
#include <numeric>
#include <stdexcept>

#include "Interface/buddy/core/Container.h"

// MemRef Shape Constructor.
// Construct a MemRef object from the data shape and initial value.
// The default initial value is 0.
template <typename T, std::size_t N>
MemRef<T, N>::MemRef(intptr_t sizes[N], T init) {
  for (size_t i = 0; i < N; i++) {
    this->sizes[i] = sizes[i];
  }
  setStrides();
  size = product(sizes);
  allocated = new T[size];
  aligned = allocated;
  std::fill(aligned, aligned + size, init);
}

// MemRef Array Constructor.
// Construct a MemRef object from the data pointer, sizes, and offset.
// The default offset is 0.
template <typename T, std::size_t N>
MemRef<T, N>::MemRef(const T *data, intptr_t sizes[N], intptr_t offset) {
  this->offset = offset;
  for (size_t i = 0; i < N; i++) {
    this->sizes[i] = sizes[i];
  }
  setStrides();
  size = product(sizes);
  allocated = new T[size];
  aligned = allocated;
  for (size_t i = 0; i < size; i++) {
    aligned[i] = data[i];
  }
}

// Copy Constructor.
// This constructor is used to initialize a MemRef object with another MemRef
// object.
// - Copy `offset` and `size` directly.
// - Elementwise copy `sizes` array.
// - Calculate `strides`.
// - Allocate new space.
// - Deep copy the data from the original object.
template <typename T, std::size_t N>
MemRef<T, N>::MemRef(const MemRef<T, N> &other)
    : offset(other.offset), size(other.size) {
  for (size_t i = 0; i < N; i++) {
    this->sizes[i] = other.sizes[i];
  }
  setStrides();
  allocated = new T[size];
  aligned = allocated;
  for (size_t i = 0; i < size; i++) {
    aligned[i] = other.aligned[i];
  }
}

// Copy Assignment Operator.
// - Check if they are the same object.
// - Copy `offset` and `size` directly.
// - Elementwise copy `sizes`.
// - Calculate the `strides`.
// - Free the data space of this object to avoid memory leaks.
// - Allocate new space and deep copy.
template <typename T, std::size_t N>
MemRef<T, N> &MemRef<T, N>::operator=(const MemRef<T, N> &other) {
  if (this != &other) {
    this->offset = other.offset;
    this->size = other.size;
    for (size_t i = 0; i < N; i++) {
      this->sizes[i] = other.sizes[i];
    }
    setStrides();
    // Free the original aligned and allocated space.
    delete[] allocated;
    // Allocate new space and deep copy.
    T *ptr = new T[size];
    for (size_t i = 0; i < size; i++) {
      ptr[i] = other.aligned[i];
    }
    aligned = ptr;
    allocated = ptr;
  }
  return *this;
}

// Move Constructor.
// This constructor is used to initialize a MemRef object from a rvalue.
// The move constructor steals the resources of the original object.
// Note that the original object no longer owns the members and spaces.
// - Steal members from the original object.
// - Assign the NULL pointer to the original aligned and allocated members to
//   avoid the double free error.
template <typename T, std::size_t N>
MemRef<T, N>::MemRef(MemRef<T, N> &&other) noexcept
    : allocated(other.allocated), aligned(other.aligned), offset(other.offset),
      size(other.size) {
  std::swap(this->sizes, other.sizes);
  std::swap(this->strides, other.strides);
  // Assign the NULL pointer to the original aligned and allocated members to
  // avoid the double free error.
  other.allocated = other.aligned = nullptr;
}

// Move Assignment Operator.
// Note that the original object no longer owns the members and spaces.
// - Check if they are the same object.
// - Free the data space of this object to avoid memory leaks.
// - Steal members from the original object.
// - Assign the NULL pointer to the original aligned and allocated members to
//   avoid the double free error.
template <typename T, std::size_t N>
MemRef<T, N> &MemRef<T, N>::operator=(MemRef<T, N> &&other) noexcept {
  if (this != &other) {
    // Free the original aligned and allocated space.
    delete[] allocated;
    // Steal members of the original object.
    std::swap(strides, other.strides);
    std::swap(offset, other.offset);
    std::swap(sizes, other.sizes);
    std::swap(size, other.size);
    std::swap(allocated, other.allocated);
    std::swap(aligned, other.aligned);
    // Assign the NULL pointer to the original aligned and allocated members to
    // avoid the double free error.
    other.allocated = other.aligned = nullptr;
  }
  return *this;
}

// MemRef Destructor.
// Note that the `allocated` and `aligned` point to the same address, so it is
// enough to release the space of the `allocated` pointer in the destructor.
template <typename T, std::size_t N> MemRef<T, N>::~MemRef() {
  delete[] allocated;
}

// Calculate the stride values for each dimension based on the sizes.
template <typename T, std::size_t N> void MemRef<T, N>::setStrides() {
  strides[N - 1] = 1;
  for (int i = N - 2; i >= 0; i--) {
    strides[i] = strides[i + 1] * sizes[i + 1];
  }
}

// Calculate the total number of elements in the MemRef container.
template <typename T, std::size_t N>
size_t MemRef<T, N>::product(intptr_t sizes[N]) const {
  size_t size = 1;
  for (size_t i = 0; i < N; i++)
    size *= sizes[i];
  return size;
}

#endif // CORE_CONTAINER_DEF