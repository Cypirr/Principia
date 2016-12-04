﻿
#pragma once

#include<vector>

#include "geometry/named_quantities.hpp"
#include "physics/massive_body.hpp"
#include "quantities/quantities.hpp"
#include "quantities/named_quantities.hpp"

namespace principia {

using geometry::Position;
using geometry::Vector;
using physics::MassiveBody;
using quantities::Acceleration;
using quantities::Force;
using quantities::Length;
using quantities::Momentum;
using quantities::Speed;
using quantities::Time;

namespace testing_utilities {

// Right-hand sides for various differential equations frequently used to test
// the properties of integrators.

// The one-dimensional unit harmonic oscillator,
//   q' = p / m,  |ComputeHarmonicOscillatorVelocity|,
//   p' = -k q, |ComputeHarmonicOscillatorForce|,
// where m = 1 kg, k = 1 N / m.

void ComputeHarmonicOscillatorForce(Time const& t,
                                    std::vector<Length> const& q,
                                    std::vector<Force>& result);

void ComputeHarmonicOscillatorVelocity(
    std::vector<Momentum> const& p,
    std::vector<Speed>& result);

// The Runge-Kutta-Nyström formulation
//   q" = -q k / m.
void ComputeHarmonicOscillatorAcceleration(
    Instant const& t,
    std::vector<Length> const& q,
    std::vector<Acceleration>& result);

// The Kepler problem with unit gravitational parameter, where the
// two-dimensional configuration space is the separation between the bodies, in
// the Runge-Kutta-Nyström formulation
//   q" = -q μ / |q|³,
// where μ = 1 m³ s⁻².
void ComputeKeplerAcceleration(Time const& t,
                               std::vector<Length> const& q,
                               std::vector<Acceleration>& result);

template<typename Frame>
void ComputeGravitationalAcceleration(
    Time const& t,
    std::vector<Position<Frame>> const& q,
    std::vector<Vector<Acceleration, Frame>>& result,
    std::vector<MassiveBody> const& bodies);

}  // namespace testing_utilities
}  // namespace principia

#include "testing_utilities/integration_body.hpp"
