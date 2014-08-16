/* Copyright 2013-2014 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#include "logger.h"
#include "loggerthread.h"

class Logger::LogEntryData : public QSharedData {
public:
  QDateTime _timestamp;
  QString _message;
  Log::Severity _severity;
  QString _task, _execId, _sourceCode;
  LogEntryData(QDateTime timestamp, QString message, Log::Severity severity,
               QString task, QString execId, QString sourceCode)
    : _timestamp(timestamp),
      _message(message), _severity(severity), _task(task), _execId(execId),
      _sourceCode(sourceCode) { }
};

Logger::LogEntry::LogEntry(QDateTime timestamp, QString message,
                           Log::Severity severity, QString task,
                           QString execId, QString sourceCode)
  : d(new LogEntryData(timestamp, message, severity, task, execId,
                       sourceCode)) {

}

Logger::LogEntry::LogEntry() {
}

Logger::LogEntry::LogEntry(const Logger::LogEntry &o) : d(o.d) {
}

Logger::LogEntry::~LogEntry() {
}

Logger::LogEntry &Logger::LogEntry::operator=(const Logger::LogEntry &o) {
  d = o.d;
  return *this;
}

bool Logger::LogEntry::isNull() const {
  return !d;
}

QDateTime Logger::LogEntry::timestamp() const {
  return d ? d->_timestamp : QDateTime();
}

QString Logger::LogEntry::message() const {
  return d ? d->_message : QString();
}

Log::Severity Logger::LogEntry::severity() const {
  return d ? d->_severity : Log::Debug;
}

QString Logger::LogEntry::severityText() const {
  return Log::severityToString(d ? d->_severity : Log::Debug);
}

QString Logger::LogEntry::task() const {
  return d ? d->_task : QString();
}

QString Logger::LogEntry::execId() const {
  return d ? d->_execId : QString();
}

QString Logger::LogEntry::sourceCode() const {
  return d ? d->_sourceCode : QString();
}

Logger::Logger(Log::Severity minSeverity, bool dedicatedThread)
  : QObject(0), _thread(dedicatedThread ? new LoggerThread(0, this) : 0),
    _minSeverity(minSeverity), _autoRemovable(true), _bufferOverflown(0),
    _buffer(dedicatedThread ? 10 : 0) { // LATER make buffer size parametrable
  //Log::fatal() << "*** Logger::Logger " << this << " " << minSeverity
  //             << " " << dedicatedThread;
  if (_thread) {
    connect(this, SIGNAL(destroyed(QObject*)), _thread, SLOT(quit()));
    connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater()));
    _thread->setObjectName("Logger-"+Log::severityToString(minSeverity)
                           +"-"+QString::number((long)this, 16));
    _thread->start();
    moveToThread(_thread);
  }
  qRegisterMetaType<Log::Severity>("Log::Severity");
  qRegisterMetaType<Logger::LogEntry>("Logger::LogEntry");
}

QString Logger::currentPath() const {
  return QString();
}

QString Logger::pathPattern() const {
  return currentPath();
}

void Logger::stopDedicatedThread() {
  if (_thread) {
    if (!_buffer.tryPut(LogEntry())) { // special message to stop the thread
      qWarning() << "Logger::stopDedicatedThread forced to call "
                    "QThread::terminate due to full thread buffer" << this;
      _thread->terminate();
    }
  }
}
