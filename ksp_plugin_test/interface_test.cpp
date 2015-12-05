﻿
#include "ksp_plugin/interface.hpp"

#include <string>

#include "base/not_null.hpp"
#include "base/pull_serializer.hpp"
#include "base/push_deserializer.hpp"
#include "geometry/epoch.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "physics/mock_dynamic_frame.hpp"
#include "quantities/si.hpp"
#include "journal/recorder.hpp"
#include "ksp_plugin/mock_plugin.hpp"

namespace principia {

using base::check_not_null;
using base::PullSerializer;
using base::PushDeserializer;
using geometry::Displacement;
using geometry::kUnixEpoch;
using physics::MockDynamicFrame;
using quantities::Pow;
using quantities::si::AstronomicalUnit;
using quantities::si::Day;
using quantities::si::Degree;
using quantities::si::Kilo;
using quantities::si::Metre;
using quantities::si::Second;
using quantities::si::Tonne;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Property;
using ::testing::ExitedWithCode;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Ref;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::_;

namespace ksp_plugin {

char const kSerializedBoringPlugin[] =
    "\x12\xD2\x1\b\0\x12\xCD\x1\n\xF\n\r\b\x83\xF0\x1\x11\0\0\0\0\0\0\xF0?\x12"
    "\xB9\x1\n\xAE\x1\n\x12\n\xE\x12\f\b\x80\b\x11\0\0\0\0\0\0\0\0\x12\0\x12"
    "\x97\x1\n\xE\x12\f\b\x80\b\x11\0\0\0\0\0\0\0\0\x12\x84\x1\n>\x12<\n:\n-\n"
    "\r\x12\v\b\x1\x11\0\0\0\0\0\0\0\0\x12\r\x12\v\b\x1\x11\0\0\0\0\0\0\0\0\x1A"
    "\r\x12\v\b\x1\x11\0\0\0\0\0\0\0\0\"\t\r\xAF\x1F\xB1y\x10\x3\x18\x1\x12"
    "B\n@\n3\n\xF\x12\r\b\x81\xF8\x1\x11\0\0\0\0\0\0\0\0\x12\xF\x12\r\b\x81\xF8"
    "\x1\x11\0\0\0\0\0\0\0\0\x1A\xF\x12\r\b\x81\xF8\x1\x11\0\0\0\0\0\0\0\0\"\t"
    "\r\xAF\x1F\xB1y\x10\x3\x18\x1\x12\x6\n\x4\b\0\x10\0\x1A\x2\n\0\"\x10\b\x80"
    "\x80\x80\x80\x80\x1\x11\0\0\0\0\0\0\0\0*\xE\x12\f\b\x80\b\x11\0\0\0\0\0\0"
    "\0\0""0\0";

char const kHexadecimalBoringPlugin[] =
    "12D201080012CD010A0F0A0D0883F00111000000000000F03F12B9010AAE010A120A0E120C"
    "08800811000000000000000012001297010A0E120C0880081100000000000000001284010A"
    "3E123C0A3A0A2D0A0D120B0801110000000000000000120D120B0801110000000000000000"
    "1A0D120B080111000000000000000022090DAF1FB1791003180112420A400A330A0F120D08"
    "81F801110000000000000000120F120D0881F8011100000000000000001A0F120D0881F801"
    "11000000000000000022090DAF1FB1791003180112060A04080010001A020A002210088080"
    "808080011100000000000000002A0E120C0880081100000000000000003000";

char const kVesselGUID[] = "NCC-1701-D";

Index const kCelestialIndex = 1;
Index const kParentIndex = 2;

double const kGravitationalParameter = 3;
double const kPlanetariumRotation = 10;
double const kTime = 11;

XYZ kParentPosition = {4, 5, 6};
XYZ kParentVelocity = {7, 8, 9};
QP kParentRelativeDegreesOfFreedom = {kParentPosition, kParentVelocity};

int const kTrajectorySize = 10;

ACTION_TEMPLATE(FillUniquePtr,
                // Note the comma between int and k:
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(ptr)) {
  std::tr1::get<k>(args)->reset(ptr);
}

class InterfaceTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    std::string const test_case_name =
        testing::UnitTest::GetInstance()->current_test_case()->name();
    recorder_ = new journal::Recorder(test_case_name + ".journal.hex");
    journal::Recorder::Activate(recorder_);
  }

  static void TearDownTestCase() {
    journal::Recorder::Deactivate();
  }

  InterfaceTest()
      : plugin_(make_not_null_unique<StrictMock<MockPlugin>>()) {}

