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
#ifndef STRINGSPARAMSPROVIDER_H
#define STRINGSPARAMSPROVIDER_H

#include "paramsprovider.h"
#include <QStringList>

/** ParamsProvider evaluating numerical params name among a string list.
 *
 * %1 is replaced by first string and so on. %0 or numbers beyond the size of
 * the strings list are invalid. */
class LIBPUMPKINSHARED_EXPORT StringsParamsProvider : public ParamsProvider {
  QStringList _strings;

public:
  StringsParamsProvider(QStringList strings) : _strings(strings) { }
  StringsParamsProvider(QString string) : _strings(string) { }
  StringsParamsProvider() { }
  QStringList strings() const { return _strings; }
  void setStrings(QStringList strings) { _strings = strings; }
  QVariant paramValue(QString key, const ParamsProvider *context = 0,
                      QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()
          ) const override;
};

#endif // STRINGSPARAMSPROVIDER_H
