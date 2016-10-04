/*
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
#include "accountsettings.h"

#include <sink/store.h>
#include <QDebug>
#include <QDir>
#include <QUrl>

AccountSettings::AccountSettings(QObject *parent)
    : QObject(parent)
{
}


void AccountSettings::setAccountIdentifier(const QByteArray &id)
{
    if (id.isEmpty()) {
        return;
    }
    mAccountIdentifier = id;

    //Clear
    mIcon = QString();
    mName = QString();
    mImapServer = QString();
    mImapUsername = QString();
    mImapPassword = QString();
    mSmtpServer = QString();
    mSmtpUsername = QString();
    mSmtpPassword = QString();
    emit changed();
    emit imapResourceChanged();
    emit smtpResourceChanged();

    load();

}

QByteArray AccountSettings::accountIdentifier() const
{
    return mAccountIdentifier;
}

void AccountSettings::setPath(const QUrl &path)
{
    auto normalizedPath = path.path();
    if (mPath != normalizedPath) {
        mPath = normalizedPath;
        emit pathChanged();
    }
}

QUrl AccountSettings::path() const
{
    return QUrl(mPath);
}

QValidator *AccountSettings::pathValidator() const
{
    class PathValidator : public QValidator {
        State validate(QString &input, int &pos) const {
            Q_UNUSED(pos);
            if (!input.isEmpty() && QDir(input).exists()) {
                return Acceptable;
            } else {
                return Intermediate;
            }
        }
    };
    static PathValidator *pathValidator = new PathValidator;
    return pathValidator;
}

QValidator *AccountSettings::imapServerValidator() const
{
    class ImapServerValidator : public QValidator {
        State validate(QString &input, int &pos) const {
            Q_UNUSED(pos);
            // imaps://mainserver.example.net:475
            const QUrl url(input);
            static QSet<QString> validProtocols = QSet<QString>() << "imap" << "imaps";
            if (url.isValid() && validProtocols.contains(url.scheme().toLower())) {
                return Acceptable;
            } else {
                return Intermediate;
            }
        }
    };
    static ImapServerValidator *validator = new ImapServerValidator;
    return validator;
}

QValidator *AccountSettings::smtpServerValidator() const
{
    class SmtpServerValidator : public QValidator {
        State validate(QString &input, int &pos) const {
            Q_UNUSED(pos);
            // smtps://mainserver.example.net:475
            const QUrl url(input);
            static QSet<QString> validProtocols = QSet<QString>() << "smtp" << "smtps";
            if (url.isValid() && validProtocols.contains(url.scheme().toLower())) {
                return Acceptable;
            } else {
                return Intermediate;
            }
        }
    };
    static SmtpServerValidator *validator = new SmtpServerValidator;
    return validator;
}

void AccountSettings::saveAccount()
{
    qDebug() << "Saving account " << mAccountIdentifier << mMailtransportIdentifier;
    Q_ASSERT(!mAccountIdentifier.isEmpty());
    Sink::ApplicationDomain::SinkAccount account(mAccountIdentifier);
    account.setProperty("type", "imap");
    account.setProperty("name", mName);
    account.setProperty("icon", mIcon);
    Q_ASSERT(!account.identifier().isEmpty());
    Sink::Store::modify(account)
        .onError([](const KAsync::Error &error) {
            qWarning() << "Error while creating account: " << error.errorMessage;;
        })
        .exec();
}

void AccountSettings::loadAccount()
{
    Q_ASSERT(!mAccountIdentifier.isEmpty());
    Sink::Store::fetchOne<Sink::ApplicationDomain::SinkAccount>(Sink::Query::IdentityFilter(mAccountIdentifier))
        .syncThen<void, Sink::ApplicationDomain::SinkAccount>([this](const Sink::ApplicationDomain::SinkAccount &account) {
            mIcon = account.getProperty("icon").toString();
            mName = account.getProperty("name").toString();
            emit changed();
        }).exec();
}

void AccountSettings::loadImapResource()
{
    Sink::Store::fetchOne<Sink::ApplicationDomain::SinkResource>(Sink::Query().filter(Sink::ApplicationDomain::SinkAccount(mAccountIdentifier)).containsFilter<Sink::ApplicationDomain::SinkResource::Capabilities>(Sink::ApplicationDomain::ResourceCapabilities::Mail::storage))
        .syncThen<void, Sink::ApplicationDomain::SinkResource>([this](const Sink::ApplicationDomain::SinkResource &resource) {
            mImapIdentifier = resource.identifier();
            mImapServer = resource.getProperty("server").toString();
            mImapUsername = resource.getProperty("username").toString();
            mImapPassword = resource.getProperty("password").toString();
            emit imapResourceChanged();
        }).onError([](const KAsync::Error &error) {
            qWarning() << "Failed to find the imap resource: " << error.errorMessage;
        }).exec();
}

void AccountSettings::loadMaildirResource()
{
    Sink::Store::fetchOne<Sink::ApplicationDomain::SinkResource>(Sink::Query().filter(Sink::ApplicationDomain::SinkAccount(mAccountIdentifier)).containsFilter<Sink::ApplicationDomain::SinkResource::Capabilities>(Sink::ApplicationDomain::ResourceCapabilities::Mail::storage))
        .syncThen<void, Sink::ApplicationDomain::SinkResource>([this](const Sink::ApplicationDomain::SinkResource &resource) {
            mMaildirIdentifier = resource.identifier();
            auto path = resource.getProperty("path").toString();
            if (mPath != path) {
                mPath = path;
                emit pathChanged();
            }
        }).onError([](const KAsync::Error &error) {
            qWarning() << "Failed to find the maildir resource: " << error.errorMessage;
        }).exec();
}

void AccountSettings::loadMailtransportResource()
{
    Sink::Store::fetchOne<Sink::ApplicationDomain::SinkResource>(Sink::Query().filter(Sink::ApplicationDomain::SinkAccount(mAccountIdentifier)).containsFilter<Sink::ApplicationDomain::SinkResource::Capabilities>(Sink::ApplicationDomain::ResourceCapabilities::Mail::transport))
        .syncThen<void, Sink::ApplicationDomain::SinkResource>([this](const Sink::ApplicationDomain::SinkResource &resource) {
            mMailtransportIdentifier = resource.identifier();
            mSmtpServer = resource.getProperty("server").toString();
            mSmtpUsername = resource.getProperty("username").toString();
            mSmtpPassword = resource.getProperty("password").toString();
            emit smtpResourceChanged();
        }).onError([](const KAsync::Error &error) {
            qWarning() << "Failed to find the smtp resource: " << error.errorMessage;
        }).exec();
}

void AccountSettings::loadIdentity()
{
    //FIXME this assumes that we only ever have one identity per account
    Sink::Store::fetchOne<Sink::ApplicationDomain::Identity>(Sink::Query().filter(Sink::ApplicationDomain::SinkAccount(mAccountIdentifier)))
        .syncThen<void, Sink::ApplicationDomain::Identity>([this](const Sink::ApplicationDomain::Identity &identity) {
            mIdentityIdentifier = identity.identifier();
            mUsername = identity.getProperty("username").toString();
            mEmailAddress = identity.getProperty("address").toString();
            emit identityChanged();
        }).onError([](const KAsync::Error &error) {
            qWarning() << "Failed to find the identity resource: " << error.errorMessage;
        }).exec();
}



template<typename ResourceType>
static QByteArray saveResource(const QByteArray &accountIdentifier, const QByteArray &identifier, const std::map<QByteArray, QVariant> &properties)
{
    if (!identifier.isEmpty()) {
        Sink::ApplicationDomain::SinkResource resource(identifier);
        for (const auto &pair : properties) {
            resource.setProperty(pair.first, pair.second);
        }
        Sink::Store::modify(resource)
            .onError([](const KAsync::Error &error) {
                qWarning() << "Error while modifying resource: " << error.errorMessage;
            })
            .exec();
    } else {
        auto resource = ResourceType::create(accountIdentifier);
        auto newIdentifier = resource.identifier();
        for (const auto &pair : properties) {
            resource.setProperty(pair.first, pair.second);
        }
        Sink::Store::create(resource)
            .onError([](const KAsync::Error &error) {
                qWarning() << "Error while creating resource: " << error.errorMessage;
            })
            .exec();
        return newIdentifier;
    }
    return identifier;
}

void AccountSettings::saveImapResource()
{
    mImapIdentifier = saveResource<Sink::ApplicationDomain::ImapResource>(mAccountIdentifier, mImapIdentifier, {
            {"server", mImapServer},
            {"username", mImapUsername},
            {"password", mImapPassword},
        });
}

void AccountSettings::saveMaildirResource()
{
    mMaildirIdentifier = saveResource<Sink::ApplicationDomain::MaildirResource>(mAccountIdentifier, mMaildirIdentifier, {
            {"path", mPath},
        });
}

void AccountSettings::saveMailtransportResource()
{
    mMailtransportIdentifier = saveResource<Sink::ApplicationDomain::MailtransportResource>(mAccountIdentifier, mMailtransportIdentifier, {
            {"server", mSmtpServer},
            {"username", mSmtpUsername},
            {"password", mSmtpPassword},
        });
}

void AccountSettings::saveIdentity()
{
    if (!mIdentityIdentifier.isEmpty()) {
        Sink::ApplicationDomain::Identity identity(mMailtransportIdentifier);
        identity.setProperty("username", mUsername);
        identity.setProperty("address", mEmailAddress);
        Sink::Store::modify(identity)
        .onError([](const KAsync::Error &error) {
            qWarning() << "Error while modifying identity: " << error.errorMessage;
        })
        .exec();
    } else {
        auto identity = Sink::ApplicationDomain::ApplicationDomainType::createEntity<Sink::ApplicationDomain::Identity>();
        mIdentityIdentifier = identity.identifier();
        identity.setProperty("account", mAccountIdentifier);
        identity.setProperty("username", mUsername);
        identity.setProperty("address", mEmailAddress);
        Sink::Store::create(identity)
        .onError([](const KAsync::Error &error) {
            qWarning() << "Error while creating identity: " << error.errorMessage;
        })
        .exec();
    }
}

void AccountSettings::removeResource(const QByteArray &identifier)
{
    if (identifier.isEmpty()) {
        qWarning() << "We're missing an identifier";
    } else {
        Sink::ApplicationDomain::SinkResource resource("", identifier, 0, QSharedPointer<Sink::ApplicationDomain::MemoryBufferAdaptor>::create());
        Sink::Store::remove(resource)
        .onError([](const KAsync::Error &error) {
            qWarning() << "Error while removing resource: " << error.errorMessage;
        })
        .exec();
    }
}

void AccountSettings::removeAccount()
{
    if (mAccountIdentifier.isEmpty()) {
        qWarning() << "We're missing an identifier";
    } else {
        Sink::ApplicationDomain::SinkAccount account("", mAccountIdentifier, 0, QSharedPointer<Sink::ApplicationDomain::MemoryBufferAdaptor>::create());
        Sink::Store::remove(account)
        .onError([](const KAsync::Error &error) {
            qWarning() << "Error while removing account: " << error.errorMessage;
        })
        .exec();
    }
}

void AccountSettings::removeIdentity()
{
    if (mIdentityIdentifier.isEmpty()) {
        qWarning() << "We're missing an identifier";
    } else {
        Sink::ApplicationDomain::Identity identity("", mIdentityIdentifier, 0, QSharedPointer<Sink::ApplicationDomain::MemoryBufferAdaptor>::create());
        Sink::Store::remove(identity)
        .onError([](const KAsync::Error &error) {
            qWarning() << "Error while removing identity: " << error.errorMessage;
        })
        .exec();
    }
}
