#pragma once

#include <QFile>
#include <QString>

class LogModel;

/*
 * Records homeserver actions, such as "alice logged in".
 *
 * Every action is timestamped, saved to a log file and added to the
 * LogModel so the window shows it. Routes call log() with a plain
 * description of what happened, not the raw API request.
 */
class Logger
{
public:
    /*
     * Opens the log file for appending.
     *
     * Lines from earlier runs are kept, and new lines are added at the end.
     */
    Logger(LogModel &model, const QString &filePath);

    /*
     * Records one action.
     *
     * Never pass tokens, passwords or message contents.
     */
    void log(const QString &action);

private:
    LogModel &m_model;
    QFile m_file;
};