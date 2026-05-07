#ifndef AUTOREMESHER_SPINNABLE_AWESOME_BUTTON_H
#define AUTOREMESHER_SPINNABLE_AWESOME_BUTTON_H
#include <QWidget>
#include <QPushButton>
#include "waitingspinnerwidget.h"

class QPaintEvent;

class SpinnableAwesomeButton : public QWidget
{
    Q_OBJECT
public:
    SpinnableAwesomeButton(QWidget *parent=nullptr);
    void setAwesomeIcon(QChar c);
    void showSpinner(bool showSpinner=true);
    bool isSpinning();
    void setCheckable(bool checkable);
    void setChecked(bool checked);
    bool isChecked();
    void setVisible(bool visible) override;
    QPushButton *button();
    void setStrikeThrough(bool strike);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QPushButton *m_button = nullptr;
    WaitingSpinnerWidget *m_spinner = nullptr;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_isSpinning = false;
    bool m_strikeThrough = false;
};

#endif
