﻿
#include <experimental/filesystem>
#include <map>
#include <string>
#include <vector>

#include "base/file.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mathematica/mathematica.hpp"
#include "physics/kepler_orbit.hpp"
#include "physics/solar_system.hpp"
#include "quantities/astronomy.hpp"
#include "rigid_motion.hpp"
#include "testing_utilities/almost_equals.hpp"
#include "testing_utilities/numerics.hpp"

namespace principia {

using base::Error;
using base::make_not_null_unique;
using base::not_null;
using base::OFStream;
using geometry::AngularVelocity;
using geometry::BarycentreCalculator;
using geometry::Frame;
using geometry::Instant;
using geometry::OrthogonalMap;
using geometry::Position;
using geometry::Sign;
using geometry::Vector;
using geometry::Velocity;
using integrators::McLachlanAtela1992Order5Optimal;
using numerics::Bisect;
using quantities::GravitationalParameter;
using quantities::Infinity;
using quantities::Mass;
using quantities::Pow;
using quantities::Time;
using quantities::astronomy::JulianYear;
using quantities::si::Day;
using quantities::si::Degree;
using quantities::si::Kilo;
using quantities::si::Metre;
using quantities::si::Milli;
using quantities::si::Minute;
using quantities::si::Radian;
using quantities::si::Second;
using testing_utilities::RelativeError;
using ::testing::Eq;
using ::testing::Lt;

namespace physics {

class ResonanceTest : public ::testing::Test {
 protected:
  using KSP = Frame<serialization::Frame::TestTag,
                    serialization::Frame::TEST,
                    /*frame_is_inertial=*/true>;

  using Periods = std::map<not_null<MassiveBody const*>, Time>;

  ResonanceTest() {
    // This test is mostly a tool for investigating orbit stability, so we want
    // logging.
    google::LogToStderr();

    solar_system_.Initialize(
        SOLUTION_DIR / "astronomy" / "ksp_gravity_model.proto.txt",
        SOLUTION_DIR / "astronomy" / "ksp_initial_state_0_0.proto.txt");
  }

  not_null<std::unique_ptr<Ephemeris<KSP>>> MakeEphemeris() {
    auto ephemeris = solar_system_.MakeEphemeris(
        /*fitting_tolerance=*/5 * Milli(Metre),
        Ephemeris<KSP>::FixedStepParameters(
            McLachlanAtela1992Order5Optimal<Position<KSP>>(),
            /*step=*/45 * Minute));
    jool_ = solar_system_.massive_body(*ephemeris, "Jool");
    pol_ = solar_system_.massive_body(*ephemeris, "Pol");
    bop_ = solar_system_.massive_body(*ephemeris, "Bop");
    tylo_ = solar_system_.massive_body(*ephemeris, "Tylo");
    vall_ = solar_system_.massive_body(*ephemeris, "Vall");
    laythe_ = solar_system_.massive_body(*ephemeris, "Laythe");

    jool_system_ = {jool_, laythe_, vall_, tylo_, bop_, pol_};
    joolian_moons_ = {laythe_, vall_, tylo_, bop_, pol_};

    for (auto const moon : joolian_moons_) {
      elements_[moon] = solar_system_.MakeKeplerianElements(
          solar_system_.keplerian_initial_state_message(moon->name()).
              elements());
      expected_periods_[moon] =
          (2 * π * Radian) / *elements_[moon].mean_motion;
      longest_joolian_period_ =
          std::max(longest_joolian_period_, expected_periods_[moon]);
    }
    LOG(INFO) << "Longest Joolian period is " << longest_joolian_period_ / Day
              << " days";

    short_term_ = solar_system_.epoch() + 30 * Day;
    mid_term_ = solar_system_.epoch() + 1 * JulianYear;
    long_term_ = solar_system_.epoch() + 100 * JulianYear;

    return std::move(ephemeris);
  }

