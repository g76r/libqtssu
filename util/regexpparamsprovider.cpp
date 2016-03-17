/* Copyright 2015-2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#include "regexpparamsprovider.h"
#include <QRegularExpression>

static QRegularExpression integerRE("\\A\\d+\\z");

QVariant RegexpParamsProvider::paramValue(
    QString key, QVariant defaultValue, QSet<QString> alreadyEvaluated) const {
  Q_UNUSED(alreadyEvaluated)
  QString value = _match.captured(key);
  if (!value.isNull())
    return value;
  QRegularExpressionMatch match = integerRE.match(key);
  if (match.hasMatch()) {
    value = _match.captured(match.captured().toInt());
    if (!value.isNull())
      return value;
  }
  return defaultValue;
}
