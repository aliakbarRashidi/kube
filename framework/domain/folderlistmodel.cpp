/*
    Copyright (c) 2016 Michael Bohlender <michael.bohlender@kdemail.net>
    Copyright (c) 2016 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "folderlistmodel.h"
#include <sink/store.h>
#include <settings/settings.h>

FolderListModel::FolderListModel(QObject *parent) : QIdentityProxyModel()
{
    Sink::Query query;
    query.liveQuery = true;
    query.requestedProperties << "name" << "icon" << "parent";
    query.parentProperty = "parent";
    runQuery(query);
}

FolderListModel::~FolderListModel()
{

}

QHash< int, QByteArray > FolderListModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[Name] = "name";
    roles[Icon] = "icon";
    roles[Id] = "id";
    roles[DomainObject] = "domainObject";

    return roles;
}

QVariant FolderListModel::data(const QModelIndex &idx, int role) const
{
    auto srcIdx = mapToSource(idx);
    switch (role) {
        case Name:
            return srcIdx.sibling(srcIdx.row(), 0).data(Qt::DisplayRole).toString();
        case Icon:
            return srcIdx.sibling(srcIdx.row(), 1).data(Qt::DisplayRole).toString();
        case Id:
            return srcIdx.data(Sink::Store::DomainObjectBaseRole).value<Sink::ApplicationDomain::ApplicationDomainType::Ptr>()->identifier();
        case DomainObject:
            return srcIdx.data(Sink::Store::DomainObjectRole);
    }
    return QIdentityProxyModel::data(idx, role);
}

void FolderListModel::runQuery(const Sink::Query &query)
{
    mModel = Sink::Store::loadModel<Sink::ApplicationDomain::Folder>(query);
    setSourceModel(mModel.data());
}

void FolderListModel::setAccountId(const QVariant &accountId)
{
    const auto account = accountId.toString().toUtf8();
    Sink::Store::fetchAll<Sink::ApplicationDomain::SinkResource>(Sink::Query::PropertyFilter("account", QVariant::fromValue(account)))
        .then<void, QList<Sink::ApplicationDomain::SinkResource::Ptr>>([this, account](const QList<Sink::ApplicationDomain::SinkResource::Ptr> &resources) {
            Sink::Query query;
            query.liveQuery = true;
            query.requestedProperties << "name" << "icon" << "parent";
            query.parentProperty = "parent";
            for (const auto &r : resources) {
                qDebug() << "Found resources for account: " << r->identifier() << account;
                query.resources << r->identifier();
            }
            runQuery(query);
        }).exec();
}

QVariant FolderListModel::accountId() const
{
    return QVariant();
}
