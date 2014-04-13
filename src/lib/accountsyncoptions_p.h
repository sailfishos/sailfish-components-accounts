/*
 * Copyright (C) 2014 Jolla Ltd.
 * Contact: Bea Lam <bea.lam@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef ACCOUNTSYNCOPTIONS_P_H
#define ACCOUNTSYNCOPTIONS_P_H

#include "accountsyncoptions.h"

// buteo-syncfw
#include <SyncSchedule.h>
#include <SyncProfile.h>

class AccountSyncSchedulePrivate
{
public:
    AccountSyncSchedulePrivate(AccountSyncSchedule *schedule);
    ~AccountSyncSchedulePrivate();

    void setDays(AccountSyncSchedule::Days d);

    static AccountSyncSchedule *fromButeoSchedule(const Buteo::SyncSchedule &source, QObject *parent);
    static Buteo::SyncSchedule toButeoSchedule(AccountSyncSchedule *source);

    static QSet<int> daysToQtDaySet(AccountSyncSchedule::Days days);
    static AccountSyncSchedule::Days daysFromQtDaySet(const QSet<int> &qtDays);
    static unsigned int intervalToMinutes(AccountSyncSchedule::Interval interval);
    static AccountSyncSchedule::Interval intervalFromMinutes(unsigned int minutes);

    QTime m_dailySyncTime;
    QTime m_peakStartTime;
    QTime m_peakEndTime;
    AccountSyncSchedule *q;
    AccountSyncSchedule::Interval m_interval;
    AccountSyncSchedule::Interval m_peakInterval;
    AccountSyncSchedule::Days m_days;
    AccountSyncSchedule::Days m_peakDays;
    bool m_modified;
    bool m_peakEnabled;
    bool m_enabled;
};

class AccountSyncOptionsPrivate
{
public:
    AccountSyncOptionsPrivate();

    static AccountSyncOptions *fromButeoProfile(const Buteo::SyncProfile &source, QObject *parent);

    static void writeToButeoProfile(AccountSyncOptions *options, Buteo::SyncProfile *profile);

    static unsigned int pastSyncPeriodToDays(AccountSyncOptions::PastSyncPeriod period);
    static AccountSyncOptions::PastSyncPeriod pastSyncPeriodFromDays(unsigned int dayCount);

    AccountSyncSchedule *m_schedule;
    AccountSyncOptions::PastSyncPeriod m_pastSyncPeriod;
    AccountSyncOptions::Direction m_direction;
    bool m_modified;
    bool m_autoSync;
};

#endif
