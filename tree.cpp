#include "tree.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include <QtCore>

Tree::Tree(NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), nameDatabase(namedb), messageProcessor(msgproc)
{
}

void Tree::input(const QString &gid, const QString &uid, const QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!tree") && lastId.isEmpty())
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);
        inpm = inpm || (args.size() > 1 && args[1].toLower().startsWith("pm"));

        QFile file("tree.dot");

        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream TS(&file);
            TS.setCodec(QTextCodec::codecForName("UTF-8"));

            TS << "digraph TelegramUsers" << endl << "{" << endl;

            QMapIterator<qint64, qint64> users(nameDatabase->userList(gidnum));
            while (users.hasNext())
            {
                users.next();
                TS << "    \"" << messageProcessor->convertToName(users.value()) << "\" -> \""
                   <<  messageProcessor->convertToName(users.key()) << "\"" << endl;
            }

            TS << "}" << endl;

            lastId = inpm ? uid.toUtf8() : gid.toUtf8();

            QProcess *graphviz = new QProcess(this);
            connect(graphviz, SIGNAL(finished(int)), this, SLOT(processTree()));

            QStringList args;
            args << "-Tpng" << "tree.dot";

            graphviz->setStandardOutputFile("tree.png");

            graphviz->start("dot", args);
        }
    }
}

void Tree::processTree()
{
    QProcess *proc = qobject_cast<QProcess *>(sender());

    if (proc->exitCode() == 0)
        messageProcessor->sendCommand("send_document " + lastId + " tree.png");

    proc->deleteLater();
    lastId.clear();
}
