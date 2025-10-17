#include "trackingmotionmode.h"
#include "controllers/gimbalcontroller.h"
#include "models/domain/systemstatemodel.h"
#include <QDebug>
#include <QtGlobal>
#include <cmath>

// Define smoothing and rate limiting constants
static const double SMOOTHING_ALPHA = 0.3;           // Position smoothing (0.0 = no smoothing, 1.0 = no filtering)
static const double VELOCITY_SMOOTHING_ALPHA = 0.2;   // Velocity smoothing (more aggressive)
static const double MAX_VELOCITY = 15.0;              // Maximum velocity in deg/s
static const double MAX_ACCELERATION = 30.0;          // Maximum acceleration in deg/s²
static const double VELOCITY_CHANGE_LIMIT = 5.0;      // Maximum velocity change per update cycle

TrackingMotionMode::TrackingMotionMode(QObject* parent)
    : GimbalMotionModeBase(parent), m_targetValid(false), m_targetAz(0.0), m_targetEl(0.0)
    , m_smoothedAzVel_dps(0.0), m_smoothedElVel_dps(0.0)
    , m_previousDesiredAzVel(0.0), m_previousDesiredElVel(0.0)  // Initialize previous velocities
{
    // Configure PID gains for STABLE and SMOOTH target tracking
    // Reduced gains to prevent motor overload
    m_azPid.Kp = 0.15;  // Reduced from 0.25
    m_azPid.Ki = 0.005; // Reduced from 0.01
    m_azPid.Kd = 0.01;  // Reduced from 0.02
    m_azPid.maxIntegral = 10.0; // Reduced from 20.0

    m_elPid.Kp = 0.15;  // Reduced from 0.25
    m_elPid.Ki = 0.005; // Reduced from 0.01
    m_elPid.Kd = 0.01;  // Reduced from 0.02
    m_elPid.maxIntegral = 10.0; // Reduced from 20.0
}

void TrackingMotionMode::enterMode(GimbalController* controller)
{
    qDebug() << "[TrackingMotionMode] Enter";
    
    // Invalidate target on entering the mode to ensure we wait for a fresh command.
    m_targetValid = false;
    m_azPid.reset();
    m_elPid.reset();
    
    // Reset previous velocities for rate limiting
    m_previousDesiredAzVel = 0.0;
    m_previousDesiredElVel = 0.0;
    
    if (controller) {
        // Set MODERATE acceleration for responsive but smooth tracking
        // Reduced from 200000 to prevent motor overload
        if (auto azServo = controller->azimuthServo()) setAcceleration(azServo, 50000);
        if (auto elServo = controller->elevationServo()) setAcceleration(elServo, 50000);
    }
}

void TrackingMotionMode::exitMode(GimbalController* controller)
{
    qDebug() << "[TrackingMotionMode] Exit";
    stopServos(controller);
}

void TrackingMotionMode::onTargetPositionUpdated(double az, double el, 
                                               double velocityAz_dps, double velocityEl_dps, 
                                               bool isValid)
{
    if (isValid) {
        if (!m_targetValid) {
             qDebug() << "[TrackingMotionMode] New valid target acquired.";
             m_azPid.reset();
             m_elPid.reset();
             m_smoothedTargetAz = az;
             m_smoothedTargetEl = el;
             // Initialize velocity smoother with first measured velocity
             m_smoothedAzVel_dps = velocityAz_dps;
             m_smoothedElVel_dps = velocityEl_dps;
        }
        m_targetValid = true;
        m_targetAz = az;
        m_targetEl = el;
        m_targetAzVel_dps = velocityAz_dps;
        m_targetElVel_dps = velocityEl_dps;

    } else {
        if (m_targetValid) {
             qDebug() << "[TrackingMotionMode] Target has been definitively lost.";
        }
        m_targetValid = false;
        // Reset velocities when target is lost
        m_targetAzVel_dps = 0.0;
        m_targetElVel_dps = 0.0;
        m_smoothedAzVel_dps = 0.0;
        m_smoothedElVel_dps = 0.0;
    }
}

// Helper function to apply rate limiting
double TrackingMotionMode::applyRateLimit(double newVelocity, double previousVelocity, double maxChange)
{
    double velocityChange = newVelocity - previousVelocity;
    if (std::abs(velocityChange) > maxChange) {
        // Limit the change to prevent sudden jumps
        return previousVelocity + (velocityChange > 0 ? maxChange : -maxChange);
    }
    return newVelocity;
}