  not_null<std::unique_ptr<StrictMock<MockPlugin>>> plugin_;
  Instant const t0_;
  static journal::Recorder* recorder_;
};

journal::Recorder* InterfaceTest::recorder_ = nullptr;

using InterfaceDeathTest = InterfaceTest;

// And there is only one thing we say to Death.
TEST_F(InterfaceDeathTest, Errors) {
  Plugin* plugin = nullptr;
  EXPECT_DEATH({
    principia__DeletePlugin(nullptr);
  }, "non NULL");
  EXPECT_DEATH({
    principia__InsertCelestial(plugin,
                               kCelestialIndex,
                               kGravitationalParameter,
                               kParentIndex,
                               kParentRelativeDegreesOfFreedom);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__UpdateCelestialHierarchy(plugin, kCelestialIndex, kParentIndex);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__UpdateCelestialHierarchy(plugin, kCelestialIndex, kParentIndex);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__InsertOrKeepVessel(plugin, kVesselGUID, kParentIndex);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__SetVesselStateOffset(plugin,
                                    kVesselGUID,
                                    kParentRelativeDegreesOfFreedom);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__VesselFromParent(plugin, kVesselGUID);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__CelestialFromParent(plugin, kCelestialIndex);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__NewBodyCentredNonRotatingNavigationFrame(plugin,
                                                        kCelestialIndex);
  }, "plugin.*non NULL");
  EXPECT_DEATH({
    principia__LogFatal("a fatal error");
  }, "a fatal error");
}

TEST_F(InterfaceTest, InitGoogleLogging1) {
  principia__InitGoogleLogging();
}

TEST_F(InterfaceDeathTest, InitGoogleLogging2) {
  // We use EXPECT_EXIT in this test to avoid interfering with the execution of
  // the other tests.
  int const kExitCode = 66;

  EXPECT_EXIT({
    google::ShutdownGoogleLogging();
    principia__InitGoogleLogging();
    exit(kExitCode);
  }, ExitedWithCode(kExitCode), "");
}

TEST_F(InterfaceDeathTest, ActivateRecorder) {
  EXPECT_DEATH({
    journal::Recorder::Deactivate();
    // Fails because the glog directory doesn't exist.
    principia__ActivateRecorder(true);
  }, "glog.Principia.JOURNAL");
}

TEST_F(InterfaceTest, Log) {
  principia__LogInfo("An info");
  principia__LogWarning("A warning");
  principia__LogError("An error");
}

TEST_F(InterfaceTest, NewPlugin) {
  std::unique_ptr<Plugin> plugin(principia__NewPlugin(
                                     kTime,
                                     kPlanetariumRotation));
  EXPECT_THAT(plugin, Not(IsNull()));
}

TEST_F(InterfaceTest, DeletePlugin) {
  Plugin const* plugin = plugin_.release();
  principia__DeletePlugin(&plugin);
  EXPECT_THAT(plugin, IsNull());
}

TEST_F(InterfaceTest, InsertCelestial) {
  EXPECT_CALL(*plugin_,
              InsertCelestial(
                  kCelestialIndex,
                  kGravitationalParameter * SIUnit<GravitationalParameter>(),
                  kParentIndex,
                  RelativeDegreesOfFreedom<AliceSun>(
                      Displacement<AliceSun>(
                          {kParentPosition.x * SIUnit<Length>(),
                           kParentPosition.y * SIUnit<Length>(),
                           kParentPosition.z * SIUnit<Length>()}),
                      Velocity<AliceSun>(
                          {kParentVelocity.x * SIUnit<Speed>(),
                           kParentVelocity.y * SIUnit<Speed>(),
                           kParentVelocity.z * SIUnit<Speed>()}))));
  principia__InsertCelestial(plugin_.get(),
                             kCelestialIndex,
                             kGravitationalParameter,
                             kParentIndex,
                             kParentRelativeDegreesOfFreedom);
}

TEST_F(InterfaceTest, InsertSun) {
  EXPECT_CALL(*plugin_,
              InsertSun(
                  kCelestialIndex,
                  kGravitationalParameter * SIUnit<GravitationalParameter>()));
  principia__InsertSun(plugin_.get(),
                       kCelestialIndex,
                       kGravitationalParameter);
}


