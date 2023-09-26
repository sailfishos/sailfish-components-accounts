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

    void setModified(bool modified);
    void setDays(AccountSyncSchedule::Days d);

    static AccountSyncSchedule *fromButeoSchedule(const Buteo::SyncProfile &profile, QObject *parent);
    static Buteo::SyncSchedule toButeoSchedule(AccountSyncSchedule *source, Buteo::SyncProfile *profile);

    static unsigned int intervalToMinutes(AccountSyncSchedule::Interval interval);
    static unsigned int longIntervalToMinutes(AccountSyncSchedule::LongInterval interval);
    static AccountSyncSchedule::Interval intervalFromMinutes(unsigned int minutes);

    QTime m_dailySyncTime;
    QTime m_peakStartTime;
    QTime m_peakEndTime;
    AccountSyncSchedule *q;
    AccountSyncSchedule::Interval m_interval;
    AccountSyncSchedule::Interval m_peakInterval;
    AccountSyncSchedule::LongInterval m_longInterval;
    AccountSyncSchedule::Days m_days;
    AccountSyncSchedule::Days m_peakDays;
    bool m_modified;
    bool m_peakEnabled;
    bool m_enabled;
    bool m_externalPeakEnabled;
};

class AccountSyncOptionsPrivate
{
public:
    AccountSyncOptionsPrivate(AccountSyncOptions *parent);

    void setModified(bool modified);

    static AccountSyncOptions *fromButeoProfile(const Buteo::SyncProfile &source, QObject *parent);

    static void writeToButeoProfile(AccountSyncOptions *options, Buteo::SyncProfile *profile);

    static unsigned int pastSyncPeriodToDays(AccountSyncOptions::PastSyncPeriod period);
    static AccountSyncOptions::PastSyncPeriod pastSyncPeriodFromDays(unsigned int dayCount);

    AccountSyncOptions *q;
    AccountSyncSchedule *m_schedule;
    AccountSyncOptions::PastSyncPeriod m_pastSyncPeriod;
    AccountSyncOptions::Direction m_direction;
    int m_allowedConnectionTypes;
    bool m_modified;
    bool m_autoSync;
    bool m_syncExternallyEnabled;
};

#endif
