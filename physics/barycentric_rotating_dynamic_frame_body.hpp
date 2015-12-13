﻿#pragma once

#include "physics/barycentric_rotating_dynamic_frame.hpp"

#include "geometry/barycentre_calculator.hpp"
#include "geometry/named_quantities.hpp"
#include "geometry/r3x3_matrix.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"

namespace principia {

using geometry::Barycentre;
using geometry::Displacement;
using geometry::R3x3Matrix;
using geometry::Velocity;
using geometry::Wedge;
using quantities::Length;
using quantities::Pow;
using quantities::Product;
using quantities::Speed;

namespace physics {

template<typename InertialFrame, typename ThisFrame>
BarycentricRotatingDynamicFrame<InertialFrame, ThisFrame>::
BarycentricRotatingDynamicFrame(
    not_null<Ephemeris<InertialFrame> const*> const ephemeris,
    not_null<MassiveBody const*> const primary,
    not_null<MassiveBody const*> const secondary)
    : ephemeris_(ephemeris),
      primary_(primary),
      secondary_(secondary),
      primary_trajectory_(ephemeris_->trajectory(primary_)),
      secondary_trajectory_(ephemeris_->trajectory(secondary_)) {}

template<typename InertialFrame, typename ThisFrame>
RigidMotion<InertialFrame, ThisFrame>
BarycentricRotatingDynamicFrame<InertialFrame, ThisFrame>::ToThisFrameAtTime(
    Instant const& t) const {
  DegreesOfFreedom<InertialFrame> const primary_degrees_of_freedom =
      primary_trajectory_->EvaluateDegreesOfFreedom(t, &primary_hint_);
  DegreesOfFreedom<InertialFrame> const secondary_degrees_of_freedom =
      secondary_trajectory_->EvaluateDegreesOfFreedom(t, &secondary_hint_);
  DegreesOfFreedom<InertialFrame> const barycentre_degrees_of_freedom =
      Barycentre<DegreesOfFreedom<InertialFrame>, GravitationalParameter>(
          {primary_degrees_of_freedom,
           secondary_degrees_of_freedom},
          {primary_->gravitational_parameter(),
           secondary_->gravitational_parameter()});

  Rotation<InertialFrame, ThisFrame> rotation =
          Rotation<InertialFrame, ThisFrame>::Identity();
  AngularVelocity<InertialFrame> angular_velocity;
  ComputeAngularDegreesOfFreedom(primary_degrees_of_freedom,
                                 secondary_degrees_of_freedom,
                                 &rotation,
                                 &angular_velocity);

  RigidTransformation<InertialFrame, ThisFrame> const
      rigid_transformation(barycentre_degrees_of_freedom.position(),
                           ThisFrame::origin,
                           rotation.Forget());
  return RigidMotion<InertialFrame, ThisFrame>(
             rigid_transformation,
             angular_velocity,
             barycentre_degrees_of_freedom.velocity());
}

template<typename InertialFrame, typename ThisFrame>
RigidMotion<ThisFrame, InertialFrame>
BarycentricRotatingDynamicFrame<InertialFrame, ThisFrame>::
FromThisFrameAtTime(Instant const& t) const {
  return ToThisFrameAtTime(t).Inverse();
}

template<typename InertialFrame, typename ThisFrame>
Vector<Acceleration, ThisFrame>
BarycentricRotatingDynamicFrame<InertialFrame, ThisFrame>::
GeometricAcceleration(
    Instant const& t,
    DegreesOfFreedom<ThisFrame> const& degrees_of_freedom) const {
  auto const to_this_frame = ToThisFrameAtTime(t);
  auto const from_this_frame = to_this_frame.Inverse();

  DegreesOfFreedom<InertialFrame> const primary_degrees_of_freedom =
      primary_trajectory_->EvaluateDegreesOfFreedom(t, &primary_hint_);
  DegreesOfFreedom<InertialFrame> const secondary_degrees_of_freedom =
      secondary_trajectory_->EvaluateDegreesOfFreedom(t, &secondary_hint_);

  // Beware, we want the angular velocity of ThisFrame as seen in the
  // InertialFrame, but pushed to ThisFrame.  Otherwise the sign is wrong.
  AngularVelocity<InertialFrame> const Ω_inertial =
      to_this_frame.angular_velocity_of_to_frame();
  AngularVelocity<ThisFrame> const Ω =
      to_this_frame.orthogonal_map()(Ω_inertial);

  Vector<Acceleration, InertialFrame> const primary_acceleration =
      ephemeris_->ComputeGravitationalAccelerationOnMassiveBody(
          primary_, t);
  Vector<Acceleration, InertialFrame> const secondary_acceleration =
      ephemeris_->ComputeGravitationalAccelerationOnMassiveBody(
          secondary_, t);

  // TODO(egg): TeX and reference.
  RelativeDegreesOfFreedom<InertialFrame> const primary_secondary =
      primary_degrees_of_freedom - secondary_degrees_of_freedom;
  Variation<AngularVelocity<ThisFrame>> const dΩ_over_dt =
      to_this_frame.orthogonal_map()
          (Wedge(primary_secondary.displacement(),
                 (primary_acceleration - secondary_acceleration)) * Radian -
           2 * Ω_inertial * InnerProduct(primary_secondary.displacement(),
                                         primary_secondary.velocity())) /
               InnerProduct(primary_secondary.displacement(),
                            primary_secondary.displacement());

  Displacement<ThisFrame> const r =
      degrees_of_freedom.position() - ThisFrame::origin;
  Vector<Acceleration, ThisFrame> const gravitational_acceleration_at_point =
      to_this_frame.orthogonal_map()(
          ephemeris_->ComputeGravitationalAccelerationOnMasslessBody(
              from_this_frame.rigid_transformation()(
                  degrees_of_freedom.position()), t));
  Vector<Acceleration, ThisFrame> const linear_acceleration =
      to_this_frame.orthogonal_map()(
          -Barycentre<Vector<Acceleration, InertialFrame>,
                      GravitationalParameter>(
              {primary_acceleration,
               secondary_acceleration},
              {primary_->gravitational_parameter(),
               secondary_->gravitational_parameter()}));
  Vector<Acceleration, ThisFrame> const coriolis_acceleration_at_point =
      -2 * Ω * degrees_of_freedom.velocity() / Radian;
  Vector<Acceleration, ThisFrame> const centrifugal_acceleration_at_point =
      -Ω * (Ω * r) / Pow<2>(Radian);
  Vector<Acceleration, ThisFrame> const euler_acceleration_at_point =
      -dΩ_over_dt * r / Radian;

  Vector<Acceleration, ThisFrame> const fictitious_acceleration =
      linear_acceleration +
      coriolis_acceleration_at_point +
      centrifugal_acceleration_at_point +
      euler_acceleration_at_point;
  return gravitational_acceleration_at_point + fictitious_acceleration;
}

template<typename InertialFrame, typename ThisFrame>
void BarycentricRotatingDynamicFrame<InertialFrame, ThisFrame>::
ComputeAngularDegreesOfFreedom(
    DegreesOfFreedom<InertialFrame> const& primary_degrees_of_freedom,
    DegreesOfFreedom<InertialFrame> const& secondary_degrees_of_freedom,
    not_null<Rotation<InertialFrame, ThisFrame>*> const rotation,
    not_null<AngularVelocity<InertialFrame>*> const angular_velocity) {
  RelativeDegreesOfFreedom<InertialFrame> const reference =
      primary_degrees_of_freedom - secondary_degrees_of_freedom;
  Displacement<InertialFrame> const& reference_direction =
      reference.displacement();
  Velocity<InertialFrame> reference_normal = reference.velocity();
  reference_direction.template Orthogonalize<Speed>(&reference_normal);
  Bivector<Product<Length, Speed>, InertialFrame> const reference_binormal =
      Wedge(reference_direction, reference_normal);
  *rotation = Rotation<InertialFrame, ThisFrame>(
                  R3x3Matrix(Normalize(reference_direction).coordinates(),
                             Normalize(reference_normal).coordinates(),
                             Normalize(reference_binormal).coordinates()));
  *angular_velocity = reference_binormal * Radian /
                      InnerProduct(reference_direction, reference_direction);
}

}  // namespace physics
}  // namespace principia