TEST_F(InterfaceTest, DirectlyInsertMassiveCelestial) {
  EXPECT_CALL(
      *plugin_,
      DirectlyInsertCelestialConstRef(
          kCelestialIndex,
          &kParentIndex,
          DegreesOfFreedom<Barycentric>(
              Barycentric::origin +
              Displacement<Barycentric>(
                  {0 * Metre,
                   23.456E-7 * Kilo(Metre),
                   -1 * AstronomicalUnit}),
              Velocity<Barycentric>(
                  {1 * AstronomicalUnit / Day,
                   1 * Kilo(Metre) / Second,
                   1 * Metre / Second})),
          Pointee(
              AllOf(Property(&MassiveBody::is_oblate, false),
                    Property(&MassiveBody::gravitational_parameter,
                             1.2345E6 * SIUnit<GravitationalParameter>())))));
  principia__DirectlyInsertCelestial(plugin_.get(),
                                     kCelestialIndex,
                                     &kParentIndex,
                                     "1.2345E6  m^3/s^2",
                                     nullptr /*axis_right_ascension*/,
                                     nullptr /*axis_declination*/,
                                     nullptr /*j2*/,
                                     nullptr /*reference_radius*/,
                                     "0 m",
                                     "23.456E-7 km",
                                     "-1 au",
                                     "1 au / d",
                                     "  1 km/s",
                                     "1  m / s");
}

TEST_F(InterfaceTest, DirectlyInsertOblateCelestial) {
  EXPECT_CALL(
      *plugin_,
      DirectlyInsertCelestialConstRef(
          kCelestialIndex,
          &kParentIndex,
          DegreesOfFreedom<Barycentric>(
              Barycentric::origin +
              Displacement<Barycentric>(
                  {0 * Metre,
                   23.456E-7 * Kilo(Metre),
                   -1 * AstronomicalUnit}),
              Velocity<Barycentric>(
                  {1 * AstronomicalUnit / Day,
                   1 * Kilo(Metre) / Second,
                   1 * Metre / Second})),
          Pointee(
              AllOf(Property(&MassiveBody::is_oblate, true),
                    Property(&MassiveBody::gravitational_parameter,
                             1.2345E6 *
                                 Pow<3>(Kilo(Metre)) / Pow<2>(Second))))));
  principia__DirectlyInsertCelestial(plugin_.get(),
                                     kCelestialIndex,
                                     &kParentIndex,
                                     "1.2345E6  km^3 / s^2",
                                     "42 deg",
                                     u8"8°",
                                     "123e-6",
                                     "1000 km",
                                     "0 m",
                                     "23.456E-7 km",
                                     "-1 au",
                                     "1 au / d",
                                     "  1 km/s",
                                     "1  m / s");
}

TEST_F(InterfaceTest, UpdateCelestialHierarchy) {
  EXPECT_CALL(*plugin_,
              UpdateCelestialHierarchy(kCelestialIndex, kParentIndex));
  principia__UpdateCelestialHierarchy(plugin_.get(),
                                      kCelestialIndex,
                                      kParentIndex);
}

TEST_F(InterfaceTest, EndInitialization) {
  EXPECT_CALL(*plugin_,
              EndInitialization());
  principia__EndInitialization(plugin_.get());
}

TEST_F(InterfaceTest, InsertOrKeepVessel) {
  EXPECT_CALL(*plugin_,
              InsertOrKeepVessel(kVesselGUID, kParentIndex));
  EXPECT_CALL(*plugin_, HasVessel(kVesselGUID))
      .WillOnce(Return(false))
      .WillOnce(Return(true));
  EXPECT_FALSE(plugin_->HasVessel(kVesselGUID));
  principia__InsertOrKeepVessel(plugin_.get(), kVesselGUID, kParentIndex);
  EXPECT_TRUE(plugin_->HasVessel(kVesselGUID));
}

TEST_F(InterfaceTest, SetVesselStateOffset) {
  EXPECT_CALL(*plugin_,
              SetVesselStateOffset(
                  kVesselGUID,
                  RelativeDegreesOfFreedom<AliceSun>(
                      Displacement<AliceSun>(
                          {kParentPosition.x * SIUnit<Length>(),
                           kParentPosition.y * SIUnit<Length>(),
                           kParentPosition.z * SIUnit<Length>()}),
                      Velocity<AliceSun>(
                          {kParentVelocity.x * SIUnit<Speed>(),
                           kParentVelocity.y * SIUnit<Speed>(),
                           kParentVelocity.z * SIUnit<Speed>()}))));
  principia__SetVesselStateOffset(plugin_.get(),
                                  kVesselGUID,
                                  kParentRelativeDegreesOfFreedom);
}

