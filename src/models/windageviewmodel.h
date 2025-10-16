#ifndef WINDAGEVIEWMODEL_H
#define WINDAGEVIEWMODEL_H

#include <QObject>
#include <QColor>

class WindageViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString instruction READ instruction NOTIFY instructionChanged)
    Q_PROPERTY(bool showWindSpeed READ showWindSpeed NOTIFY showWindSpeedChanged)
    Q_PROPERTY(float windSpeed READ windSpeed NOTIFY windSpeedChanged)
    Q_PROPERTY(QString windSpeedLabel READ windSpeedLabel NOTIFY windSpeedLabelChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)


public:
    explicit WindageViewModel(QObject *parent = nullptr);

    bool visible() const { return m_visible; }
    QString title() const { return m_title; }
    QString instruction() const { return m_instruction; }
    bool showWindSpeed() const { return m_showWindSpeed; }
    float windSpeed() const { return m_windSpeed; }
    QString windSpeedLabel() const { return m_windSpeedLabel; }
    QColor accentColor() const { return m_accentColor; }

public slots:
    void setVisible(bool visible);
    void setTitle(const QString& title);
    void setInstruction(const QString& instruction);
    void setShowWindSpeed(bool show);
    void setWindSpeed(float speed);
    void setWindSpeedLabel(const QString& label);
    void setAccentColor(const QColor& color);

signals:
    void visibleChanged();
    void titleChanged();
    void instructionChanged();
    void showWindSpeedChanged();
    void windSpeedChanged();
    void windSpeedLabelChanged();
    void accentColorChanged();

private:
    bool m_visible;
    QString m_title;
    QString m_instruction;
    bool m_showWindSpeed;
    float m_windSpeed;
    QString m_windSpeedLabel;
    QColor m_accentColor = QColor(70, 226, 165); // Default green
};

#endif // WINDAGEVIEWMODEL_H
