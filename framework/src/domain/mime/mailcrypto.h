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

#pragma once

#include "framework/src/errors.h"

#include <KMime/Message>

#include <QByteArray>
#include <QVariant>

#include <functional>
#include <memory>
#include "crypto.h"

namespace MailCrypto {

Expected<Crypto::Error, std::unique_ptr<KMime::Content>>
processCrypto(std::unique_ptr<KMime::Content> content, const std::vector<Crypto::Key> &signingKeys,
    const std::vector<Crypto::Key> &encryptionKeys, const Crypto::Key &attachedKey);

}; // namespace MailCrypto

