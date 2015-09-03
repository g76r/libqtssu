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
#include "inmemorydatabasedocumentmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QtDebug>

InMemoryDatabaseDocumentManager::InMemoryDatabaseDocumentManager(QObject *parent)
  : InMemorySharedUiItemDocumentManager(parent) {
}

InMemoryDatabaseDocumentManager::InMemoryDatabaseDocumentManager(
    QSqlDatabase db, QObject *parent)
  : InMemorySharedUiItemDocumentManager(parent), _db(db) {
}

bool InMemoryDatabaseDocumentManager::registerItemType(
    QString idQualifier, Setter setter, Creator creator, int idSection,
    QString *errorString) {
  QString reason;
  InMemorySharedUiItemDocumentManager::registerItemType(
        idQualifier, setter, creator);
  _idSections.insert(idQualifier, idSection);
  if (!createTableAndSelectData(idQualifier, setter, creator, idSection,
                                &reason)) {
    if (errorString)
      *errorString = reason;
    qWarning() << "InMemoryDatabaseDocumentManager" << reason;
    return false;
  }
  return true;
}

bool InMemoryDatabaseDocumentManager::prepareChangeItem(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  Q_ASSERT(errorString != 0);
  if (!changeItemInDatabase(newItem, oldItem, idQualifier, errorString, true)) {
    qDebug() << "InMemoryDatabaseDocumentManager::prepareChangeItem: "
                "test transaction failed:" << *errorString;
    return false;
  }
  transaction->changeItem(newItem, oldItem, idQualifier);
  return true;
}

void InMemoryDatabaseDocumentManager::commitChangeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier) {
  QString errorString;
  if (!changeItemInDatabase(newItem, oldItem, idQualifier, &errorString,
                            false)) {
    // this should only occur on sever technical error (filesystem full,
    // network connection to the database lost, etc.)
    qWarning() << "InMemoryDatabaseDocumentManager cannot write to database "
                  "prepared change:" << newItem << oldItem << ":"
               << errorString;
  } else {
    //qDebug() << "InMemoryDatabaseDocumentManager::commitChangeItem"
    //         << newItem << oldItem;
    InMemorySharedUiItemDocumentManager::commitChangeItem(
          newItem, oldItem, idQualifier);
  }
}

bool InMemoryDatabaseDocumentManager::changeItemInDatabase(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
    QString *errorString, bool dryRun) {
  Q_ASSERT(errorString != 0);
  Q_ASSERT(!newItem.isNull() || !oldItem.isNull());
  if (!_db.transaction()) {
    *errorString = "database error: cannot start transaction "
        +_db.lastError().text();
    return false;
  }
  if (!oldItem.isNull()) {
    QSqlQuery query(_db);
    query.prepare("delete from "+idQualifier+" where "
                  +protectedColumnName(oldItem.uiHeaderString(
                                         _idSections.value(idQualifier)))
                  +" = ?");
    query.bindValue(0, oldItem.id());
    if (!query.exec()) {
      *errorString = "database error: cannot delete from table "+idQualifier+" "
          +oldItem.id()+" "+query.lastError().text()+" "+query.executedQuery();
      goto failed;
    }
  }
  if (!newItem.isNull() && !insertItemInDatabase(newItem, errorString)) {
    if (!_db.rollback()) {
      qDebug() << "InMemoryDatabaseDocumentManager database error: cannot "
                  "rollback transaction" << _db.lastError().text();
    }
    goto failed;
  }
  if (dryRun) {
    if (!_db.rollback()) {
      qDebug() << "InMemoryDatabaseDocumentManager database error: cannot "
                  "rollback transaction" << _db.lastError().text();
      return false;
    }
  } else {
    if (!_db.commit()) {
      *errorString = "database error: cannot commit transaction "
          +_db.lastError().text();
      goto failed;
    }
  }
  return true;
failed:;
  _db.rollback();
  qWarning() << "InMemoryDatabaseDocumentManager" << *errorString;
  return false;
}

