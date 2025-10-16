#ifndef SYSTEMSTATEDATA_H
#define SYSTEMSTATEDATA_H

/**
 * @file SystemStateData.h
 * @brief Core data structures and enumerations for Remote Controlled Weapon System (RCWS)
 * 
 * This header defines all fundamental data structures, enumerations, and constants
 * used throughout the RCWS application for state management and system operation.
 * 
 * MAIN COMPONENTS:
 * 1. System Constants - Color definitions and default values
 * 2. Core Enumerations - System modes, states, and operational parameters
 * 3. Zone Management Structures - Area zones, sector scan zones, and target reference points
 * 4. System State Structure - Complete system state with organized data categories
 * 
 * DATA ORGANIZATION WITHIN SystemStateData:
 * • Operational State & Modes - Current and previous operational modes
 * • Display & UI Configuration - Visual settings and screen parameters
 * • Zone Management - All zone types and active zone tracking
 * • Camera Systems - Day/night camera status and control
 * • Gimbal & Positioning System - Physical positioning and motor status
 * • Orientation & Stabilization - Platform orientation and stabilization
 * • Laser Range Finder (LRF) - Distance measurement and status
 * • Joystick & Manual Controls - User input and manual control states
 * • Weapon System Control (PLC21) - Weapon arming and fire control
 * • Gimbal Station Hardware (PLC42) - Hardware sensors and monitoring
 * • Tracking System - Target tracking and movement control
 * • Ballistics & Fire Control - Zeroing, windage, and lead compensation
 * • Status & Information Display - System messages and status text
 * 
 * HELPER FUNCTIONS:
 * • System readiness checks
 * • Health monitoring utilities
 * • Comparison operators for state management
 *
 * @author MB
 * @date [Current Date]
 * @version 1.0
 */

#include <QString>
#include <QColor>
#include <QDateTime>
#include <QPointF>
#include <QtGlobal> // For qFuzzyCompare
#include <vector>
#include "utils/colorutils.h" // For ColorUtils
#include <vpi/algo/DCFTracker.h> // VPITrackingState, VPIDCFTrackedBoundingBox

// =================================
// CONSTANTS
// =================================

/**
 * @brief Color constants for tracking system status indication
 */
const QColor COLOR_TRACKING_ACQUIRING = Qt::yellow;      ///< Target acquisition in progress
const QColor COLOR_TRACKING_ACTIVE = QColor(70, 226, 165);  ///< Active tracking engaged
const QColor COLOR_TRACKING_COASTING = Qt::cyan;        ///< Coasting mode (temporary track loss)
const QColor COLOR_TRACKING_LOST = QColor(200,20,40);             ///< Target tracking lost
const QColor COLOR_TRACKING_DEFAULT = QColor(70, 226, 165); ///< Default tracking color
const QColor COLOR_TRACKING_FIRING = QColor(255, 255, 0); ///< Firing mode active (green)

// =================================
// ENUMERATIONS
// =================================

/**
 * @brief Available reticle types for weapon aiming system
 */
enum class ReticleType {
    Basic,              ///< Simple crosshair reticle
    BoxCrosshair,       ///< Box-style crosshair with corner markers
    StandardCrosshair,  ///< Standard military crosshair
    PrecisionCrosshair, ///< High-precision crosshair with fine markings
    MilDot,            ///< Military dot reticle for range estimation
    COUNT              ///< Total number of reticle types (for iteration)
};

/**
 * @brief Weapon firing modes available in the system
 */
enum class FireMode { 
    SingleShot,  ///< Single round per trigger pull
    ShortBurst,  ///< Short controlled burst
    LongBurst,   ///< Extended burst fire
    Unknown      ///< Unknown or uninitialized fire mode
};

/**
 * @brief High-level operational modes of the weapon system
 */
enum class OperationalMode { 
    Idle,         ///< System idle, no active operations
    Surveillance, ///< Area surveillance mode
    Tracking,     ///< Target tracking mode
    Engagement ,   ///< Active engagement mode
    EmergencyStop , ///< Emergency stop mode
};

/**
 * @brief Motion control modes for gimbal and weapon positioning
 */
