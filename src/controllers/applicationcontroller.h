#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include <QObject>

#include "models/menuviewmodel.h"

class MainMenuController;
class ReticleMenuController;
class ColorMenuController;
class MenuViewModel;

class ApplicationController : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);
    void initialize();

public slots:
    // Called by physical button handlers or QML
    void onMenuButtonPressed();
    void onUpButtonPressed();
    void onDownButtonPressed();
    void onSelectButtonPressed();
    void onBackButtonPressed();

private slots:
    // Main Menu handlers
    void handlePersonalizeReticle();
    void handlePersonalizeColors();
    void handleAdjustBrightness();
    void handleZeroing();
    void handleClearZero();
    void handleWindage();
    void handleClearWindage();
    void handleZoneDefinitions();
    void handleSystemStatus();
    void handleRadarTargetList();
    void handleHelpAbout();

    // Submenu handlers
    void handleReticleMenuFinished();
    void handleColorMenuFinished();
    void handleReturnToMainMenu();

private:
    enum class MenuState {
        None,
        MainMenu,
        ReticleMenu,
        ColorMenu,
        BrightnessAdjust,
        ZeroingProcedure,
        WindageProcedure,
        ZoneDefinition,
        SystemStatus,
        RadarTargets,
        HelpAbout
    };

    void showMainMenu();
    void hideAllMenus();
    void setMenuState(MenuState state);

    void connectMainMenuSignals();
    void disconnectMainMenuSignals();
    void connectReticleMenuSignals();
    void disconnectReticleMenuSignals();
    void connectColorMenuSignals();
    void disconnectColorMenuSignals();

    MenuState m_currentMenuState;

    // Controllers
    MainMenuController* m_mainMenuController;
    ReticleMenuController* m_reticleMenuController;
    ColorMenuController* m_colorMenuController;
    // Add other controllers as needed
};

#endif // APPLICATIONCONTROLLER_H
