/* Copyright 2015 Hallowyn and others.
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
#ifndef SHAREDUIITEMDOCUMENTMANAGER_H
#define SHAREDUIITEMDOCUMENTMANAGER_H

#include <QObject>
#include "libqtssu_global.h"
#include "modelview/shareduiitem.h"

/** Base class for SharedUiItem-based document managers.
 * @see SharedUiItem */
class LIBQTSSUSHARED_EXPORT SharedUiItemDocumentManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemDocumentManager)

public:
  explicit SharedUiItemDocumentManager(QObject *parent = 0);
  /** Method that user interface should call to create a new default item with
   * an automatically generated unique id.  */
  virtual SharedUiItem createNewItem(QString idQualifier) = 0;
  /** Method that user interface should call to change an item, one field at a
   * time.
   *
   * Suited for model/view edition. */
  virtual bool changeItemByUiData(SharedUiItem oldItem, int section,
                                  const QVariant &value) = 0;
  /** Method that user interface or non-interactive code should call to change
   * an item at a whole. */
  virtual bool changeItem(SharedUiItem newItem, SharedUiItem oldItem) = 0;
  virtual SharedUiItem itemById(QString idQualifier, QString id) const = 0;
  /** Default: parses qualifiedId and calls itemById(QString,QString). */
  virtual SharedUiItem itemById(QString qualifiedId) const;

signals:
  /** Emited whenever an item changes.
   *
   * When an item is created oldItem.isNull().
   * When an item is destroyed newItem.isNull().
   * When an item is renamed (changes id) newItem.id() != oldItem.id().
   * Otherwise the item is updated (changes anything apart its id).
   *
   * Notes: oldItem and newItem cannot be null at the same time, if none is null
   * then oldItem.idQualifier() == newItem.idQualifier().
   *
   * Subclasses may (should?) add specialized signals for their different item
   * types, e.g. "void customerChanged(Customer newItem, Customer oldItem)"
   * where Customer is a SharedUiItem.
   */
  void itemChanged(SharedUiItem newItem, SharedUiItem oldItem);

protected:
  /** Can be called by createNewItem() implementations to generate a new id not
   * currently in use for the given idQualifier item type.
   * Generate id of the form idQualifier+number (e.g. "foobar1"). */
  QString genererateNewId(QString idQualifier);
};

#endif // SHAREDUIITEMDOCUMENTMANAGER_H