TEST_F(InterfaceTest, AdvanceTime) {
  EXPECT_CALL(*plugin_,
              AdvanceTime(t0_ + kTime * SIUnit<Time>(),
                          kPlanetariumRotation * Degree));
  principia__AdvanceTime(plugin_.get(), kTime, kPlanetariumRotation);
}

TEST_F(InterfaceTest, ForgetAllHistoriesBefore) {
  EXPECT_CALL(*plugin_,
              ForgetAllHistoriesBefore(t0_ + kTime * SIUnit<Time>()));
  principia__ForgetAllHistoriesBefore(plugin_.get(), kTime);
}

TEST_F(InterfaceTest, VesselFromParent) {
  EXPECT_CALL(*plugin_,
              VesselFromParent(kVesselGUID))
      .WillOnce(Return(RelativeDegreesOfFreedom<AliceSun>(
                           Displacement<AliceSun>(
                               {kParentPosition.x * SIUnit<Length>(),
                                kParentPosition.y * SIUnit<Length>(),
                                kParentPosition.z * SIUnit<Length>()}),
                           Velocity<AliceSun>(
                               {kParentVelocity.x * SIUnit<Speed>(),
                                kParentVelocity.y * SIUnit<Speed>(),
                                kParentVelocity.z * SIUnit<Speed>()}))));
  QP const result = principia__VesselFromParent(plugin_.get(),
                                                kVesselGUID);
  EXPECT_THAT(result, Eq(kParentRelativeDegreesOfFreedom));
}

TEST_F(InterfaceTest, CelestialFromParent) {
  EXPECT_CALL(*plugin_,
              CelestialFromParent(kCelestialIndex))
      .WillOnce(Return(RelativeDegreesOfFreedom<AliceSun>(
                           Displacement<AliceSun>(
                               {kParentPosition.x * SIUnit<Length>(),
                                kParentPosition.y * SIUnit<Length>(),
                                kParentPosition.z * SIUnit<Length>()}),
                           Velocity<AliceSun>(
                               {kParentVelocity.x * SIUnit<Speed>(),
                                kParentVelocity.y * SIUnit<Speed>(),
                                kParentVelocity.z * SIUnit<Speed>()}))));
  QP const result = principia__CelestialFromParent(plugin_.get(),
                                                    kCelestialIndex);
  EXPECT_THAT(result, Eq(kParentRelativeDegreesOfFreedom));
}

TEST_F(InterfaceTest, NewBodyCentredNonRotatingNavigationFrame) {
  StrictMock<MockDynamicFrame<Barycentric, Navigation>>* const
     mock_navigation_frame =
         new StrictMock<MockDynamicFrame<Barycentric, Navigation>>;
  EXPECT_CALL(*plugin_,
              FillBodyCentredNonRotatingNavigationFrame(kCelestialIndex, _))
      .WillOnce(FillUniquePtr<1>(mock_navigation_frame));
  std::unique_ptr<NavigationFrame> navigation_frame(
      principia__NewBodyCentredNonRotatingNavigationFrame(plugin_.get(),
                                                          kCelestialIndex));
  EXPECT_EQ(mock_navigation_frame, navigation_frame.get());
}

TEST_F(InterfaceTest, NewBarycentricRotatingNavigationFrame) {
  StrictMock<MockDynamicFrame<Barycentric, Navigation>>* const
     mock_navigation_frame =
         new StrictMock<MockDynamicFrame<Barycentric, Navigation>>;
  EXPECT_CALL(*plugin_,
              FillBarycentricRotatingNavigationFrame(kCelestialIndex,
                                                     kParentIndex,
                                                     _))
      .WillOnce(FillUniquePtr<2>(mock_navigation_frame));
  std::unique_ptr<NavigationFrame> navigation_frame(
      principia__NewBarycentricRotatingNavigationFrame(plugin_.get(),
                                                       kCelestialIndex,
                                                       kParentIndex));
  EXPECT_EQ(mock_navigation_frame, navigation_frame.get());
}

TEST_F(InterfaceTest, DeleteNavigationFrame) {
  StrictMock<MockDynamicFrame<Barycentric, Navigation>>* const
     mock_navigation_frame =
         new StrictMock<MockDynamicFrame<Barycentric, Navigation>>;
  EXPECT_CALL(*plugin_,
              FillBarycentricRotatingNavigationFrame(kCelestialIndex,
                                                     kParentIndex,
                                                     _))
      .WillOnce(FillUniquePtr<2>(mock_navigation_frame));
  NavigationFrame* navigation_frame(
      principia__NewBarycentricRotatingNavigationFrame(plugin_.get(),
                                                       kCelestialIndex,
                                                       kParentIndex));
  EXPECT_EQ(mock_navigation_frame, navigation_frame);
  principia__DeleteNavigationFrame(&navigation_frame);
  EXPECT_THAT(navigation_frame, IsNull());
}