bool InMemoryDatabaseDocumentManager::insertItemInDatabase(
    SharedUiItem newItem, QString *errorString) {
  Q_ASSERT(errorString != 0);
  Creator creator = _creators.value(newItem.idQualifier());
  if (newItem.isNull() || !creator) {
    *errorString = "cannot insert into database item: "+newItem.qualifiedId()
        +" creator: "+!!creator;
    return false;
  }
  QString idQualifier = newItem.idQualifier();
  QStringList columnNames, protectedColumnNames, placeholders;
  SharedUiItem item = creator(QStringLiteral("dummy"));
  for (int i = 0; i < item.uiSectionCount(); ++i) {
    QString headerName = item.uiHeaderString(i);
    columnNames << headerName;
    protectedColumnNames << protectedColumnName(headerName);
    placeholders << QStringLiteral("?");
  }
  QSqlQuery query(_db);
  query.prepare("insert into "+idQualifier+" ("+protectedColumnNames.join(',')
                +") values ("+placeholders.join(',')+") ");
  for (int i = 0; i < newItem.uiSectionCount(); ++i)
    query.bindValue(i, newItem.uiData(i, SharedUiItem::ExternalDataRole));
  if (!query.exec()) {
    *errorString = "database error: cannot insert into table "+idQualifier+" "
        +newItem.id()+" "+query.lastError().text();
    qDebug() << "InMemoryDatabaseDocumentManager" << *errorString;
    return false;
  }
  return true;
}

bool InMemoryDatabaseDocumentManager::setDatabase(
    QSqlDatabase db, QString *errorString) {
  _repository.clear();
  _db = db;
  bool successful = true;
  QString reason;
  foreach (const QString &idQualifier, _idSections.keys()) {
    QString oneReason;
    if (!createTableAndSelectData(
          idQualifier, _setters.value(idQualifier),
          _creators.value(idQualifier), _idSections.value(idQualifier),
          &oneReason)) {
      successful = false;
      if (!reason.isEmpty())
        reason = reason+"\n"+oneReason;
      else
        reason = oneReason;
    }
  }
  if (!successful && errorString)
    *errorString = reason;
  return successful;
}

bool InMemoryDatabaseDocumentManager::createTableAndSelectData(
    QString idQualifier, Setter setter, Creator creator, int idSection,
    QString *errorString) {
  Q_ASSERT(errorString != 0);
  Q_UNUSED(idSection)
  Q_ASSERT_X((creator && setter),
             "InMemoryDatabaseDocumentManager::createTableAndSelectData",
             "invalid parameters");
  QStringList columnNames, protectedColumnNames;
  SharedUiItem item = creator(QStringLiteral("dummy"));
  for (int i = 0; i < item.uiSectionCount(); ++i) {
    QString headerName = item.uiHeaderString(i);
    columnNames << headerName;
    protectedColumnNames << protectedColumnName(headerName);
  }
  QSqlQuery query(_db);
  query.exec("select count(*) from "+idQualifier);
  if (query.lastError().type() != QSqlError::NoError) {
    QString q = "create table "+idQualifier+" ( ";
    for (int i = 0; i < protectedColumnNames.size(); ++ i) {
      const QString &columnName = protectedColumnNames[i];
      if (i)
        q += ", ";
      q = q+columnName+" text"; // LATER use a more portable text data type
    }
    q += " )";
    query.exec(q);
    if (query.lastError().type() != QSqlError::NoError) {
      *errorString = "database error: cannot create table: "+idQualifier
          +query.lastError().text();
      return false;
    }
  }
  // TODO alter table, if needed, @see QSqlDatabase::record()
  /*
sqlite> alter table connection rename to foo;
sqlite> create table connection as select Id,Url,Login,Password,'' as Proxy_Id from foo;
sqlite> .h on
sqlite> select * from connection;
Id|URL|Login|Password|Proxy_Id
connection4||||
sqlite> drop table foo;
   */
  query.exec("select "+protectedColumnNames.join(',')+" from "+idQualifier);
  if (query.lastError().type() != QSqlError::NoError) {
    *errorString = "database error: cannot select from table: "+idQualifier
        +query.lastError().text();
    return false;
  }
  //qDebug() << "***** selected:" << query.executedQuery();
  SharedUiItemDocumentTransaction transaction(this);
  while (query.next()) {
    item = creator(QStringLiteral("dummy"));
    for (int i = 0; i < item.uiSectionCount(); ++i) {
      QString errorString;
      bool ok = setter(&item, i, query.value(i), &errorString, &transaction,
                       SharedUiItem::ExternalDataRole);
      if (!ok) {
        // TODO do not log this
        qDebug() << "InMemoryDatabaseDocumentManager cannot set value for item"
                 << item.qualifiedId() << errorString;
      }
    }
    //qDebug() << "  have item:" << item.qualifiedId();
    InMemorySharedUiItemDocumentManager::commitChangeItem(
          item, SharedUiItem(), idQualifier);
  }
  return true;
}

static QRegularExpression unallowedColumnCharsSequence {
  "(^[^a-zA-Z_]+)|([^a-zA-Z0-9_]+)" };

QString InMemoryDatabaseDocumentManager::protectedColumnName(QString columnName) {
  return columnName.replace(unallowedColumnCharsSequence,
                            QStringLiteral("_"));
}
