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
#include "settingsbinder.h"

#include <QMetaMethod>
#include <QJsonObject>
#include <QStack>
#include "settingsbinderhelper.h"

struct SettingsBinderPrivate {
//    QMap<QString, QPair<QObject*, QMetaProperty>> bindings;
    QMap<QString, QPair<std::function<void (QJsonValue)> /* Load function */, std::function<QJsonValue()> /* Save function */>> bindings;
};

SettingsBinder::SettingsBinder(QObject* parent) : QObject(parent) {
    d = new SettingsBinderPrivate;
}

SettingsBinder::~SettingsBinder() {
    delete d;
}

void SettingsBinder::bind(QObject* object, const char* property, QString jsonElement) {
    const QMetaObject* meta = object->metaObject();
    QMetaProperty prop = meta->property(meta->indexOfProperty(property));

    connect(object, prop.notifySignal(), this, this->metaObject()->method(this->metaObject()->indexOfSignal("shouldSave()")));

    d->bindings.insert(jsonElement, {[ = ](QJsonValue value) {
        prop.write(object, value);
    }, [ = ]() {
        return QJsonValue::fromVariant(prop.read(object));
    }
        });
}

void SettingsBinder::bind(std::function<void (QJsonValue)> load, std::function<QJsonValue ()> save, QString jsonElement) {
    d->bindings.insert(jsonElement, {load, save});
}

void SettingsBinder::load(QJsonObject object) {
    for (QString binding : d->bindings.keys()) {
        QStringList elements = binding.split(".");

        std::function<void (QJsonValue)> load = d->bindings.value(binding).first;

        QJsonObject partObject = object;
        while (!elements.isEmpty()) {
            QString part = elements.takeFirst();
            if (elements.isEmpty()) {
                load(partObject.value(part));
            } else {
                partObject = partObject.value(part).toObject();
            }
        }
    }
}

QJsonObject SettingsBinder::save() {
    QJsonObject object;
    for (QString binding : d->bindings.keys()) {
        QStringList elements = binding.split(".");

        std::function<QJsonValue()> save = d->bindings.value(binding).second;

        //First, read all the objects
        QStack<QJsonObject> parts;
        parts.push(object);
        while (!elements.isEmpty()) {
            QString part = elements.takeFirst();
            if (elements.isEmpty()) {
                parts.top().insert(part, save());
            } else {
                if (parts.top().contains(part)) {
                    parts.push(parts.top().value(part).toObject());
                } else {
                    parts.push(QJsonObject());
                }
            }
        }

        //Now go back and save each property
        elements = binding.split(".");
        elements.takeLast(); //Remove the actual property name
        while (parts.length() > 1) {
            QString partName = elements.takeLast();
            QJsonObject part = parts.pop();
            parts.top().insert(partName, part);
        }
        object = parts.pop();
    }
    return object;
}
