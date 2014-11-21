#include "broadcast.h"
#include "messageprocessor.h"
#include "namedatabase.h"
#include <QtCore>

Broadcast::Broadcast(NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), nameDatabase(namedb), messageProcessor(msgproc)
{
}

void Broadcast::input(const QString &gid, const QString &uid, const QString &str)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    qDebug() << gid << uid << str << "debug";

    if (nameDatabase->groups().keys().contains(gidnum) && nameDatabase->groups()[gidnum].first == uidnum
        && str.startsWith("!broadcast"))
    {
        int idx = str.indexOf(' ');

        if (idx != -1)
        {
            while (idx < str.size() && str[idx] == ' ')
                ++idx;

            if (idx < str.size())
            {
                QString msg = str.mid(idx).trimmed();
                QString totalMsg = QString("Broadcast message from \"%1\" group:\\n%2")
                                   .arg(nameDatabase->groups()[gidnum].second)
                                   .arg(msg);

                foreach (qint64 uid, nameDatabase->userList(gidnum))
                    sendMessage(uid, totalMsg);

                int numMemebers = nameDatabase->userList(gidnum).size();

                messageProcessor->sendCommand("msg " + gid.toLatin1() +
                                              QString(" \"Sent broadcast message to %1 group member(s)!\"")
                                              .arg(numMemebers).toLatin1());
            }
        }
    }
}

void Broadcast::sendMessage(qint64 uid, QString str)
{
    messageProcessor->sendCommand("msg user#" + QByteArray::number(uid) + " \"" +
                                  str.replace('"', "\\\"").toLatin1() + '"');
}
