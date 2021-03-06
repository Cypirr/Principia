﻿
#pragma once

#include <string>

#include "base/not_null.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/r3x3_matrix.hpp"
#include "quantities/named_quantities.hpp"
#include "serialization/geometry.pb.h"

namespace principia {
namespace geometry {
namespace internal_symmetric_bilinear_form {

using base::not_null;
using quantities::Product;

template<typename Scalar, typename Frame>
class SymmetricBilinearForm {
 public:
  SymmetricBilinearForm& operator+=(SymmetricBilinearForm const& right);
  SymmetricBilinearForm& operator-=(SymmetricBilinearForm const& right);
  SymmetricBilinearForm& operator*=(double right);
  SymmetricBilinearForm& operator/=(double right);

  template<typename LScalar, typename RScalar>
  Product<Scalar, Product<LScalar, RScalar>> operator()(
      Vector<LScalar, Frame> const& left,
      Vector<RScalar, Frame> const& right) const;

  template<typename LScalar, typename RScalar>
  Product<Scalar, Product<LScalar, RScalar>> operator()(
      Bivector<LScalar, Frame> const& left,
      Bivector<RScalar, Frame> const& right) const;

  void WriteToMessage(
      not_null<serialization::SymmetricBilinearForm*> message) const;
  static SymmetricBilinearForm ReadFromMessage(
      serialization::SymmetricBilinearForm const& message);

 private:
  explicit SymmetricBilinearForm(R3x3Matrix<Scalar> const& matrix);
  explicit SymmetricBilinearForm(R3x3Matrix<Scalar>&& matrix);

  // All the operations on this class must ensure that this matrix remains
  // symmetric.
  R3x3Matrix<Scalar> matrix_;

  template<typename Frame>
  friend SymmetricBilinearForm<double, Frame> const& InnerProductForm();

  template<typename Scalar, typename Frame>
  friend SymmetricBilinearForm<Scalar, Frame> operator+(
      SymmetricBilinearForm<Scalar, Frame> const& right);
  template<typename Scalar, typename Frame>
  friend SymmetricBilinearForm<Scalar, Frame> operator-(
      SymmetricBilinearForm<Scalar, Frame> const& right);

  template<typename Scalar, typename Frame>
  friend SymmetricBilinearForm<Scalar, Frame> operator+(
      SymmetricBilinearForm<Scalar, Frame> const& left,
      SymmetricBilinearForm<Scalar, Frame> const& right);
  template<typename Scalar, typename Frame>
  friend SymmetricBilinearForm<Scalar, Frame> operator-(
      SymmetricBilinearForm<Scalar, Frame> const& left,
      SymmetricBilinearForm<Scalar, Frame> const& right);

  template<typename Scalar, typename Frame>
  friend SymmetricBilinearForm<Scalar, Frame> operator*(
      double left,
      SymmetricBilinearForm<Scalar, Frame> const& right);
  template<typename Scalar, typename Frame>
  friend SymmetricBilinearForm<Scalar, Frame> operator*(
      SymmetricBilinearForm<Scalar, Frame> const& left,
      double right);
  template<typename Scalar, typename Frame>
  friend SymmetricBilinearForm<Scalar, Frame> operator/(
      SymmetricBilinearForm<Scalar, Frame> const& left,
      double right);

  template<typename LScalar, typename RScalar, typename Frame>
  friend Vector<Product<LScalar, RScalar>, Frame> operator*(
      SymmetricBilinearForm<LScalar, Frame> const& left,
      Vector<RScalar, Frame> const& right);

  template<typename LScalar, typename RScalar, typename Frame>
  friend Bivector<Product<LScalar, RScalar>, Frame> operator*(
      SymmetricBilinearForm<LScalar, Frame> const& left,
      Bivector<RScalar, Frame> const& right);

  template<typename LScalar, typename RScalar, typename Frame>
  friend Vector<Product<LScalar, RScalar>, Frame> operator*(
      Vector<LScalar, Frame> const& left,
      SymmetricBilinearForm<RScalar, Frame> const& right);

  template<typename LScalar, typename RScalar, typename Frame>
  friend Bivector<Product<LScalar, RScalar>, Frame> operator*(
      Bivector<LScalar, Frame> const& left,
      SymmetricBilinearForm<RScalar, Frame> const& right);

  template<typename LScalar, typename RScalar, typename Frame>
  friend SymmetricBilinearForm<Product<LScalar, RScalar>, Frame>
  SymmetricProduct(Vector<LScalar, Frame> const& left,
                   Vector<RScalar, Frame> const& right);

  template<typename LScalar, typename RScalar, typename Frame>
  friend SymmetricBilinearForm<Product<LScalar, RScalar>, Frame>
  SymmetricProduct(Bivector<LScalar, Frame> const& left,
                   Bivector<RScalar, Frame> const& right);