enum class MotionMode { 
    Manual,        ///< Manual joystick control
    Pattern,       ///< Predefined pattern scanning
    AutoTrack,     ///< Automatic target tracking
    ManualTrack,   ///< Manual target tracking
    RadarTracking, ///< Radar-assisted tracking
    Idle,          ///< No motion, idle state
    AutoSectorScan,///< Automatic sector scanning
    TRPScan,        ///< Target Reference Point scanning
    RadarSlew
};

enum class TrackingPhase {
    Off,                  // Tracking is completely inactive.
    Acquisition,          // User is positioning/sizing the initial tracking gate.
    Tracking_LockPending, // System has a gate, attempting to lock (e.g., solid yellow box).
    Tracking_ActiveLock,  // System has a solid lock, gimbal is actively following (e.g., dashed red box).
    Tracking_Coast,       // Target is temporarily lost/occluded, system is predicting (e.g., dashed yellow box).
    Tracking_Firing       // Weapon has fired while locked, system holds position (e.g., dashed green box).
};

/**
 * @brief Zone classification types for operational areas
 */
enum class ZoneType {
    None,                  ///< No zone type assigned
    Safety,                ///< Safety zone (general restriction)
    NoTraverse,           ///< No-traverse zone (movement restricted)
    NoFire,               ///< No-fire zone (firing prohibited)
    AutoSectorScan,       ///< Automatic sector scan area
    TargetReferencePoint  ///< Target reference point zone
};

/**
 * @brief Lead angle compensation system status
 */
enum class LeadAngleStatus {
    Off,     ///< Lead angle compensation disabled
    On,      ///< Lead angle compensation active and functioning
    Lag,     ///< Lead angle calculation at maximum limit
    ZoomOut  ///< Lead angle too large for current FOV, zoom out required
};

// =================================
// ZONE STRUCTURES
// =================================

/**
 * @brief Defines a 3D area zone with azimuth, elevation, and range constraints
 */
struct AreaZone {
    int id = -1;                        ///< Unique identifier for the zone
    ZoneType type = ZoneType::Safety;   ///< Type of zone (safety, no-fire, etc.)
    bool isEnabled = false;             ///< Whether the zone is currently active
    bool isFactorySet = false;          ///< Whether this is a factory-configured zone
    bool isOverridable = false;         ///< Whether the zone can be overridden by operator
    float startAzimuth = 0.0f;          ///< Starting azimuth angle in degrees
    float endAzimuth = 0.0f;            ///< Ending azimuth angle in degrees
    float minElevation = 0.0f;          ///< Minimum elevation angle in degrees
    float maxElevation = 0.0f;          ///< Maximum elevation angle in degrees
    float minRange = 0.0f;              ///< Minimum range in meters
    float maxRange = 0.0f;              ///< Maximum range in meters
    QString name = "";                  ///< Human-readable zone name

    AreaZone() = default;
    
    /**
     * @brief Equality comparison operator for AreaZone
     * @param other The other AreaZone to compare with
     * @return True if zones are identical, false otherwise
     */
    bool operator==(const AreaZone& other) const {
        return id == other.id && 
               type == other.type && 
               isEnabled == other.isEnabled &&
               isFactorySet == other.isFactorySet && 
               isOverridable == other.isOverridable &&
               qFuzzyCompare(startAzimuth, other.startAzimuth) && 
               qFuzzyCompare(endAzimuth, other.endAzimuth) &&
               qFuzzyCompare(minElevation, other.minElevation) && 
               qFuzzyCompare(maxElevation, other.maxElevation) &&
               qFuzzyCompare(minRange, other.minRange) && 
               qFuzzyCompare(maxRange, other.maxRange) &&
               name == other.name;
    }
    
    /**
     * @brief Inequality comparison operator for AreaZone
     * @param other The other AreaZone to compare with
     * @return True if zones are different, false otherwise
     */
    bool operator!=(const AreaZone& other) const { 
        return !(*this == other); 
    }
};

/**
 * @brief Defines an automatic sector scanning zone with two boundary points
 */
