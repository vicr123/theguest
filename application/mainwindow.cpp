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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <tcsdtools.h>
#include <thelpmenu.h>
#include <tinputdialog.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "guestitem.h"
#include "guestmanager.h"

struct MainWindowPrivate {
    tCsdTools csd;
    QList<GuestItem*> guestItems;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    d = new MainWindowPrivate();
    d->csd.installMoveAction(ui->topWidget);
    d->csd.installResizeAction(this);

    if (tCsdGlobal::windowControlsEdge() == tCsdGlobal::Left) {
        ui->leftCsdLayout->addWidget(d->csd.csdBoxForWidget(this));
    } else {
        ui->rightCsdLayout->addWidget(d->csd.csdBoxForWidget(this));
    }

    QMenu* menu = new QMenu(this);

    menu->addMenu(new tHelpMenu(this));
    menu->addAction(ui->actionExit);

    ui->menuButton->setIconSize(SC_DPI_T(QSize(24, 24), QSize));
    ui->menuButton->setMenu(menu);

    ui->titleLabel->setDrawBottomBorder(false);

    connect(GuestManager::instance(), &GuestManager::guestsUpdated, this, &MainWindow::loadGuests);
    this->loadGuests();
}

MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::on_actionExit_triggered() {

}


void MainWindow::on_addVmButton_clicked() {
    bool ok;
    QString name = tInputDialog::getText(this, tr("Create Virtual Machine"), tr("What do you want to call this Virtual Machine?"), QLineEdit::Normal, "", &ok);
    if (ok) {
        QFile defaultDefinitionFile(":/default-machine-definition.json");
        defaultDefinitionFile.open(QFile::ReadOnly);

        QJsonObject definition = QJsonDocument::fromJson(defaultDefinitionFile.readAll()).object();
        defaultDefinitionFile.close();

        definition.insert("name", name);

        QString backendName = GuestManager::instance()->suggestBackendName(name);
        GuestManager::instance()->setGuestSettings(backendName, definition);
    }
}

void MainWindow::loadGuests() {
    for (GuestItem* item : d->guestItems) {
        ui->vmLayout->removeWidget(item);
        item->deleteLater();
    }
    d->guestItems.clear();

    for (QString guest : GuestManager::instance()->guestList()) {
        GuestItem* item = new GuestItem(guest);
        ui->vmLayout->addWidget(item);
        d->guestItems.append(item);
    }
}

