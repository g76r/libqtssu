/* Copyright 2015-2017 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitemdocumenttransaction.h"
#include "shareduiitemdocumentmanager.h"

void SharedUiItemDocumentTransaction::storeItemChange(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier) {
  ChangeItemCommand *command =
      new ChangeItemCommand(_dm, newItem, oldItem, idQualifier, this);
  switch (childCount()) {
  case 1:
    setText(command->text());
    break;
  case 2:
    setText(text()+" and other changes");
    break;
  }
  QString oldId = oldItem.id(), newId = newItem.id();
  QHash<QString,SharedUiItem> &changingItems = _changingItems[idQualifier];
  if (!oldItem.isNull() && !changingItems.contains(oldId))
    _originalItems[idQualifier].insert(oldId, oldItem);
  if (!oldItem.isNull())
    changingItems.insert(oldId, SharedUiItem());
  if (!newItem.isNull())
    changingItems.insert(newId, newItem);
}

SharedUiItem SharedUiItemDocumentTransaction::itemById(
    QString idQualifier, QString id) const {
  const QHash<QString,SharedUiItem> newItems = _changingItems[idQualifier];
  return newItems.contains(id) ? newItems.value(id)
                               : _dm->itemById(idQualifier, id);
}

SharedUiItemList<> SharedUiItemDocumentTransaction::itemsByIdQualifier(
    QString idQualifier) const {
  QHash<QString,SharedUiItem> changingItems = _changingItems.value(idQualifier);
  SharedUiItemList<> items;
  foreach (const SharedUiItem &item, changingItems.values())
    if (!item.isNull())
      items.append(item);
  foreach (const SharedUiItem &item, _dm->itemsByIdQualifier(idQualifier))
    if (!changingItems.contains(item.id()))
      items.append(item);
  return items;
}

SharedUiItemList<> SharedUiItemDocumentTransaction::changingItems() const {
  SharedUiItemList<> items;
  foreach (const QString &idQualifier, _changingItems.keys())
    foreach (const SharedUiItem &item,
             _changingItems.value(idQualifier).values()) {
      if (!item.isNull())
        items.append(item);
    }
  return items;
}

SharedUiItemList<> SharedUiItemDocumentTransaction::originalItems() const {
  SharedUiItemList<> items;
  foreach (const QString &idQualifier, _originalItems.keys())
    foreach (const SharedUiItem &item,
             _originalItems.value(idQualifier).values()) {
      if (!item.isNull())
        items.append(item);
    }
  return items;
}

/*SharedUiItem SharedUiItemDocumentTransaction::oldItemIdByChangingItem(
    SharedUiItem changingItem) const {
  Q_UNUSED(changingItem)
  // LATER (together with compression)
  return SharedUiItem();
}*/

SharedUiItemList<> SharedUiItemDocumentTransaction::foreignKeySources(
    QString sourceQualifier, int sourceSection, QString referenceId) const {
  SharedUiItemList<> sources;
  QHash<QString,SharedUiItem> changingItems =
      _changingItems.value(sourceQualifier);
  foreach (const SharedUiItem &item, changingItems.values()) {
    if (item.uiData(sourceSection) == referenceId)
      sources.append(item);
  }
  foreach (const SharedUiItem &item,
           _dm->itemsByIdQualifier(sourceQualifier)) {
    if (item.uiData(sourceSection) == referenceId
        && !changingItems.contains(item.id()))
      sources.append(item);
  }
  return sources;
}

bool SharedUiItemDocumentTransaction::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  QString idQualifier = oldItem.idQualifier();
  SharedUiItemDocumentManager::Setter setter =
      _dm->_setters.value(idQualifier);
  SharedUiItem newItem = oldItem;
  if (setter) {
    // LATER always EditRole ?
    // LATER simplify constraints processing since only one section is touched
    if (setter(&newItem, section, value, errorString, this, Qt::EditRole)) {
      if (_dm->processConstraintsAndPrepareChangeItem(
            this, newItem, oldItem, idQualifier, errorString)) {
        return true;
      }
    }
  } else {
    *errorString = "No setter registred for item type "+oldItem.idQualifier();
  }
  return false;
}

bool SharedUiItemDocumentTransaction::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  return _dm->processConstraintsAndPrepareChangeItem(
        this, newItem, oldItem, idQualifier, errorString);
}

SharedUiItem SharedUiItemDocumentTransaction::createNewItem(
    QString idQualifier, PostCreationModifier modifier, QString *errorString) {
  SharedUiItemDocumentManager::Creator creator =
      _dm->_creators.value(idQualifier);
  SharedUiItem nullItem;
  if (creator) {
    QString id = _dm->generateNewId(this, idQualifier);
    SharedUiItem newItem = creator(this, id, errorString);
    if (newItem.isNull()) {
      return nullItem;
    } else {
      if (modifier)
        modifier(this, &newItem, errorString);
      if (!_dm->processConstraintsAndPrepareChangeItem(
            this, newItem, nullItem, idQualifier, errorString))
        return nullItem;
    }
    return newItem;
  } else {
    *errorString = "No creator registered for item of type "+idQualifier;
    return nullItem;
  }
}

QString SharedUiItemDocumentTransaction::generateNewId(
    QString idQualifier, QString prefix) const {
  return _dm->generateNewId(this, idQualifier, prefix);
}

SharedUiItemDocumentTransaction::ChangeItemCommand::ChangeItemCommand(
    SharedUiItemDocumentManager *dm, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, CoreUndoCommand *parent)
  : CoreUndoCommand(parent), _dm(dm), _newItem(newItem), _oldItem(oldItem),
    _idQualifier(idQualifier)  {
  if (newItem.isNull())
    setText("Deleting a "+oldItem.idQualifier());
  else if (oldItem.isNull())
    setText("Creating a "+newItem.idQualifier());
  else
    setText("Changing a "+oldItem.idQualifier());
}

void SharedUiItemDocumentTransaction::ChangeItemCommand::redo() {
  if (_dm)
    _dm->commitChangeItem(_newItem, _oldItem, _idQualifier);
}

void SharedUiItemDocumentTransaction::ChangeItemCommand::undo() {
  if (_dm)
    _dm->commitChangeItem(_oldItem, _newItem, _idQualifier);
}

int	SharedUiItemDocumentTransaction::ChangeItemCommand::id() const {
  return 42;
}

bool SharedUiItemDocumentTransaction::ChangeItemCommand::mergeWith(
    const CoreUndoCommand *command) {
  const ChangeItemCommand *other =
      static_cast<const ChangeItemCommand *>(command);
  Q_UNUSED(other)
  // LATER implement this or find another way to compress commands within a transaction in case the same item is changed several times in chain
  return false;
}
