#pragma once

#include <vector>

#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/man�uvre.hpp"
#include "physics/discrete_trajectory.hpp"

namespace principia {
namespace ksp_plugin {

class FlightPlan {
 public:
  FlightPlan(not_null<DiscreteTrajectory<Barycentric>*> root);
  ~FlightPlan() = default;

  int size() const;
  Man�uvre<Barycentric, Navigation> const& Get(int const index) const;
  void Delete(int const index);

  // The following three functions have no effect and return false if the
  // man�uvre is invalid.
  bool InsertBefore(int const index,
                    Man�uvre<Barycentric, Navigation> const& man�uvre);
  bool InsertAfter(int const index,
                   Man�uvre<Barycentric, Navigation> const& man�uvre);
  bool Replace(int const index,
               Man�uvre<Barycentric, Navigation> const& man�uvre);


 private:
  std
  std::vector<not_null<DiscreteTrajectory<Barycentric>*>> trajectories_;
  std::vector<Man�uvre<Barycentric, Navigation>> man�uvres_;
};

}  // namespace ksp_plugin
}  // namespace principia