  void LogEphemeris(Ephemeris<KSP> const& ephemeris,
                    bool const reference,
                    std::string const& name) {
    Instant const begin = reference ? solar_system_.epoch() : mid_term_;
    Instant const end = reference ? short_term_ : long_term_;
    std::string const purpose = reference ? "reference" : "comparison";
    // Mathematica tends to be slow when dealing with quantities, so we give
    // everything in SI units.
    std::vector<double> times;
    // Indexed chronologically, then by body.
    std::vector<std::vector<Vector<double, KSP>>> barycentric_positions;
    for (Instant t = begin; t < end; t += 45 * Minute) {
      auto const position = [&ephemeris, t](not_null<MassiveBody const*> body) {
        return ephemeris.trajectory(body)->EvaluatePosition(t);
      };

      times.emplace_back((t - solar_system_.epoch()) / Second);

      BarycentreCalculator<Position<KSP>, GravitationalParameter>
          jool_system_barycentre;
      for (auto const body : jool_system_) {
        jool_system_barycentre.Add(position(body),
                                   body->gravitational_parameter());
      }
      barycentric_positions.emplace_back();
      for (auto const body : jool_system_) {
        // TODO(egg): when our dynamic frames support that, it would make sense
        // to use a nonrotating dynamic frame centred at the barycentre of the
        // Jool system, instead of computing the barycentre and difference
        // ourselves.
        barycentric_positions.back().emplace_back(
            (position(body) - jool_system_barycentre.Get()) / Metre);
      }
    }
    OFStream file(TEMP_DIR / (name + "_" + purpose + ".generated.wl"));
    file << mathematica::Assign(name + purpose + "q", barycentric_positions);
    file << mathematica::Assign(name + purpose + "t", times);
  }

  // Compute and log the measured periods of the moons.
  Periods ComputePeriods(Ephemeris<KSP> const& ephemeris,
                         Instant const t) const {
    Periods actual_periods;

    auto const position = [this, &ephemeris](
        not_null<MassiveBody const*> body, Instant const& t) {
      return ephemeris.trajectory(body)->EvaluatePosition(t);
    };
    auto const barycentre = [this, &position](Instant const& t) {
      BarycentreCalculator<Position<KSP>, Mass> result;
      for (auto const body : jool_system_) {
        result.Add(position(body, t), body->mass());
      }
      return result.Get();
    };
    auto const barycentric_position =
        [this, &barycentre, &ephemeris, &position](
        not_null<MassiveBody const*> body,
        Instant const& t) {
      return position(body, t) - barycentre(t);
    };

    LOG(INFO) << "Periods at " << t;
    for (auto const moon : {laythe_, vall_, tylo_, pol_, bop_}) {
      auto const moon_y =
          [this, &barycentric_position, moon](Instant const& t) {
        return barycentric_position(moon, t).coordinates().y;
      };

      Instant t1 = t;
      Sign s0(0);
      if (t1 <= ephemeris.t_max()) {
        s0 = Sign(moon_y(t1));
      }
      Time const Δt = 45 * Minute;
      while (t1 <= ephemeris.t_max() && Sign(moon_y(t1)) == s0) {
        //LOG(INFO)<<t1<<" "<<moon_y(t1);
        t1 += Δt;
      }
      // The moon crosses the xz plane between t1 and t1 - Δt.

      Instant t2 = t1;
      while (t2 <= ephemeris.t_max() && Sign(moon_y(t2)) != s0) {
        t2 += Δt;
      }
      // The crossing of the xz plane halfway through the orbit occurs between
      // t2 and t2 - Δt.
      while (t2 <= ephemeris.t_max() && Sign(moon_y(t2)) == s0) {
        t2 += Δt;
      }
      // The orbit ends between t2 and t2 - Δt.

      LOG(INFO) << "  " << moon->name();
      if (t1 > ephemeris.t_max() || t2 > ephemeris.t_max()) {
        LOG(INFO) << "    Aperiodic";
        actual_periods[moon] = Infinity<Time>();
      } else {
        actual_periods[moon] =
            Bisect(moon_y, t2 - Δt, t2) - Bisect(moon_y, t1 - Δt, t1);
        LOG(INFO) << "    actual period   : " << actual_periods[moon];
        LOG(INFO) << "    expected period : " << expected_periods_.at(moon);
        LOG(INFO) << "    error           : "
                  << RelativeError(actual_periods[moon],
                                   expected_periods_.at(moon));
      }
    }
    return actual_periods;
  }

