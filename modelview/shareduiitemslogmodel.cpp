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
#include "shareduiitemslogmodel.h"
#include <QDateTime>
#include "modelview/shareduiitem.h"

class LIBQTSSUSHARED_EXPORT SharedUiItemLogWrapperData
    : public SharedUiItemData {
public:
  SharedUiItem _wrapped;
  QDateTime _timestamp;
  int _timestampSection;

  SharedUiItemLogWrapperData(SharedUiItem wrapped, QDateTime timestamp)
    : _wrapped(wrapped), _timestamp(timestamp),
      _timestampSection(wrapped.uiSectionCount()) { }
  SharedUiItemLogWrapperData() : _timestampSection(0) { }
  QString id() const { return _wrapped.id(); }
  QString idQualifier() const { return _wrapped.idQualifier(); }
  int uiSectionCount() const { return _timestampSection+1; }
  QVariant uiData(int section, int role) const {
    return (role == Qt::DisplayRole && section == _timestampSection)
        ? _timestamp : _wrapped.uiData(section, role); }
  QVariant uiHeaderData(int section, int role) const {
    return (role == Qt::DisplayRole && section == _timestampSection)
        ? QStringLiteral("Timestamp") : _wrapped.uiHeaderData(section, role); }
  Qt::ItemFlags uiFlags(int section) const {
  return (section == _timestampSection)
      ? Qt::ItemIsSelectable|Qt::ItemIsEnabled : _wrapped.uiFlags(section); }
  // LATER also wrap setUiData, or warn it is not wrapped
  //bool setUiData(int section, const QVariant &value, QString *errorString,
  //               int role, const SharedUiItemDocumentManager *dm) {
  //  return _wrapped.setUiData(section, value, errorString, role, dm); }
};

class LIBQTSSUSHARED_EXPORT SharedUiItemLogWrapper : public SharedUiItem {
public:
  SharedUiItemLogWrapper(SharedUiItem wrapped)
    : SharedUiItem(new SharedUiItemLogWrapperData(
                     wrapped, QDateTime::currentDateTime())) { }
};

SharedUiItemsLogModel::SharedUiItemsLogModel(QObject *parent, int maxrows)
  : SharedUiItemsTableModel(parent), _timestampColumn(0) {
  setDefaultInsertionPoint(FirstItem);
  setMaxrows(maxrows);
}

void SharedUiItemsLogModel::setHeaderDataFromTemplate(
    SharedUiItem templateItem, int role) {
  _timestampColumn = templateItem.uiSectionCount();
  SharedUiItemsTableModel::setHeaderDataFromTemplate(
        SharedUiItemLogWrapper(templateItem), role);
}

void SharedUiItemsLogModel::logItem(SharedUiItem newItem) {
  if (newItem)
    SharedUiItemsTableModel::changeItem(SharedUiItemLogWrapper(newItem), SharedUiItem());
}
