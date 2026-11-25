#include "logmodel.h"

/*
 * Creates an empty log.
 *
 * The parent is passed to QAbstractListModel so Qt deletes this model
 * when its parent is deleted.
 */
LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

/*
 * Returns how many log lines are stored.
 *
 * Qt's model system is shared by lists, tables and trees, so Qt can also
 * ask how many rows are inside a given row. A flat list never has rows
 * inside rows, so that case returns 0.
 */
int LogModel::rowCount(const QModelIndex &parent) const
{
    // A valid parent means Qt is asking about rows inside a row.
    if (parent.isValid())
        return 0;

    return m_lines.size();
}

/*
 * Returns the text of one log line.
 *
 * QML calls this for each visible row. An empty QVariant means
 * "nothing here".
 */
QVariant LogModel::data(const QModelIndex &index, int role) const
{
    // Ignore bad or out-of-range rows.
    if (!index.isValid() || index.row() >= m_lines.size())
        return QVariant();

    // DisplayRole is the main text of the row.
    if (role == Qt::DisplayRole)
        return m_lines.at(index.row());

    return QVariant();
}

/*
 * Gives each piece of row data a name QML can use.
 *
 * The window's delegate declares "required property string message",
 * and this mapping is what fills it.
 */
QHash<int, QByteArray> LogModel::roleNames() const
{
    return {{Qt::DisplayRole, "message"}};
}

/*
 * Adds one line to the end of the log.
 *
 * Changes must be wrapped in begin/end calls so the ListView knows to
 * redraw. Once MaxLines is reached, the oldest line is removed first.
 */
void LogModel::append(const QString &line)
{
    // Drop the oldest line (row 0) when the log is full.
    if (m_lines.size() >= MaxLines) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_lines.removeFirst();
        endRemoveRows();
    }

    // The new line goes at the end, so its row is the current size.
    const int row = m_lines.size();
    beginInsertRows(QModelIndex(), row, row);
    m_lines.append(line);
    endInsertRows();
}