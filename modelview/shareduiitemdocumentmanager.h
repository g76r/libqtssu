/* Copyright 2015-2018 Hallowyn, Gregoire Barbier and others.
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
#ifndef SHAREDUIITEMDOCUMENTMANAGER_H
#define SHAREDUIITEMDOCUMENTMANAGER_H

#include <QObject>
#include "shareduiitemdocumenttransaction.h"
#include <functional>

/** Base class for SharedUiItem-based document managers.
 *
 * Quick implementation guide:
 * Document manager Implementations SHOULD implement/override this methods:
 * - prepareChangeItem()
 * - commitChangeItem()
 * - itemById() (taking care not to hide overloaded forms)
 * - itemsByIdQualifier() (taking care not to hide overloaded forms)
 *
 * @see SharedUiItem */
class LIBPUMPKINSHARED_EXPORT SharedUiItemDocumentManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemDocumentManager)

public:
  using Setter = std::function<bool(SharedUiItem *item, int section,
  const QVariant &value, QString *errorString,
  SharedUiItemDocumentTransaction *transaction, int role)>;
  // following adapter is needed because members pointer are not convertible
  // to base class member pointer, therefore &Foobar::setUiData cannot be
  // holded by function<bool(SharedUiItem*...)> even if Foobar is a subclass
  // of SharedUiItem
  template <class T>
  using MemberSetter = bool (T::*)(int section, const QVariant &value,
  QString *errorString, SharedUiItemDocumentTransaction *transaction,
  int role);
  using Creator = std::function<SharedUiItem(
  SharedUiItemDocumentTransaction *transaction, QString id,
  QString *errorString)>;
  using SimplestCreator = std::function<SharedUiItem(QString id)>;
  using PostCreationModifier =
  SharedUiItemDocumentTransaction::PostCreationModifier;
  using ChangeItemTrigger = std::function<bool(
  SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
  SharedUiItem oldItem, QString idQualifier, QString *errorString)>;
  enum OnChangePolicy {
    /** no automatic action on update or delete */
    NoAction,
    /** set referencing section to QVariant() on referencing item */
    SetNull,
    /** on update: update referencing section on referencing item,
     * on delete: delete referencing item */
    Cascade,
    /** on update: update referencing section on referencing item every time
     * referenced item changes, even if changing sections do not include
     * referenced key section,
     * on delete: same as Cascade */
    CascadeAnySection
  };
  enum TriggerFlag {
    NoTrigger = 0,
    BeforeUpdate = 1,
    AfterUpdate = 2,
    BeforeCreate = 4,
    AfterCreate = 8,
    BeforeDelete = 16,
    AfterDelete = 32
    // LATER add BeforeCommit trigger for integrity checks
  };
  Q_DECLARE_FLAGS(TriggerFlags, TriggerFlag)
  Q_FLAGS(TriggerFlags)

private:
  struct ForeignKey {
    QString _sourceQualifier;
    int _sourceSection;
    QString _referenceQualifier;
    int _referenceSection;
    OnChangePolicy _onDeletePolicy;
    OnChangePolicy _onUpdatePolicy;
    ForeignKey(QString sourceQualifier, int sourceSection,
               QString referenceQualifier, int referenceSection,
               OnChangePolicy onDeletePolicy, OnChangePolicy onUpdatePolicy)
      : _sourceQualifier(sourceQualifier), _sourceSection(sourceSection),
        _referenceQualifier(referenceQualifier),
        _referenceSection(referenceSection), _onDeletePolicy(onDeletePolicy),
        _onUpdatePolicy(onUpdatePolicy) { }
  };

protected:
  QHash<QString,Setter> _setters;
  QHash<QString,Creator> _creators;
  QList<ForeignKey> _foreignKeys;
  QMultiHash<QString,ChangeItemTrigger> _triggersBeforeUpdate;
  QMultiHash<QString,ChangeItemTrigger> _triggersAfterUpdate;
  QMultiHash<QString,ChangeItemTrigger> _triggersBeforeCreate;
  QMultiHash<QString,ChangeItemTrigger> _triggersAfterCreate;
  QMultiHash<QString,ChangeItemTrigger> _triggersBeforeDelete;
  QMultiHash<QString,ChangeItemTrigger> _triggersAfterDelete;