struct AutoSectorScanZone {
    int id = -1;                ///< Unique identifier for the scan zone
    bool isEnabled = false;     ///< Whether the scan zone is currently active
    float az1 = 0.0f;          ///< First boundary point azimuth in degrees
    float el1 = 0.0f;          ///< First boundary point elevation in degrees
    float az2 = 0.0f;          ///< Second boundary point azimuth in degrees
    float el2 = 0.0f;          ///< Second boundary point elevation in degrees
    float scanSpeed = 20.0f;   ///< Scanning speed in degrees per second

    AutoSectorScanZone() = default;
    
    /**
     * @brief Equality comparison operator for AutoSectorScanZone
     * @param other The other AutoSectorScanZone to compare with
     * @return True if scan zones are identical, false otherwise
     */
    bool operator==(const AutoSectorScanZone& other) const {
        return id == other.id && 
               isEnabled == other.isEnabled &&
               qFuzzyCompare(az1, other.az1) && 
               qFuzzyCompare(el1, other.el1) &&
               qFuzzyCompare(az2, other.az2) && 
               qFuzzyCompare(el2, other.el2) &&
               qFuzzyCompare(scanSpeed, other.scanSpeed);
    }
    
    /**
     * @brief Inequality comparison operator for AutoSectorScanZone
     * @param other The other AutoSectorScanZone to compare with
     * @return True if scan zones are different, false otherwise
     */
    bool operator!=(const AutoSectorScanZone& other) const { 
        return !(*this == other); 
    }
};

/**
 * @brief Defines a target reference point for navigation and scanning
 */
struct TargetReferencePoint {
    int id = -1;                ///< Unique identifier for the TRP
    int locationPage = 1;       ///< Location page number for organization
    int trpInPage = 1;         ///< TRP number within the page
    float azimuth = 0.0f;      ///< TRP azimuth position in degrees
    float elevation = 0.0f;    ///< TRP elevation position in degrees
    float haltTime = 0.0f;     ///< Halt time at TRP in seconds

    TargetReferencePoint() = default;
    
    /**
     * @brief Equality comparison operator for TargetReferencePoint
     * @param other The other TargetReferencePoint to compare with
     * @return True if TRPs are identical, false otherwise
     */
    bool operator==(const TargetReferencePoint& other) const {
        return id == other.id && 
               locationPage == other.locationPage && 
               trpInPage == other.trpInPage &&
               qFuzzyCompare(azimuth, other.azimuth) && 
               qFuzzyCompare(elevation, other.elevation) &&
               qFuzzyCompare(haltTime, other.haltTime);
    }
    
    /**
     * @brief Inequality comparison operator for TargetReferencePoint
     * @param other The other TargetReferencePoint to compare with
     * @return True if TRPs are different, false otherwise
     */
    bool operator!=(const TargetReferencePoint& other) const { 
        return !(*this == other); 
    }
};

struct SimpleRadarPlot {
    quint32 id;
    float azimuth;
    float range;
    float relativeCourse;
    float relativeSpeed;

    bool operator==(const SimpleRadarPlot &other) const {
        return id == other.id &&
               qFuzzyCompare(azimuth, other.azimuth) &&
               qFuzzyCompare(relativeSpeed, other.relativeSpeed) &&
               qFuzzyCompare(range, other.range) &&
               qFuzzyCompare(relativeCourse, other.relativeCourse);
    }
};

// =================================
// MAIN SYSTEM STATE STRUCTURE
// =================================

/**
 * @brief Comprehensive system state structure containing all RCWS operational data
 * 
 * This structure serves as the central data repository for the entire RCWS system,
 * organizing all operational parameters, sensor data, control states, and status
 * information into logical categories for efficient access and management.
 */
struct SystemStateData {
    
    // =================================
    // OPERATIONAL STATE & MODES
    // =================================
    OperationalMode opMode = OperationalMode::Idle;         ///< Current operational mode
    OperationalMode previousOpMode = OperationalMode::Idle; ///< Previous operational mode for state transitions
    MotionMode motionMode = MotionMode::Idle;               ///< Current motion control mode
    MotionMode previousMotionMode = MotionMode::Idle;       ///< Previous motion mode for state transitions
    
