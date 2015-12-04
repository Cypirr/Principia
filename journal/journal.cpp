#include "journal/journal.hpp"

#include <fstream>
#include <list>
#include <string>
#include <type_traits>
#include <vector>

#include "base/array.hpp"
#include "base/get_line.hpp"
#include "base/hexadecimal.hpp"
#include "base/map_util.hpp"
#include "glog/logging.h"

namespace principia {
namespace ksp_plugin {

using base::Bytes;
using base::FindOrDie;
using base::GetLine;
using base::HexadecimalDecode;
using base::HexadecimalEncode;
using base::UniqueBytes;

namespace {

template<typename T>
void Insert(not_null<PointerMap*> const pointer_map,
            std::uint64_t const address,
            T* const pointer) {
  auto inserted = pointer_map->emplace(address, pointer);
  CHECK(inserted.second) << address;
}

template<typename T,
         typename = typename std::enable_if<std::is_pointer<T>::value>::type>
T DeserializePointer(PointerMap const& pointer_map,
                     std::uint64_t const address) {
  return reinterpret_cast<T>(FindOrDie(pointer_map, address));
}

WXYZ DeserializeWXYZ(serialization::WXYZ const& wxyz) {
  return {wxyz.w(), wxyz.x(), wxyz.y(), wxyz.z()};
}

XYZ DeserializeXYZ(serialization::XYZ const& xyz) {
  return {xyz.x(), xyz.y(), xyz.z()};
}

XYZSegment DeserializeXYZSegment(serialization::XYZSegment const& xyz_segment) {
  return {DeserializeXYZ(xyz_segment.begin()),
          DeserializeXYZ(xyz_segment.end())};
}

QP DeserializeQP(serialization::QP const& qp) {
  return {DeserializeXYZ(qp.q()), DeserializeXYZ(qp.p())};
}

KSPPart DeserializeKSPPart(serialization::KSPPart const& ksp_part) {
  return {DeserializeXYZ(ksp_part.world_position()),
          DeserializeXYZ(ksp_part.world_velocity()),
          ksp_part.mass(),
          DeserializeXYZ(
              ksp_part.gravitational_acceleration_to_be_applied_by_ksp()),
          ksp_part.id()};
}

template<typename T>
std::uint64_t SerializePointer(T* t) {
  return reinterpret_cast<std::uint64_t>(t);
}

serialization::WXYZ SerializeWXYZ(WXYZ const& wxyz) {
  serialization::WXYZ m;
  m.set_w(wxyz.w);
  m.set_x(wxyz.x);
  m.set_y(wxyz.y);
  m.set_z(wxyz.z);
  return m;
}

serialization::XYZ SerializeXYZ(XYZ const& xyz) {
  serialization::XYZ m;
  m.set_x(xyz.x);
  m.set_y(xyz.y);
  m.set_z(xyz.z);
  return m;
}

serialization::XYZSegment SerializeXYZSegment(XYZSegment const& xyz_segment) {
  serialization::XYZSegment m;
  *m.mutable_begin() = SerializeXYZ(xyz_segment.begin);
  *m.mutable_end() = SerializeXYZ(xyz_segment.end);
  return m;
}

serialization::QP SerializeQP(QP const& qp) {
  serialization::QP m;
  *m.mutable_p() = SerializeXYZ(qp.p);
  *m.mutable_q() = SerializeXYZ(qp.q);
  return m;
}

serialization::KSPPart SerializeKSPPart(KSPPart const& ksp_part) {
  serialization::KSPPart m;
  *m.mutable_world_position() = SerializeXYZ(ksp_part.world_position);
  *m.mutable_world_velocity() = SerializeXYZ(ksp_part.world_velocity);
  m.set_mass(ksp_part.mass);
  *m.mutable_gravitational_acceleration_to_be_applied_by_ksp() =
      SerializeXYZ(ksp_part.gravitational_acceleration_to_be_applied_by_ksp);
  m.set_id(ksp_part.id);
  return m;
}

}  // namespace

void InitGoogleLogging::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {}

void ActivateJournal::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_activate(in.activate);
}

void ActivateJournal::Run(Message const& message,
                          not_null<PointerMap*> const pointer_map) {
  // Do not run ActivateJournal when replaying because it might create another
  // journal and we must go deeper.
}

void SetBufferedLogging::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_max_severity(in.max_severity);
}

void SetBufferedLogging::Run(Message const& message,
                             not_null<PointerMap*> const pointer_map) {
  principia__SetBufferedLogging(message.in().max_severity());
}

void GetBufferedLogging::Fill(Return const& result,
                              not_null<Message*> const message) {
  message->mutable_return_()->set_get_buffered_logging(result);
}

void GetBufferedLogging::Run(Message const& message,
                             not_null<PointerMap*> const pointer_map) {
  CHECK_EQ(message.return_().get_buffered_logging(),
           principia__GetBufferedLogging());
}

