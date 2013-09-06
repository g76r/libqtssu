/* Copyright 2013 Hallowyn and others.
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
#ifndef AUTHORIZER_H
#define AUTHORIZER_H

#include "libqtssu_global.h"
#include "usersdatabase.h"
#include <QDateTime>
#include <QObject>

/** Authorization service interface */
class LIBQTSSUSHARED_EXPORT Authorizer : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Authorizer)
  UsersDatabase *_usersDatabase;
  bool _ownUsersDatabase;

public:
  explicit Authorizer(QObject *parent = 0);
  ~Authorizer();
  /** Test if a given user is authorized to perform a given action scope
   * (e.g. "delete" or "modify.delete") on a given data scope (e.g.
   * "business.accounting.invoices") at a given time.
   * Of course there can be authorization definitions that ignore some of the
   * criteria (e.g. that only check the actionScope).
   * This method is thread-safe */
  virtual bool authorizeUserData(UserData user, QString actionScope,
                                 QString dataScope = QString(),
                                 QDateTime timestamp = QDateTime()) const = 0;
  /** Same as previous, using the users database to resolve UserData from
   * userId. Always return false if the users database is not set.
   * This method is thread-safe */
  virtual bool authorize(QString userId, QString actionScope,
                         QString dataScope = QString(),
                         QDateTime timestamp = QDateTime()) const;
  Authorizer &setUsersDatabase(UsersDatabase *db, bool takeOwnership);
};

#endif // AUTHORIZER_H