#include "radarslewmotionmode.h"
#include "controllers/gimbalcontroller.h" // For GimbalController and SystemStateData
#include "models/domain/systemstatemodel.h" // For SimpleRadarPlot
#include <QDebug>
#include <cmath>
#include <QtGlobal>

RadarSlewMotionMode::RadarSlewMotionMode(QObject* parent)
    : GimbalMotionModeBase(parent), // Call base constructor
    m_currentTargetId(0),
    m_isSlewInProgress(false)
{
    // Configure PID gains for fast but stable slewing.
    // These will likely be more aggressive than the TRP scan PIDs.
    m_azPid.Kp = 1.5; m_azPid.Ki = 0.08; m_azPid.Kd = 0.15; m_azPid.maxIntegral = 30.0;
    m_elPid.Kp = 1.5; m_elPid.Ki = 0.08; m_elPid.Kd = 0.15; m_elPid.maxIntegral = 30.0;
}

void RadarSlewMotionMode::enterMode(GimbalController* controller)
{
    qDebug() << "[RadarSlewMotionMode] Enter. Awaiting slew command.";
    m_isSlewInProgress = false;
    m_currentTargetId = 0;
    m_previousDesiredAzVel = 0.0;
    m_previousDesiredElVel = 0.0;

    // The GimbalController::setMotionMode already called exitMode on the previous mode,
    // which should have already stopped the servos. Calling it again here is
    // redundant and is likely triggering the problematic signal feedback loop.
    // Let's remove it and rely on the exitMode of the previous state.
    // if (controller) {
    //     stopServos(controller); // <<< TRY COMMENTING THIS OUT
    // }

    if (controller) {
        if (auto azServo = controller->azimuthServo()) setAcceleration(azServo, 100000);
        if (auto elServo = controller->elevationServo()) setAcceleration(elServo, 100000);
    }
}

void RadarSlewMotionMode::exitMode(GimbalController* controller)
{
    qDebug() << "[RadarSlewMotionMode] Exit.";
    stopServos(controller);
}

