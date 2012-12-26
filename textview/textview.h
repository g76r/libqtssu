#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include <QObject>
#include <QAbstractItemModel>
#include "libqtssu_global.h"

/** Class to use Qt's Model-View UI framework for text-oriented display, such
 * as web applications, REST APIs and command line interfaces.
 */
class LIBQTSSUSHARED_EXPORT TextView : public QObject {
  Q_OBJECT
  QAbstractItemModel *_model;
public:
  explicit TextView(QObject *parent = 0);

signals:
  void modelChanged();

public slots:
  virtual void setModel(QAbstractItemModel *model);
  QAbstractItemModel *model() const { return _model; }
  /** Provide the text view of the model, e.g. a HTML string that can be pasted
   * within a HTML page body, a JSON document or an ASCII art string fo a
   * text interface.
   * This method must be thread-safe, since it may be called by any thread,
   * e.g. a HTTP server thread.
   */
  virtual QString text() const = 0;
};

#endif // TEXTVIEW_H