TEST_F(InterfaceTest, RenderedPrediction) {
  StrictMock<MockDynamicFrame<Barycentric, Navigation>>* const
     mock_navigation_frame =
         new StrictMock<MockDynamicFrame<Barycentric, Navigation>>;
  EXPECT_CALL(*plugin_,
              FillBarycentricRotatingNavigationFrame(kCelestialIndex,
                                                    kParentIndex,
                                                    _))
      .WillOnce(FillUniquePtr<2>(mock_navigation_frame));
  NavigationFrame* navigation_frame =
      principia__NewBarycentricRotatingNavigationFrame(plugin_.get(),
                                                      kCelestialIndex,
                                                      kParentIndex);

  // Construct a test rendered trajectory.
  RenderedTrajectory<World> rendered_trajectory;
  Position<World> position =
      World::origin + Displacement<World>({1 * SIUnit<Length>(),
                                           2 * SIUnit<Length>(),
                                           3 * SIUnit<Length>()});
  for (int i = 0; i < kTrajectorySize; ++i) {
    Position<World> next_position =
        position + Displacement<World>({10 * SIUnit<Length>(),
                                        20 * SIUnit<Length>(),
                                        30 * SIUnit<Length>()});
    LineSegment<World> line_segment(position, next_position);
    rendered_trajectory.push_back(line_segment);
    position = next_position;
  }

  EXPECT_CALL(*plugin_,
              RenderedPrediction(
                  kVesselGUID,
                  World::origin + Displacement<World>(
                                      {kParentPosition.x * SIUnit<Length>(),
                                       kParentPosition.y * SIUnit<Length>(),
                                       kParentPosition.z * SIUnit<Length>()})))
      .WillOnce(Return(rendered_trajectory));
  LineAndIterator* line_and_iterator =
      principia__RenderedPrediction(plugin_.get(),
                                    kVesselGUID,
                                    navigation_frame,
                                    kParentPosition);
  EXPECT_EQ(kTrajectorySize, line_and_iterator->rendered_trajectory.size());
  EXPECT_EQ(kTrajectorySize, principia__NumberOfSegments(line_and_iterator));

  // Traverse it and check that we get the right data.
  for (int i = 0; i < kTrajectorySize; ++i) {
    EXPECT_FALSE(principia__AtEnd(line_and_iterator));
    XYZSegment const segment = principia__FetchAndIncrement(line_and_iterator);
    EXPECT_EQ(1 + 10 * i, segment.begin.x);
    EXPECT_EQ(2 + 20 * i, segment.begin.y);
    EXPECT_EQ(3 + 30 * i, segment.begin.z);
    EXPECT_EQ(11 + 10 * i, segment.end.x);
    EXPECT_EQ(22 + 20 * i, segment.end.y);
    EXPECT_EQ(33 + 30 * i, segment.end.z);
  }
  EXPECT_TRUE(principia__AtEnd(line_and_iterator));

  // Delete it.
  EXPECT_THAT(line_and_iterator, Not(IsNull()));
  principia__DeleteLineAndIterator(&line_and_iterator);
  EXPECT_THAT(line_and_iterator, IsNull());
  EXPECT_EQ(mock_navigation_frame, navigation_frame);
  principia__DeleteNavigationFrame(&navigation_frame);
  EXPECT_THAT(navigation_frame, IsNull());
}