  SolarSystem<KSP> solar_system_;

  MassiveBody const* jool_;
  MassiveBody const* laythe_;
  MassiveBody const* vall_;
  MassiveBody const* tylo_;
  MassiveBody const* bop_;
  MassiveBody const* pol_;
  std::vector<not_null<MassiveBody const*>> jool_system_;
  std::vector<not_null<MassiveBody const*>> joolian_moons_;
  std::map<not_null<MassiveBody const*>, KeplerianElements<KSP>> elements_;
  Time longest_joolian_period_;
  Periods expected_periods_;
  // TODO(egg): Frame::unmoving_origin, I have to do this in several places.
  DegreesOfFreedom<KSP> const origin_ = {KSP::origin, Velocity<KSP>()};

  Instant short_term_;
  Instant mid_term_;
  Instant long_term_;
};

#if !defined(_DEBUG)

TEST_F(ResonanceTest, Stock) {
  auto const ephemeris = MakeEphemeris();
  ephemeris->Prolong(short_term_);
  EXPECT_OK(ephemeris->last_severe_integration_status());

  auto const periods_at_epoch =
      ComputePeriods(*ephemeris, ephemeris->t_min());
  EXPECT_THAT(RelativeError(periods_at_epoch.at(laythe_),
                            expected_periods_.at(laythe_)), Lt(1.5e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(vall_),
                            expected_periods_.at(vall_)), Lt(2.6e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(tylo_),
                            expected_periods_.at(tylo_)), Lt(1.0e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(bop_),
                            expected_periods_.at(bop_)), Lt(9.0e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(pol_),
                            expected_periods_.at(pol_)), Lt(5.7e-3));

  auto const periods_at_short_term =
      ComputePeriods(*ephemeris,
                     ephemeris->t_max() - 2 * longest_joolian_period_);
  EXPECT_THAT(RelativeError(periods_at_short_term.at(laythe_),
                            expected_periods_.at(laythe_)), Lt(1.6e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(vall_),
                            expected_periods_.at(vall_)), Lt(20.8e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(tylo_),
                            expected_periods_.at(tylo_)), Lt(10.4e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(bop_),
                            expected_periods_.at(bop_)), Lt(63.5e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(pol_),
                            expected_periods_.at(pol_)), Lt(4.2e-3));

  ephemeris->Prolong(mid_term_);
  EXPECT_OK(ephemeris->last_severe_integration_status());
  auto const periods_at_mid_term = 
      ComputePeriods(*ephemeris,
                     ephemeris->t_max() - 2 * longest_joolian_period_);
  EXPECT_THAT(periods_at_mid_term.at(laythe_), Eq(Infinity<Time>()));
  EXPECT_THAT(periods_at_mid_term.at(vall_), Eq(Infinity<Time>()));
  EXPECT_THAT(periods_at_mid_term.at(tylo_), Eq(Infinity<Time>()));
  EXPECT_THAT(RelativeError(periods_at_mid_term.at(bop_),
                            expected_periods_.at(bop_)), Lt(92.5e-3));
  EXPECT_THAT(RelativeError(periods_at_mid_term.at(pol_),
                            expected_periods_.at(pol_)), Lt(31.4e-3));


  //LogEphemeris(*ephemeris, /*reference=*/true, "stock");
}

TEST_F(ResonanceTest, Corrected) {
  // Create a first ephemeris to obtain the elements of the stock game.
  auto ephemeris = MakeEphemeris();

  // Instead of putting the moons in a 1:2:4 resonance, put them in a
  // 1:4/φ:16/φ^2 dissonance.
  constexpr double φ = 1.61803398875;
  elements_[vall_].mean_motion =
      *elements_[laythe_].mean_motion / (4.0 / φ);
  *elements_[tylo_].mean_motion =
      *elements_[laythe_].mean_motion / (16.0 / (φ * φ));

  // All hail Retrobop!
  elements_[bop_].inclination = 180 * Degree - elements_[bop_].inclination;
  *elements_[bop_].mean_motion = *elements_[pol_].mean_motion / 0.7;

  solar_system_.ReplaceElements("Vall", elements_[vall_]);
  solar_system_.ReplaceElements("Tylo", elements_[tylo_]);
  solar_system_.ReplaceElements("Bop", elements_[bop_]);

  // Recreate the ephemeris to use the corrected system.
  ephemeris = MakeEphemeris();
  ephemeris->Prolong(short_term_);
  EXPECT_OK(ephemeris->last_severe_integration_status());

  auto const periods_at_epoch =
      ComputePeriods(*ephemeris, ephemeris->t_min());
  EXPECT_THAT(RelativeError(periods_at_epoch.at(laythe_),
                            expected_periods_.at(laythe_)), Lt(3.9e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(vall_),
                            expected_periods_.at(vall_)), Lt(5.0e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(tylo_),
                            expected_periods_.at(tylo_)), Lt(0.8e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(bop_),
                            expected_periods_.at(bop_)), Lt(12.9e-3));
  EXPECT_THAT(RelativeError(periods_at_epoch.at(pol_),
                            expected_periods_.at(pol_)), Lt(10.6e-3));

  auto const periods_at_short_term =
      ComputePeriods(*ephemeris,
                     ephemeris->t_max() - 2 * longest_joolian_period_);
  EXPECT_THAT(RelativeError(periods_at_short_term.at(laythe_),
                            expected_periods_.at(laythe_)), Lt(5.0e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(vall_),
                            expected_periods_.at(vall_)), Lt(7.8e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(tylo_),
                            expected_periods_.at(tylo_)), Lt(0.8e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(bop_),
                            expected_periods_.at(bop_)), Lt(7.6e-3));
  EXPECT_THAT(RelativeError(periods_at_short_term.at(pol_),
                            expected_periods_.at(pol_)), Lt(21.9-3));

  ephemeris->Prolong(mid_term_);
  EXPECT_OK(ephemeris->last_severe_integration_status());
  auto const periods_at_mid_term = 
      ComputePeriods(*ephemeris,
                     ephemeris->t_max() - 2 * longest_joolian_period_);
  EXPECT_THAT(RelativeError(periods_at_mid_term.at(laythe_),
                            expected_periods_.at(laythe_)), Lt(3.8e-3));
  EXPECT_THAT(RelativeError(periods_at_mid_term.at(vall_),
                            expected_periods_.at(vall_)), Lt(1.2e-3));
  EXPECT_THAT(RelativeError(periods_at_mid_term.at(tylo_),
                            expected_periods_.at(tylo_)), Lt(0.8e-3));
  EXPECT_THAT(RelativeError(periods_at_mid_term.at(bop_),
                            expected_periods_.at(bop_)), Lt(59.8e-3));
  EXPECT_THAT(RelativeError(periods_at_mid_term.at(pol_),
                            expected_periods_.at(pol_)), Lt(19.1e-3));

  ephemeris->Prolong(long_term_);
  EXPECT_OK(ephemeris->last_severe_integration_status());
  auto const periods_at_long_term = 
      ComputePeriods(*ephemeris,
                     ephemeris->t_max() - 2 * longest_joolian_period_);
  EXPECT_THAT(RelativeError(periods_at_long_term.at(laythe_),
                            expected_periods_.at(laythe_)), Lt(5.2e-3));
  EXPECT_THAT(RelativeError(periods_at_long_term.at(vall_),
                            expected_periods_.at(vall_)), Lt(10.0e-3));
  EXPECT_THAT(RelativeError(periods_at_long_term.at(tylo_),
                            expected_periods_.at(tylo_)), Lt(0.7e-3));
  EXPECT_THAT(RelativeError(periods_at_long_term.at(bop_),
                            expected_periods_.at(bop_)), Lt(7.7e-3));
  EXPECT_THAT(RelativeError(periods_at_long_term.at(pol_),
                            expected_periods_.at(pol_)), Lt(4.8e-3));

//  LogEphemeris(*ephemeris, /*reference=*/false, "corrected");
}

#endif

}  // namespace physics
}  // namespace principia