void SetBufferDuration::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_seconds(in.seconds);
}

void SetBufferDuration::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {
  principia__SetBufferDuration(message.in().seconds());
}

void GetBufferDuration::Fill(Return const& result,
                             not_null<Message*> const message) {
  message->mutable_return_()->set_get_buffer_duration(result);
}

void GetBufferDuration::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {
  CHECK_EQ(message.return_().get_buffer_duration(),
           principia__GetBufferDuration());
}

void SetSuppressedLogging::Fill(In const& in,
                                not_null<Message*> const message) {
  message->mutable_in()->set_min_severity(in.min_severity);
}

void SetSuppressedLogging::Run(Message const& message,
                               not_null<PointerMap*> const pointer_map) {
  principia__SetSuppressedLogging(message.in().min_severity());
}

void GetSuppressedLogging::Fill(Return const& result,
                                not_null<Message*> const message) {
  message->mutable_return_()->set_get_suppressed_logging(result);
}

void GetSuppressedLogging::Run(Message const& message,
                               not_null<PointerMap*> const pointer_map) {
  CHECK_EQ(message.return_().get_suppressed_logging(),
           principia__GetSuppressedLogging());
}

void SetVerboseLogging::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_level(in.level);
}

void SetVerboseLogging::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {
  principia__SetVerboseLogging(message.in().level());
}

void GetVerboseLogging::Fill(Return const& result,
                             not_null<Message*> const message) {
  message->mutable_return_()->set_get_verbose_logging(result);
}

void GetVerboseLogging::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {
  CHECK_EQ(message.return_().get_verbose_logging(),
           principia__GetVerboseLogging());
}

void SetStderrLogging::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_min_severity(in.min_severity);
}

void SetStderrLogging::Run(Message const& message,
                           not_null<PointerMap*> const pointer_map) {
  principia__SetStderrLogging(message.in().min_severity());
}

void GetStderrLogging::Fill(Return const& result,
                            not_null<Message*> const message) {
  message->mutable_return_()->set_get_stderr_logging(result);
}

void GetStderrLogging::Run(Message const& message,
                           not_null<PointerMap*> const pointer_map) {
  CHECK_EQ(message.return_().get_stderr_logging(),
           principia__GetStderrLogging());
}

void LogInfo::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_message(in.message);
}

void LogInfo::Run(Message const& message,
                  not_null<PointerMap*> const pointer_map) {
  principia__LogInfo(message.in().message().c_str());
}

void LogWarning::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_message(in.message);
}

void LogWarning::Run(Message const& message,
                     not_null<PointerMap*> const pointer_map) {
  principia__LogWarning(message.in().message().c_str());
}

void LogError::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_message(in.message);
}

void LogError::Run(Message const& message,
                   not_null<PointerMap*> const pointer_map) {
  principia__LogError(message.in().message().c_str());
}

void LogFatal::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_message(in.message);
}

void LogFatal::Run(Message const& message,
                   not_null<PointerMap*> const pointer_map) {
  principia__LogFatal(message.in().message().c_str());
}

void NewPlugin::Fill(In const& in, not_null<Message*> const message) {
  auto* mutable_in = message->mutable_in();
  mutable_in->set_initial_time(in.initial_time);
  mutable_in->set_planetarium_rotation_in_degrees(
      in.planetarium_rotation_in_degrees);
}

void NewPlugin::Fill(Return const& result, not_null<Message*> const message) {
  message->mutable_return_()->set_new_plugin(SerializePointer(result));
}

void NewPlugin::Run(Message const& message,
                    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = principia__NewPlugin(in.initial_time(),
                                      in.planetarium_rotation_in_degrees());
  Insert(pointer_map, message.return_().new_plugin(), plugin);
}

void DeletePlugin::Fill(In const& in, not_null<Message*> const message) {
  message->mutable_in()->set_plugin(SerializePointer(*in.plugin));
}

void DeletePlugin::Fill(Out const& out, not_null<Message*> const message) {
  message->mutable_out()->set_plugin(SerializePointer(*out.plugin));
}

void DeletePlugin::Run(Message const& message,
                       not_null<PointerMap*> const pointer_map) {
  auto* plugin = DeserializePointer<Plugin const*>(*pointer_map,
                                                   message.in().plugin());
  principia__DeletePlugin(&plugin);
  // TODO(phl): should we do something with out() here?
}

