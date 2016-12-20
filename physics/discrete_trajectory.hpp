﻿
#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>

#include "base/not_null.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/named_quantities.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/forkable.hpp"
#include "quantities/named_quantities.hpp"
#include "serialization/physics.pb.h"

namespace principia {
namespace physics {
namespace internal_discrete_trajectory {

using base::not_null;
using geometry::Instant;
using geometry::Vector;
using geometry::Velocity;
using quantities::Acceleration;
using quantities::Length;
using quantities::Speed;

template<typename Frame>
class DiscreteTrajectory;

}  // namespace internal_discrete_trajectory

// Reopening |internal_forkable| to specialize a template.
namespace internal_forkable {

using internal_discrete_trajectory::DiscreteTrajectory;

template<typename Frame>
struct ForkableTraits<DiscreteTrajectory<Frame>> final {
  using TimelineConstIterator =
      typename std::map<Instant, DegreesOfFreedom<Frame>>::const_iterator;
  static Instant const& time(TimelineConstIterator const it);
};

template<typename Frame>
class DiscreteTrajectoryIterator
    : public ForkableIterator<DiscreteTrajectory<Frame>,
                              DiscreteTrajectoryIterator<Frame>> {
 public:
  Instant const& time() const;
  DegreesOfFreedom<Frame> const& degrees_of_freedom() const;

 protected:
  not_null<DiscreteTrajectoryIterator*> that() override;
  not_null<DiscreteTrajectoryIterator const*> that() const override;
};

}  // namespace internal_forkable

namespace internal_discrete_trajectory {

using internal_forkable::DiscreteTrajectoryIterator;

template <typename Frame>
class DiscreteTrajectory : public Forkable<DiscreteTrajectory<Frame>,
                                           DiscreteTrajectoryIterator<Frame>> {
  using Timeline = std::map<Instant, DegreesOfFreedom<Frame>>;
  using TimelineConstIterator = typename Forkable<
      DiscreteTrajectory<Frame>,
      DiscreteTrajectoryIterator<Frame>>::TimelineConstIterator;

 public:
  using Iterator = DiscreteTrajectoryIterator<Frame>;

  DiscreteTrajectory() = default;
  DiscreteTrajectory(DiscreteTrajectory const&) = delete;
  DiscreteTrajectory(DiscreteTrajectory&&) = delete;
  DiscreteTrajectory& operator=(DiscreteTrajectory const&) = delete;
  DiscreteTrajectory& operator=(DiscreteTrajectory&&) = delete;

  // Returns an iterator at the last point of the trajectory.  Complexity is
  // O(1).  The trajectory must not be empty.
  // TODO(phl): This is really RBegin, but Forkable doesn't have reverse
  // iterators.
  Iterator last() const;

  // Creates a new child trajectory forked at time |time|, and returns it.  The
  // child trajectory shares its data with the current trajectory for times less
  // than or equal to |time|, and is an exact copy of the current trajectory for
  // times greater than |time|.  It may be changed independently from the
  // parent trajectory for any time (strictly) greater than |time|.  The child
  // trajectory is owned by its parent trajectory.  Deleting the parent
  // trajectory deletes all child trajectories.  |time| must be one of the times
  // of this trajectory, and must be at or after the fork time, if any.
  not_null<DiscreteTrajectory<Frame>*> NewForkWithCopy(Instant const& time);

  // Same as above, except that the parent trajectory after the fork point is
  // not copied.
  not_null<DiscreteTrajectory<Frame>*> NewForkWithoutCopy(Instant const& time);

  // Same as above, except that the fork is created at the last point of the
  // trajectory.
  not_null<DiscreteTrajectory<Frame>*> NewForkAtLast();

  // The first point of |fork| is removed from |fork| and appended (using
  // Append) to this trajectory.  Then |fork| is made a fork of this trajectory
  // at the newly-inserted point.  |fork| must be a non-empty root.
  void AttachFork(not_null<std::unique_ptr<DiscreteTrajectory<Frame>>> fork);

  // This object must not be a root.  It is detached from its parent and becomes
  // a root.  A point corresponding to the fork point is prepended to this
  // object (so it's never empty) and an owning pointer to it is returned.
  not_null<std::unique_ptr<DiscreteTrajectory<Frame>>> DetachFork();

  // Appends one point to the trajectory.
  void Append(Instant const& time,
              DegreesOfFreedom<Frame> const& degrees_of_freedom);

  // Removes all data for times (strictly) greater than |time|, as well as all
  // child trajectories forked at times (strictly) greater than |time|.  |time|
  // must be at or after the fork time, if any.
  void ForgetAfter(Instant const& time);

  // Removes all data for times (strictly) less than |time|, and checks that
  // there are no child trajectories forked at times (strictly) less than
  // |time|.  This trajectory must be a root.
  void ForgetBefore(Instant const& time);

  // This trajectory must be a root.  Only the given |forks| are serialized.
  // They must be descended from this trajectory.  The pointers in |forks| may
  // be null at entry.
  void WriteToMessage(
      not_null<serialization::DiscreteTrajectory*> const message,
      std::vector<DiscreteTrajectory<Frame>*> const& forks)
      const;

  // |forks| must have a size appropriate for the |message| being deserialized
  // and the orders of the |forks| must be consistent during serialization and
  // deserialization.  All pointers designated by the pointers in |forks| must
  // be null at entry; they may be null at exit.
  static not_null<std::unique_ptr<DiscreteTrajectory>> ReadFromMessage(
      serialization::DiscreteTrajectory const& message,
      std::vector<DiscreteTrajectory<Frame>**> const& forks);

 protected:
  // The API inherited from Forkable.
  not_null<DiscreteTrajectory*> that() override;
  not_null<DiscreteTrajectory const*> that() const override;

  TimelineConstIterator timeline_begin() const override;
  TimelineConstIterator timeline_end() const override;
  TimelineConstIterator timeline_find(Instant const& time) const override;
  TimelineConstIterator timeline_lower_bound(
                            Instant const& time) const override;
  bool timeline_empty() const override;

 private:
  // This trajectory need not be a root.
  void WriteSubTreeToMessage(
      not_null<serialization::DiscreteTrajectory*> const message,
      std::vector<DiscreteTrajectory<Frame>*>& forks) const;

  void FillSubTreeFromMessage(
      serialization::DiscreteTrajectory const& message,
      std::vector<DiscreteTrajectory<Frame>**> const& forks);

  Timeline timeline_;

  template<typename, typename>
  friend class internal_forkable::ForkableIterator;
  template<typename, typename>
  friend class internal_forkable::Forkable;

  // For using the private constructor in maps.
  template<typename, typename>
  friend struct std::pair;
};

}  // namespace internal_discrete_trajectory

using internal_discrete_trajectory::DiscreteTrajectory;

}  // namespace physics
}  // namespace principia

#include "physics/discrete_trajectory_body.hpp"
