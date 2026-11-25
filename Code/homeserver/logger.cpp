#include "logger.h"
#include "logmodel.h"

#include <QDateTime>
#include <QDebug>

/*
 * Stores the model and opens the log file.
 *
 * If the file can't be opened, the server keeps running. Actions still
 * show in the window, but they aren't saved.
 */
Logger::Logger(LogModel &model, const QString &filePath)
    : m_model(model)
    , m_file(filePath)
{
    // Append keeps old lines, and Text uses the right line endings on Windows.
    if (!m_file.open(QIODevice::Append | QIODevice::Text))
        qWarning() << "Could not open log file" << filePath;
}

/*
 * Timestamps an action, saves it to the file, and shows it in the window.
 */
void Logger::log(const QString &action)
{
    const QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    const QString line = time + "  " + action;

    // Write right away so nothing is lost if the server crashes.
    if (m_file.isOpen()) {
        m_file.write(line.toUtf8() + '\n');
        m_file.flush();
    }

    m_model.append(line);

    // Also print to the console. noquote() stops Qt from adding quote marks.
    qInfo().noquote() << line;
}