void DirectlyInsertCelestial::Fill(In const& in,
                                   not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_celestial_index(in.celestial_index);
  if (in.parent_index != nullptr) {
    m->set_parent_index(*in.parent_index);
  }
  m->set_gravitational_parameter(in.gravitational_parameter);
  if (in.axis_right_ascension != nullptr) {
    m->set_axis_right_ascension(in.axis_right_ascension);
  }
  if (in.axis_declination != nullptr) {
    m->set_axis_declination(in.axis_declination);
  }
  if (in.j2 != nullptr) {
    m->set_j2(in.j2);
  }
  if (in.reference_radius != nullptr) {
    m->set_reference_radius(in.reference_radius);
  }
  m->set_x(in.x);
  m->set_y(in.y);
  m->set_z(in.z);
  m->set_vx(in.vx);
  m->set_vy(in.vy);
  m->set_vz(in.vz);
}

void DirectlyInsertCelestial::Run(Message const& message,
                                  not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  int const parent_index = in.parent_index();
  principia__DirectlyInsertCelestial(
      plugin,
      in.celestial_index(),
      in.has_parent_index() ? &parent_index : nullptr,
      in.gravitational_parameter().c_str(),
      in.has_axis_right_ascension() ?
          in.axis_right_ascension().c_str() : nullptr,
      in.has_axis_declination() ? in.axis_declination().c_str() : nullptr,
      in.has_j2() ? in.j2().c_str() : nullptr,
      in.has_reference_radius() ? in.reference_radius().c_str() : nullptr,
      in.x().c_str(), in.y().c_str(), in.z().c_str(),
      in.vx().c_str(), in.vy().c_str(), in.vz().c_str());
}

void InsertCelestial::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_celestial_index(in.celestial_index);
  m->set_gravitational_parameter(in.gravitational_parameter);
  m->set_parent_index(in.parent_index);
  *m->mutable_from_parent() = SerializeQP(in.from_parent);
}

void InsertCelestial::Run(Message const& message,
                          not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__InsertCelestial(plugin,
                             in.celestial_index(),
                             in.gravitational_parameter(),
                             in.parent_index(),
                             DeserializeQP(in.from_parent()));
}

void InsertSun::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_celestial_index(in.celestial_index);
  m->set_gravitational_parameter(in.gravitational_parameter);
}

void InsertSun::Run(Message const& message,
                    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__InsertSun(plugin,
                       in.celestial_index(),
                       in.gravitational_parameter());
}

void UpdateCelestialHierarchy::Fill(In const& in,
                                    not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_celestial_index(in.celestial_index);
  m->set_parent_index(in.parent_index);
}

void UpdateCelestialHierarchy::Run(Message const& message,
                                   not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__UpdateCelestialHierarchy(plugin,
                                      in.celestial_index(),
                                      in.parent_index());
}

void EndInitialization::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
}

void EndInitialization::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map,
                                             message.in().plugin());
  principia__EndInitialization(plugin);
}

void InsertOrKeepVessel::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  m->set_parent_index(in.parent_index);
}

void InsertOrKeepVessel::Fill(Return const& result,
                              not_null<Message*> const message) {
  auto* m = message->mutable_return_();
  m->set_insert_or_keep_vessel(result);
}

void InsertOrKeepVessel::Run(Message const& message,
                             not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK_EQ(message.return_().insert_or_keep_vessel(),
           principia__InsertOrKeepVessel(plugin,
                                         in.vessel_guid().c_str(),
                                         in.parent_index()));
}

void SetVesselStateOffset::Fill(In const& in,
                                not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  *m->mutable_from_parent() = SerializeQP(in.from_parent);
}

void SetVesselStateOffset::Run(Message const& message,
                               not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__SetVesselStateOffset(plugin,
                                  in.vessel_guid().c_str(),
                                  DeserializeQP(in.from_parent()));
}

void AdvanceTime::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_t(in.t);
  m->set_planetarium_rotation(in.planetarium_rotation);
}

void AdvanceTime::Run(Message const& message,
                      not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__AdvanceTime(plugin, in.t(), in.planetarium_rotation());
}

void ForgetAllHistoriesBefore::Fill(In const& in,
                                    not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_t(in.t);
}

void ForgetAllHistoriesBefore::Run(Message const& message,
                                   not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__ForgetAllHistoriesBefore(plugin, in.t());
}

void VesselFromParent::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
}

void VesselFromParent::Fill(Return const& result,
                            not_null<Message*> const message) {
  *message->mutable_return_()->mutable_vessel_from_parent() =
      SerializeQP(result);
}

void VesselFromParent::Run(Message const& message,
                           not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeQP(message.return_().vessel_from_parent()) ==
            principia__VesselFromParent(plugin, in.vessel_guid().c_str()));
}

void CelestialFromParent::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_celestial_index(in.celestial_index);
}

void CelestialFromParent::Fill(Return const& result,
                               not_null<Message*> const message) {
  *message->mutable_return_()->mutable_celestial_from_parent() =
      SerializeQP(result);
}

void CelestialFromParent::Run(Message const& message,
                              not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeQP(message.return_().celestial_from_parent()) ==
            principia__CelestialFromParent(plugin, in.celestial_index()));
}

