/****************************************************************************************
** Copyright (c) 2013 - 2023 Jolla Ltd.
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

#ifndef SAILFISH_ACCOUNTS__SERVICEMODEL_H
#define SAILFISH_ACCOUNTS__SERVICEMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

class Q_DECL_EXPORT ServiceModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    class ServiceModelPrivate;
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString serviceTypeFilter READ serviceTypeFilter WRITE setServiceTypeFilter NOTIFY serviceTypeFilterChanged)

public:
    enum Roles {
        ServiceNameRole = Qt::UserRole + 1,
        ServiceDisplayNameRole,
        ServiceIconRole,
        ServiceTypeRole,
        ServiceTagsRole,
        ServiceProviderNameRole
    };

    ServiceModel(QObject* parent = 0);
    ~ServiceModel();

    QString serviceTypeFilter() const;
    void setServiceTypeFilter(const QString &serviceFilter);

    int rowCount( const QModelIndex & index = QModelIndex() ) const;
    QVariant data( const QModelIndex &index, int role ) const;

    void classBegin();
    void componentComplete();

signals:
    void serviceTypeFilterChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    ServiceModelPrivate* d_ptr;
    Q_DISABLE_COPY(ServiceModel)
    Q_DECLARE_PRIVATE(ServiceModel);
};

#endif
