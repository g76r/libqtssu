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
#include "graphvizimagehttphandler.h"
#include <QCoreApplication>
#include "log/log.h"

#define UPDATE_EVENT (QEvent::Type(QEvent::User+1))

GraphvizImageHttpHandler::GraphvizImageHttpHandler(QObject *parent)
  : ImageHttpHandler(parent), _renderer(Dot), _renderingRequested(false),
    _renderingRunning(false), _mutex(QMutex::Recursive),
    _process(new QProcess(this)) {
  connect(_process, SIGNAL(finished(int,QProcess::ExitStatus)),
          this, SLOT(processFinished(int,QProcess::ExitStatus)));
  connect(_process, SIGNAL(error(QProcess::ProcessError)),
          this, SLOT(processError(QProcess::ProcessError)));
  connect(_process, SIGNAL(readyReadStandardOutput()),
          this, SLOT(readyReadStandardOutput()));
  connect(_process, SIGNAL(readyReadStandardError()),
          this, SLOT(readyReadStandardError()));
}

QByteArray GraphvizImageHttpHandler::imageData(ParamsProvider *params) const {
  Q_UNUSED(params)
  QMutexLocker ml(&_mutex);
  return _imageData;
}

QString GraphvizImageHttpHandler::contentType(ParamsProvider *params) const {
  Q_UNUSED(params)
  QMutexLocker ml(&_mutex);
  return _contentType;
}

QString GraphvizImageHttpHandler::source(ParamsProvider *params) const {
  Q_UNUSED(params)
  QMutexLocker ml(&_mutex);
  return _source;
}

void GraphvizImageHttpHandler::setSource(QString source) {
  QMutexLocker ml(&_mutex);
  _source = source;
  if (!_renderingRequested) {
    _renderingRequested = true;
    QCoreApplication::postEvent(this, new QEvent(UPDATE_EVENT));
  }
}

void GraphvizImageHttpHandler::customEvent(QEvent *event) {
  if (event->type() == UPDATE_EVENT) {
    QCoreApplication::removePostedEvents(this, UPDATE_EVENT);
    startRendering();
  } else {
    ImageHttpHandler::customEvent(event);
  }
}

void GraphvizImageHttpHandler::startRendering() {
  QMutexLocker ml(&_mutex);
  if (_renderingRunning)
    return; // postponing after currently running rendering
  _renderingRequested = false;
  _renderingRunning = true;
  _tmp.clear();
  _stderr.clear();
  QString cmd = "dot"; // default to dot
  switch (_renderer) {
  case Neato:
    cmd = "neato";
    break;
  case TwoPi:
    cmd = "twopi";
    break;
  case Circo:
    cmd = "circo";
    break;
  case Dot:
    cmd = "dot";
    break;
  case Fdp:
    cmd = "fdp";
    break;
  case Sfdp:
    cmd = "sfdp";
    break;
  case Osage:
    cmd = "osage";
    break;
  }
  QStringList args;
  args << "-Tpng";
  QByteArray ba = _source.toUtf8();
  Log::debug() << "starting graphviz rendering with this data: "
               << _source;
  _process->start(cmd, args);
  _process->waitForStarted();
  qint64 written = _process->write(ba);
  if (written != ba.size())
    Log::debug() << "cannot write to graphviz processor "
                 << written << " " << ba.size() << " "
                 << _process->error() << " " << _process->errorString();
  _process->closeWriteChannel();
}

void GraphvizImageHttpHandler::processError(QProcess::ProcessError error) {
  readyReadStandardError();
  readyReadStandardOutput();
  Log::warning() << "graphviz rendering process crashed with "
                    "QProcess::ProcessError code " << error << " ("
                 << _process->errorString() << ") and stderr content: "
                 << _stderr;
  _process->kill();
  processFinished(-1, QProcess::CrashExit);
}

void GraphvizImageHttpHandler::processFinished(
    int exitCode, QProcess::ExitStatus exitStatus) {
  readyReadStandardError();
  readyReadStandardOutput();
  bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
  QMutexLocker ml(&_mutex);
  if (success) {
    Log::debug() << "graphviz rendering process successful with return code "
                 << exitCode << " and QProcess::ExitStatus " << exitStatus
                 << " having produced a " << _tmp.size() << " bytes output";
    _contentType = "image/png";
    _imageData = _tmp;
  } else {
    Log::warning() << "graphviz rendering process failed with return code "
                   << exitCode << ", QProcess::ExitStatus " << exitStatus
                   << " and stderr content: " << _stderr;
    _contentType = "text/plain;charset=UTF-8";
    _imageData = _stderr.toUtf8(); // LATER placeholder image
  }
  _renderingRunning = false;
  if (_renderingRequested)
    startRendering();
  ml.unlock();
  emit contentChanged();
  _tmp.clear();
  _stderr.clear();
}

void GraphvizImageHttpHandler::readyReadStandardOutput() {
  _process->setReadChannel(QProcess::StandardOutput);
  QByteArray ba;
  while (!(ba = _process->read(1024)).isEmpty())
    _tmp.append(ba);
}

void GraphvizImageHttpHandler::readyReadStandardError() {
  _process->setReadChannel(QProcess::StandardError);
  QByteArray ba;
  while (!(ba = _process->read(1024)).isEmpty())
    _stderr.append(QString::fromUtf8(ba));
}