    // =================================
    // DISPLAY & UI CONFIGURATION
    // =================================
    ReticleType reticleType = ReticleType::BoxCrosshair;    ///< Current reticle style
    ColorStyle osdColorStyle = ColorStyle::Green;           ///< On-screen display color style
    QColor colorStyle = QColor(70, 226, 165);              ///< Current UI color theme
    int currentImageWidthPx = 1024;                         ///< Current image width in pixels
    int currentImageHeightPx = 768;                         ///< Current image height in pixels
    float reticleAimpointImageX_px = currentImageWidthPx / 2.0f;  ///< Reticle X position in image coordinates
    float reticleAimpointImageY_px = currentImageHeightPx / 2.0f; ///< Reticle Y position in image coordinates
    
    // =================================
    // ZONE MANAGEMENT
    // =================================
    std::vector<AreaZone> areaZones;                        ///< Collection of all area zones
    std::vector<AutoSectorScanZone> sectorScanZones;       ///< Collection of all sector scan zones
    std::vector<TargetReferencePoint> targetReferencePoints; ///< Collection of all target reference points
    int activeAutoSectorScanZoneId = 1;                     ///< Currently active sector scan zone ID
    int activeTRPLocationPage = 1;                          ///< Currently active TRP location page
    QString currentScanName;                                ///< Name of current scanning operation
    QString currentTRPScanName;                             ///< Name of current TRP scan operation
    bool isReticleInNoFireZone = false;                     ///< Whether reticle is in a no-fire zone
    bool isReticleInNoTraverseZone = false;                 ///< Whether reticle is in a no-traverse zone
    
    // =================================
    // CAMERA SYSTEMS
    // =================================
    // Day Camera
    double dayZoomPosition = 0.0;       ///< Day camera zoom position (0-1 normalized)
    double dayCurrentHFOV = 9.0;        ///< Day camera current horizontal field of view in degrees
    bool dayCameraConnected = false;    ///< Day camera connection status
    bool dayCameraError = false;        ///< Day camera error status
    quint8 dayCameraStatus = 0;         ///< Day camera detailed status code
    
    // Night Camera
    double nightZoomPosition = 0.0;     ///< Night camera zoom position (0-1 normalized)
    double nightCurrentHFOV = 8.0;      ///< Night camera current horizontal field of view in degrees
    bool nightCameraConnected = false;  ///< Night camera connection status
    bool nightCameraError = false;      ///< Night camera error status
    quint8 nightCameraStatus = 0;       ///< Night camera detailed status code
    
    // Camera Control
    bool activeCameraIsDay = false;     ///< True if day camera is active, false if night camera
    
    // =================================
    // GIMBAL & POSITIONING SYSTEM
    // =================================
    double gimbalAz = 0.0;              ///< Current gimbal azimuth position in degrees
    double gimbalEl = 0.0;              ///< Current gimbal elevation position in degrees
    float azMotorTemp = 0.0f;           ///< Azimuth motor temperature in Celsius
    float azDriverTemp = 0.0f;          ///< Azimuth driver temperature in Celsius
    float elMotorTemp = 0.0f;           ///< Elevation motor temperature in Celsius
    float elDriverTemp = 0.0f;          ///< Elevation driver temperature in Celsius
    float reticleAz = 0.0f;             ///< Reticle azimuth position in degrees
    float reticleEl = 0.0f;             ///< Reticle elevation position in degrees
    double actuatorPosition = 0.0;       ///< Linear actuator position
    
