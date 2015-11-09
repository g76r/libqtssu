/* Copyright 2013-2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include "log.h"
#include <QDateTime>
#include "util/paramset.h"
#include "thread/circularbuffer.h"
#include <QThread>
#include "modelview/shareduiitem.h"

class MultiplexerLogger;
class LoggerThread;

/** Base class to be extended by logger implementations.
 * Handle common behavior to all loggers, including optionnaly a
 * queuing mechanism using a dedicated thread intended for loggers that
 * may block (filesystem, network...).
 * One must never delete a Logger, he must always call Logger::deleteLater()
 * instead.
 */
class LIBQTSSUSHARED_EXPORT Logger : public QObject {
  friend class MultiplexerLogger;
  friend class LoggerThread;

public:
  class LogEntryData;
  class LogEntry : public SharedUiItem {
  public:
    LogEntry();
    LogEntry(QDateTime timestamp, QString message, Log::Severity severity,
             QString task, QString execId, QString sourceCode);
    LogEntry(const LogEntry &other);
    LogEntry &operator=(const LogEntry &other) {
      SharedUiItem::operator=(other); return *this; }
    QDateTime timestamp() const;
    QString message() const;
    Log::Severity severity() const;
    QString severityToString() const;
    QString task() const;
    QString execId() const;
    QString sourceCode() const;

  private:
    const LogEntryData *data() const {
      return (const LogEntryData*)SharedUiItem::data(); }
  };
  enum ThreadModel {
    DirectCall, // the logger is already thread-safe and cannot block
    DedicatedThread, // the logger needs a dedicated thread in case it blocks
    RootLogger // root logger needs a dedicated thread and an input mutex
  };

private:
  Q_OBJECT
  Q_DISABLE_COPY(Logger)
  QThread *_thread;
  Log::Severity _minSeverity;
  bool _autoRemovable;
  QAtomicInt _bufferOverflown;
  CircularBuffer<LogEntry> *_buffer;

public:
  // Loggers never have a parent (since they are owned and destroyed by Log
  // static methods)
  Logger(Log::Severity minSeverity, ThreadModel threadModel);
  /** This method is thread-safe. */
  inline void log(LogEntry entry) {
    // this method must be callable from any thread, whereas the logger
    // implementation may not be threadsafe and/or may need a protection
    // against i/o latency: slow disk, NFS stall (for those fool enough to
    // write logs over NFS), etc.
    if (entry.severity() >= _minSeverity) {
      if (_thread) {
        if (!_buffer->tryPut(entry)) {
          // warn only once in the Logger lifetime
          if (_bufferOverflown.fetchAndStoreOrdered(1) == 0)
            qWarning() << QDateTime::currentDateTime()
                          .toString("yyyy-MM-dd hh:mm:ss,zzz")
                       << "Logger::log discarded at less one log entry due to "
                          "thread buffer full" << this << entry.message();
          // LATER have a way to reset flag after a while, to warn again if needed
        }
      } else {
        doLog(entry);
      }
    }
  }
  ~Logger();
  /** Return current logging path, e.g. "/var/log/qron-20181231.log"
   * To be used by implementation only when relevant.
   * Default: QString() */
  virtual QString currentPath() const;
  /** Return the path pattern, e.g. "/var/log/qron-%!yyyy%!mm%!dd.log"
   * To be used by implementation only when relevant.
   * Default: same as currentPath() */
  virtual QString pathPattern() const;
  /** Return the path regexp pattern, e.g. "/var/log/qron-.*\\.log" */
  QString pathMathchingPattern() const {
    return ParamSet::matchingPattern(pathPattern()); }
  /** Return the path regexp pattern, e.g. "/var/log/qron-.*\\.log" */
  QRegExp pathMatchingRegexp() const {
    return ParamSet::matchingRegexp(pathPattern()); }
  Log::Severity minSeverity() const { return _minSeverity; }
  /** Delete later both this and, if any, its dedicated thread. */
  void deleteLater();

protected:
  /** Method to be implemented by the actual logger.
   * Either the Logger must be created with dedicatedThread = true or this
   * method must be threadsafe (= able to handle calls from any thread at any
   * time). */
  virtual void doLog(const LogEntry entry) = 0;
};

Q_DECLARE_TYPEINFO(Logger::LogEntry, Q_MOVABLE_TYPE);

#endif // LOGGER_H
