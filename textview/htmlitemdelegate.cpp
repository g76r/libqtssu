/* Copyright 2013-2017 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "htmlitemdelegate.h"
#include "format/stringutils.h"

HtmlItemDelegate::HtmlItemDelegate(QObject *parent)
  : TextViewItemDelegate(parent) {
}

QString HtmlItemDelegate::dataAffix(const TextMapper &m,
                                const QModelIndex &index) const {
  QString affix = m._text;
  if (m._argIndex >= 0) {
    QString arg = index.model()->data(
          index.model()->index(index.row(), m._argIndex, index.parent()))
        .toString();
    affix = affix.arg(m._transcodeMap.isEmpty()
                      ? arg
                      : m._transcodeMap.value(arg));
  }
  return affix;
}

QString HtmlItemDelegate::rowHeaderAffix(
    const TextMapper &m, const QAbstractItemModel* model, int row) const {
  QString affix = m._text;
  if (m._argIndex >= 0) {
    QString arg = model->data(model->index(row, m._argIndex, QModelIndex()))
        .toString();
    affix = affix.arg(m._transcodeMap.isEmpty()
                      ? arg
                      : m._transcodeMap.value(arg));
  }
  return affix;
}

QString HtmlItemDelegate::text(const QModelIndex &index) const {
  if (!index.isValid())
    return QString();
  QString data = index.data().toString();
  formatCell(&data);
  // prefix
  if (_rowPrefixes.contains(index.row()))
    data.prepend(dataAffix(_rowPrefixes[index.row()], index));
  else if (_columnPrefixes.contains(index.column()))
    data.prepend(dataAffix(_columnPrefixes[index.column()], index));
  else if (_rowPrefixes.contains(AllSections))
    data.prepend(dataAffix(_rowPrefixes[AllSections], index));
  else if (_columnPrefixes.contains(AllSections))
    data.prepend(dataAffix(_columnPrefixes[AllSections], index));
  // suffix
  if (_rowSuffixes.contains(index.row()))
    data.append(dataAffix(_rowSuffixes[index.row()], index));
  else if (_columnSuffixes.contains(index.column()))
    data.append(dataAffix(_columnSuffixes[index.column()], index));
  else if (_rowSuffixes.contains(AllSections))
    data.append(dataAffix(_rowSuffixes[AllSections], index));
  else if (_columnSuffixes.contains(AllSections))
    data.append(dataAffix(_columnSuffixes[AllSections], index));
  return data;
}

QString HtmlItemDelegate::headerText(int section, Qt::Orientation orientation,
                                     const QAbstractItemModel* model) const {
  if (!model)
    return QString();
  QString data = model->headerData(section, orientation).toString();
  formatCell(&data);
  switch (orientation) {
  case Qt::Vertical:
    // prefix
    if (_rowHeaderPrefixes.contains(section))
      data.prepend(rowHeaderAffix(_rowHeaderPrefixes[section], model, section));
    else if (_rowHeaderPrefixes.contains(AllSections))
      data.prepend(rowHeaderAffix(_rowHeaderPrefixes[AllSections], model,
                                  section));
    // suffix
    if (_rowHeaderSuffixes.contains(section))
      data.append(rowHeaderAffix(_rowHeaderSuffixes[section], model, section));
    else if (_rowHeaderSuffixes.contains(AllSections))
      data.append(rowHeaderAffix(_rowHeaderSuffixes[AllSections], model,
                                 section));
    break;
  case Qt::Horizontal:
    // prefix
    data.prepend(_columnHeaderPrefixes.value(
                   section, _columnHeaderPrefixes.value(AllSections)));
    // suffix
    data.append(_columnHeaderSuffixes.value(
                  section, _columnHeaderSuffixes.value(AllSections)));
    break;
  }
  return data;
}

void HtmlItemDelegate::setTextConversion(TextConversion conversion) {
  HtmlTableFormatter::setTextConversion(conversion);
  emit textChanged();
}

HtmlItemDelegate *HtmlItemDelegate::setPrefixForColumn(
    int column, QString pattern, int argIndex,
    QHash<QString,QString> transcodeMap) {
  _columnPrefixes.insert(column, TextMapper(pattern, argIndex, transcodeMap));
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::setSuffixForColumn(
    int column, QString pattern, int argIndex,
    QHash<QString,QString> transcodeMap){
  _columnSuffixes.insert(column, TextMapper(pattern, argIndex, transcodeMap));
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::setPrefixForRow(
    int row, QString pattern, int argIndex,
    QHash<QString,QString> transcodeMap){
  _rowPrefixes.insert(row, TextMapper(pattern, argIndex, transcodeMap));
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::setSuffixForRow(
    int row, QString pattern, int argIndex,
    QHash<QString,QString> transcodeMap){
  _rowSuffixes.insert(row, TextMapper(pattern, argIndex, transcodeMap));
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::setPrefixForColumnHeader(
    int column, QString text) {
  _columnHeaderPrefixes.insert(column, text);
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::setSuffixForColumnHeader(
    int column, QString text) {
  _columnHeaderSuffixes.insert(column, text);
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::setPrefixForRowHeader(
    int row, QString pattern, int argIndex,
    QHash<QString,QString> transcodeMap){
  _rowHeaderPrefixes.insert(row, TextMapper(pattern, argIndex, transcodeMap));
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::setSuffixForRowHeader(
    int row, QString pattern, int argIndex,
    QHash<QString,QString> transcodeMap) {
  _rowHeaderSuffixes.insert(row, TextMapper(pattern, argIndex, transcodeMap));
  emit textChanged();
  return this;
}

HtmlItemDelegate *HtmlItemDelegate::clearAffixes() {
  _columnPrefixes.clear();
  _columnSuffixes.clear();
  _rowPrefixes.clear();
  _rowSuffixes.clear();
  _columnHeaderPrefixes.clear();
  _columnHeaderSuffixes.clear();
  _rowHeaderPrefixes.clear();
  _rowHeaderSuffixes.clear();
  emit textChanged();
  return this;
}

void HtmlItemDelegate::setMaxCellContentLength(int maxCellContentLength) {
  HtmlTableFormatter::setMaxCellContentLength(maxCellContentLength);
  emit textChanged();
}