void NewBodyCentredNonRotatingNavigationFrame::Fill(
    In const& in,
    not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_reference_body_index(in.reference_body_index);
}

void NewBodyCentredNonRotatingNavigationFrame::Fill(
    Return const& result,
    not_null<Message*> const message) {
  message->mutable_return_()->
      set_new_body_centred_non_rotating_navigation_frame(
          SerializePointer(result));
}

void NewBodyCentredNonRotatingNavigationFrame::Run(
    Message const& message,
    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  auto* navigation_frame = principia__NewBodyCentredNonRotatingNavigationFrame(
                               plugin, in.reference_body_index());
  Insert(pointer_map,
         message.return_().new_body_centred_non_rotating_navigation_frame(),
         navigation_frame);
}

void NewBarycentricRotatingNavigationFrame::Fill(
    In const& in,
    not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_primary_index(in.primary_index);
  m->set_secondary_index(in.secondary_index);
}

void NewBarycentricRotatingNavigationFrame::Fill(
    Return const& result,
    not_null<Message*> const message) {
  message->mutable_return_()->set_new_barycentric_rotating_navigation_frame(
      SerializePointer(result));
}

void NewBarycentricRotatingNavigationFrame::Run(
    Message const& message,
    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  auto* navigation_frame = principia__NewBarycentricRotatingNavigationFrame(
                              plugin, in.primary_index(), in.secondary_index());
  Insert(pointer_map,
         message.return_().new_barycentric_rotating_navigation_frame(),
         navigation_frame);
}

void DeleteNavigationFrame::Fill(In const& in,
                                not_null<Message*> const message) {
  message->mutable_in()->set_navigation_frame(
      SerializePointer(*in.navigation_frame));
}

void DeleteNavigationFrame::Fill(Out const& out,
                                not_null<Message*> const message) {
  message->mutable_out()->set_navigation_frame(
      SerializePointer(*out.navigation_frame));
}

void DeleteNavigationFrame::Run(Message const& message,
                               not_null<PointerMap*> const pointer_map) {
  auto* navigation_frame = DeserializePointer<NavigationFrame*>(
                               *pointer_map, message.in().navigation_frame());
  principia__DeleteNavigationFrame(&navigation_frame);
  // TODO(phl): should we do something with out() here?
}

void UpdatePrediction::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
}

void UpdatePrediction::Run(Message const& message,
                           not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__UpdatePrediction(plugin, in.vessel_guid().c_str());
}

void RenderedVesselTrajectory::Fill(In const& in,
                                    not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  m->set_navigation_frame(SerializePointer(in.navigation_frame));
  *m->mutable_sun_world_position() = SerializeXYZ(in.sun_world_position);
}

void RenderedVesselTrajectory::Fill(Return const& result,
                                    not_null<Message*> const message) {
  message->mutable_return_()->set_rendered_vessel_trajectory(
      SerializePointer(result));
}

void RenderedVesselTrajectory::Run(Message const& message,
                                   not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  auto* navigation_frame = DeserializePointer<NavigationFrame*>(
                              *pointer_map, in.navigation_frame());
  auto* line_and_iterator = principia__RenderedVesselTrajectory(
                                plugin,
                                in.vessel_guid().c_str(),
                                navigation_frame,
                                DeserializeXYZ(in.sun_world_position()));
  Insert(pointer_map,
         message.return_().rendered_vessel_trajectory(),
         line_and_iterator);
}

void HasPrediction::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
}

void HasPrediction::Fill(Return const& result,
                         not_null<Message*> const message) {
  message->mutable_return_()->set_has_prediction(result);
}

void HasPrediction::Run(Message const& message,
                        not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK_EQ(message.return_().has_prediction(),
           principia__HasPrediction(plugin, in.vessel_guid().c_str()));
}

void RenderedPrediction::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  m->set_navigation_frame(SerializePointer(in.navigation_frame));
  *m->mutable_sun_world_position() = SerializeXYZ(in.sun_world_position);
}

void RenderedPrediction::Fill(Return const& result,
                              not_null<Message*> const message) {
  message->mutable_return_()->set_rendered_prediction(
      SerializePointer(result));
}

void RenderedPrediction::Run(Message const& message,
                             not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  auto* navigation_frame = DeserializePointer<NavigationFrame*>(
                              *pointer_map, in.navigation_frame());
  auto* line_and_iterator = principia__RenderedPrediction(
                                plugin,
                                in.vessel_guid().c_str(),
                                navigation_frame,
                                DeserializeXYZ(in.sun_world_position()));
  Insert(pointer_map,
         message.return_().rendered_prediction(),
         line_and_iterator);
}

void FlightPlanSize::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
}

void FlightPlanSize::Fill(Return const& result,
                          not_null<Message*> const message) {
  message->mutable_return_()->set_flight_plan_size(result);
}

