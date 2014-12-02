﻿#pragma once

#include <vector>

#include "geometry/grassmann.hpp"
#include "physics/massive_body.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"

using principia::geometry::Vector;
using principia::quantities::GravitationalParameter;
using principia::quantities::Length;
using principia::quantities::Mass;
using principia::quantities::Order2ZonalCoefficient;

namespace principia {
namespace physics {

template<typename Frame>
class OblateBody : public MassiveBody {
  static_assert(Frame::is_inertial, "Frame must be inertial");

 public:
  OblateBody(GravitationalParameter const& gravitational_parameter,
             double const j2,
             Length const& radius,
             Vector<double, Frame> const& axis);
  OblateBody(Mass const& mass,
             double const j2,
             Length const& radius,
             Vector<double, Frame> const& axis);
  OblateBody(GravitationalParameter const& gravitational_parameter,
             Order2ZonalCoefficient const& j2,
             Vector<double, Frame> const& axis);
  OblateBody(Mass const& mass,
             Order2ZonalCoefficient const& j2,
             Vector<double, Frame> const& axis);
  ~OblateBody() = default;

  // Returns the j2 coefficient.
  Order2ZonalCoefficient const& j2() const;

  // Returns the axis passed at construction.
  Vector<double, Frame> const& axis() const;

  // Returns false.
  bool is_massless() const;

  // Returns true.
  bool is_oblate() const;

 private:
  Order2ZonalCoefficient const j2_;
  Vector<double, Frame> const axis_;
};

}  // namespace physics
}  // namespace principia

#include "physics/oblate_body_body.hpp"