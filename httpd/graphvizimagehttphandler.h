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
#ifndef GRAPHVIZIMAGEHTTPHANDLER_H
#define GRAPHVIZIMAGEHTTPHANDLER_H

#include "imagehttphandler.h"
#include <QMutex>
#include <QProcess>
#include <QBuffer>

class LIBQTSSUSHARED_EXPORT GraphvizImageHttpHandler : public ImageHttpHandler {
  Q_OBJECT
public:
  enum GraphvizRenderer { Dot, Neato, TwoPi, Circo, Fdp, Sfdp, Osage };

private:
  GraphvizRenderer _renderer;
  QString _source, _stderr, _contentType;
  bool _renderingRequested, _renderingRunning;
  mutable QMutex _mutex;
  QProcess *_process;
  QByteArray _imageData, _tmp;

public:
  explicit GraphvizImageHttpHandler(QObject *parent = 0);
  QByteArray imageData(ParamsProvider *params = 0) const;
  QString contentType(ParamsProvider *params = 0) const;
  QString source(ParamsProvider *params = 0) const;
  GraphvizRenderer renderer() const { return _renderer; }
  void setRenderer(GraphvizRenderer renderer) { _renderer = renderer; }

public slots:
  /** Set new graphviz-format source and trigger image layout processing */
  void setSource(QString source);

protected:
  void customEvent(QEvent *event);

private slots:
  void processError(QProcess::ProcessError error);
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void readyReadStandardOutput();
  void readyReadStandardError();

private:
  void startRendering();
};

#endif // GRAPHVIZIMAGEHTTPHANDLER_H