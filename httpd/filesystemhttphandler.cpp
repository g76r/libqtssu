/* Copyright 2012-2013 Hallowyn and others.
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
#include "filesystemhttphandler.h"
#include <QFile>
#include <QDir>
#include "util/ioutils.h"
#include <QtDebug>
#include "util/standardformats.h"

FilesystemHttpHandler::FilesystemHttpHandler(
    QObject *parent, const QString urlPrefix, const QString documentRoot) :
  HttpHandler(parent), _urlPrefix(urlPrefix), _documentRoot(documentRoot) {
  appendDirectoryIndex("index.html");
  appendMimeType("\\.html$", "text/html;charset=UTF-8");
  appendMimeType("\\.js$", "application/javascript");
  appendMimeType("\\.css$", "text/css");
  appendMimeType("\\.png$", "image/png");
  appendMimeType("\\.jpeg$", "image/jpeg");
  appendMimeType("\\.svg$", "image/svg+xml");
  appendMimeType("\\.tiff$", "image/tiff");
  appendMimeType("\\.csv$", "text/csv");
  appendMimeType("\\.pdf$", "application/pdf");
  appendMimeType("\\.json$", "application/json");
  appendMimeType("\\.xml$", "application/xml");
  appendMimeType("\\.zip$", "application/zip");
  appendMimeType("\\.gz$", "application/gzip");
  appendMimeType("\\.htm$", "text/html;charset=UTF-8");
  appendMimeType("\\.jpg$", "image/jpeg");
  appendMimeType("\\.gif$", "image/gif");
  appendMimeType("\\.ico$", "image/vnd.microsoft.icon");
}

QString FilesystemHttpHandler::name() const {
  return "FilesystemHttpHandler";
}

bool FilesystemHttpHandler::acceptRequest(HttpRequest req) {
  return _urlPrefix.isEmpty() || req.url().path().startsWith(_urlPrefix);
}

void FilesystemHttpHandler::handleRequest(HttpRequest req, HttpResponse res) {
  handleRequestWithContext(req, res, 0);
}

void FilesystemHttpHandler::handleRequestWithContext(
    HttpRequest req, HttpResponse res, ParamsProvider *values) {
  QString path = req.url().path();
  path.remove(0, _urlPrefix.length());
  while (path.size() && path.at(path.size()-1) == '/')
    path.chop(1);
  QFile file(_documentRoot+path);
  //qDebug() << "try file" << file.fileName();
  // Must try QDir::exists() before QFile::exists() because QFile::exists()
  // returns true for resources directories.
  QDir dir(_documentRoot+path);
  if (dir.exists()) {
    foreach (QString index, _directoryIndex) {
      file.setFileName(_documentRoot+path+"/"+index);
      //qDebug() << "try file" << file.fileName();
      if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QString location;
        QString reqPath = req.url().path();
        if (!reqPath.isEmpty() && reqPath.at(reqPath.size()-1) != '/') {
          int i = reqPath.lastIndexOf('/');
          location.append(reqPath.mid(i == -1 ? 0 : i+1));
          location.append('/');
        }
        location.append(index);
        res.redirect(location);
        return;
      }
    }
    res.setStatus(403);
    res.output()->write("Directory list denied.");
  }
  if (file.open(QIODevice::ReadOnly)) {
    sendLocalResource(req, res, &file, values);
    return;
  }
  int status = file.error() == QFile::PermissionsError ? 403 : 404;
  //qDebug() << "failure" << status;
  res.setStatus(status);
  res.output()->write(status == 403 ? "Permission denied."
                                    : "Document not found.");
  //qDebug() << "Cannot serve HTTP static resource" << req.url()
  //         << file.fileName() << status;
}

void FilesystemHttpHandler::sendLocalResource(
    HttpRequest req, HttpResponse res, QFile *file,
    ParamsProvider *values, QString scope) {
  Q_UNUSED(req)
  Q_UNUSED(values)
  Q_UNUSED(scope)
  //qDebug() << "success";
  QString filename(file->fileName());
  if (!handleCacheHeadersAndSend304(file, req, res)) {
    setMimeTypeByName(filename, res);
    res.setContentLength(file->size());
    IOUtils::copyAll(res.output(), file);
  }
}

void FilesystemHttpHandler::setMimeTypeByName(QString name, HttpResponse res) {
  // LATER check if performance can be enhanced (regexp)
  typedef QPair<QRegExp,QString> QRegExpQString;
  //qDebug() << "setMimeTypeByName" << name;
  foreach (QRegExpQString pair, _mimeTypes) {
    QRegExp re(pair.first);
    //qDebug() << "check" << pair.first;
    if (re.indexIn(name) >= 0) {
      //qDebug() << "match" << pair.second;
      res.setContentType(pair.second);
      return;
    }
  }
}

// LATER handle ETag / If-None-Match
bool FilesystemHttpHandler::handleCacheHeadersAndSend304(
    QFile *file, HttpRequest req, HttpResponse res) {
  static QDateTime startTime(QDateTime::currentDateTimeUtc());
  if (file) {
    QString filename(file->fileName());
    QFileInfo info(*file);
    QDateTime lastModified;
    if (filename.startsWith("qrc:") || filename.startsWith(":"))
      lastModified = startTime;
    else
      lastModified = info.lastModified().toUTC();
    if (lastModified.isValid())
      res.setHeader("Last-Modified", StandardFormats::toRfc2822DateTime(
                      lastModified));
    QDateTime ifModifiedSince(StandardFormats::fromRfc2822DateTime(
                                req.header("If-Modified-Since")).toUTC());
    // compare to If-Modified-Since +1" against rounding issues
    if (ifModifiedSince.isValid() && lastModified.isValid()
        && lastModified <= ifModifiedSince.addSecs(1)) {
      res.setStatus(304);
      return true;
    }
    return false;
  }
  return false;
}