  template<typename Scalar, typename Frame>
  friend bool operator==(SymmetricBilinearForm<Scalar, Frame> const& left,
                         SymmetricBilinearForm<Scalar, Frame> const& right);
  template<typename Scalar, typename Frame>
  friend bool operator!=(SymmetricBilinearForm<Scalar, Frame> const& left,
                         SymmetricBilinearForm<Scalar, Frame> const& right);

  template<typename Scalar, typename Frame>
  friend std::string DebugString(
      SymmetricBilinearForm<Scalar, Frame> const& form);

  template<typename Scalar, typename Frame>
  friend std::ostream& operator<<(
      std::ostream& out,
      SymmetricBilinearForm<Scalar, Frame> const& form);

  friend class SymmetricBilinearFormTest;
};

// |InnerProductForm()| is the symmetric bilinear form such that for all v and
// w, |InnerProductForm()(v, w) == InnerProduct(v, w)|.
template<typename Frame>
SymmetricBilinearForm<double, Frame> const& InnerProductForm();

template<typename Scalar, typename Frame>
SymmetricBilinearForm<Scalar, Frame> operator+(
    SymmetricBilinearForm<Scalar, Frame> const& right);
template<typename Scalar, typename Frame>
SymmetricBilinearForm<Scalar, Frame> operator-(
    SymmetricBilinearForm<Scalar, Frame> const& right);

template<typename Scalar, typename Frame>
SymmetricBilinearForm<Scalar, Frame> operator+(
    SymmetricBilinearForm<Scalar, Frame> const& left,
    SymmetricBilinearForm<Scalar, Frame> const& right);
template<typename Scalar, typename Frame>
SymmetricBilinearForm<Scalar, Frame> operator-(
    SymmetricBilinearForm<Scalar, Frame> const& left,
    SymmetricBilinearForm<Scalar, Frame> const& right);

template<typename Scalar, typename Frame>
SymmetricBilinearForm<Scalar, Frame> operator*(
    double left,
    SymmetricBilinearForm<Scalar, Frame> const& right);
template<typename Scalar, typename Frame>
SymmetricBilinearForm<Scalar, Frame> operator*(
    SymmetricBilinearForm<Scalar, Frame> const& left,
    double right);
template<typename Scalar, typename Frame>
SymmetricBilinearForm<Scalar, Frame> operator/(
    SymmetricBilinearForm<Scalar, Frame> const& left,
    double right);

template<typename LScalar, typename RScalar, typename Frame>
Vector<Product<LScalar, RScalar>, Frame> operator*(
    SymmetricBilinearForm<LScalar, Frame> const& left,
    Vector<RScalar, Frame> const& right);

template<typename LScalar, typename RScalar, typename Frame>
Bivector<Product<LScalar, RScalar>, Frame> operator*(
    SymmetricBilinearForm<LScalar, Frame> const& left,
    Bivector<RScalar, Frame> const& right);

template<typename LScalar, typename RScalar, typename Frame>
Vector<Product<LScalar, RScalar>, Frame> operator*(
    Vector<LScalar, Frame> const& left,
    SymmetricBilinearForm<RScalar, Frame> const& right);

template<typename LScalar, typename RScalar, typename Frame>
Bivector<Product<LScalar, RScalar>, Frame> operator*(
    Bivector<LScalar, Frame> const& left,
    SymmetricBilinearForm<RScalar, Frame> const& right);

// |SymmetricProduct(v, w)| is v ⊙ w ≔ (v ⊗ w + w ⊗ v) / 2.
template<typename LScalar, typename RScalar, typename Frame>
SymmetricBilinearForm<Product<LScalar, RScalar>, Frame> SymmetricProduct(
    Vector<LScalar, Frame> const& left,
    Vector<RScalar, Frame> const& right);

template<typename LScalar, typename RScalar, typename Frame>
SymmetricBilinearForm<Product<LScalar, RScalar>, Frame> SymmetricProduct(
    Bivector<LScalar, Frame> const& left,
    Bivector<RScalar, Frame> const& right);

template<typename Scalar, typename Frame>
bool operator==(SymmetricBilinearForm<Scalar, Frame> const& left,
                SymmetricBilinearForm<Scalar, Frame> const& right);
template<typename Scalar, typename Frame>
bool operator!=(SymmetricBilinearForm<Scalar, Frame> const& left,
                SymmetricBilinearForm<Scalar, Frame> const& right);

template<typename Scalar, typename Frame>
std::string DebugString(SymmetricBilinearForm<Scalar, Frame> const& form);

template<typename Scalar, typename Frame>
std::ostream& operator<<(std::ostream& out,
                         SymmetricBilinearForm<Scalar, Frame> const& form);

}  // namespace internal_symmetric_bilinear_form

using internal_symmetric_bilinear_form::InnerProductForm;
using internal_symmetric_bilinear_form::SymmetricBilinearForm;
using internal_symmetric_bilinear_form::SymmetricProduct;

}  // namespace geometry
}  // namespace principia

#include "geometry/symmetric_bilinear_form_body.hpp"
