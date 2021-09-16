/****************************************
 *
 *   INSERT-PROJECT-NAME-HERE - INSERT-GENERIC-NAME-HERE
 *   Copyright (C) 2021 Victor Tran
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * *************************************/
#ifndef SETTINGSBINDER_H
#define SETTINGSBINDER_H

#include <QObject>

struct SettingsBinderPrivate;
class SettingsBinder : public QObject {
        Q_OBJECT
    public:
        explicit SettingsBinder(QObject* parent = nullptr);
        ~SettingsBinder();

//        void bind(QObject* object, QString property, std::function<QVariant(QJsonObject)> load, std::function<void(QVariant, QJsonObject&)> save);
        void bind(QObject* object, const char* property, QString jsonElement);
        void bind(std::function<void(QJsonValue)> load, std::function<QJsonValue()> save, QString jsonElement);

        void load(QJsonObject object);
        QJsonObject save();

    signals:
        void shouldSave();

    private:
        SettingsBinderPrivate* d;
};

#endif // SETTINGSBINDER_H