void FlightPlanSize::Run(Message const& message,
                         not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK_EQ(message.return_().flight_plan_size(),
           principia__FlightPlanSize(plugin, in.vessel_guid().c_str()));
}

void RenderedFlightPlan::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  m->set_plan_phase(in.plan_phase);
  m->set_navigation_frame(SerializePointer(in.navigation_frame));
  *m->mutable_sun_world_position() = SerializeXYZ(in.sun_world_position);
}

void RenderedFlightPlan::Fill(Return const& result,
                              not_null<Message*> const message) {
  message->mutable_return_()->set_rendered_flight_plan(
      SerializePointer(result));
}

void RenderedFlightPlan::Run(Message const& message,
                             not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  auto* line_and_iterator = principia__RenderedFlightPlan(
                                plugin,
                                in.vessel_guid().c_str(),
                                in.plan_phase(),
                                DeserializePointer<NavigationFrame*>(
                                    *pointer_map, in.navigation_frame()),
                                DeserializeXYZ(in.sun_world_position()));
  Insert(pointer_map,
         message.return_().rendered_flight_plan(),
         line_and_iterator);
}

void SetPredictionLength::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_t(in.t);
}

void SetPredictionLength::Run(Message const& message,
                              not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__SetPredictionLength(plugin, in.t());
}

void SetPredictionLengthTolerance::Fill(In const& in,
                                        not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_l(in.l);
}

void SetPredictionLengthTolerance::Run(
    Message const& message,
    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__SetPredictionLengthTolerance(plugin, in.l());
}

void SetPredictionSpeedTolerance::Fill(In const& in,
                                       not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_v(in.v);
}

void SetPredictionSpeedTolerance::Run(Message const& message,
                                      not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  principia__SetPredictionSpeedTolerance(plugin, in.v());
}

void HasVessel::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
}

void HasVessel::Fill(Return const& result, not_null<Message*> const message) {
  message->mutable_return_()->set_has_vessel(result);
}

void HasVessel::Run(Message const& message,
                    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK_EQ(message.return_().has_vessel(),
           principia__HasVessel(plugin, in.vessel_guid().c_str()));
}

void NumberOfSegments::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_line_and_iterator(SerializePointer(in.line_and_iterator));
}

void NumberOfSegments::Fill(Return const& result,
                            not_null<Message*> const message) {
  message->mutable_return_()->set_number_of_segments(result);
}

void NumberOfSegments::Run(Message const& message,
                           not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  CHECK_EQ(message.return_().number_of_segments(),
           principia__NumberOfSegments(
               DeserializePointer<LineAndIterator const*>(
                   *pointer_map, in.line_and_iterator())));
}

void FetchAndIncrement::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_line_and_iterator(SerializePointer(in.line_and_iterator));
}

void FetchAndIncrement::Fill(Return const& result,
                             not_null<Message*> const message) {
  *message->mutable_return_()->mutable_fetch_and_increment() =
      SerializeXYZSegment(result);
}

void FetchAndIncrement::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  CHECK(DeserializeXYZSegment(message.return_().fetch_and_increment()) ==
            principia__FetchAndIncrement(DeserializePointer<LineAndIterator*>(
                *pointer_map, in.line_and_iterator())));
}

void AtEnd::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_line_and_iterator(SerializePointer(in.line_and_iterator));
}

void AtEnd::Fill(Return const& result, not_null<Message*> const message) {
  message->mutable_return_()->set_at_end(result);
}

void AtEnd::Run(Message const& message,
                not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  CHECK_EQ(message.return_().at_end(),
           principia__AtEnd(DeserializePointer<LineAndIterator*>(
               *pointer_map, in.line_and_iterator())));
}

void DeleteLineAndIterator::Fill(In const& in,
                                 not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_line_and_iterator(SerializePointer(*in.line_and_iterator));
}

void DeleteLineAndIterator::Fill(Out const& out,
                                 not_null<Message*> const message) {
  message->mutable_out()->set_line_and_iterator(
      SerializePointer(*out.line_and_iterator));
}

void DeleteLineAndIterator::Run(Message const& message,
                                not_null<PointerMap*> const pointer_map) {
  auto* line_and_iterator = DeserializePointer<LineAndIterator*>(
                                *pointer_map, message.in().line_and_iterator());
  principia__DeleteLineAndIterator(&line_and_iterator);
  // TODO(phl): should we do something with out() here?
}

void AddVesselToNextPhysicsBubble::Fill(In const& in,
                                        not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  for (KSPPart const* part = in.parts; part < in.parts + in.count; ++part) {
    *m->add_parts() = SerializeKSPPart(*part);
  }
}