TEST_F(InterfaceTest, LineAndIterator) {
  StrictMock<MockDynamicFrame<Barycentric, Navigation>>* const
     mock_navigation_frame =
         new StrictMock<MockDynamicFrame<Barycentric, Navigation>>;
  EXPECT_CALL(*plugin_,
              FillBarycentricRotatingNavigationFrame(kCelestialIndex,
                                                     kParentIndex,
                                                     _))
      .WillOnce(FillUniquePtr<2>(mock_navigation_frame));
  NavigationFrame* navigation_frame =
      principia__NewBarycentricRotatingNavigationFrame(plugin_.get(),
                                                       kCelestialIndex,
                                                       kParentIndex);

  // Construct a test rendered trajectory.
  RenderedTrajectory<World> rendered_trajectory;
  Position<World> position =
      World::origin + Displacement<World>(
                          {1 * SIUnit<Length>(),
                           2 * SIUnit<Length>(),
                           3 * SIUnit<Length>()});
  for (int i = 0; i < kTrajectorySize; ++i) {
    Position<World> next_position =
        position + Displacement<World>({10 * SIUnit<Length>(),
                                        20 * SIUnit<Length>(),
                                        30 * SIUnit<Length>()});
    LineSegment<World> line_segment(position, next_position);
    rendered_trajectory.push_back(line_segment);
    position = next_position;
  }

  // Construct a LineAndIterator.
  EXPECT_CALL(*plugin_,
              RenderedVesselTrajectory(
                  kVesselGUID,
                  World::origin + Displacement<World>(
                                      {kParentPosition.x * SIUnit<Length>(),
                                       kParentPosition.y * SIUnit<Length>(),
                                       kParentPosition.z * SIUnit<Length>()})))
      .WillOnce(Return(rendered_trajectory));
  LineAndIterator* line_and_iterator =
      principia__RenderedVesselTrajectory(plugin_.get(),
                                          kVesselGUID,
                                          navigation_frame,
                                          kParentPosition);
  EXPECT_EQ(kTrajectorySize, line_and_iterator->rendered_trajectory.size());
  EXPECT_EQ(kTrajectorySize, principia__NumberOfSegments(line_and_iterator));

  // Traverse it and check that we get the right data.
  for (int i = 0; i < kTrajectorySize; ++i) {
    EXPECT_FALSE(principia__AtEnd(line_and_iterator));
    XYZSegment const segment = principia__FetchAndIncrement(line_and_iterator);
    EXPECT_EQ(1 + 10 * i, segment.begin.x);
    EXPECT_EQ(2 + 20 * i, segment.begin.y);
    EXPECT_EQ(3 + 30 * i, segment.begin.z);
    EXPECT_EQ(11 + 10 * i, segment.end.x);
    EXPECT_EQ(22 + 20 * i, segment.end.y);
    EXPECT_EQ(33 + 30 * i, segment.end.z);
  }
  EXPECT_TRUE(principia__AtEnd(line_and_iterator));

  // Delete it.
  EXPECT_THAT(line_and_iterator, Not(IsNull()));
  principia__DeleteLineAndIterator(&line_and_iterator);
  EXPECT_THAT(line_and_iterator, IsNull());
  EXPECT_EQ(mock_navigation_frame, navigation_frame);
  principia__DeleteNavigationFrame(&navigation_frame);
  EXPECT_THAT(navigation_frame, IsNull());
}

TEST_F(InterfaceTest, PredictionGettersAndSetters) {
  EXPECT_CALL(*plugin_, SetPredictionLength(42 * Second));
  principia__SetPredictionLength(plugin_.get(), 42);
  EXPECT_CALL(*plugin_, SetPredictionLengthTolerance(1729 * Metre));
  principia__SetPredictionLengthTolerance(plugin_.get(), 1729);
  EXPECT_CALL(*plugin_, SetPredictionSpeedTolerance(163 * Metre / Second));
  principia__SetPredictionSpeedTolerance(plugin_.get(), 163);
}

TEST_F(InterfaceTest, PhysicsBubble) {
  KSPPart parts[3] = {{{1, 2, 3}, {10, 20, 30}, 300.0, {0, 0, 0}, 1},
                      {{4, 5, 6}, {40, 50, 60}, 600.0, {3, 3, 3}, 4},
                      {{7, 8, 9}, {70, 80, 90}, 900.0, {6, 6, 6}, 7}};
  EXPECT_CALL(*plugin_,
              AddVesselToNextPhysicsBubbleConstRef(
                  kVesselGUID,
                  ElementsAre(
                      testing::Pair(1, Pointee(Property(&Part<World>::mass,
                                                        300.0 * Tonne))),
                      testing::Pair(4, Pointee(Property(&Part<World>::mass,
                                                        600.0 * Tonne))),
                      testing::Pair(7, Pointee(Property(&Part<World>::mass,
                                                        900.0 * Tonne))))));
  principia__AddVesselToNextPhysicsBubble(plugin_.get(),
                                          kVesselGUID,
                                          &parts[0],
                                          3);

  EXPECT_CALL(*plugin_,
              BubbleDisplacementCorrection(
                  World::origin + Displacement<World>(
                                      {kParentPosition.x * SIUnit<Length>(),
                                       kParentPosition.y * SIUnit<Length>(),
                                       kParentPosition.z * SIUnit<Length>()})))
      .WillOnce(Return(Displacement<World>({77 * SIUnit<Length>(),
                                            88 * SIUnit<Length>(),
                                            99 * SIUnit<Length>()})));
  XYZ const displacement =
      principia__BubbleDisplacementCorrection(plugin_.get(), kParentPosition);
  EXPECT_THAT(displacement, Eq(XYZ{77, 88, 99}));

  EXPECT_CALL(*plugin_, BubbleVelocityCorrection(kParentIndex))
      .WillOnce(Return(Velocity<World>({66 * SIUnit<Speed>(),
                                        55 * SIUnit<Speed>(),
                                        44 * SIUnit<Speed>()})));
  XYZ const velocity =
      principia__BubbleVelocityCorrection(plugin_.get(), kParentIndex);
  EXPECT_THAT(velocity, Eq(XYZ{66, 55, 44}));

  EXPECT_CALL(*plugin_, PhysicsBubbleIsEmpty()).WillOnce(Return(true));
  bool const empty = principia__PhysicsBubbleIsEmpty(plugin_.get());
  EXPECT_TRUE(empty);
}