public:
  explicit SharedUiItemDocumentManager(QObject *parent = nullptr);
  /** Method that user interface should call to create a new default item with
   * an automatically generated unique id.
   *
   * Actually, this method create a Transaction and calls item type creator
   * which is the right place to customize item creation from document manager
   * implementations.
   * No other class than DtpDocumentManager should override createNewItem().
   */
  virtual SharedUiItem createNewItem(
      QString idQualifier, PostCreationModifier modifier = nullptr,
      QString *errorString = nullptr);
  /** Convenience template performing downcast. */
  template<class T>
  T createNewItem(QString idQualifier, PostCreationModifier modifier = nullptr,
                  QString *errorString = nullptr) {
    Q_UNUSED(modifier)
    SharedUiItem item = createNewItem(idQualifier, errorString);
    return static_cast<T&>(item);
  }
  /** Convenience method with modifier = 0 */
  SharedUiItem createNewItem(
      QString idQualifier, QString *errorString) {
    return createNewItem(idQualifier, nullptr, errorString);
  }
  /** Convenience template performing downcast. */
  template<class T>
  T createNewItem(QString idQualifier, QString *errorString) {
    SharedUiItem item = createNewItem(idQualifier, nullptr, errorString);
    return static_cast<T&>(item);
  }
  /** Method that user interface should call to change an item, one field at a
   * time.
   *
   * Suited for model/view edition.
   *
   * Actually, this method create a Transaction and calls
   * prepareChangeItem() which is the method to override by document manager
   * implementations.
   * No other class than DtpDocumentManager should override changeItem().
   */
  virtual bool changeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value,
      QString *errorString = nullptr);
  /** Method that user interface or non-interactive code should call to change
   *
   * an item at whole.
   *
   * It is up to the caller to warn the user on error.
   *
   * If oldItem is found in data and newItem.isNull(), delete old item.
   * If oldItem is found in data and newItem is not null, update the item,
   * taking care that old and new item ids may be different is the item is
   * being renamed.
   * If oldItem is not found in data and new item is not null, create it,
   * regardless oldItem is null or garbage or equals to newItem.
   *
   * This method perform change on actual data, while
   * SharedUiItemDocumentManager::changeItem() only perform change in a model
   * data (and thus should not be called directly but rather connected to
   * SharedUiItemDocumentManager::itemChanged() which is emited just after
   * actual data change).
   *
   * Actually, this method create a Transaction and calls
   * prepareChangeItem() which is the method to override by document manager
   * implementations.
   * No other class than DtpDocumentManager should override changeItem().
   */
  virtual bool changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                          QString idQualifier, QString *errorString = nullptr);
  virtual SharedUiItem itemById(QString idQualifier, QString id) const = 0;
  /** Default: parses qualifiedId and calls itemById(QString,QString). */
  virtual SharedUiItem itemById(QString qualifiedId) const;
  /** Convenience template performing downcast. */
  template<class T>
  T itemById(QString idQualifier, QString id) const {
    SharedUiItem item = itemById(idQualifier, id);
    return static_cast<T&>(item);
  }
  /** Convenience template performing downcast. */
  template<class T>
  T itemById(QString qualifiedId) const {
    SharedUiItem item = itemById(qualifiedId);
    return static_cast<T&>(item);
  }
  /** This method build a list of every item currently holded, given their
   * qualifiedId. */
  virtual SharedUiItemList<SharedUiItem> itemsByIdQualifier(
      QString idQualifier) const = 0;
  /** This method build a list of every item currently holded, given their class
   * (T) and qualifiedId. */
  template<class T>
  SharedUiItemList<T> itemsByIdQualifier(QString idQualifier) const {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
    SharedUiItemList<SharedUiItem> list = itemsByIdQualifier(idQualifier);
    if (!list.isEmpty() && list[0].idQualifier() != idQualifier) {
      // LATER output warning
      //qWarning() << "SharedUiItemList<T>::itemsByIdQualifier called with "
      //              "inconsistent types and qualifier";
      return SharedUiItemList<T>();
    }
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
  /** Change items order.
   *
   * Items list may contain a mix of several items type (i.e. with different
   * id qualifiers) and may be partial (i.e. not contains all items of a given
   * type).
   * In most cases items order are not significant but some document managers
   * may keep memory of their order (or of the orders of some item types).
   * Default implementation does nothing.
   */
  virtual void reorderItems(QList<SharedUiItem> items);
  /** This method must be called for every item type the document manager will
   * hold, to enable it to create and modify such items. */
  void registerItemType(QString idQualifier, Setter setter, Creator creator);
  /** Convenience method. */
  void registerItemType(QString idQualifier, Setter setter,
                        SimplestCreator creator) {
    registerItemType(idQualifier, setter, [creator](
                     SharedUiItemDocumentTransaction *, QString id, QString *) {
      return creator(id);
    });
  }
  /** Convenience method. */
  template <class T>
  void registerItemType(QString idQualifier, MemberSetter<T> setter,
                        Creator creator) {
    registerItemType(idQualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, creator);
  }
  /** Convenience method. */
  template <class T>
  void registerItemType(QString idQualifier, MemberSetter<T> setter,
                        SimplestCreator creator) {
    registerItemType(idQualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, [creator](SharedUiItemDocumentTransaction *, QString id, QString *) {
      return creator(id);
    });
  }
  // FIXME doc
  void addForeignKey(QString sourceQualifier, int sourceSection,
                     QString referenceQualifier, int referenceSection = 0,
                     OnChangePolicy onUpdatePolicy = NoAction,
                     OnChangePolicy onDeletePolicy = NoAction);
  // FIXME doc
  void addChangeItemTrigger(QString idQualifier, TriggerFlags flags,
                            ChangeItemTrigger trigger);
  /** Can be called to generate a new id not currently in use for the given
   * idQualifier item type.
   * Generate id of the form prefix+number (e.g. "foobar1"), most of the time
   * one should choose idQualifier as prefix, which is the default (= if prefix
   * is left empty). */
  QString generateNewId(QString idQualifier,
                          QString prefix = QString()) const {
    return generateNewId(nullptr, idQualifier, prefix);
  }

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
   * Subclasses may add specialized signals for their different item
   * types, e.g. "void customerChanged(Customer newItem, Customer oldItem)"
   * where Customer is a SharedUiItem, however using the generic itemChanged()
   * signal is often more powerful and therefore most of the time they should
   * not specialize signals.
   */
  void itemChanged(SharedUiItem newItem, SharedUiItem oldItem,
                   QString idQualifier);
  /** Emited when all data is reset as a whole, for instance when switching
   * from a document to another one. */
  void dataReset();

protected:
  /** Can be called by createNewItem() implementations to generate a new id not
   * currently in use for the given idQualifier item type.
   * Generate id of the form prefix+number (e.g. "foobar1"), most of the time
   * one should choose idQualifier as prefix, which is the default (= if prefix
   * is left empty). */
  QString generateNewId(
      const SharedUiItemDocumentTransaction *transaction, QString idQualifier,
      QString prefix = QString()) const;
  /** Prepare change: ensure that the change can be performed, record them
   * through transaction->changeItem(), and return true only on success.
   *
   * Although changeItem() is more tolerant, this method is not needed to
   * support createOrUpdate or deleteIfExist semantics, since it is called
   * by changeItem() which ensure that only the following parameters conditions
   * can occur for prepareChangeItem():
   * - delete: newItem.isNull() and !oldItem.isNull(), oldItem is a currently
   *     holded data item, not a placeholder only bearing ids.
   * - create: !newItem.isNull() and oldItem.isNull(), there is not currently
   *     any data item matching newItem.qualifiedId()
   * - update (incl. rename): neither is null, oldItem is a currently holded
   *     data item
   *
   * @param errorString is guaranted not to be null
   */
  virtual bool prepareChangeItem(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      SharedUiItem oldItem, QString idQualifier, QString *errorString) = 0;
  /** Perform actual change and emit itemChanged().
   *
   * Default impl only emit signal. Implementations must called base class
   * method after performing actual change.
   *
   * This method is called just after prepareChangeItem() returned true, and
   * whenever the transaction (or associated QUndoCommand) is undone or redone.
   * Since both old and new items are kept in the transaction, commitChangeItem
   * is called with its parameters swapped to undo what has been (re)done.
   *
   * Method commitChangeItem() can even be called several times per
   * prepareChangeItem() call since prepareChangeItem() may add several changes,
   * and constraints (especially foreign keys) or triggers may also add
   * additional changes. */
  virtual void commitChangeItem(SharedUiItem newItem, SharedUiItem oldItem,
                                QString idQualifier);
  // FIXME doc
  void storeItemChange(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      SharedUiItem oldItem, QString idQualifier) {
    transaction->storeItemChange(newItem, oldItem, idQualifier);
  }

  /** To be called by createNewItem().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual SharedUiItemDocumentTransaction *internalCreateNewItem(
      SharedUiItem *newItem, QString idQualifier, PostCreationModifier modifier,
      QString *errorString);
  /** To be called by changeItem().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual SharedUiItemDocumentTransaction *internalChangeItem(
      SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
      QString *errorString);
  /** To be called by changeItemByUiData().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual SharedUiItemDocumentTransaction *internalChangeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value,
      QString *errorString);

private:
  // FIXME doc
  bool processConstraintsAndPrepareChangeItem(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      SharedUiItem oldItem, QString idQualifier, QString *errorString);
  // FIXME doc×6
  bool processBeforeUpdate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
      SharedUiItem oldItem, QString idQualifier, QString *errorString);
  bool processBeforeCreate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
      QString idQualifier, QString *errorString);
  bool processBeforeDelete(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem,
      QString idQualifier, QString *errorString);
  bool processAfterUpdate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      SharedUiItem oldItem, QString idQualifier, QString *errorString);
  bool processAfterCreate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      QString idQualifier, QString *errorString);
  bool processAfterDelete(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem,
      QString idQualifier, QString *errorString);
  // id as primary key (uniqueness, not empty, same qualifiers before and after)
  bool checkIdsConstraints(SharedUiItemDocumentTransaction *transaction,
                           SharedUiItem newItem, SharedUiItem oldItem,
                           QString idQualifier, QString *errorString);
  /** Perform checks that are delayed until commit, such as referential
   * integrity checks. */
  bool delayedChecks(SharedUiItemDocumentTransaction *transaction,
                     QString *errorString);

  friend class SharedUiItemDocumentTransaction; // needed for many methods and fields
  friend class SharedUiItemDocumentTransaction::ChangeItemCommand; // needed to call back commitChangeItem()
  friend class DtpDocumentManagerWrapper; // needed to wrapp internalChangeItem
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SharedUiItemDocumentManager::TriggerFlags)

// LATER Q_DECLARE_TYPEINFO(SharedUiItemDocumentManager::ForeignKey, Q_MOVABLE_TYPE);

#endif // SHAREDUIITEMDOCUMENTMANAGER_H
