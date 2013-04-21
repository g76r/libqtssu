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
#include "httprequest.h"
#include <QtDebug>
#include <QRegExp>
#include "httpcommon.h"
#include <QSharedData>

class HttpRequestData : public QSharedData {
public:
  QAbstractSocket *_input;
  HttpRequest::HttpRequestMethod _method;
  QMultiMap<QString,QString> _headers;
  QHash<QString,QString> _cookies, _paramsCache;
  QUrl _url;
  HttpRequestData(QAbstractSocket *input = 0)  : _input(input),
    _method(HttpRequest::NONE) { }
  HttpRequestData(const HttpRequestData &other) : QSharedData(),
    _input(other._input), _method(other._method), _headers(other._headers),
    _cookies(other._cookies), _url(other._url) { }
};

HttpRequest::HttpRequest(QAbstractSocket *input)
  : d(new HttpRequestData(input)) {
}

HttpRequest::HttpRequest() {
}

HttpRequest::HttpRequest(const HttpRequest &other) : d(other.d) {
}

HttpRequest::~HttpRequest() {
}

HttpRequest &HttpRequest::operator=(const HttpRequest &other) {
  if (this != &other)
    d.operator=(other.d);
  return *this;
}

QString HttpRequest::methodName() const {
  return methodName(method());
}

QString HttpRequest::methodName(HttpRequestMethod method) {
  switch(method) {
  case NONE:
    return "NONE";
  case HEAD:
    return "HEAD";
  case GET:
    return "GET";
  case POST:
    return "POST";
  case PUT:
    return "PUT";
  case DELETE:
    return "DELETE";
  case ANY:
    return "ANY";
  }
  return "UNKNOWN";
}

bool HttpRequest::parseAndAddHeader(const QString rawHeader) {
  if (!d)
    return false;
  int i = rawHeader.indexOf(':');
  if (i == -1)
    return false;
  // MAYDO remove special chars from keys and values?
  // TODO support multi-line headers
  QString key = rawHeader.left(i).trimmed();
  QString value = rawHeader.right(rawHeader.size()-i-1).trimmed();
  //qDebug() << "header:" << rawHeader << key << value;
  d->_headers.insertMulti(key, value);
  if (key.compare("Cookie", Qt::CaseInsensitive) == 0)
    parseAndAddCookie(value);
  return true;
}

void HttpRequest::parseAndAddCookie(const QString rawHeaderValue) {
  // LATER ensure that utf8 is supported as specified in RFC6265
  // LATER enhance regexp performance
  if (!d)
    return;
  QRegExp re("\\s*;?\\s*(" RFC2616_TOKEN_OCTET_RE "*)\\s*=\\s*(("
             RFC6265_COOKIE_OCTET_RE "*|\"" RFC6265_COOKIE_OCTET_RE
             "+\"))\\s*;?\\s*");
  int pos = 0;
  //qDebug() << "parseAndAddCookie" << rawHeaderValue;
  while ((re.indexIn(rawHeaderValue, pos)) != -1) {
    const QString name = re.cap(1), value = re.cap(2);
    //qDebug() << "  " << name << value << pos;
    d->_cookies.insert(name, value);
    pos += re.matchedLength();
  }
}

QString HttpRequest::param(QString key) const {
  // TODO better handle parameters, including POST and multi-valued params
  QString value;
  if (d) {
    if (d->_paramsCache.contains(key))
      return d->_paramsCache.value(key);
    value = d->_url.queryItemValue(key);
    d->_paramsCache.insert(key, value);
  }
  return value;
}

void HttpRequest::overrideParam(QString key, QString value) {
  if (d)
    d->_paramsCache.insert(key, value);
}

void HttpRequest::overrideUnsetParam(QString key) {
  if (d)
    d->_paramsCache.insert(key, QString());
}

void HttpRequest::discardParamsCache() {
  if (d)
    d->_paramsCache.clear();
}

ParamSet HttpRequest::paramsAsParamSet() const {
  if (d) {
    cacheAllParams();
    return d->_paramsCache;
  } else
    return ParamSet();
}

void HttpRequest::cacheAllParams() const {
  if (d) {
    QListIterator<QPair<QString,QString> > it(d->_url.queryItems());
    while (it.hasNext()) {
      QPair<QString,QString> p(it.next());
      if (!d->_paramsCache.contains(p.first))
        d->_paramsCache.insert(p.first, p.second);
    }
  }
}

HttpRequest::operator QString() const {
  if (!d)
    return "HttpRequest{}";
  QString s;
  QTextStream ts(&s, QIODevice::WriteOnly);
  ts << "HttpRequest{ " << methodName() << ", " << url().toString() << ", { ";
  foreach (QString key, d->_headers.keys()) {
    ts << key << ":{ ";
    foreach (QString value, d->_headers.values(key)) {
      ts << value << " ";
    }
    ts << "} ";
  }
  ts << "} }";
  return s;
}

void HttpRequest::overrideUrl(QUrl url) {
  if (d)
    d->_url = url;
}

QUrl HttpRequest::url() const {
  return d ? d->_url : QUrl();
}

QAbstractSocket *HttpRequest::input() {
  return d ? d->_input : 0;
}

void HttpRequest::setMethod(HttpRequestMethod method) {
  if (d)
    d->_method = method;
}

HttpRequest::HttpRequestMethod HttpRequest::method() const {
  return d ? d->_method : NONE;
}

QString HttpRequest::header(QString name, QString defaultValue) const {
  if (!d)
    return defaultValue;
  const QString v = d->_headers.value(name);
  return v.isNull() ? defaultValue : v;
  // if multiple, the last one is returned
  // whereas in the J2EE API it's the first one
}

QStringList HttpRequest::headers(QString name) const {
  return d ? d->_headers.values(name) : QStringList();
}

QMultiMap<QString,QString> HttpRequest::headers() const {
  return d ? d->_headers : QMultiMap<QString,QString>();
}

QString HttpRequest::cookie(QString name, QString defaultValue) const {
  if (!d)
    return defaultValue;
  const QString v = d->_cookies.value(name);
  return v.isNull() ? defaultValue : v;
}

QString HttpRequest::base64Cookie(QString name, QString defaultValue) const {
  if (!d)
    return defaultValue;
  const QString v = d->_cookies.value(name);
  return v.isNull() ? defaultValue
                    : QString::fromUtf8(QByteArray::fromBase64(v.toAscii()));
}

QByteArray HttpRequest::base64BinaryCookie(QString name,
                                           QByteArray defaultValue) const {
  if (!d)
    return defaultValue;
  const QString v = d->_cookies.value(name);
  return v.isNull() ? defaultValue
                    : QByteArray::fromBase64(cookie(name).toAscii());
}
