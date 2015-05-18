
#include "integrators/explicit_embedded_runge_kutta_nystr�m_integrator.hpp"

#include "base/macros.hpp"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "quantities/si.hpp"

namespace principia {

using quantities::Length;
using si::Metre;
using si::Second;

namespace integrators {

class ExplicitEmbeddedRungeKuttaNystr�mIntegratorTest : public ::testing::Test {
 protected:
  ExplicitEmbeddedRungeKuttaNystr�mIntegratorTest()
    : integrator_({}, {{0.0}}, {0.0}, {0.0}, {}, {}) {}
  ExplicitEmbeddedRungeKuttaNystr�mIntegrator integrator_;
};

TEST_F(ExplicitEmbeddedRungeKuttaNystr�mIntegratorTest, Dummy) {
  integrator_.Solve<Length>(
      nullptr,
      {{},{},0 * Second},
      0 * Second,
      0 * Second,
      nullptr,
      0,
      nullptr);
}

}  // namespace integrators
}  // namespace principia
