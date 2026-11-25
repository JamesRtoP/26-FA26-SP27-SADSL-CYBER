#pragma once

#include <QAbstractListModel>
#include <QStringList>

/*
 * Holds the homeserver's log lines so the window can show them.
 *
 * Each row is one log line. Once MaxLines is reached, the oldest
 * lines are dropped.
 */
class LogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit LogModel(QObject *parent = nullptr);

    /* Number of log lines currently held. */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /* Returns the text of one line. QML reads it as "message". */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /* Names the data QML can ask for ("message"). */
    QHash<int, QByteArray> roleNames() const override;

    /* Adds one line to the end of the log. */
    void append(const QString &line);

private:
    static constexpr int MaxLines = 1000;
    QStringList m_lines;
};