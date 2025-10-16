#ifndef RETICLEMENUCONTROLLER_H
#define RETICLEMENUCONTROLLER_H

#include <QObject>
#include "models/menuviewmodel.h"
#include "models/domain/systemstatedata.h"

// Forward declaration - adjust to match your actual enum location
/*enum class ReticleType {
    Basic = 0,
    BoxCrosshair,
    StandardCrosshair,
    PrecisionCrosshair,
    MilDot,
    COUNT
};*/

class OsdViewModel; // Your OSD model

class ReticleMenuController : public QObject
{
    Q_OBJECT
public:
    explicit ReticleMenuController(QObject *parent = nullptr);
    void initialize();

public slots:
    void show();
    void hide();
    void onUpButtonPressed();
    void onDownButtonPressed();
    void onSelectButtonPressed();
    void onBackButtonPressed();

signals:
    void menuFinished();
    void returnToMainMenu();

private slots:
    void handleMenuOptionSelected(const QString& option);
    void handleCurrentItemChanged(int index);

private:
    MenuViewModel* m_viewModel;
    OsdViewModel* m_osdViewModel; // To update reticle preview

    QStringList buildReticleOptions() const;
    QString reticleTypeToString(ReticleType type) const;
    ReticleType stringToReticleType(const QString& str) const;
    ReticleType m_originalReticleType; // To restore on cancel
};

#endif // RETICLEMENUCONTROLLER_H
