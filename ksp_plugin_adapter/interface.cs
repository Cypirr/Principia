﻿using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace principia {
namespace ksp_plugin_adapter {

[StructLayout(LayoutKind.Sequential)]
internal struct XYZ {
  public double x, y, z;
  public static explicit operator XYZ(Vector3d v) {
    return new XYZ{x = v.x, y = v.y, z = v.z};
  }
  public static explicit operator Vector3d(XYZ v) {
    return new Vector3d{x = v.x, y = v.y, z = v.z};
  }
}

[StructLayout(LayoutKind.Sequential)]
internal struct WXYZ {
  public double w, x, y, z;
  public static explicit operator WXYZ(UnityEngine.QuaternionD q) {
    return new WXYZ{w = q.w, x = q.x, y = q.y, z = q.z};
  }
  public static explicit operator UnityEngine.QuaternionD(WXYZ q) {
    return new UnityEngine.QuaternionD{w = q.w, x = q.x, y = q.y, z = q.z};
  }
}

[StructLayout(LayoutKind.Sequential)]
internal struct LineSegment {
  public XYZ begin, end;
};

[StructLayout(LayoutKind.Sequential)]
internal struct QP {
  public XYZ q, p;
}

[StructLayout(LayoutKind.Sequential)]
internal struct KSPPart {
  public XYZ world_position;
  public XYZ world_velocity;
  public double mass;
  public XYZ gravitational_acceleration_to_be_applied_by_ksp;
  public uint id;
}

internal static class Interface {
#if __MonoCS__
  internal const string kDllPath = "GameData/Principia/principia.so";
#else
  internal const string kDllPath = "GameData/Principia/principia.dll";
#endif

  // Plugin interface.

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__SayHello",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern IntPtr SayHello();

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__NewPlugin",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern IntPtr NewPlugin(
      double initial_time,
      double planetarium_rotation_in_degrees);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__DeletePlugin",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void DeletePlugin(ref IntPtr plugin);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__DirectlyInsertCelestial",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void DirectlyInsertCelestial(
      this IntPtr plugin,
      int celestial_index,
      ref int parent_index,
      [MarshalAs(UnmanagedType.LPStr)] String gravitational_parameter,
      [MarshalAs(UnmanagedType.LPStr)] String axis_right_ascension,
      [MarshalAs(UnmanagedType.LPStr)] String axis_declination,
      [MarshalAs(UnmanagedType.LPStr)] String j2,
      [MarshalAs(UnmanagedType.LPStr)] String reference_radius,
      [MarshalAs(UnmanagedType.LPStr)] String x,
      [MarshalAs(UnmanagedType.LPStr)] String y,
      [MarshalAs(UnmanagedType.LPStr)] String z,
      [MarshalAs(UnmanagedType.LPStr)] String vx,
      [MarshalAs(UnmanagedType.LPStr)] String vy,
      [MarshalAs(UnmanagedType.LPStr)] String vz);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__DirectlyInsertCelestial",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void DirectlyInsertCelestial(
      this IntPtr plugin,
      int celestial_index,
      IntPtr parent_index,
      [MarshalAs(UnmanagedType.LPStr)] String gravitational_parameter,
      [MarshalAs(UnmanagedType.LPStr)] String axis_right_ascension,
      [MarshalAs(UnmanagedType.LPStr)] String axis_declination,
      [MarshalAs(UnmanagedType.LPStr)] String j2,
      [MarshalAs(UnmanagedType.LPStr)] String reference_radius,
      [MarshalAs(UnmanagedType.LPStr)] String x,
      [MarshalAs(UnmanagedType.LPStr)] String y,
      [MarshalAs(UnmanagedType.LPStr)] String z,
      [MarshalAs(UnmanagedType.LPStr)] String vx,
      [MarshalAs(UnmanagedType.LPStr)] String vy,
      [MarshalAs(UnmanagedType.LPStr)] String vz);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__InsertCelestial",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void InsertCelestial(
      this IntPtr plugin,
      int celestial_index,
      double gravitational_parameter,
      int parent_index,
      QP from_parent);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__InsertSun",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void InsertSun(
      this IntPtr plugin,
      int celestial_index,
      double gravitational_parameter);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__EndInitialization",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void EndInitialization(this IntPtr plugin);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__UpdateCelestialHierarchy",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void UpdateCelestialHierarchy(
      this IntPtr plugin,
      int celestial_index,
      int parent_index);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__InsertOrKeepVessel",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern bool InsertOrKeepVessel(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      int parent_index);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__SetVesselStateOffset",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void SetVesselStateOffset(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      QP from_parent);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__AdvanceTime",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void AdvanceTime(
      this IntPtr plugin, 
      double t,
      double planetarium_rotation);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__ForgetAllHistoriesBefore",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void ForgetAllHistoriesBefore(this IntPtr plugin,
                                                       double t);

  [DllImport(dllName: kDllPath,
             EntryPoint        = "principia__VesselFromParent",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern QP VesselFromParent(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__CelestialFromParent",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern QP CelestialFromParent(
      this IntPtr plugin,
      int celestial_index);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__RenderedVesselTrajectory",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern IntPtr RenderedVesselTrajectory(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      IntPtr navigation_frame,
      XYZ sun_world_position);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__UpdatePrediction",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void UpdatePrediction(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__HasPrediction",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern bool HasPrediction(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__RenderedPrediction",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern IntPtr RenderedPrediction(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      IntPtr navigation_frame,
      XYZ sun_world_position);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__NumberOfSegments",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern int NumberOfSegments(this IntPtr line);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__FetchAndIncrement",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern LineSegment FetchAndIncrement(this IntPtr line);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__AtEnd",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern bool AtEnd(this IntPtr line);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__DeleteLineAndIterator",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void DeleteLineAndIterator(ref IntPtr line);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__SetPredictionLength",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void SetPredictionLength(this IntPtr plugin,
                                                  double t);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__SetPredictionLengthTolerance",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void SetPredictionLengthTolerance(this IntPtr plugin,
                                                           double t);
  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__SetPredictionSpeedTolerance",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void SetPredictionSpeedTolerance(this IntPtr plugin,
                                                          double t);

  [DllImport(dllName             : kDllPath,
             EntryPoint =        "principia__HasVessel",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern bool HasVessel(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid);

  [DllImport(dllName: kDllPath,
             EntryPoint        = "principia__AddVesselToNextPhysicsBubble",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void AddVesselToNextPhysicsBubble(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      KSPPart[] parts,
      int count);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__PhysicsBubbleIsEmpty",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern bool PhysicsBubbleIsEmpty(this IntPtr plugin);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__BubbleDisplacementCorrection",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern XYZ BubbleDisplacementCorrection(this IntPtr plugin,
                                                          XYZ sun_position);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__BubbleVelocityCorrection",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern XYZ BubbleVelocityCorrection(
      this IntPtr plugin,
      int reference_body_index);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__NavballOrientation",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern WXYZ NavballOrientation(
      this IntPtr plugin,
      IntPtr navigation_frame,
      XYZ sun_world_position,
      XYZ ship_world_position);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__VesselTangent",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern XYZ VesselTangent(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      IntPtr navigation_frame);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__VesselNormal",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern XYZ VesselNormal(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      IntPtr navigation_frame);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__VesselBinormal",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern XYZ VesselBinormal(
      this IntPtr plugin,
      [MarshalAs(UnmanagedType.LPStr)] String vessel_guid,
      IntPtr navigation_frame);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__CurrentTime",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern double CurrentTime(this IntPtr plugin);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__SerializePlugin",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern IntPtr SerializePlugin(this IntPtr plugin,
                                               ref IntPtr serializer);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__DeletePluginSerialization",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void DeletePluginSerialization(
      ref IntPtr serialization);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__DeserializePlugin",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void DeserializePlugin(
      [MarshalAs(UnmanagedType.LPStr)] String serialization,
      int serialization_size,
      ref IntPtr deserializer,
      ref IntPtr plugin);

  [DllImport(dllName           : kDllPath,
             EntryPoint        =
                 "principia__NewBodyCentredNonRotatingNavigationFrame",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern IntPtr NewBodyCentredNonRotatingNavigationFrame(
      this IntPtr plugin,
      int reference_body_index);

  [DllImport(
       dllName           : kDllPath,
       EntryPoint        = "principia__NewBarycentricRotatingNavigationFrame",
       CallingConvention = CallingConvention.Cdecl)]
  internal static extern IntPtr NewBarycentricRotatingNavigationFrame(
      this IntPtr plugin,
      int primary_index,
      int secondary_index);

  [DllImport(dllName           : kDllPath,
             EntryPoint        = "principia__DeleteNavigationFrame",
             CallingConvention = CallingConvention.Cdecl)]
  internal static extern void DeleteNavigationFrame(ref IntPtr navigation_frame);
}

}  // namespace ksp_plugin_adapter
}  // namespace principia
