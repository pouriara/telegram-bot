#ifndef TREE_H
#define TREE_H

#include <QObject>

class NameDatabase;
class MessageProcessor;

class Tree : public QObject
{
    Q_OBJECT
private:
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    QByteArray lastId;

public:
    explicit Tree(NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm);

public slots:
    void processTree();
};

#endif // TREE_H
