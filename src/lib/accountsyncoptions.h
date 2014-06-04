/*
 * Copyright (C) 2014 Jolla Ltd.
 * Contact: Bea Lam <bea.lam@jollamobile.com>
 *
 * License: Proprietary
 */

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
    Q_PROPERTY(bool peakScheduleEnabled READ peakScheduleEnabled WRITE setPeakScheduleEnabled NOTIFY peakScheduleEnabledChanged)
    Q_PROPERTY(QTime peakStartTime READ peakStartTime NOTIFY peakStartTimeChanged)
    Q_PROPERTY(QTime peakEndTime READ peakEndTime NOTIFY peakEndTimeChanged)
    Q_PROPERTY(Interval peakInterval READ peakInterval NOTIFY peakIntervalChanged)
    Q_PROPERTY(int peakDays READ peakDays NOTIFY peakDaysChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_ENUMS(Interval)
    Q_ENUMS(Day)

public:
    enum Interval {
        Every15Minutes,
        Every30Minutes,
        EveryHour,
        TwiceDailyInterval
    };

    enum Day {
        Monday = 0x01,
        Tuesday = 0x02,
        Wednesday = 0x04,
        Thursday = 0x08,
        Friday = 0x10,
        Saturday = 0x20,
        Sunday = 0x40
    };
    Q_DECLARE_FLAGS(Days, Day);

    AccountSyncSchedule(QObject *parent = 0);
    ~AccountSyncSchedule();

    void setEnabled(bool enabled);
    bool enabled() const;

    Q_INVOKABLE void setDailySyncMode(const QTime &time, int days = everyday());
    Q_INVOKABLE void setIntervalSyncMode(Interval interval, int days = everyday());

    void setPeakScheduleEnabled(bool enable);
    bool peakScheduleEnabled() const;
    Q_INVOKABLE void setPeakSchedule(const QTime &peakStart,
                                     const QTime &peakEnd,
                                     Interval peakInterval,
                                     int peakDays);
    Q_INVOKABLE void setDefaultPeakSchedule();

    bool modified() const;

    int days() const;
    Interval interval() const;
    QTime dailySyncTime() const;

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
    void peakIntervalChanged();
    void dailySyncTimeChanged();
    void peakScheduleEnabledChanged();
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
    Q_PROPERTY(PastSyncPeriod pastSyncPeriod READ pastSyncPeriod WRITE setPastSyncPeriod NOTIFY pastSyncPeriodChanged)
    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(AccountSyncSchedule *schedule READ schedule WRITE setSchedule NOTIFY scheduleChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_ENUMS(PastSyncPeriod)
    Q_ENUMS(Direction)
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

    AccountSyncOptions(QObject *parent = 0);
    ~AccountSyncOptions();

    bool modified() const;

    void setAutomaticSyncEnabled(bool enabled);
    bool automaticSyncEnabled() const;

    PastSyncPeriod pastSyncPeriod() const;
    void setPastSyncPeriod(PastSyncPeriod period);

    Direction direction() const;
    void setDirection(Direction direction);

    AccountSyncSchedule *schedule() const;
    void setSchedule(AccountSyncSchedule *schedule);

Q_SIGNALS:
    void automaticSyncEnabledChanged();
    void pastSyncPeriodChanged();
    void directionChanged();
    void scheduleChanged();
    void modifiedChanged();

private:
    friend class AccountSyncOptionsPrivate;
    AccountSyncOptionsPrivate *d;
};

#endif