void AddVesselToNextPhysicsBubble::Run(
    Message const& message,
    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  std::vector<KSPPart> deserialized_parts;
  deserialized_parts.reserve(in.parts_size());
  for (auto const& part : in.parts()) {
    deserialized_parts.push_back(DeserializeKSPPart(part));
  }
  principia__AddVesselToNextPhysicsBubble(plugin,
                                          in.vessel_guid().c_str(),
                                          &deserialized_parts[0],
                                          deserialized_parts.size());
}

void PhysicsBubbleIsEmpty::Fill(In const& in,
                                not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
}

void PhysicsBubbleIsEmpty::Fill(Return const& result,
                                not_null<Message*> const message) {
  message->mutable_return_()->set_physics_bubble_is_empty(result);
}

void PhysicsBubbleIsEmpty::Run(Message const& message,
                               not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK_EQ(message.return_().physics_bubble_is_empty(),
           principia__PhysicsBubbleIsEmpty(plugin));
}

void BubbleDisplacementCorrection::Fill(In const& in,
                                        not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  *m->mutable_sun_position() = SerializeXYZ(in.sun_position);
}

void BubbleDisplacementCorrection::Fill(Return const& result,
                                        not_null<Message*> const message) {
  *message->mutable_return_()->mutable_bubble_displacement_correction() =
      SerializeXYZ(result);
}

void BubbleDisplacementCorrection::Run(
    Message const& message,
    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeXYZ(message.return_().bubble_displacement_correction()) ==
            principia__BubbleDisplacementCorrection(
                plugin, DeserializeXYZ(in.sun_position())));
}

void BubbleVelocityCorrection::Fill(In const& in,
                                    not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_reference_body_index(in.reference_body_index);
}

void BubbleVelocityCorrection::Fill(Return const& result,
                                    not_null<Message*> const message) {
  *message->mutable_return_()->mutable_bubble_velocity_correction() =
      SerializeXYZ(result);
}

void BubbleVelocityCorrection::Run(Message const& message,
                                   not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeXYZ(message.return_().bubble_velocity_correction()) ==
            principia__BubbleVelocityCorrection(
                plugin, in.reference_body_index()));
}

void NavballOrientation::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_navigation_frame(SerializePointer(in.navigation_frame));
  *m->mutable_sun_world_position() = SerializeXYZ(in.sun_world_position);
  *m->mutable_ship_world_position() = SerializeXYZ(in.ship_world_position);
}

void NavballOrientation::Fill(Return const& result,
                              not_null<Message*> const message) {
  *message->mutable_return_()->mutable_navball_orientation() =
      SerializeWXYZ(result);
}

void NavballOrientation::Run(Message const& message,
                             not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeWXYZ(message.return_().navball_orientation()) ==
            principia__NavballOrientation(
                plugin,
                DeserializePointer<NavigationFrame*>(
                    *pointer_map, in.navigation_frame()),
                DeserializeXYZ(in.sun_world_position()),
                DeserializeXYZ(in.ship_world_position())));
}

void VesselTangent::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  m->set_navigation_frame(SerializePointer(in.navigation_frame));
}

void VesselTangent::Fill(Return const& result,
                         not_null<Message*> const message) {
  *message->mutable_return_()->mutable_vessel_tangent() =
      SerializeXYZ(result);
}

void VesselTangent::Run(Message const& message,
                        not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeXYZ(message.return_().vessel_tangent()) ==
            principia__VesselTangent(plugin,
                                     in.vessel_guid().c_str(),
                                     DeserializePointer<NavigationFrame*>(
                                         *pointer_map, in.navigation_frame())));
}

void VesselNormal::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  m->set_navigation_frame(SerializePointer(in.navigation_frame));
}

void VesselNormal::Fill(Return const& result,
                        not_null<Message*> const message) {
  *message->mutable_return_()->mutable_vessel_normal() =
      SerializeXYZ(result);
}

void VesselNormal::Run(Message const& message,
                       not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeXYZ(message.return_().vessel_normal()) ==
            principia__VesselNormal(plugin,
                                    in.vessel_guid().c_str(),
                                    DeserializePointer<NavigationFrame*>(
                                        *pointer_map, in.navigation_frame())));
}

void VesselBinormal::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_vessel_guid(in.vessel_guid);
  m->set_navigation_frame(SerializePointer(in.navigation_frame));
}

void VesselBinormal::Fill(Return const& result,
                          not_null<Message*> const message) {
  *message->mutable_return_()->mutable_vessel_binormal() =
      SerializeXYZ(result);
}

void VesselBinormal::Run(Message const& message,
                         not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK(DeserializeXYZ(message.return_().vessel_binormal()) ==
            principia__VesselBinormal(
                plugin,
                in.vessel_guid().c_str(),
                DeserializePointer<NavigationFrame*>(
                    *pointer_map, in.navigation_frame())));
}

void CurrentTime::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
}

void CurrentTime::Fill(Return const& result,
                       not_null<Message*> const message) {
  message->mutable_return_()->set_current_time(result);
}

