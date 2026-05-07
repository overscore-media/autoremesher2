#include "spinnableawesomebutton.h"
#include <QWidget>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include "theme.h"

SpinnableAwesomeButton::SpinnableAwesomeButton(QWidget *parent) :
    QWidget(parent)
{
    setFixedSize(Theme::toolIconSize, Theme::toolIconSize);

    m_button = new QPushButton(this);
    Theme::initAwesomeButton(m_button);
    
    m_spinner = new WaitingSpinnerWidget(this);
    m_spinner->setColor(Theme::white);
    m_spinner->setInnerRadius(Theme::toolIconSize / 8);
    m_spinner->setLineLength(Theme::toolIconSize / 4);
    m_spinner->setNumberOfLines(9);
    m_spinner->hide();
}

void SpinnableAwesomeButton::setAwesomeIcon(QChar c)
{
    m_button->setText(c);
}

void SpinnableAwesomeButton::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (m_isSpinning) {
        if (m_button) m_button->setVisible(false);
        if (m_spinner) m_spinner->setVisible(visible);
    } else {
        if (m_button) m_button->setVisible(visible);
        if (m_spinner) m_spinner->setVisible(false);
    }
}

void SpinnableAwesomeButton::showSpinner(bool showSpinner)
{
    m_isSpinning = showSpinner;
    if (showSpinner) {
        m_spinner->start();
        m_spinner->setVisible(isVisible());
        m_button->setVisible(false);
    } else {
        m_spinner->stop();
        m_spinner->hide();
        m_button->setVisible(isVisible());
    }
}

QPushButton *SpinnableAwesomeButton::button()
{
    return m_button;
}


bool SpinnableAwesomeButton::isSpinning()
{
    return m_spinner->isVisible();
}

void SpinnableAwesomeButton::setCheckable(bool checkable)
{
    m_checkable = checkable;
    if (m_button) {
        m_button->setCheckable(checkable);
    }
}

void SpinnableAwesomeButton::setChecked(bool checked)
{
    m_checked = checked;
    if (m_button) {
        m_button->setChecked(checked);
    }
}

bool SpinnableAwesomeButton::isChecked()
{
    return m_checked;
}

void SpinnableAwesomeButton::setStrikeThrough(bool strike)
{
    m_strikeThrough = strike;
    update();
}

void SpinnableAwesomeButton::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (!m_strikeThrough)
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(220, 220, 230), 2));
    QRect r = rect().adjusted(4, 4, -4, -4);
    p.drawLine(r.topLeft(), r.bottomRight());
}
