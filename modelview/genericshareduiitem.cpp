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
#include "genericshareduiitem.h"

class GenericSharedUiItemData : public SharedUiItemData {
public:
  QString _idQualifier, _id;
  QVariantList _headers, _values;

  GenericSharedUiItemData() { }
  GenericSharedUiItemData(
      QString idQualifier, QString id, QVariantList headers,
      QVariantList values)
    : _idQualifier(idQualifier), _id(id), _headers(headers), _values(values) { }
  GenericSharedUiItemData(QString idQualifier, QString id)
    : _idQualifier(idQualifier), _id(id) { }
  explicit GenericSharedUiItemData(QString qualifiedId) {
    int i = qualifiedId.indexOf(':');
    if (i < 0) {
      _idQualifier = QStringLiteral("generic");
      _id = qualifiedId;
    } else {
      _idQualifier = qualifiedId.left(i);
      _id = qualifiedId.mid(i+1);
    }
  }
  QString id() const { return _id; }
  QString idQualifier() const { return _idQualifier; }
  int uiSectionCount() const { return qMax(_headers.size(), _values.size()); }
  QVariant uiData(int section, int role) const {
    if (role == Qt::DisplayRole || role == Qt::EditRole)
      return _values.value(section);
    return QVariant();
  }
  QVariant uiHeaderData(int section, int role) const {
    return role == Qt::DisplayRole ? _headers.value(section) : QVariant();
  }
  //Qt::ItemFlags uiFlags(int section) const;
  //bool setUiData(int section, const QVariant &value, QString *errorString,
  //               int role, const SharedUiItemDocumentManager *dm);
};

GenericSharedUiItem::GenericSharedUiItem() {
}

GenericSharedUiItem::GenericSharedUiItem(const GenericSharedUiItem &other)
  : SharedUiItem(other) {
}

GenericSharedUiItem::GenericSharedUiItem(
    QString idQualifier, QString id, QVariantList headers,
    QVariantList values)
  : SharedUiItem(new GenericSharedUiItemData(idQualifier, id, headers,
                                             values)) {
}

GenericSharedUiItem::GenericSharedUiItem(QString idQualifier, QString id)
  : SharedUiItem(new GenericSharedUiItemData(idQualifier, id)) {
}

GenericSharedUiItem::GenericSharedUiItem(QString qualifiedId)
  : SharedUiItem(new GenericSharedUiItemData(qualifiedId)) {
}


QList<GenericSharedUiItem> GenericSharedUiItem::fromCsv(
    CsvFile *csvFile, int idColumn, QString idQualifier) {
  QList<GenericSharedUiItem> list;
  if (!csvFile)
    return list;
  QVariantList vHeaders;
  foreach (QString header, csvFile->headers())
    vHeaders.append(header);
  for (int i = 0; i < csvFile->rowCount(); ++i) {
    QStringList values = csvFile->row(i);
    QString id;
    if (values.size() > idColumn)
      id = values[idColumn];
    QVariantList vValues;
    foreach (QString value, values)
      vValues.append(value);
    list.append(GenericSharedUiItem(idQualifier, id, vHeaders, vValues));
  }
  return list;
}