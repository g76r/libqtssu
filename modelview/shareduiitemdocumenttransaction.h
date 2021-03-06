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
#ifndef SHAREDUIITEMDOCUMENTTRANSACTION_H
#define SHAREDUIITEMDOCUMENTTRANSACTION_H

#include <QPointer>
#include "shareduiitemlist.h"
#include "util/coreundocommand.h"
#include <functional>

class SharedUiItemDocumentManager;

/** Transaction that can be used by changeItem()/prepareChangeItem()/
 * commitChangeItem() to create ChangeItemCommands and access to changes
 * performed within the transaction but not yet commited to the DM. */
class LIBPUMPKINSHARED_EXPORT SharedUiItemDocumentTransaction
    : public CoreUndoCommand {
  SharedUiItemDocumentManager *_dm;
  QHash<QString,QHash<QString,SharedUiItem>> _changingItems, _originalItems;

public:
  using PostCreationModifier = std::function<void(
  SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
  QString *errorString)>;
  class LIBPUMPKINSHARED_EXPORT ChangeItemCommand : public CoreUndoCommand {
    QPointer<SharedUiItemDocumentManager> _dm;
    SharedUiItem _newItem, _oldItem;
    QString _idQualifier;

  public:
    ChangeItemCommand(SharedUiItemDocumentManager *dm, SharedUiItem newItem,
                      SharedUiItem oldItem, QString idQualifier,
                      CoreUndoCommand *parent);
    void redo();
    void undo();
    int	id() const;
    bool mergeWith(const CoreUndoCommand *command);
  };

  SharedUiItemDocumentTransaction(SharedUiItemDocumentManager *dm) : _dm(dm) { }
  SharedUiItem itemById(QString idQualifier, QString id) const;
  template <class T>
  inline T itemById(QString idQualifier, QString id) const {
    SharedUiItem item = itemById(idQualifier, id);
    return static_cast<T&>(item);
  }
  SharedUiItem itemById(QString qualifiedId) const {
    int colon = qualifiedId.indexOf(':');
    if (colon >= 0)
        return itemById(qualifiedId.left(colon), qualifiedId.mid(colon+1));
    return SharedUiItem();
  }
  template <class T>
  inline T itemById(QString qualifiedId) const {
    SharedUiItem item = itemById(qualifiedId);
    return static_cast<T&>(item);
  }
  SharedUiItemList<> itemsByIdQualifier(QString idQualifier) const;
  template <class T>
  inline SharedUiItemList<T> itemsByIdQualifier(QString idQualifier) const {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
    SharedUiItemList<> list = itemsByIdQualifier(idQualifier);
    union {
      SharedUiItemList<SharedUiItem> *generic;
      SharedUiItemList<T> *specialized;
    } pointer_alias_friendly_union;
    // the implicit reinterpret_cast done through the union is safe because the
    // static_cast at the begining would fail if T wasn't a ShareUiItem
    // reinterpret_cast mustn't be used since it triggers a "dereferencing
    // type-punned pointer will break strict-aliasing rules" warning, hence
    // using a union instead, for explicit (or gcc-friendly) aliasing
    pointer_alias_friendly_union.generic = &list;
    return *pointer_alias_friendly_union.specialized;
  }
  SharedUiItemList<> foreignKeySources(
      QString sourceQualifier, int sourceSection, QString referenceId) const;

  bool changeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value,
      QString *errorString);
  bool changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                  QString idQualifier, QString *errorString);
  SharedUiItem createNewItem(
      QString idQualifier, PostCreationModifier modifier, QString *errorString);
  /** @see SharedUiItemDocumentManager::generateNewId() */
  QString generateNewId(QString idQualifier, QString prefix = QString()) const;

private:
  void storeItemChange(SharedUiItem newItem, SharedUiItem oldItem,
                       QString idQualifier);
  SharedUiItemList<> changingItems() const;
  SharedUiItemList<> originalItems() const;
  //SharedUiItem oldItemIdByChangingItem(SharedUiItem changingItem) const;
  friend class SharedUiItemDocumentManager;
};


#endif // SHAREDUIITEMDOCUMENTTRANSACTION_H