void RadarSlewMotionMode::update(GimbalController* controller)
{
    // --- 1. Safety and Pre-condition Checks ---
    if (!controller || !controller->systemStateModel()) { return; }
    if (!checkSafetyConditions(controller)) {
        if (m_isSlewInProgress) {
            qWarning() << "[RadarSlewMotionMode] Safety condition failed during slew. Stopping.";
            stopServos(controller);
            m_isSlewInProgress = false;
        }
        return;
    }

    SystemStateData data = controller->systemStateModel()->data();

    // --- 2. Check for a New Slew Command from the Model ---
    if (data.selectedRadarTrackId != 0 && data.selectedRadarTrackId != m_currentTargetId) {
        qInfo() << "[RadarSlewMotionMode] New slew command received for Target ID:" << data.selectedRadarTrackId;
        m_currentTargetId = data.selectedRadarTrackId;

        // Find the full plot data for the commanded target ID
        auto it = std::find_if(data.radarPlots.begin(), data.radarPlots.end(),
                               [&](const SimpleRadarPlot& p){ return p.id == m_currentTargetId; });

        if (it != data.radarPlots.end()) {
            m_targetAz = it->azimuth;
            m_targetEl = atan2(-SYSTEM_HEIGHT_METERS, it->range) * (180.0 / M_PI);

            m_isSlewInProgress = true;
            m_azPid.reset();
            m_elPid.reset();

            // Reset velocity smoothing for new target
            m_previousDesiredAzVel = 0.0;
            m_previousDesiredElVel = 0.0;

            qDebug() << "[RadarSlewMotionMode] Target set to Az:" << m_targetAz << "| Calculated El:" << m_targetEl;
        } else {
            qWarning() << "[RadarSlewMotionMode] Could not find commanded target ID" << m_currentTargetId << "in model data. Slew aborted.";
            m_isSlewInProgress = false;
            m_currentTargetId = 0;
        }
    }

    // --- 3. Execute Movement if a Slew is in Progress ---
    if (!m_isSlewInProgress) {
        stopServos(controller);
        return;
    }

    // Calculate error to the target
    double errAz = m_targetAz - data.gimbalAz; // Azimuth still uses encoder
    double errEl = m_targetEl - data.imuPitchDeg; // Elevation now uses IMU Pitch

    // Normalize Azimuth error to take the shortest path
    while (errAz > 180.0)  errAz -= 360.0;
    while (errAz < -180.0) errAz += 360.0;

    // Check for arrival at the destination
    // Corrected to use both Az and El errors for 2D distance.
    if (std::abs(errAz) < ARRIVAL_THRESHOLD_DEG && std::abs(errEl) < ARRIVAL_THRESHOLD_DEG) {
        qInfo() << "[RadarSlewMotionMode] Arrived at target ID:" << m_currentTargetId;
        stopServos(controller);
        m_isSlewInProgress = false;
        return;
    }

    // --- 4. MOTION PROFILING SIMILAR TO TRP SCAN ---
    double distanceToTarget = std::sqrt(errAz * errAz + errEl * errEl);
    double desiredAzVelocity = 0.0;
    double desiredElVelocity = 0.0;

    // Define constants for motion profiling
    static const double DECELERATION_DISTANCE_DEG = 5.0;  // Start decelerating when within 5 degrees
    static const double CRUISE_SPEED_DEGS = 12.0;         // Cruise speed for slewing
    static const double MAX_VELOCITY_CHANGE = 3.0;        // Maximum velocity change per update cycle

    if (distanceToTarget < DECELERATION_DISTANCE_DEG) {
        // DECELERATION ZONE: Use PID to slow down smoothly
        qDebug() << "[RadarSlewMotionMode] Decelerating. Distance:" << distanceToTarget;
        desiredAzVelocity = pidCompute(m_azPid, errAz, UPDATE_INTERVAL_S);
        desiredElVelocity = pidCompute(m_elPid, errEl, UPDATE_INTERVAL_S);
    } else {
        // CRUISING ZONE: Move at constant speed toward target
        double dirAz = errAz / distanceToTarget;
        double dirEl = errEl / distanceToTarget;
        desiredAzVelocity = dirAz * CRUISE_SPEED_DEGS;
        desiredElVelocity = dirEl * CRUISE_SPEED_DEGS;

        // Reset PID during cruise to prevent integral windup
        m_azPid.reset();
        m_elPid.reset();
    }

    // --- 5. VELOCITY SMOOTHING AND RATE LIMITING (from TrackingMotionMode) ---

    // Apply velocity constraints
    desiredAzVelocity = qBound(-MAX_SLEW_SPEED_DEGS, desiredAzVelocity, MAX_SLEW_SPEED_DEGS);
    desiredElVelocity = qBound(-MAX_SLEW_SPEED_DEGS, desiredElVelocity, MAX_SLEW_SPEED_DEGS);

    // Apply rate limiting to prevent sudden velocity changes
    double velocityChangeAz = desiredAzVelocity - m_previousDesiredAzVel;
    if (std::abs(velocityChangeAz) > MAX_VELOCITY_CHANGE) {
        desiredAzVelocity = m_previousDesiredAzVel + (velocityChangeAz > 0 ? MAX_VELOCITY_CHANGE : -MAX_VELOCITY_CHANGE);
    }

    double velocityChangeEl = desiredElVelocity - m_previousDesiredElVel;
    if (std::abs(velocityChangeEl) > MAX_VELOCITY_CHANGE) {
        desiredElVelocity = m_previousDesiredElVel + (velocityChangeEl > 0 ? MAX_VELOCITY_CHANGE : -MAX_VELOCITY_CHANGE);
    }

    // Store current velocities for next cycle
    m_previousDesiredAzVel = desiredAzVelocity;
    m_previousDesiredElVel = desiredElVelocity;

    // Debug output
    static int debugCounter = 0;
    if (++debugCounter % 25 == 0) { // Print every 25 updates (~0.5 seconds at 50Hz)
        qDebug() << "[RadarSlewMotionMode] Error(Az,El):" << errAz << "," << errEl
                 << "| Vel(Az,El):" << desiredAzVelocity << "," << desiredElVelocity
                 << "| Distance:" << distanceToTarget;
    }

    // Let the base class handle stabilization and sending the final command
    sendStabilizedServoCommands(controller, desiredAzVelocity, desiredElVelocity, true);
}


