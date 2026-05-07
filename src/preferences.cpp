/*
 *  Copyright (c) 2020 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */
#include "preferences.h"
#include <QThread>

Preferences &Preferences::instance()
{
    static Preferences *s_preferences = nullptr;
    if (nullptr == s_preferences) {
        s_preferences = new Preferences;
    }
    return *s_preferences;
}

Preferences::Preferences()
{
}

QSize Preferences::mainWindowSize() const
{
    return m_settings.value("mainWindowSize", QSize(1280, 720)).toSize();
}

void Preferences::setMainWindowSize(const QSize& size)
{
    m_settings.setValue("mainWindowSize", size);
}

int Preferences::threadCount() const
{
    int stored = m_settings.value("threadCount", 1).toInt();
    if (stored <= 0) {
        stored = QThread::idealThreadCount();
        if (stored <= 0) stored = 1;
    }
    return stored;
}

void Preferences::setThreadCount(int count)
{
    m_settings.setValue("threadCount", count);
}

int Preferences::trackVertCount() const
{
    return m_settings.value("trackVertCount", 0).toInt();
}

void Preferences::setTrackVertCount(int count)
{
    m_settings.setValue("trackVertCount", count);
}

int Preferences::trackFaceCount() const
{
    return m_settings.value("trackFaceCount", 0).toInt();
}

void Preferences::setTrackFaceCount(int count)
{
    m_settings.setValue("trackFaceCount", count);
}

bool Preferences::showCompareBar() const
{
    return m_settings.value("showCompareBar", true).toBool();
}

void Preferences::setShowCompareBar(bool show)
{
    m_settings.setValue("showCompareBar", show);
}

void Preferences::reset()
{
    m_settings.clear();
}
