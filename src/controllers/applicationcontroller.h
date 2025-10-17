#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include <QObject>

class MainMenuController;
class ReticleMenuController;
class ColorMenuController;
class ZeroingController;
class WindageController;
class ZoneDefinitionController;
class SystemStateModel;

class ApplicationController : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);

    // Dependency injection - called by SystemController
    void setMainMenuController(MainMenuController* controller);
    void setReticleMenuController(ReticleMenuController* controller);
    void setColorMenuController(ColorMenuController* controller);
    void setZeroingController(ZeroingController* controller);
    void setWindageController(WindageController* controller);
    void setZoneDefinitionController(ZoneDefinitionController* controller);
    void setSystemStateModel(SystemStateModel* model);

    void initialize();

    // Menu state enum
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

public slots:
    // Button handlers (called from QML or hardware controller)
    void onMenuValButtonPressed();
    void onUpButtonPressed();
    void onDownButtonPressed();

    void showMainMenu();

private slots:
    // Main menu action handlers
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

    // Completion handlers
    void handleMainMenuFinished();
    void handleReticleMenuFinished();
    void handleColorMenuFinished();
    void handleZeroingFinished();
    void handleWindageFinished();
    void handleZoneDefinitionFinished();
    void handleReturnToMainMenu();

private:
    void setMenuState(MenuState state);
    void hideAllMenus();

    // State-specific button handlers
    void handleMenuValInNoMenuState();
    void handleMenuValInMainMenu();
    void handleMenuValInSubmenu();
    void handleMenuValInProcedure();

    MenuState m_currentMenuState;

    // Dependencies (injected by SystemController)
    MainMenuController* m_mainMenuController;
    ReticleMenuController* m_reticleMenuController;
    ColorMenuController* m_colorMenuController;
    ZeroingController* m_zeroingController;
    WindageController* m_windageController;
    ZoneDefinitionController* m_zoneDefinitionController;
    SystemStateModel* m_systemStateModel;
};

#endif // APPLICATIONCONTROLLER_H