TEST_F(InterfaceTest, NavballOrientation) {
  StrictMock<MockDynamicFrame<Barycentric, Navigation>>* const
     mock_navigation_frame =
         new StrictMock<MockDynamicFrame<Barycentric, Navigation>>;
  EXPECT_CALL(*plugin_,
              FillBarycentricRotatingNavigationFrame(kCelestialIndex,
                                                kParentIndex,
                                                _))
      .WillOnce(FillUniquePtr<2>(mock_navigation_frame));
  NavigationFrame* navigation_frame =
      principia__NewBarycentricRotatingNavigationFrame(plugin_.get(),
                                                       kCelestialIndex,
                                                       kParentIndex);
  Position<World> sun_position =
      World::origin + Displacement<World>(
                          {1 * SIUnit<Length>(),
                           2 * SIUnit<Length>(),
                           3 * SIUnit<Length>()});
  auto const rotation =
      Rotation<World, World>(π / 2 * Radian,
                             Bivector<double, World>({4, 5, 6}));
  EXPECT_CALL(*plugin_, Navball(sun_position))
      .WillOnce(
          Return([rotation](Position<World> const& q) { return rotation; }));
  WXYZ q = principia__NavballOrientation(plugin_.get(),
                                         navigation_frame,
                                         {1, 2, 3},
                                         {2, 3, 5});
  EXPECT_EQ(q.w, rotation.quaternion().real_part());
  EXPECT_EQ(q.x, rotation.quaternion().imaginary_part().x);
  EXPECT_EQ(q.y, rotation.quaternion().imaginary_part().y);
  EXPECT_EQ(q.z, rotation.quaternion().imaginary_part().z);

  EXPECT_EQ(mock_navigation_frame, navigation_frame);
  principia__DeleteNavigationFrame(&navigation_frame);
  EXPECT_THAT(navigation_frame, IsNull());
}

TEST_F(InterfaceTest, Frenet) {
  StrictMock<MockDynamicFrame<Barycentric, Navigation>>* const
     mock_navigation_frame =
         new StrictMock<MockDynamicFrame<Barycentric, Navigation>>;
  EXPECT_CALL(*plugin_,
              FillBarycentricRotatingNavigationFrame(kCelestialIndex,
                                                kParentIndex,
                                                _))
      .WillOnce(FillUniquePtr<2>(mock_navigation_frame));
  NavigationFrame* navigation_frame =
      principia__NewBarycentricRotatingNavigationFrame(plugin_.get(),
                                                       kCelestialIndex,
                                                       kParentIndex);
  {
    auto const tangent = Vector<double, World>({4, 5, 6});
    EXPECT_CALL(*plugin_, VesselTangent(kVesselGUID)).WillOnce(Return(tangent));
    XYZ t =
        principia__VesselTangent(plugin_.get(), kVesselGUID, navigation_frame);
    EXPECT_EQ(t.x, tangent.coordinates().x);
    EXPECT_EQ(t.y, tangent.coordinates().y);
    EXPECT_EQ(t.z, tangent.coordinates().z);
  }
  {
    auto const normal = Vector<double, World>({-13, 7, 5});
    EXPECT_CALL(*plugin_, VesselNormal(kVesselGUID)).WillOnce(Return(normal));
    XYZ n =
        principia__VesselNormal(plugin_.get(), kVesselGUID, navigation_frame);
    EXPECT_EQ(n.x, normal.coordinates().x);
    EXPECT_EQ(n.y, normal.coordinates().y);
    EXPECT_EQ(n.z, normal.coordinates().z);
  }
  {
    auto const binormal = Vector<double, World>({43, 67, 163});
    EXPECT_CALL(*plugin_, VesselBinormal(kVesselGUID))
        .WillOnce(Return(binormal));
    XYZ b =
        principia__VesselBinormal(plugin_.get(), kVesselGUID, navigation_frame);
    EXPECT_EQ(b.x, binormal.coordinates().x);
    EXPECT_EQ(b.y, binormal.coordinates().y);
    EXPECT_EQ(b.z, binormal.coordinates().z);
  }

  EXPECT_EQ(mock_navigation_frame, navigation_frame);
  principia__DeleteNavigationFrame(&navigation_frame);
  EXPECT_THAT(navigation_frame, IsNull());
}