    // =================================
    // ORIENTATION & STABILIZATION
    // =================================
    double imuRollDeg = 0.0;            ///< IMU Roll angle in degrees
    double imuPitchDeg = 0.0;           ///< IMU Pitch angle in degrees
    double imuYawDeg = 0.0;             ///< IMU Yaw angle in degrees
    double GyroX = 0.0;                 ///< Gyro X-axis rate in deg/s
    double GyroY = 0.0;                 ///< Gyro Y-axis rate in deg/s
    double GyroZ = 0.0;                 ///< Gyro Z-axis rate in deg/s
    double AccelX = 0.0;                ///< Accelerometer X-axis in G
    double AccelY = 0.0;                ///< Accelerometer Y-axis in G
    double AccelZ = 0.0;                ///< Accelerometer Z-axis in G
    bool isStabilizationActive = false; ///< Stabilization system active status
    double temperature = 0.0;           ///< Current system temperature in Celsius
    // Stationary detection variables
    bool isVehicleStationary = false;   ///< Flag indicating if the vehicle is stationary
    double previousAccelMagnitude = 0.0; ///< Previous accelerometer magnitude for delta calculation
    QDateTime stationaryStartTime;      ///< Timestamp when stationary conditions began
    // =================================
    // LASER RANGE FINDER (LRF)
    // =================================
    double lrfDistance = 0.0;           ///< Last measured distance in meters
    quint8 lrfSystemStatus = 0;         ///< LRF system status code
    quint8 isOverTemperature = 0;         ///< LRF over-temperature status (1 = true, 0 = false)
    // =================================
    // --- Radar Data ---
    // =================================
    QVector<SimpleRadarPlot> radarPlots;         // The latest list of all detected plots
    quint32 selectedRadarTrackId = 0;      // The ID of the target the user has selected from the list
    
    // =================================
    // JOYSTICK & MANUAL CONTROLS
    // =================================
    bool deadManSwitchActive = false;   ///< Safety dead man switch status
    float joystickAzValue = 0.0f;       ///< Joystick azimuth axis value (-1.0 to 1.0)
    float joystickElValue = 0.0f;       ///< Joystick elevation axis value (-1.0 to 1.0)
    bool upTrackButton = false;         ///< Up track button status
    bool downTrackButton = false;       ///< Down track button status
    bool menuUp = false;                  ///< Up switch status
    bool menuDown = false;                ///< Down switch status
    bool menuVal = false;             ///< Menu validation switch status
    int joystickHatDirection = 0;         ///< Joystick hat switch direction (0-7, 0 = center, 1 = up, 2 = up-right, etc.)

    
    // =================================
    // WEAPON SYSTEM CONTROL (PLC21)
    // =================================
    bool stationEnabled = true;         ///< Weapon station enable status
    bool gotoHomePosition = false;                ///< Home position switch status
    bool gunArmed = false;              ///< Weapon arming status
    bool ammoLoaded = false;            ///< Ammunition loaded status
    bool authorized = false;            ///< System authorization status
    bool detectionEnabled = false;      ///< Target detection enable status
    FireMode fireMode = FireMode::Unknown; ///< Current weapon fire mode
    double gimbalSpeed = 2.0;               ///< Speed switch setting
    bool enableStabilization = true;   ///< Platform stabilization enable switch

    // =================================
    // GIMBAL STATION HARDWARE (PLC42)
    // =================================
    // Limit Sensors
    bool upperLimitSensorActive = false; ///< Upper travel limit sensor status
    bool lowerLimitSensorActive = false; ///< Lower travel limit sensor status
    bool emergencyStopActive = false;    ///< Emergency stop activation status
    
    // Station Inputs
    bool stationAmmunitionLevel = false; ///< Station ammunition level sensor
    bool stationInput1 = false;          ///< General station input 1
    bool stationInput2 = false;          ///< General station input 2
    bool stationInput3 = false;          ///< General station input 3
    
    // Environmental Monitoring
    int panelTemperature = 0;            ///< Control panel temperature in Celsius
    int stationTemperature = 0;          ///< Station ambient temperature in Celsius
    int stationPressure = 0;             ///< Station atmospheric pressure
    
    // Control States
    uint16_t solenoidMode = 0;           ///< Solenoid valve mode setting
    uint16_t gimbalOpMode = 0;           ///< Gimbal operational mode
    uint32_t azimuthSpeed = 0;           ///< Azimuth movement speed setting
    uint32_t elevationSpeed = 0;         ///< Elevation movement speed setting
    uint16_t azimuthDirection = 0;       ///< Azimuth movement direction
    uint16_t elevationDirection = 0;     ///< Elevation movement direction
    uint16_t solenoidState = 0;          ///< Current solenoid state
    uint16_t resetAlarm = 0;             ///< Alarm reset control
    