void CurrentTime::Run(Message const& message,
                      not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  CHECK_EQ(message.return_().current_time(),
           principia__CurrentTime(plugin));
}

void SerializePlugin::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_plugin(SerializePointer(in.plugin));
  m->set_serializer(SerializePointer(*in.serializer));
}

void SerializePlugin::Fill(Out const& out, not_null<Message*> const message) {
  auto* m = message->mutable_out();
  m->set_serializer(SerializePointer(*out.serializer));
}

void SerializePlugin::Fill(Return const& result,
                           not_null<Message*> const message) {
  if (result != nullptr) {
    message->mutable_return_()->set_serialize_plugin(result);
  }
}

void SerializePlugin::Run(Message const& message,
                          not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin*>(*pointer_map, in.plugin());
  auto* serializer = DeserializePointer<PullSerializer*>(
                         *pointer_map, in.serializer());
  CHECK_EQ(message.return_().serialize_plugin(),
           principia__SerializePlugin(plugin, &serializer));
}

void DeletePluginSerialization::Fill(In const& in,
                                     not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_serialization(SerializePointer(*in.serialization));
}

void DeletePluginSerialization::Fill(Out const& out,
                                     not_null<Message*> const message) {
  auto* m = message->mutable_out();
  m->set_serialization(SerializePointer(*out.serialization));
}

void DeletePluginSerialization::Run(Message const& message,
                                    not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* serialization = DeserializePointer<char const*>(
                            *pointer_map, in.serialization());
  principia__DeletePluginSerialization(&serialization);
}

void DeserializePlugin::Fill(In const& in, not_null<Message*> const message) {
  auto* m = message->mutable_in();
  m->set_serialization(std::string(in.serialization, in.serialization_size));
  m->set_deserializer(SerializePointer(*in.deserializer));
  m->set_plugin(SerializePointer(*in.plugin));
}

void DeserializePlugin::Fill(Out const& out, not_null<Message*> const message) {
  auto* m = message->mutable_out();
  m->set_deserializer(SerializePointer(*out.deserializer));
  m->set_plugin(SerializePointer(*out.plugin));
}

void DeserializePlugin::Run(Message const& message,
                            not_null<PointerMap*> const pointer_map) {
  auto const& in = message.in();
  auto* plugin = DeserializePointer<Plugin const*>(*pointer_map, in.plugin());
  auto* deserializer = DeserializePointer<PushDeserializer*>(
                           *pointer_map, in.deserializer());
  principia__DeserializePlugin(in.serialization().c_str(),
                               in.serialization().size(),
                               &deserializer,
                               &plugin);
}

void SayHello::Fill(Return const& result, not_null<Message*> const message) {
  message->mutable_return_()->set_say_hello(result);
}

void SayHello::Run(Message const& message,
                   not_null<PointerMap*> const pointer_map) {
  CHECK_EQ(message.return_().say_hello(),
           principia__SayHello());
}

Journal::Journal(std::experimental::filesystem::path const& path)
    : stream_(path, std::ios::out) {
  CHECK(!stream_.fail()) << path;
}

Journal::~Journal() {
  stream_.close();
}

void Journal::Write(serialization::Method const& method) {
  UniqueBytes bytes(method.ByteSize());
  method.SerializeToArray(bytes.data.get(), static_cast<int>(bytes.size));

  std::int64_t const hexadecimal_size = (bytes.size << 1) + 2;
  UniqueBytes hexadecimal(hexadecimal_size);
  HexadecimalEncode({bytes.data.get(), bytes.size}, hexadecimal.get());
  hexadecimal.data.get()[hexadecimal_size - 2] = '\n';
  hexadecimal.data.get()[hexadecimal_size - 1] = '\0';
  stream_ << hexadecimal.data.get();
  stream_.flush();
}

void Journal::Activate(base::not_null<Journal*> const journal) {
  CHECK(active_ == nullptr);
  active_ = journal;
}

void Journal::Deactivate() {
  CHECK(active_ != nullptr);
  delete active_;
  active_ = nullptr;
}

bool Journal::IsActivated() {
  return active_ != nullptr;
}

Player::Player(std::experimental::filesystem::path const& path)
    : stream_(path, std::ios::in) {}

