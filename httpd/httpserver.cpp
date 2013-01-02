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
#include "httpserver.h"
#include "httpworker.h"
#include <QMutexLocker>

// LATER this QObject should have its Q_OBJECT macro
class DefaultHandler : public HttpHandler {
public:
  DefaultHandler(QObject *parent) : HttpHandler(parent) { }

  virtual QString name() const {
    return "default";
  }

  void handleRequest(HttpRequest &req, HttpResponse &res) {
    Q_UNUSED(req)
    // LATER handle HEAD request (I don't know yet the most usefull way)
    res.setStatus(404);
    res.output()->write("Error 404 - Not found");
    //qDebug() << "serving with default handler" << req.methodName() << req.url();
  }

  bool acceptRequest(const HttpRequest &req) {
    Q_UNUSED(req)
    return true;
  }
};

HttpServer::HttpServer(QObject *parent) : QTcpServer(parent),
  _defaultHandler(new DefaultHandler(this)) {
}

HttpServer::~HttpServer() {
}

void HttpServer::incomingConnection(int socketDescriptor)  {
  HttpWorker *worker = new HttpWorker(socketDescriptor, this);
  connect(worker, SIGNAL(taskFinished(long)), worker, SLOT(deleteLater()));
  worker->start();
}

void HttpServer::appendHandler(HttpHandler *handler) {
  QMutexLocker ml(&_mutex);
  _handlers.append(handler);
  handler->setParent(this);
}

void HttpServer::prependHandler(HttpHandler *handler) {
  QMutexLocker ml(&_mutex);
  _handlers.prepend(handler);
  handler->setParent(this);
}

HttpHandler *HttpServer::chooseHandler(const HttpRequest &req) {
  QMutexLocker ml(&_mutex);
  foreach (HttpHandler *h, _handlers) {
    if (h->acceptRequest(req))
      return h;
  }
  return _defaultHandler;
}