    // =================================
    // TRACKING SYSTEM
    // =================================
    bool upTrack = false;                ///< Up tracking button status
    bool downTrack = false;              ///< Down tracking button status
    bool valTrack = false;               ///< Track validation button status
    bool startTracking = false;          ///< Start tracking command status
    bool requestTrackingRestart = false; ///< Tracking restart request status
    bool trackingActive = false;         ///< Current tracking active status
    double targetAz = 0.0;               ///< Current target azimuth in degrees
    double targetEl = 0.0;               ///< Current target elevation in degrees
    float trackedTargetVelocityX_px_s = 0.0;
    float trackedTargetVelocityY_px_s = 0.0;
    // --- Active Tracked Target Data (from Video Processor) ---

    float currentCameraHfovDegrees = 45.0f; // Set by CameraController

    bool  trackerHasValidTarget = false;
    float trackedTargetCenterX_px = 0.0f;
    float trackedTargetCenterY_px = 0.0f;
    float trackedTargetWidth_px = 0.0f;
    float trackedTargetHeight_px = 0.0f;
    VPITrackingState trackedTargetState = VPI_TRACKING_STATE_LOST; // Store the raw tracker state
    TrackingPhase currentTrackingPhase = TrackingPhase::Off;

    // The acquisition gate/box, defined by user before lock-on.
    // In IMAGE PIXEL coordinates.
    float acquisitionBoxX_px = 512.0f;
    float acquisitionBoxY_px = 384.0f;
    float acquisitionBoxW_px = 100.0f; // Default size
    float acquisitionBoxH_px = 100.0f;

    // =================================
    // BALLISTICS & FIRE CONTROL
    // =================================
    // Zeroing System
    bool zeroingModeActive = false;         ///< Weapon zeroing mode active status
    float zeroingAzimuthOffset = 0.0f;      ///< Zeroing azimuth offset in degrees
    float zeroingElevationOffset = 0.0f;    ///< Zeroing elevation offset in degrees
    bool zeroingAppliedToBallistics = false; ///< Whether zeroing is applied to ballistic calculations
    
    // Windage Compensation
    bool windageModeActive = false;         ///< Windage compensation mode active status
    float windageSpeedKnots = 0.0f;         ///< Wind speed for windage calculation in knots
    bool windageAppliedToBallistics = false; ///< Whether windage is applied to ballistic calculations
    
    // Lead Angle Compensation
    bool leadAngleCompensationActive = false;           ///< Lead angle compensation active status
    LeadAngleStatus currentLeadAngleStatus = LeadAngleStatus::Off; ///< Current lead angle system status
    float leadAngleOffsetAz = 0.0f;                     ///< Lead angle azimuth offset in degrees
    float leadAngleOffsetEl = 0.0f;                     ///< Lead angle elevation offset in degrees
    
    // Target Parameters for Ballistics
    float currentTargetRange = 2000.0f;                 ///< Current target range in meters
    float currentTargetAngularRateAz = 0.0f;            ///< Target angular rate in azimuth (degrees/second)
    float currentTargetAngularRateEl = 0.0f;            ///< Target angular rate in elevation (degrees/second)
    float muzzleVelocityMPS = 900.0f;                   ///< Projectile muzzle velocity in meters per second
    
    // =================================
    // STATUS & INFORMATION DISPLAY
    // =================================
    QString weaponSystemStatus;         ///< Weapon system status message
    QString targetInformation;          ///< Current target information display
    QString gpsCoordinates;             ///< GPS coordinate information
    QString sensorReadings;             ///< Sensor readings summary
    QString alertsWarnings;             ///< System alerts and warnings
    QString leadStatusText = "";        ///< Lead angle status text for display
    QString zeroingStatusText = "";     ///< Zeroing status text for display
    
    // =================================
    // HELPER FUNCTIONS
    // =================================
    
    /**
     * @brief Checks if the weapon system is ready for operation
     * @return True if all safety and authorization conditions are met
     */
    bool isReady() const {
        return gunArmed && ammoLoaded && deadManSwitchActive && authorized;
    }
    