bool Player::Play() {
  std::unique_ptr<serialization::Method> method = Read();
  if (method == nullptr) {
    return false;
  }

  bool ran = false;
  ran |= RunIfAppropriate<ActivateJournal>(*method);
  ran |= RunIfAppropriate<AddVesselToNextPhysicsBubble>(*method);
  ran |= RunIfAppropriate<AdvanceTime>(*method);
  ran |= RunIfAppropriate<AtEnd>(*method);
  ran |= RunIfAppropriate<BubbleDisplacementCorrection>(*method);
  ran |= RunIfAppropriate<BubbleVelocityCorrection>(*method);
  ran |= RunIfAppropriate<CelestialFromParent>(*method);
  ran |= RunIfAppropriate<CurrentTime>(*method);
  ran |= RunIfAppropriate<DeleteLineAndIterator>(*method);
  ran |= RunIfAppropriate<DeleteNavigationFrame>(*method);
  ran |= RunIfAppropriate<DeletePlugin>(*method);
  ran |= RunIfAppropriate<DeletePluginSerialization>(*method);
  ran |= RunIfAppropriate<DeserializePlugin>(*method);
  ran |= RunIfAppropriate<DirectlyInsertCelestial>(*method);
  ran |= RunIfAppropriate<EndInitialization>(*method);
  ran |= RunIfAppropriate<FetchAndIncrement>(*method);
  ran |= RunIfAppropriate<FlightPlanSize>(*method);
  ran |= RunIfAppropriate<ForgetAllHistoriesBefore>(*method);
  ran |= RunIfAppropriate<GetBufferDuration>(*method);
  ran |= RunIfAppropriate<GetBufferedLogging>(*method);
  ran |= RunIfAppropriate<GetStderrLogging>(*method);
  ran |= RunIfAppropriate<GetSuppressedLogging>(*method);
  ran |= RunIfAppropriate<GetVerboseLogging>(*method);
  ran |= RunIfAppropriate<HasPrediction>(*method);
  ran |= RunIfAppropriate<HasVessel>(*method);
  ran |= RunIfAppropriate<InitGoogleLogging>(*method);
  ran |= RunIfAppropriate<InsertCelestial>(*method);
  ran |= RunIfAppropriate<InsertOrKeepVessel>(*method);
  ran |= RunIfAppropriate<InsertSun>(*method);
  ran |= RunIfAppropriate<LogError>(*method);
  ran |= RunIfAppropriate<LogFatal>(*method);
  ran |= RunIfAppropriate<LogInfo>(*method);
  ran |= RunIfAppropriate<LogWarning>(*method);
  ran |= RunIfAppropriate<NavballOrientation>(*method);
  ran |= RunIfAppropriate<NewBarycentricRotatingNavigationFrame>(*method);
  ran |= RunIfAppropriate<NewBodyCentredNonRotatingNavigationFrame>(*method);
  ran |= RunIfAppropriate<NewPlugin>(*method);
  ran |= RunIfAppropriate<NumberOfSegments>(*method);
  ran |= RunIfAppropriate<PhysicsBubbleIsEmpty>(*method);
  ran |= RunIfAppropriate<RenderedFlightPlan>(*method);
  ran |= RunIfAppropriate<RenderedPrediction>(*method);
  ran |= RunIfAppropriate<RenderedVesselTrajectory>(*method);
  ran |= RunIfAppropriate<SayHello>(*method);
  ran |= RunIfAppropriate<SerializePlugin>(*method);
  ran |= RunIfAppropriate<SetBufferDuration>(*method);
  ran |= RunIfAppropriate<SetBufferedLogging>(*method);
  ran |= RunIfAppropriate<SetPredictionLength>(*method);
  ran |= RunIfAppropriate<SetPredictionLengthTolerance>(*method);
  ran |= RunIfAppropriate<SetPredictionSpeedTolerance>(*method);
  ran |= RunIfAppropriate<SetStderrLogging>(*method);
  ran |= RunIfAppropriate<SetSuppressedLogging>(*method);
  ran |= RunIfAppropriate<SetVerboseLogging>(*method);
  ran |= RunIfAppropriate<SetVesselStateOffset>(*method);
  ran |= RunIfAppropriate<UpdateCelestialHierarchy>(*method);
  ran |= RunIfAppropriate<UpdatePrediction>(*method);
  ran |= RunIfAppropriate<VesselFromParent>(*method);
  ran |= RunIfAppropriate<VesselBinormal>(*method);
  ran |= RunIfAppropriate<VesselNormal>(*method);
  ran |= RunIfAppropriate<VesselTangent>(*method);
  CHECK(ran) << method->DebugString();

  return true;
}

std::unique_ptr<serialization::Method> Player::Read() {
  std::string const line = GetLine(&stream_);
  if (line.empty()) {
    return nullptr;
  }

  uint8_t const* const hexadecimal =
      reinterpret_cast<uint8_t const*>(line.c_str());
  int const hexadecimal_size = strlen(line.c_str());
  UniqueBytes bytes(hexadecimal_size >> 1);
  HexadecimalDecode({hexadecimal, hexadecimal_size},
                    {bytes.data.get(), bytes.size});
  auto method = std::make_unique<serialization::Method>();
  CHECK(method->ParseFromArray(bytes.data.get(),
                               static_cast<int>(bytes.size)));

  return method;
}

Journal* Journal::active_ = nullptr;

}  // namespace ksp_plugin
}  // namespace principia
