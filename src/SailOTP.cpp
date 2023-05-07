/*
 * Copyright (C) 2019-2023 Slava Monich <slava@monich.com>
 * Copyright (C) 2019-2022 Jolla Ltd.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the names of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING
 * IN ANY WAY OUT OF THE USE OR INABILITY TO USE THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "SailOTP.h"

#include "FoilAuthModel.h"
#include "FoilAuthToken.h"

#include "HarbourBase32.h"
#include "HarbourDebug.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCryptographicHash>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

// ==========================================================================
// SailOTP::Private
// ==========================================================================

class SailOTP::Private :
    public FoilAuthTypes {
public:
    class Token {
    public:
        Token() : iFavorite(false) {}

    public:
        FoilAuthToken iToken;
        bool iFavorite;
    };

    static const QString DB_TYPE;
    static const QString DB_NAME;
    static const QString DB_TABLE;

    static const QString COL_TITLE;
    static const QString COL_SECRET;
    static const QString COL_TYPE;
    static const QString COL_COUNTER;
    static const QString COL_FAV;
    static const QString COL_SORT;
    static const QString COL_LEN;
    static const QString COL_DIFF;

    Private(QStringList, FoilAuthModel*);

private:
    static QString databasePath(QString);

    void fetchTokens(FoilAuthModel*);
    void fetchTokens(FoilAuthModel*, QString);
    void fetchTokens(FoilAuthModel*, QSqlDatabase);

public:
    QList<Token> iTokens;
    QStringList iImportedTokens;
};

#define TABLE_NAME "OTPStorage"
#define DATABASE_NAME "harbour-sailotp"
const QString SailOTP::Private::DB_TYPE("QSQLITE");
const QString SailOTP::Private::DB_NAME(DATABASE_NAME);
const QString SailOTP::Private::DB_TABLE(TABLE_NAME);

#define SORT_COL "sort"
const QString SailOTP::Private::COL_TITLE("title");     // TEXT
const QString SailOTP::Private::COL_SECRET("secret");   // TEXT
const QString SailOTP::Private::COL_TYPE("type");       // TEXT
const QString SailOTP::Private::COL_COUNTER("counter"); // INTEGER
const QString SailOTP::Private::COL_FAV("fav");         // INTEGER
const QString SailOTP::Private::COL_SORT(SORT_COL);     // INTEGER
const QString SailOTP::Private::COL_LEN("len");         // INTEGER
const QString SailOTP::Private::COL_DIFF("diff");       // INTEGER

SailOTP::Private::Private(
    QStringList aImportedTokens,
    FoilAuthModel* aDestModel) :
    iImportedTokens(aImportedTokens)
{
    // SqlDatabase object needs to go out of scope before we can
    // call QSqlDatabase::removeDatabase
    fetchTokens(aDestModel);
    QSqlDatabase::removeDatabase(DB_NAME);
}

QString
SailOTP::Private::databasePath(
    QString aOrg)
{
    // This is how LocalStorage plugin generates database file name
    const QDir dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QDir::separator() + aOrg + "/harbour-sailotp/QML/OfflineStorage/Databases");

    return dir.path() + QDir::separator() +
        QLatin1String(QCryptographicHash::hash(DATABASE_NAME, QCryptographicHash::Md5).toHex()) +
        ".sqlite";
}

void
SailOTP::Private::fetchTokens(
    FoilAuthModel* aDestModel)
{
    fetchTokens(aDestModel, databasePath("org.seiichiro0185"));
    fetchTokens(aDestModel, databasePath("harbour-sailotp"));
}

void
SailOTP::Private::fetchTokens(
    FoilAuthModel* aDestModel,
    QString aDbPath)
{
    if (QFile(aDbPath).exists()) {
        QSqlDatabase db(QSqlDatabase::database(DB_NAME));
        if (!db.isValid()) {
            HDEBUG("adding database" << DB_NAME);
            db = QSqlDatabase::addDatabase(DB_TYPE, DB_NAME);
        }
        db.setDatabaseName(aDbPath);
        if (db.open()) {
            HDEBUG("Opened " << qPrintable(aDbPath));
            fetchTokens(aDestModel, db);
            db.close();
        }
    }
}

void
SailOTP::Private::fetchTokens(
    FoilAuthModel* aDestModel,
    QSqlDatabase db)
{
    QSqlQuery query(db);
    if (query.exec("SELECT * FROM " TABLE_NAME " ORDER BY " SORT_COL) ||
        query.exec("SELECT * FROM " TABLE_NAME)) {
        while (query.next()) {
            const QString type(query.value(COL_TYPE).toString());
            const bool isTOTP = !type.compare(FoilAuthToken::TYPE_TOTP, Qt::CaseInsensitive);
            const bool isHOTP = !type.compare(FoilAuthToken::TYPE_HOTP, Qt::CaseInsensitive);
            const bool isSteam = !type.compare(QStringLiteral("TOTP_STEAM"), Qt::CaseInsensitive);
            if (isTOTP || isHOTP || isSteam) {
                const QString base32(query.value(COL_SECRET).toString());
                const QByteArray secret(HarbourBase32::fromBase32(base32));
                if (!secret.isEmpty()) {
                    const QString title(query.value(COL_TITLE).toString());
                    const QString secretHash(QString(QCryptographicHash::hash(secret,
                        QCryptographicHash::Sha1).toHex()).toLower());
                    if (iImportedTokens.contains(secretHash) ||
                        (aDestModel && aDestModel->containsSecret(secret))) {
                        HDEBUG("SailOTP token" << title << "is already imported");
                    } else {
                        bool lenOk, diffOk, counterOk;
                        const int lenVal = query.value(COL_LEN).toInt(&lenOk);
                        const int diffVal = query.value(COL_DIFF).toInt(&diffOk);
                        const int counterVal = query.value(COL_COUNTER).toInt(&counterOk);

                        Token token;
                        token.iFavorite = (query.value(COL_FAV).toInt() != 0);
                        token.iToken = FoilAuthToken(isHOTP ? FoilAuthTypes::AuthTypeHOTP :
                            isSteam ? FoilAuthTypes::AuthTypeSteam : FoilAuthTypes::AuthTypeTOTP,
                            secret, title, isSteam ? QString("Stream") : QString(),
                            isSteam ? DEFAULT_STEAM_DIGITS :
                            (lenVal && lenVal > 0) ? lenVal : DEFAULT_DIGITS,
                            (counterOk && counterVal > 0) ? counterVal : DEFAULT_COUNTER,
                            diffOk ? diffVal : DEFAULT_TIMESHIFT, DEFAULT_ALGORITHM);

                        HDEBUG(token.iToken << token.iFavorite);
                        iTokens.append(token);
                        iImportedTokens.append(secretHash);
                    }
                }
            }
        }
    } else {
        HWARN(query.lastError());
    }
}

// ==========================================================================
// SailOTP
// ==========================================================================

SailOTP::SailOTP(QObject* aParent) :
    QObject(aParent),
    iPrivate(Q_NULLPTR)
{
}

SailOTP::~SailOTP()
{
    delete iPrivate;
}

QStringList
SailOTP::importedTokens() const
{
    return iImportedTokens;
}

void
SailOTP::setImportedTokens(
    QStringList aList)
{
    if (iImportedTokens != aList) {
        iImportedTokens = aList;
        Q_EMIT importedTokensChanged();
    }
}

// Callback for qmlRegisterSingletonType<SailOTP>
QObject*
SailOTP::createSingleton(
    QQmlEngine*,
    QJSEngine*)
{
    return new SailOTP;
}

int
SailOTP::fetchNewTokens(
    QObject* aDestModel)
{
    delete iPrivate;
    iPrivate = new Private(iImportedTokens, qobject_cast<FoilAuthModel*>(aDestModel));
    if (!iPrivate->iTokens.isEmpty()) {
        const int count = iPrivate->iTokens.count();
        HDEBUG("Fetched" << count << "new token(s)");
        setImportedTokens(iPrivate->iImportedTokens);
        return count;
    }
    delete iPrivate;
    iPrivate = Q_NULLPTR;
    return 0;
}

void
SailOTP::importTokens(
    QObject* aDestModel)
{
    if (iPrivate) {
        FoilAuthModel* model = qobject_cast<FoilAuthModel*>(aDestModel);
        if (model) {
            const int n = iPrivate->iTokens.count();
            for (int i = 0; i < n; i++) {
                const Private::Token& token(iPrivate->iTokens.at(i));
                if (model->contains(token.iToken)) {
                    HDEBUG(token.iToken.label() << "is already there");
                } else {
                    model->addToken(token.iToken, token.iFavorite);
                }
            }
        }
        delete iPrivate;
        iPrivate = Q_NULLPTR;
    }
}

void
SailOTP::dropTokens()
{
    delete iPrivate;
    iPrivate = Q_NULLPTR;
}