    /**
     * @brief Checks if the tracking system is ready for operation
     * @return True if tracking system is operational and safe
     */
    bool isTrackingReady() const {
        return trackingActive && !emergencyStopActive && stationEnabled;
    }
    
    /**
     * @brief Checks if the camera system is healthy and operational
     * @return True if the active camera is connected and error-free
     */
    bool isCameraSystemHealthy() const {
        return activeCameraIsDay ? (!dayCameraError && dayCameraConnected) 
                                 : (!nightCameraError && nightCameraConnected);
    }
    
    // =================================
    // COMPARISON OPERATORS
    // =================================
    
    /**
     * @brief Equality comparison operator for complete system state
     * @param other The other SystemStateData to compare with
     * @return True if all system state parameters are identical
     */
    bool operator==(const SystemStateData& other) const {
        return opMode == other.opMode && 
               motionMode == other.motionMode && 
               previousOpMode == other.previousOpMode &&
               previousMotionMode == other.previousMotionMode &&
               reticleType == other.reticleType &&
               osdColorStyle == other.osdColorStyle &&
               colorStyle == other.colorStyle &&
               currentImageWidthPx == other.currentImageWidthPx &&
               currentImageHeightPx == other.currentImageHeightPx &&
               qFuzzyCompare(reticleAimpointImageX_px, other.reticleAimpointImageX_px) &&
               qFuzzyCompare(reticleAimpointImageY_px, other.reticleAimpointImageY_px) &&
               areaZones == other.areaZones &&
               sectorScanZones == other.sectorScanZones &&
               targetReferencePoints == other.targetReferencePoints &&
               activeAutoSectorScanZoneId == other.activeAutoSectorScanZoneId &&
               activeTRPLocationPage == other.activeTRPLocationPage &&
               currentScanName == other.currentScanName &&
               currentTRPScanName == other.currentTRPScanName &&
               isReticleInNoFireZone == other.isReticleInNoFireZone &&
               isReticleInNoTraverseZone == other.isReticleInNoTraverseZone &&
               qFuzzyCompare(dayZoomPosition, other.dayZoomPosition) &&
               qFuzzyCompare(dayCurrentHFOV, other.dayCurrentHFOV) &&
               dayCameraConnected == other.dayCameraConnected &&
               dayCameraError == other.dayCameraError &&
               dayCameraStatus == other.dayCameraStatus &&
               qFuzzyCompare(nightZoomPosition, other.nightZoomPosition) &&
               qFuzzyCompare(nightCurrentHFOV, other.nightCurrentHFOV) &&
               nightCameraConnected == other.nightCameraConnected &&
               nightCameraError == other.nightCameraError &&
               nightCameraStatus == other.nightCameraStatus &&
               activeCameraIsDay == other.activeCameraIsDay &&
               qFuzzyCompare(gimbalAz, other.gimbalAz) &&
               qFuzzyCompare(gimbalEl, other.gimbalEl) &&
               qFuzzyCompare(azMotorTemp, other.azMotorTemp) &&
               qFuzzyCompare(azDriverTemp, other.azDriverTemp) &&
               qFuzzyCompare(elMotorTemp, other.elMotorTemp) &&
               qFuzzyCompare(elDriverTemp, other.elDriverTemp) &&
               qFuzzyCompare(reticleAz, other.reticleAz) &&
               qFuzzyCompare(reticleEl, other.reticleEl) &&
               qFuzzyCompare(actuatorPosition, other.actuatorPosition) &&
               qFuzzyCompare(imuRollDeg, other.imuRollDeg) &&
               qFuzzyCompare(imuPitchDeg, other.imuPitchDeg) &&
               qFuzzyCompare(imuYawDeg, other.imuYawDeg) &&
                qFuzzyCompare(temperature, other.temperature) &&
                AccelX == other.AccelX &&
                AccelY == other.AccelY &&
                AccelZ == other.AccelZ &&
                GyroX == other.GyroX &&
                GyroY == other.GyroY &&
                GyroZ == other.GyroZ &&
               enableStabilization == other.enableStabilization &&
               qFuzzyCompare(lrfDistance, other.lrfDistance) &&
               lrfSystemStatus == other.lrfSystemStatus &&
               deadManSwitchActive == other.deadManSwitchActive &&
               qFuzzyCompare(joystickAzValue, other.joystickAzValue) &&
               qFuzzyCompare(joystickElValue, other.joystickElValue) &&
               upTrackButton == other.upTrackButton &&
               downTrackButton == other.downTrackButton &&
               menuUp == other.menuUp &&
               menuDown == other.menuDown &&
               menuVal == other.menuVal &&
               stationEnabled == other.stationEnabled &&
               gotoHomePosition == other.gotoHomePosition &&
               gunArmed == other.gunArmed &&
               ammoLoaded == other.ammoLoaded &&

               authorized == other.authorized &&
               detectionEnabled == other.detectionEnabled &&
               fireMode == other.fireMode &&
               qFuzzyCompare(gimbalSpeed, other.gimbalSpeed) &&
               upperLimitSensorActive == other.upperLimitSensorActive &&
               lowerLimitSensorActive == other.lowerLimitSensorActive &&
               emergencyStopActive == other.emergencyStopActive &&
               stationAmmunitionLevel == other.stationAmmunitionLevel &&
               stationInput1 == other.stationInput1 &&
               stationInput2 == other.stationInput2 &&
               stationInput3 == other.stationInput3 &&
               panelTemperature == other.panelTemperature &&
               stationTemperature == other.stationTemperature &&
               stationPressure == other.stationPressure &&
               solenoidMode == other.solenoidMode &&
               gimbalOpMode == other.gimbalOpMode &&
               azimuthSpeed == other.azimuthSpeed &&
               elevationSpeed == other.elevationSpeed &&
               azimuthDirection == other.azimuthDirection &&
               elevationDirection == other.elevationDirection &&
               solenoidState == other.solenoidState &&
               resetAlarm == other.resetAlarm &&
               upTrack == other.upTrack &&
               downTrack == other.downTrack &&
               valTrack == other.valTrack &&
               startTracking == other.startTracking &&
               requestTrackingRestart == other.requestTrackingRestart &&
               trackingActive == other.trackingActive &&
               qFuzzyCompare(targetAz, other.targetAz) &&
               qFuzzyCompare(targetEl, other.targetEl) &&
               zeroingModeActive == other.zeroingModeActive &&
               qFuzzyCompare(zeroingAzimuthOffset, other.zeroingAzimuthOffset) &&
               qFuzzyCompare(zeroingElevationOffset, other.zeroingElevationOffset) &&
               zeroingAppliedToBallistics == other.zeroingAppliedToBallistics &&
               windageModeActive == other.windageModeActive &&
               qFuzzyCompare(windageSpeedKnots, other.windageSpeedKnots) &&
               windageAppliedToBallistics == other.windageAppliedToBallistics &&
               leadAngleCompensationActive == other.leadAngleCompensationActive &&
               currentLeadAngleStatus == other.currentLeadAngleStatus &&
               qFuzzyCompare(leadAngleOffsetAz, other.leadAngleOffsetAz) &&
               qFuzzyCompare(leadAngleOffsetEl, other.leadAngleOffsetEl) &&
               qFuzzyCompare(currentTargetRange, other.currentTargetRange) &&
               qFuzzyCompare(currentTargetAngularRateAz, other.currentTargetAngularRateAz) &&
               qFuzzyCompare(currentTargetAngularRateEl, other.currentTargetAngularRateEl) &&
               qFuzzyCompare(muzzleVelocityMPS, other.muzzleVelocityMPS) &&
               weaponSystemStatus == other.weaponSystemStatus &&
               targetInformation == other.targetInformation &&
               gpsCoordinates == other.gpsCoordinates &&
               sensorReadings == other.sensorReadings &&
               alertsWarnings == other.alertsWarnings &&
               leadStatusText == other.leadStatusText &&
               zeroingStatusText == other.zeroingStatusText&&
               (radarPlots == other.radarPlots);
    }
    
    bool operator!=(const SystemStateData& other) const {
        return !(*this == other);
    }
};

#endif // SYSTEMSTATEDATA_H
