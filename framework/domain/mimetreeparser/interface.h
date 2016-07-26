/*
    Copyright (c) 2016 Sandro Knauß <knauss@kolabsystems.com>

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

#include <functional>
#include <memory>

#include <QDateTime>
#include <QUrl>
#include <QMimeType>

class Part;
class PartPrivate;

class MailMime;
class MailMimePrivate;

class AlternativePart;
class AlternativePartPrivate;

class SinglePart;
class SinglePartPrivate;

class EncryptionPart;
class EncryptionPartPrivate;

class EncapsulatedPart;
class EncapsulatedPartPrivate;

class Content;
class ContentPrivate;

class CertContent;
class CertContentPrivate;

class EncryptionError;

class Key;
class Signature;
class Encryption;

class Parser;
class ParserPrivate;

/* 
 * A MessagePart that is based on a KMime::Content
 */
class MailMime
{
public:
    typedef std::shared_ptr<MailMime> Ptr;
    /**
     *  Various possible values for the "Content-Disposition" header.
     */
    enum Disposition {
        Invalid,           ///< Default, invalid value
        Inline,            ///< inline
        Attachment        ///< attachment
    };

    MailMime();

    // interessting header parts of a KMime::Content
    QMimeType mimetype() const;
    Disposition disposition() const;
    QUrl label() const;
    QByteArray cid() const;
    QByteArray charset() const;
    QString filename() const;

    // Unique identifier to ecactly this KMime::Content
    QByteArray link() const;

    QByteArray content() const;
    //Use default charset
    QString encodedContent() const;

    // overwrite default charset with given charset
    QString encodedContent(QByteArray charset) const;

    bool isFirstTextPart() const;

private:
    std::unique_ptr<MailMimePrivate> d;
};

class Content
{
public:
    typedef std::shared_ptr<Content> Ptr;
    Content(const QByteArray &content, Part *parent);
    virtual ~Content();

    QByteArray content() const;

    QByteArray charset() const;

    //Use default charset
    QString encodedContent() const;

    // overwrite default charset with given charset
    QString encodedContent(QByteArray charset) const;

    virtual QVector<Signature> signatures() const;
    virtual QVector<Encryption> encryptions() const;
    MailMime::Ptr mailMime() const;
    virtual QByteArray type() const;
private:
    std::unique_ptr<ContentPrivate> d;
};

class PlainTextContent : public Content
{
public:
    PlainTextContent(const QByteArray &content, Part *parent);
    QByteArray type() const Q_DECL_OVERRIDE;
};

class HtmlContent : public Content
{
public:
    HtmlContent(const QByteArray &content, Part *parent);
    QByteArray type() const Q_DECL_OVERRIDE;
};

/*
 * importing a cert GpgMe::ImportResult
 * checking a cert (if it is a valid cert)
 */

class CertContent : public Content
{
public:
    typedef std::shared_ptr<CertContent> Ptr;
    CertContent(const QByteArray &content, Part *parent);

    QByteArray type() const Q_DECL_OVERRIDE;
    enum CertType {
        Pgp,
        SMime
    };

    enum CertSubType {
        Public,
        Private
    };

    CertType certType() const;
    CertSubType certSubType() const;
    int keyLength() const;

private:
    std::unique_ptr<CertContentPrivate> d;    
};

class Part
{
public:
    typedef std::shared_ptr<Part> Ptr;
    Part();
    virtual QByteArray type() const;

    virtual QVector<QByteArray> availableContents() const;
    virtual QVector<Content::Ptr> content(const QByteArray& ct) const;
    QVector<Content::Ptr> content() const;

    bool hasSubParts() const;
    QVector<Part::Ptr> subParts() const;
    Part *parent() const;

    virtual QVector<Signature> signatures() const;
    virtual QVector<Encryption> encryptions() const;
    virtual MailMime::Ptr mailMime() const;
private:
    std::unique_ptr<PartPrivate> d;
    friend class ParserPrivate;
    friend class PartPrivate;
};

class AlternativePart : public Part
{
public:
    typedef std::shared_ptr<AlternativePart> Ptr;

    AlternativePart();
    virtual ~AlternativePart();

    QVector<QByteArray> availableContents() const Q_DECL_OVERRIDE;
    QVector<Content::Ptr> content(const QByteArray& ct) const Q_DECL_OVERRIDE;

    QByteArray type() const Q_DECL_OVERRIDE;

private:
    std::unique_ptr<AlternativePartPrivate> d;

    friend class ParserPrivate;
};

class SinglePart : public Part
{
 public:
    typedef std::shared_ptr<SinglePart> Ptr;

    SinglePart();
    virtual ~SinglePart();

    QVector<Content::Ptr> content(const QByteArray& ct) const Q_DECL_OVERRIDE;
    QVector<QByteArray> availableContents() const Q_DECL_OVERRIDE;

    QByteArray type() const Q_DECL_OVERRIDE;
private:
    std::unique_ptr<SinglePartPrivate> d;

    friend class ParserPrivate;   
};


class EncryptionPart : public Part
{
public:
    typedef std::shared_ptr<EncryptionPart> Ptr;
    QByteArray type() const Q_DECL_OVERRIDE;

    EncryptionError error() const;
private:
    std::unique_ptr<EncryptionPartPrivate> d;    
};


/*
 * we want to request complete headers like:
 * from/to...
 */

class EncapsulatedPart : public SinglePart
{
public:
    typedef std::shared_ptr<EncapsulatedPart> Ptr;
    QByteArray type() const Q_DECL_OVERRIDE;

    //template <class T> QByteArray header<T>();
private:
    std::unique_ptr<EncapsulatedPartPrivate> d;    
};

class EncryptionError
{
public:
    int errorId() const;
    QString errorString() const;
};

class Key
{
    QString keyid() const;
    QString name() const;
    QString email() const;
    QString comment() const;
    QVector<QString> emails() const;
    enum KeyTrust {
        Unknown, Undefined, Never, Marginal, Full, Ultimate
    };
    KeyTrust keyTrust() const;

    bool isRevokation() const;
    bool isInvalid() const;
    bool isExpired() const;

    std::vector<Key> subkeys();
    Key parentkey() const;
};

class Signature
{
    Key key() const;
    QDateTime creationDateTime() const;
    QDateTime expirationTime() const;
    bool neverExpires() const;

    //template <> StatusObject<SignatureVerificationResult> verify() const;
};

/*
 * Normally the Keys for encryption are subkeys
 * for clients the parentkeys are "more interessting", because they store the name, email etc.
 * but a client may also wants show to what subkey the mail is really encrypted, an if this subkey isRevoked or something else
 */
class Encryption
{
    std::vector<Key> recipients() const;
};

class Parser
{
public:
    typedef std::shared_ptr<Parser> Ptr;
    Parser(const QByteArray &mimeMessage);
    ~Parser();

    Part::Ptr getPart(QUrl url);

    QVector<Part::Ptr> collect(const Part::Ptr &start, std::function<bool(const Part::Ptr &)> select, std::function<bool(const Content::Ptr &)> filter) const;
    QVector<Part::Ptr> collectContentParts() const;
    QVector<Part::Ptr> collectAttachmentParts() const;
    //template <> QVector<ContentPart::Ptr> collect<ContentPart>() const;

    //template <> static StatusObject<SignatureVerificationResult> verifySignature(const Signature signature) const;
    //template <> static StatusObject<Part> decrypt(const EncryptedPart part) const;

signals:
    void partsChanged();

private:
    std::unique_ptr<ParserPrivate> d;

    friend class InterfaceTest;
};

