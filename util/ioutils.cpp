/* Copyright 2012 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#include "ioutils.h"
#include <QIODevice>
#include <QRegExp>
#include <QDir>
#include <QtDebug>

QString IOUtils::url2path(const QUrl &url) {
  if (url.scheme() == "file") {
    QString path = url.path();
    QRegExp rx("/[A-Z]:/.*");
    if (rx.exactMatch(path))
      return path.mid(1); // remove leading "/" in "/C:/path/to/file.jpg"
    return path;
  }
  if (url.scheme() == "qrc")
    return QString(":%1").arg(url.path());
  return QString();
}

qint64 IOUtils::copyAll(QIODevice &dest, QIODevice &src, qint64 bufsize) {
  char buf[bufsize];
  int total = 0, n, m;
  for (;;) {
    if (src.bytesAvailable() < 1)
      src.waitForReadyRead(30000);
    n = src.read(buf, bufsize);
    if (n < 0)
      return -1;
    if (n == 0)
      return total;
    m = dest.write(buf, n);
    if (m != n)
      return -1;
    total += n;
  }
}

qint64 IOUtils::copy(QIODevice &dest, QIODevice &src, qint64 max,
                     qint64 bufsize) {
  char buf[bufsize];
  int total = 0, n, m;
  while (total < max) {
    if (src.bytesAvailable() < 1)
      src.waitForReadyRead(30000);
    n = src.read(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    m = dest.write(buf, n);
    if (m != n)
      return -1;
    total += n;
  }
  return total;
}

qint64 IOUtils::grepString(QIODevice *dest, QIODevice *src, qint64 max,
                           const QString pattern, qint64 maxLineSize) {
  char buf[maxLineSize+1];
  int total = 0, n, m;
  while (total < max) {
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(30000);
    n = src->readLine(buf, sizeof buf);
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    if (QString::fromUtf8(buf).contains(pattern)) {
      m = dest->write(buf, n);
      if (m != n)
        return -1;
      total += n;
    }
  }
  return total;

}

qint64 IOUtils::grepRegexp(QIODevice *dest, QIODevice *src, qint64 max,
                                const QString pattern, qint64 maxLineSize) {
  char buf[maxLineSize+1];
  QRegExp re(pattern);
  int total = 0, n, m;
  while (total < max) {
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(30000);
    n = src->readLine(buf, sizeof buf);
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    if (QString::fromUtf8(buf).contains(re)) {
      m = dest->write(buf, n);
      if (m != n)
        return -1;
      total += n;
    }
  }
  return total;

}

static void findFiles(QDir dir, QStringList &files, const QRegExp pattern) {
  //qDebug() << "findFiles:" << dir.path() << dir.entryInfoList().size() << files.size() << pattern.pattern();
  foreach (const QFileInfo fi,
           dir.entryInfoList(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot,
                             QDir::Name)) {
    const QString path = fi.filePath();
    //qDebug() << "  QFileInfo:" << path << fi.isDir() << fi.isFile();
    if (fi.isDir()) {
      //qDebug() << "  Going down:" << path;
      findFiles(QDir(path), files, pattern);
    } else if (fi.isFile() && pattern.exactMatch(path)) {
      //qDebug() << "  Appending:" << path;
      files.append(path);
    }
  }
}

QStringList IOUtils::findFiles(const QString pattern) {
  QStringList files;
  QString pat = QDir().absoluteFilePath(QDir::fromNativeSeparators(pattern));
  int i = pat.indexOf(QRegExp("/[^/]*[*?[]|\\]"));
  QString dir = i >= 0 ? pat.left(i+1) : pat;
  QRegExp re(pat, Qt::CaseSensitive, QRegExp::Wildcard);
  ::findFiles(QDir(dir), files, re);
  return files;
}