double TrackingMotionMode::applyVelocityScaling(double velocity, double error) 
{
    // When error is zero, keep at 30% of full feed‑forward;
    // as error approaches threshold, ramp up to 100%.
    static constexpr double ERROR_THRESHOLD = 2.0;  // [deg]
    static constexpr double MIN_SCALE      = 0.3;   // 30%
    
    double absErr = std::abs(error);
    if (absErr >= ERROR_THRESHOLD) {
        // Outside threshold: no scaling
        return velocity;
    }
    
    // Compute normalized error [0,1]
    double norm = absErr / ERROR_THRESHOLD;
    // Quadratic blend: smoother ramp than linear
    double scale = MIN_SCALE + (1.0 - MIN_SCALE) * (norm * norm);
    
    return velocity * scale;
}

void TrackingMotionMode::update(GimbalController* controller)
{
    if (!m_targetValid) {
        stopServos(controller);
        return;
    }
    qint64 ms_elapsed = m_velocityTimer.restart(); // restart() returns elapsed and resets
    double dt_s = ms_elapsed / 1000.0;    
    SystemStateData data = controller->systemStateModel()->data();

    // 1. Smooth the Target Position (for PID feedback)
    m_smoothedTargetAz = (SMOOTHING_ALPHA * m_targetAz) + (1.0 - SMOOTHING_ALPHA) * m_smoothedTargetAz;
    m_smoothedTargetEl = (SMOOTHING_ALPHA * m_targetEl) + (1.0 - SMOOTHING_ALPHA) * m_smoothedTargetEl;

    // 2. Smooth the Target Velocity (for feed-forward)
    m_smoothedAzVel_dps = (VELOCITY_SMOOTHING_ALPHA * m_targetAzVel_dps) + (1.0 - VELOCITY_SMOOTHING_ALPHA) * m_smoothedAzVel_dps;
    m_smoothedElVel_dps = (VELOCITY_SMOOTHING_ALPHA * m_targetElVel_dps) + (1.0 - VELOCITY_SMOOTHING_ALPHA) * m_smoothedElVel_dps;

    // 3. Calculate Position Error
    double errAz = m_smoothedTargetAz - data.gimbalAz; // Azimuth still uses encoder
    double errEl = m_smoothedTargetEl - data.imuPitchDeg; // Elevation now uses IMU Pitch
    
    // Normalize azimuth error to [-180, 180] range
    while (errAz > 180.0) errAz -= 360.0;
    while (errAz < -180.0) errAz += 360.0;

    // 4. Calculate PID output (Feedback)
    bool useDerivativeOnMeasurement = true;
    // CRITICAL FIX: Use the measured dt_s, not the constant UPDATE_INTERVAL_S
    double pidAzVelocity = pidCompute(m_azPid, errAz, m_smoothedTargetAz, data.gimbalAz, useDerivativeOnMeasurement, dt_s);
    double pidElVelocity = pidCompute(m_elPid, errEl, m_smoothedTargetEl, data.imuPitchDeg, useDerivativeOnMeasurement, dt_s); // Use imuPitchDeg for derivative measurement

    // 5. Add Feed-forward term (scaled down to prevent aggressive response)
    const double FEEDFORWARD_GAIN = 0.5; // Scale down the feed-forward contribution
    double desiredAzVelocity = pidAzVelocity + (FEEDFORWARD_GAIN * m_smoothedAzVel_dps);
    double desiredElVelocity = pidElVelocity + (FEEDFORWARD_GAIN * m_smoothedElVel_dps);

    // 6. Apply velocity scaling based on error magnitude
    desiredAzVelocity = applyVelocityScaling(desiredAzVelocity, errAz);
    desiredElVelocity = applyVelocityScaling(desiredElVelocity, errEl);

    // 7. Apply system velocity constraints
    desiredAzVelocity = qBound(-MAX_VELOCITY, desiredAzVelocity, MAX_VELOCITY);
    desiredElVelocity = qBound(-MAX_VELOCITY, desiredElVelocity, MAX_VELOCITY);

    // 8. Apply rate limiting to prevent sudden velocity changes
    desiredAzVelocity = applyRateLimit(desiredAzVelocity, m_previousDesiredAzVel, VELOCITY_CHANGE_LIMIT);
    desiredElVelocity = applyRateLimit(desiredElVelocity, m_previousDesiredElVel, VELOCITY_CHANGE_LIMIT);

    // 9. Store current velocities for next cycle
    m_previousDesiredAzVel = desiredAzVelocity;
    m_previousDesiredElVel = desiredElVelocity;

    // Debug output (reduced frequency to avoid spam)
    /*static int debugCounter = 0;
    if (++debugCounter % 10 == 0) { // Print every 10 updates
        qDebug() << "Tracking - Error(Az,El):" << errAz << "," << errEl 
                 << "| Vel(Az,El):" << desiredAzVelocity << "," << desiredElVelocity
                 << "| FF(Az,El):" << m_smoothedAzVel_dps << "," << m_smoothedElVel_dps
                 << "| elapsed(s):" << dt_s;
    }*/

    // 10. Send final commands
    sendStabilizedServoCommands(controller, desiredAzVelocity, desiredElVelocity);
}

