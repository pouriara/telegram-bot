#include "sup.h"
#include "database.h"
#include "messageprocessor.h"
#include "namedatabase.h"
#include "subscribe.h"
#include <QtCore>
#include <QSqlQuery>

const int Sup::MaxSupPerGroup = 10;
const int Sup::DistanceBetweenSup = 30;

Sup::Sup(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Subscribe *sub, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc), subscribe(sub)
{
    loadData();
    freshSup();
}

void Sup::freshSup()
{
    foreach (qint64 gid, nameDatabase->groups().keys())
        lastSup[gid] = DistanceBetweenSup;
}

void Sup::loadData()
{
    data.clear();

    QSqlQuery query;

    query.prepare("SELECT id, gid, text FROM tf_sup");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 id = query.value(0).toLongLong();
        qint64 gid = query.value(1).toLongLong();
        QString text = query.value(2).toString();

        data[gid].append(SupEntry(id, text));
    }
}

void Sup::input(const QString &gid, const QString &uid, const QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum))
        ++lastSup[gidnum];

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!sup"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;

        if (args.size() > 2 && (args[1].toLower().startsWith("del") || args[1].toLower().startsWith("rem")))
        {
            int start, end;
            int idx = args[2].indexOf('-');
            bool ok1, ok2;

            if (idx == -1)
            {
                start = end = args[2].toInt(&ok1);
                ok2 = true;
            }
            else
            {
                start = args[2].mid(0, idx).toInt(&ok1);
                end = args[2].mid(idx + 1).toInt(&ok2);
            }

            if (ok1 && ok2 && start > 0 && end > 0 && end >= start && end <= data[gidnum].size())
            {
                qDebug() << "Deleting: " << start << ' ' << end;
                for (int i = start; i <= end; ++i)
                    delEntry(gidnum, start);
                message = "Deleted 'sup entry(ies)!";
            }
        }
        else if (args.size() > 2 && args[1].toLower().startsWith("add"))
        {
            int idx = str.indexOf("add", 0, Qt::CaseInsensitive);
            int seenSpace = 0;
            while (idx < str.length())
            {
                if (str[idx] == ' ')
                    seenSpace = 1;
                else if (seenSpace == 1)
                {
                    seenSpace = 2;
                    break;
                }

                ++idx;
            }

            if (seenSpace == 2)
            {
                if (addEntry(gidnum, str.mid(idx)))
                {
                    message = "Added 'sup entry!";
                    qDebug() << str.mid(idx).length() << " added";
                    freshSup();

                    QString subscribeMessage = QString("New 'Sup Entry: %1").arg(str.mid(idx));
                    subscribe->postToSubscribed(gidnum, subscribeMessage);
                }
                else
                    message = "Couldn't add new 'sup entry; Maybe delete some entries first.";
            }
        }
        else
        {
            inpm = inpm || (args.size() > 1 && args[1].toLower().startsWith("pm"));

            if (data[gidnum].isEmpty())
                message = "Nothing important!";
            else
            {
                if (!inpm && lastSup[gidnum] < DistanceBetweenSup)
                    message = "Public 'sup have been already shown recently. Please scroll up or use \"!sup pm\".";
                else
                {
                    for (int i = 0; i < data[gidnum].size(); ++i)
                    {
                        message += QString("%1- %2").arg(i + 1).arg(data[gidnum][i].text());
                        if (i != data[gidnum].size() - 1)
                            message += "\\n";
                    }

                    if (!inpm)
                    {
                        lastSup[gidnum] = 0;
                        message += "\\n\\n:|";
                    }
                }
            }
        }

        QByteArray sendee = inpm ? uid.toUtf8() : gid.toUtf8();

        if (!message.isEmpty())
            messageProcessor->sendCommand("msg " + sendee + " \"" + message.replace('"', "\\\"").toUtf8() + '"');
    }
}

bool Sup::addEntry(qint64 gid, const QString &str)
{
    if (data[gid].size() < MaxSupPerGroup)
    {
        QSqlQuery query;
        query.prepare("INSERT INTO tf_sup (gid, text) VALUES(:gid, :text)");
        query.bindValue(":gid", gid);
        query.bindValue(":text", str);
        database->executeQuery(query);
        qint64 rid = query.lastInsertId().toLongLong();

        if (rid != -1)
        {
            data[gid].append(SupEntry(rid, str));
            return true;
        }
    }

    return false;
}

void Sup::delEntry(qint64 gid, int id)
{
    if (id <= data[gid].size())
    {
        int realId = data[gid][id - 1].Id();

        QSqlQuery query;
        query.prepare("DELETE FROM tf_sup WHERE id=:id");
        query.bindValue(":id", realId);
        database->executeQuery(query);

        data[gid].removeAt(id - 1);
    }
}
