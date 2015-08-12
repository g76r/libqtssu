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
#include "inmemoryshareduiitemdocumentmanager.h"

InMemorySharedUiItemDocumentManager::InMemorySharedUiItemDocumentManager(
    QObject *parent) : SharedUiItemDocumentManager(parent) {
}

SharedUiItem InMemorySharedUiItemDocumentManager::createNewItem(
    QString idQualifier, QString *errorString) {
  //qDebug() << "createNewItem" << idQualifier;
  Creator creator = _creators.value(idQualifier);
  SharedUiItem newItem;
  if (creator) {
    QString id = genererateNewId(idQualifier);
    newItem = (*creator)(id);
    _repository[idQualifier][id] = newItem;
    emit itemChanged(newItem, SharedUiItem(), idQualifier);
    //qDebug() << "created";
  } else {
    if (errorString)
      *errorString = "no creator registred for item of type "+idQualifier;
  }
  return newItem;
}

bool InMemorySharedUiItemDocumentManager::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  QString reason;
  if (!oldItem.isNull() && !_repository[idQualifier].contains(oldItem.id())) {
    // reject createOrUpdate and deleteIfExist behaviors
    reason = "old item "+oldItem.qualifiedId()+" not found";
  } else {
    if (!oldItem.isNull() && newItem != oldItem) { // renamed or deleted
      _repository[idQualifier].remove(oldItem.id());
    }
    if (!newItem.isNull()) { // created or updated
      _repository[idQualifier][newItem.id()] = newItem;
    }
    emit itemChanged(newItem, oldItem, idQualifier);
  }
  if (!reason.isEmpty() && errorString)
    *errorString = reason;
  return reason.isEmpty();
}

bool InMemorySharedUiItemDocumentManager::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  SharedUiItem newItem;
  return changeItemByUiData(oldItem, section, value, &newItem, errorString);
}

bool InMemorySharedUiItemDocumentManager::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    SharedUiItem *newItem, QString *errorString) {
  *newItem = oldItem;
  Setter setter = _setters.value(oldItem.idQualifier());
  //qDebug() << "changeItemByUiData" << oldItem.qualifiedId() << section
  //         << value << setter;
  if (setter && (newItem->*setter)(section, value, errorString,
                                   Qt::EditRole, this))
    return changeItem(*newItem, oldItem, oldItem.idQualifier(), errorString);
  return false;
}

SharedUiItem InMemorySharedUiItemDocumentManager::itemById(
    QString idQualifier, QString id) const {
  return _repository.value(idQualifier).value(id);
}

InMemorySharedUiItemDocumentManager &InMemorySharedUiItemDocumentManager
::registerItemType(QString idQualifier,
                    InMemorySharedUiItemDocumentManager::Setter setter,
                    InMemorySharedUiItemDocumentManager::Creator creator) {
  _setters.insert(idQualifier, setter);
  _creators.insert(idQualifier, creator);
  //qDebug() << "registered" << idQualifier;
  return *this;
}

SharedUiItemList<SharedUiItem> InMemorySharedUiItemDocumentManager
::itemsByIdQualifier(QString idQualifier) const {
  SharedUiItemList<SharedUiItem> list;
  foreach (SharedUiItem item, _repository.value(idQualifier))
    list.append(item);
  return list;
}
