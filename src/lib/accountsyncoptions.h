/****************************************************************************************
** Copyright (c) 2014 - 2023 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
**
** All rights reserved.
**
** This file is part of Sailfish Accounts components package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#ifndef ACCOUNTSYNCOPTIONS_H
#define ACCOUNTSYNCOPTIONS_H

#include <QObject>
#include <QTime>

class AccountSyncSchedulePrivate;
class AccountSyncOptionsPrivate;

class Q_DECL_EXPORT AccountSyncSchedule : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int days READ days NOTIFY daysChanged)
    Q_PROPERTY(QTime dailySyncTime READ dailySyncTime NOTIFY dailySyncTimeChanged)
    Q_PROPERTY(Interval interval READ interval NOTIFY intervalChanged)
    Q_PROPERTY(LongInterval longInterval READ longInterval NOTIFY longIntervalChanged)
    Q_PROPERTY(bool peakScheduleEnabled READ peakScheduleEnabled WRITE setPeakScheduleEnabled NOTIFY peakScheduleEnabledChanged)
    Q_PROPERTY(bool syncExternallyDuringPeak READ syncExternallyDuringPeak WRITE setSyncExternallyDuringPeak NOTIFY syncExternallyDuringPeakChanged)
    Q_PROPERTY(QTime peakStartTime READ peakStartTime NOTIFY peakStartTimeChanged)
    Q_PROPERTY(QTime peakEndTime READ peakEndTime NOTIFY peakEndTimeChanged)
    Q_PROPERTY(Interval peakInterval READ peakInterval NOTIFY peakIntervalChanged)
    Q_PROPERTY(int peakDays READ peakDays NOTIFY peakDaysChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)

public:
    enum Interval {
        Every15Minutes,
        Every30Minutes,
        EveryHour,
        TwiceDailyInterval,
        Every5Minutes,
        NoInterval = 100
    };
    Q_ENUM(Interval)

    enum LongInterval {
        MonthLongInterval,
        FirstDayOfMonthInterval,
        LastDayOfMonthInterval,
        NoLongInterval = 100
    };
    Q_ENUM(LongInterval)

    enum Day {
        Monday = 0x01,
        Tuesday = 0x02,
        Wednesday = 0x04,
        Thursday = 0x08,
        Friday = 0x10,
        Saturday = 0x20,
        Sunday = 0x40,
        WeekDays = Monday | Tuesday | Wednesday | Thursday | Friday,
        WeekendDays = Saturday | Sunday,
        EveryDay = WeekDays | WeekendDays,
    };
    Q_ENUM(Day)
    Q_DECLARE_FLAGS(Days, Day)

    AccountSyncSchedule(QObject *parent = 0);
    ~AccountSyncSchedule();

    void setEnabled(bool enabled);
    bool enabled() const;

    Q_INVOKABLE void setDailySyncMode(const QTime &time, int days = everyday());
    Q_INVOKABLE void setIntervalSyncMode(Interval interval, int days = everyday());
    Q_INVOKABLE void setLongIntervalSyncMode(LongInterval interval, const QTime &time);

    void setPeakScheduleEnabled(bool enable);
    bool peakScheduleEnabled() const;
    void setSyncExternallyDuringPeak(bool enable);
    bool syncExternallyDuringPeak() const;

    Q_INVOKABLE void setPeakSchedule(const QTime &peakStart,
                                     const QTime &peakEnd,
                                     Interval peakInterval,
                                     int peakDays);
    Q_INVOKABLE void setDefaultPeakSchedule();

    bool modified() const;

    int days() const;
    Interval interval() const;
    QTime dailySyncTime() const;
    LongInterval longInterval() const;

    int peakDays() const;
    Interval peakInterval() const;
    QTime peakStartTime() const;
    QTime peakEndTime() const;

    static Days everyday();
    static Days weekdays();
    static Days weekendDays();

Q_SIGNALS:
    void enabledChanged();
    void daysChanged();
    void intervalChanged();
    void longIntervalChanged();
    void peakIntervalChanged();
    void dailySyncTimeChanged();
    void peakScheduleEnabledChanged();
    void syncExternallyDuringPeakChanged();
    void peakStartTimeChanged();
    void peakEndTimeChanged();
    void peakDaysChanged();
    void modifiedChanged();

private:
    friend class AccountSyncSchedulePrivate;
    AccountSyncSchedulePrivate *d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AccountSyncSchedule::Days);


class Q_DECL_EXPORT AccountSyncOptions : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool automaticSyncEnabled READ automaticSyncEnabled WRITE setAutomaticSyncEnabled NOTIFY automaticSyncEnabledChanged)
    Q_PROPERTY(bool syncExternallyEnabled READ syncExternallyEnabled WRITE setSyncExternallyEnabled NOTIFY syncExternallyEnabledChanged)
    Q_PROPERTY(PastSyncPeriod pastSyncPeriod READ pastSyncPeriod WRITE setPastSyncPeriod NOTIFY pastSyncPeriodChanged)
    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(int allowedConnectionTypes READ allowedConnectionTypes WRITE setAllowedConnectionTypes NOTIFY allowedConnectionTypesChanged)
    Q_PROPERTY(AccountSyncSchedule *schedule READ schedule WRITE setSchedule NOTIFY scheduleChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_ENUMS(PastSyncPeriod)
    Q_ENUMS(Direction)
    Q_ENUMS(NetworkConnection)
public:
    enum PastSyncPeriod {
        OneDayAgo,
        ThreeDaysAgo,
        OneWeekAgo,
        TwoWeeksAgo,
        OneMonthAgo
    };

    enum Direction {
        OneWayToDevice,
        OneWayFromDevice,
        TwoWaySync
    };

    enum NetworkConnection {
        Ethernet = 0x01,
        Wlan = 0x02,
        Cellular = 0x04,
        Bluetooth = 0x08
    };
    Q_DECLARE_FLAGS(NetworkConnections, NetworkConnection)

    AccountSyncOptions(QObject *parent = 0);
    ~AccountSyncOptions();

    bool modified() const;

    void setAutomaticSyncEnabled(bool enabled);
    bool automaticSyncEnabled() const;

    void setSyncExternallyEnabled(bool enabled);
    bool syncExternallyEnabled() const;

    PastSyncPeriod pastSyncPeriod() const;
    void setPastSyncPeriod(PastSyncPeriod period);

    Direction direction() const;
    void setDirection(Direction direction);

    int allowedConnectionTypes() const;
    void setAllowedConnectionTypes(int allowedConnectionTypes);

    AccountSyncSchedule *schedule() const;
    void setSchedule(AccountSyncSchedule *schedule);

Q_SIGNALS:
    void automaticSyncEnabledChanged();
    void syncExternallyEnabledChanged();
    void pastSyncPeriodChanged();
    void directionChanged();
    void allowedConnectionTypesChanged();
    void scheduleChanged();
    void modifiedChanged();

private:
    friend class AccountSyncOptionsPrivate;
    AccountSyncOptionsPrivate *d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AccountSyncOptions::NetworkConnections);

#endif
