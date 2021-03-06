/*
    Copyright (c) 2018 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "teststore.h"

#include <sink/store.h>
#include <sink/resourcecontrol.h>
#include <sink/secretstore.h>

#include <kmime/kmime_message.h>

#include <KCalCore/Event>
#include <KCalCore/ICalFormat>

#include <QDebug>
#include <QUuid>
#include <QVariant>

#include "framework/src/domain/mime/mailtemplates.h"

using namespace Kube;

static void iterateOverObjects(const QVariantList &list, std::function<void(const QVariantMap &)> callback)
{
    for (const auto &entry : list) {
        auto object = entry.toMap();
        callback(object);
    }
}

static QStringList toStringList(const QVariantList &list)
{
    QStringList s;
    for (const auto &e : list) {
        s << e.toString();
    }
    return s;
}

static QByteArrayList toByteArrayList(const QVariantList &list)
{
    QByteArrayList s;
    for (const auto &e : list) {
        s << e.toByteArray();
    }
    return s;
}

static void createMail(const QVariantMap &object, const QByteArray &folder = {})
{
    using namespace Sink::ApplicationDomain;

    auto toAddresses = toStringList(object["to"].toList());
    auto ccAddresses = toStringList(object["cc"].toList());
    auto bccAddresses = toStringList(object["bcc"].toList());

    QList<Attachment> attachments = {};
    if (object.contains("attachments")) {
        auto attachmentSpecs = object["attachments"].toList();
        for (int i = 0; i < attachmentSpecs.size(); ++i) {
            auto const &spec = attachmentSpecs.at(i).toMap();
            attachments << Attachment{spec["name"].toString(),
                spec["name"].toString(),
                spec["mimeType"].toByteArray(),
                false,
                spec["data"].toByteArray()};
        }
    }

    KMime::Types::Mailbox mb;
    mb.fromUnicodeString("identity@example.org");
    auto msg = MailTemplates::createMessage({},
            toAddresses,
            ccAddresses,
            bccAddresses,
            mb,
            object["subject"].toString(),
            object["body"].toString(),
            object["bodyIsHtml"].toBool(),
            attachments,
            {},
            {});
    if (object.contains("messageId")) {
        msg->messageID(true)->from7BitString(object["messageId"].toByteArray());
    }
    if (object.contains("inReplyTo")) {
        msg->inReplyTo(true)->from7BitString(object["inReplyTo"].toByteArray());
    }
    if (object.contains("date")) {
        msg->date(true)->setDateTime(QDateTime::fromString(object["date"].toString(), Qt::ISODate));
    }

    msg->assemble();

    auto mail = ApplicationDomainType::createEntity<Mail>(object["resource"].toByteArray());
    mail.setMimeMessage(msg->encodedContent(true));
    mail.setUnread(object["unread"].toBool());
    if (!folder.isEmpty()) {
        mail.setFolder(folder);
    }
    Sink::Store::create(mail).exec().waitForFinished();
}

static void createFolder(const QVariantMap &object)
{
    using namespace Sink::ApplicationDomain;
    auto folder = ApplicationDomainType::createEntity<Folder>(object["resource"].toByteArray());
    folder.setName(object["name"].toString());
    folder.setSpecialPurpose(toByteArrayList(object["specialpurpose"].toList()));
    Sink::Store::create(folder).exec().waitForFinished();

    iterateOverObjects(object.value("mails").toList(), [=](const QVariantMap &object) {
        createMail(object, folder.identifier());
    });
}

static void createEvent(const QVariantMap &object, const QByteArray &calendarId = {})
{
    using Sink::ApplicationDomain::ApplicationDomainType;
    using Sink::ApplicationDomain::Event;

    auto sinkEvent = ApplicationDomainType::createEntity<Event>(object["resource"].toByteArray());

    auto calcoreEvent = QSharedPointer<KCalCore::Event>::create();

    QString uid;
    if (object.contains("uid")) {
        uid = object["uid"].toString();
    } else {
        uid = QUuid::createUuid().toString();
    }
    calcoreEvent->setUid(uid);

    auto summary = object["summary"].toString();
    calcoreEvent->setSummary(summary);

    if (object.contains("description")) {
        auto description = object["description"].toString();
        calcoreEvent->setDescription(description);
    }

    auto startTime = object["starts"].toDateTime();
    auto endTime = object["ends"].toDateTime();

    calcoreEvent->setDtStart(startTime);
    calcoreEvent->setDtEnd(endTime);

    if (object.contains("allDay")) {
        calcoreEvent->setAllDay(object["allDay"].toBool());
    }

    auto ical = KCalCore::ICalFormat().toICalString(calcoreEvent);
    sinkEvent.setIcal(ical.toUtf8());

    sinkEvent.setCalendar(calendarId);

    Sink::Store::create(sinkEvent).exec().waitForFinished();
}

static void createCalendar(const QVariantMap &object)
{
    using Sink::ApplicationDomain::Calendar;
    using Sink::ApplicationDomain::ApplicationDomainType;

    auto calendar = ApplicationDomainType::createEntity<Calendar>(object["resource"].toByteArray());
    calendar.setName(object["name"].toString());
    Sink::Store::create(calendar).exec().waitForFinished();

    auto calendarId = calendar.identifier();
    iterateOverObjects(object.value("events").toList(),
        [calendarId](const QVariantMap &object) { createEvent(object, calendarId); });
}

void TestStore::setup(const QVariantMap &map)
{
    using namespace Sink::ApplicationDomain;

    //Cleanup any old data
    const auto accounts = Sink::Store::read<SinkAccount>({});
    for (const auto &account : accounts) {
        Sink::Store::remove(account).exec().waitForFinished();
    }

    iterateOverObjects(map.value("accounts").toList(), [&] (const QVariantMap &object) {
        auto account = ApplicationDomainType::createEntity<SinkAccount>("", object["id"].toByteArray());
        account.setName(object["name"].toString());
        Sink::Store::create(account).exec().waitForFinished();
    });
    QByteArrayList resources;
    iterateOverObjects(map.value("resources").toList(), [&] (const QVariantMap &object) {
        resources << object["id"].toByteArray();
        auto resource = [&] {
            using namespace Sink::ApplicationDomain;
            auto resource = ApplicationDomainType::createEntity<SinkResource>("", object["id"].toByteArray());
            if (object["type"] == "dummy") {
                resource.setResourceType("sink.dummy");
            } else if (object["type"] == "mailtransport") {
                resource.setResourceType("sink.mailtransport");
                resource.setProperty("testmode", true);
            } else if (object["type"] == "caldav") {
                resource.setResourceType("sink.caldav");
                resource.setProperty("testmode", true);
            } else {
                Q_ASSERT(false);
            }
            return resource;
        }();
        resource.setAccount(object["account"].toByteArray());
        Sink::Store::create(resource).exec().waitForFinished();
        Sink::SecretStore::instance().insert(resource.identifier(), "secret");
    });

    iterateOverObjects(map.value("identities").toList(), [] (const QVariantMap &object) {
        auto identity = Sink::ApplicationDomain::Identity{};
        identity.setAccount(object["account"].toByteArray());
        identity.setAddress(object["address"].toString());
        identity.setName(object["name"].toString());
        Sink::Store::create(identity).exec().waitForFinished();
    });

    iterateOverObjects(map.value("folders").toList(), createFolder);
    iterateOverObjects(map.value("mails").toList(), [] (const QVariantMap &map) {
        createMail(map);
    });

    iterateOverObjects(map.value("calendars").toList(), createCalendar);

    Sink::ResourceControl::flushMessageQueue(resources).exec().waitForFinished();
}

QVariant TestStore::load(const QByteArray &type, const QVariantMap &filter)
{
    using namespace Sink::ApplicationDomain;
    const auto list = loadList(type, filter);
    if (!list.isEmpty()) {
        if (list.size() > 1) {
            qWarning() << "While loading" << type << "with filter" << filter
                       << "; got multiple elements, but returning the first one.";
        }
        return list.first();
    }
    return {};
}
template <typename T>
QVariantList toVariantList(const QList<T> &list)
{
    QVariantList result;
    std::transform(list.constBegin(), list.constEnd(), std::back_inserter(result), [] (const T &m) {
        return QVariant::fromValue(T::Ptr::create(m));
    });
    Q_ASSERT(list.size() == result.size());
    return result;
}

QVariantList TestStore::loadList(const QByteArray &type, const QVariantMap &filter)
{
    using namespace Sink::ApplicationDomain;
    Sink::Query query;
    if (filter.contains("resource")) {
        query.resourceFilter(filter.value("resource").toByteArray());
    }

    for (QVariantMap::const_iterator it = filter.begin(); it != filter.end(); ++it) {
        if (it.key() == "messageId") {
            query.filter<Mail::MessageId>(it.value());
        } else if (it.key() == "draft") {
            query.filter<Mail::Draft>(it.value());
        } else if (it.key() == "subject") {
            query.filter<Mail::Subject>(it.value());
        }
    }

    if (type == "mail") {
        return toVariantList(Sink::Store::read<Mail>(query));
    }
    if (type == "folder") {
        return toVariantList(Sink::Store::read<Folder>(query));
    }
    if (type == "resource") {
        return toVariantList(Sink::Store::read<SinkResource>(query));
    }
    if (type == "account") {
        return toVariantList(Sink::Store::read<SinkAccount>(query));
    }

    Q_ASSERT(false);
    return {};
}

QVariantMap TestStore::read(const QVariant &object)
{
    using namespace Sink::ApplicationDomain;
    QVariantMap map;
    if (auto mail = object.value<Mail::Ptr>()) {
        map.insert("uid", mail->identifier());
        map.insert("subject", mail->getSubject());
        map.insert("draft", mail->getDraft());
        return map;
    }
    Q_ASSERT(false);
    return {};
}