TEST_F(InterfaceTest, CurrentTime) {
  EXPECT_CALL(*plugin_, CurrentTime()).WillOnce(Return(kUnixEpoch));
  double const current_time = principia__CurrentTime(plugin_.get());
  EXPECT_THAT(t0_ + current_time * Second, Eq(kUnixEpoch));
}

TEST_F(InterfaceTest, SerializePlugin) {
  PullSerializer* serializer = nullptr;
  std::string const message_bytes =
      std::string(kSerializedBoringPlugin,
                  (sizeof(kSerializedBoringPlugin) - 1) / sizeof(char));
  principia::serialization::Plugin message;
  message.ParseFromString(message_bytes);

  EXPECT_CALL(*plugin_, WriteToMessage(_)).WillOnce(SetArgPointee<0>(message));
  char const* serialization =
      principia__SerializePlugin(plugin_.get(), &serializer);
  EXPECT_STREQ(kHexadecimalBoringPlugin, serialization);
  EXPECT_EQ(nullptr, principia__SerializePlugin(plugin_.get(), &serializer));
  principia__DeletePluginSerialization(&serialization);
  EXPECT_THAT(serialization, IsNull());
}

TEST_F(InterfaceTest, DeserializePlugin) {
  PushDeserializer* deserializer = nullptr;
  Plugin const* plugin = nullptr;
  principia__DeserializePlugin(
          kHexadecimalBoringPlugin,
          (sizeof(kHexadecimalBoringPlugin) - 1) / sizeof(char),
          &deserializer,
          &plugin);
  principia__DeserializePlugin(kHexadecimalBoringPlugin,
                               0,
                               &deserializer,
                               &plugin);
  EXPECT_THAT(plugin, NotNull());
  EXPECT_EQ(Instant(), plugin->CurrentTime());
  principia__DeletePlugin(&plugin);
}

TEST_F(InterfaceDeathTest, SettersAndGetters) {
  // We use EXPECT_EXITs in this test to avoid interfering with the execution of
  // the other tests.
  int const kExitCode = 66;
  char const kExitMessage[] = "Exiting";

  EXPECT_EXIT({
    principia__SetBufferedLogging(100);
    ASSERT_EQ(100, principia__GetBufferedLogging());
    std::cerr << kExitMessage;
    exit(kExitCode);
  }, ExitedWithCode(kExitCode), kExitMessage);

  EXPECT_EXIT({
    principia__SetBufferDuration(101);
    ASSERT_EQ(101, principia__GetBufferDuration());
    std::cerr << kExitMessage;
    exit(kExitCode);
  }, ExitedWithCode(kExitCode), kExitMessage);

  EXPECT_EXIT({
    principia__SetSuppressedLogging(102);
    ASSERT_EQ(102, principia__GetSuppressedLogging());
    std::cerr << kExitMessage;
    exit(kExitCode);
  }, ExitedWithCode(kExitCode), kExitMessage);

  EXPECT_EXIT({
    principia__SetVerboseLogging(103);
    ASSERT_EQ(103, principia__GetVerboseLogging());
    std::cerr << kExitMessage;
    exit(kExitCode);
  }, ExitedWithCode(kExitCode), kExitMessage);

  EXPECT_EXIT({
    principia__SetStderrLogging(2);
    ASSERT_EQ(2, principia__GetStderrLogging());
    std::cerr << kExitMessage;
    exit(kExitCode);
  }, ExitedWithCode(kExitCode), kExitMessage);
}

}  // namespace ksp_plugin
}  // namespace principia
