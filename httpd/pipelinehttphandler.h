/* Copyright 2013 Hallowyn and others.
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
#ifndef PIPELINEHTTPHANDLER_H
#define PIPELINEHTTPHANDLER_H

#include "httphandler.h"

/** HttpHandler that will call pipelined handlers one after another until
 * one of them returns false.
 * Usefull to prepend and append technical processing (authentication,
 * session data, etc.) to the main handler. */
class LIBQTSSUSHARED_EXPORT PipelineHttpHandler : public HttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(PipelineHttpHandler)
  QString _urlPathPrefix;
  QList<HttpHandler*> _handlers;

public:
  explicit inline PipelineHttpHandler(QObject *parent = 0)
    : HttpHandler(parent) { }
  explicit inline PipelineHttpHandler(QString urlPathPrefix = QString(),
                                      QObject *parent = 0)
    : HttpHandler(parent), _urlPathPrefix(urlPathPrefix) { }
  explicit inline PipelineHttpHandler(HttpHandler *handler,
                                      QString urlPathPrefix = QString(),
                                      QObject *parent = 0)
    : HttpHandler(parent), _urlPathPrefix(urlPathPrefix) {
    _handlers.append(handler); }
  /** Append a handler to the pipeline and take its ownership (it will become
   * a PipelineHttpHandler child, be deleted by PipelineHttpHandler and cannot
   * be shared with another PipelineHttpHandler or whatever other usage). */
  inline PipelineHttpHandler &appendHandler(HttpHandler *handler) {
    if (handler) {
      _handlers.append(handler);
      handler->setParent(this);
    }
    return *this; }
  /** Append a handler to the pipeline and take its ownership (it will become
   * a PipelineHttpHandler child, be deleted by PipelineHttpHandler and cannot
   * be shared with another PipelineHttpHandler or whatever other usage). */
  inline PipelineHttpHandler &prependHandler(HttpHandler *handler) {
    if (handler) {
      _handlers.prepend(handler);
      handler->setParent(this);
    }
    return *this; }
  inline PipelineHttpHandler &clearHandlers() {
    qDeleteAll(_handlers); _handlers.clear(); return *this; }
  bool acceptRequest(HttpRequest req);
  bool handleRequest(HttpRequest req, HttpResponse res,
                     HttpRequestContext ctxt);
};

#endif // PIPELINEHTTPHANDLER_H
