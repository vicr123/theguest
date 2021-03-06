
/* http://www.spice-space.org/page/Whiteboard/AgentProtocol#Clipboard
 * To do so, the guest agent and the client play a symmetric role:
 * they can both claim ownership (GRAB), RELEASE ownership,
 * REQUEST clipboard data and send CLIPBOARD data. For example,
 * the GRAB message is sent after receiving a system notification of clipboard data
 * available after a Copy operation in some application.
 * When the clipboard is emptied, the grab must be RELEASEd. The other side can REQUEST
 * the data while the GRAB is active, and should expect a CLIPBOARD reply with the data.
 * Important:
 * If a GRAB message has been sent and is currently active,
 * then a successive GRAB message is received from the peer,
 * no RELEASE message should be sent to the peer for the previous active grab.
 * It has been implicitly released by the peer. Sending an extra RELEASE message
 * would only confuse the peer.
 * ------------------------------------------------------------------------------------
 * Then will implemented the copy/paste cross blocking.
 */

#include <spice-client.h>
#include <spice/vd_agent.h>
#include "qspice-widget.h"
#include "qspice-smartcard-widget.h"
#include "qspice-usbdevice-widget.h"

#include <QApplication>
#include <QMouseEvent>
#include <QTime>
#include <QDate>
#include <QFileDialog>
#include <QPainter>
#include <QMessageBox>
#include <QShortcut>

#define MARGIN 0

QSpiceWidget::QSpiceWidget(QWidget* parent) :
    QWidget(parent) {
    spiceSession = new QSpiceSession(this);
    display = Q_NULLPTR;
    inputs = Q_NULLPTR;
    cursor = Q_NULLPTR;
    smartcard = Q_NULLPTR;
    usbredir = Q_NULLPTR;
    webdav = Q_NULLPTR;
    playback = Q_NULLPTR;
    usbDevManager = Q_NULLPTR;
    smartcardManager = Q_NULLPTR;
    spiceAudio = Q_NULLPTR;
    _width = 0;
    _height = 0;
    init_h = 0;
    init_w = 0;
    d_X = d_Y = 0;
    zoom = 1.0;
    downloadProgress = 0;
    scaled = false;
    is_FullScreen = false;
    captured = false;
    prepareRelease = false;
    blockMouse = false;

    tr_mode = Qt::SmoothTransformation;
    img = Q_NULLPTR;

    resizeTimer.setSingleShot( true );
    connect(&resizeTimer, SIGNAL(timeout()), SLOT(resizeDone()));

    connect(spiceSession, SIGNAL(channelInited(QSpiceChannel*)),
        SLOT(setChannel(QSpiceChannel*)));
    connect(spiceSession, SIGNAL(channelDestroyed(int)),
        SLOT(obstructChannel(int)));
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    installEventFilter( this );
    setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

    QShortcut* releaseCaptureShortcut = new QShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_Alt), this);
    connect(releaseCaptureShortcut, &QShortcut::activated, this, [ = ] {
        this->releaseMouse();
    });
}
QSpiceWidget::~QSpiceWidget() {
    // set NULL for drop signals from
    // display-channel with changes data
    if ( display != Q_NULLPTR ) {
        display->setParentWidget(Q_NULLPTR);
    };
    disconnectFromSpiceSource();
}

bool QSpiceWidget::connectToSpiceSource(const QUrl& uri, const QString& passwd) {
    spiceSession->setPassword(passwd);
    spiceSession->setUri(uri.toString());
    return spiceSession->openSession();
}

bool QSpiceWidget::connectToSpiceSource(const QString& unixPath, const QString& passwd) {
    spiceSession->setPassword(passwd);
    spiceSession->setUnixPath(unixPath);
    return spiceSession->openSession();
}

void QSpiceWidget::disconnectFromSpiceSource() {
    spiceSession->closeSession();
}

void QSpiceWidget::sendKeySequience(Qt::Key key) {
    sendKeySequence(key);
}

void QSpiceWidget::sendKeySequence(Qt::Key key) {
    if ( Q_NULLPTR != inputs ) {
        // inputs->inputsKeyPressAndRelease()
        // does not give the desired effect

        QList<Qt::Key> keys;
        if (key & Qt::ControlModifier) keys.append(Qt::Key_Control);
        if (key & Qt::AltModifier) keys.append(Qt::Key_Alt);
        if (key & Qt::ShiftModifier) keys.append(Qt::Key_Shift);
        if (key & Qt::MetaModifier) keys.append(Qt::Key_Meta);
        keys.append(static_cast<Qt::Key>(key & (~Qt::ControlModifier & ~Qt::AltModifier & ~Qt::ShiftModifier & ~Qt::MetaModifier)));

        for (auto press = keys.begin(); press != keys.end(); press++) {
            inputs->inputsQKeyPress(*press);
        }
        for (auto release = keys.rbegin(); release != keys.rend(); release++) {
            inputs->inputsQKeyRelease(*release);
        }
    };
}

void QSpiceWidget::fileCopyAsync(QStringList& fileNames) {
    if ( Q_NULLPTR != main ) {
        main->fileCopyAsync(fileNames);
    };
}

void QSpiceWidget::cancelFileCopyAsync() {
    if ( Q_NULLPTR != main ) {
        main->cancelFileCopyAsync();
    };
}

void QSpiceWidget::copyToClipboardFromGuest() {
    if ( Q_NULLPTR != main ) {
        emit copyPasteStateChanged(false);
        main->initClipboardSelectionRequestOnHost();
    };
}

void QSpiceWidget::pasteClipboardToGuest() {
    if ( Q_NULLPTR != main ) {
        emit copyPasteStateChanged(false);
        main->initClipboardSelectionRequestOnGuest();
    };
}

void QSpiceWidget::sendClipboardDataToGuest(uint selection, quint32 type, const uchar* _data, long _size) {
    if ( Q_NULLPTR != main ) {
        main->clipboardSelectionNotify(selection, type, _data, _size);
    };
    emit copyPasteStateChanged(true);
}

bool QSpiceWidget::isScaledScreen() const {
    return scaled;
}

bool QSpiceWidget::isConnectedWithDisplay() const {
    if ( display == Q_NULLPTR ) return false;
    return display->isConnected();
}

/* private slots */
void QSpiceWidget::setChannel(QSpiceChannel* channel) {
    QSpiceMainChannel* _main =
        dynamic_cast<QSpiceMainChannel*>(channel);
    if (_main) {
        main = _main;
        connect(main, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(main, SIGNAL(agentUpdated()),
            SLOT(mainAgentUpdate()));
        connect(main, SIGNAL(guestClipboardSelectionReceived(uint, void*, uint)),
            SLOT(pasteClipboardSelectionFromGuest(uint, void*, uint)));
        //connect(main, SIGNAL(clipboardSelectionGrabbed(uint,void*,uint)),
        //        SLOT(clipboardSelectionGrab()));
        connect(main, SIGNAL(guestClipboardSelectionReleased(uint)),
            SLOT(releaseClipboardSelection(uint)));
        connect(main, SIGNAL(clipboardSelectionRequestedFromGuest(uint, uint)),
            SLOT(checkGuestClipboardSelectionRequest(uint, uint)));
        connect(main, SIGNAL(mouseUpdated()),
            SLOT(mainMouseUpdate()));
        connect(main, SIGNAL(downloaded(qint64, qint64)),
            this, SLOT(setDownloadProgress(qint64, qint64)));
        connect(main, SIGNAL(cancelled()),
            this, SIGNAL(fileTransferIsCancelled()));
        connect(main, SIGNAL(downloadCompleted()),
            this, SIGNAL(fileTransferIsCompleted()));
        connect(main, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        main->connectToChannel();
        return;
    }

    QSpiceDisplayChannel* _display =
        dynamic_cast<QSpiceDisplayChannel*>(channel);
    if (_display) {
        display = _display;
        display->setParentWidget(static_cast<void*>(this));
        connect(display, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(display,
            SIGNAL(displayPrimaryCreated(int, int, int, int, int, void*)),
            SLOT(displayPrimaryCreate(int, int, int, int, int, void*)));
        connect(display, SIGNAL(displayInvalidated(int, int, int, int)),
            SLOT(displayInvalidate(int, int, int, int)));
        connect(display, SIGNAL(displayPrimaryDestroyed()),
            SLOT(displayPrimaryDestroy()));
        connect(display, SIGNAL(displayMarked(int)),
            SLOT(displayMark(int)));
        connect(display, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));

        bool online = display->connectToChannel();
        emit displayChannelChanged(online);
        return;
    }

    QSpiceInputsChannel* _inputs =
        dynamic_cast<QSpiceInputsChannel*>(channel);
    if (_inputs) {
        inputs = _inputs;
        connect(inputs, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(inputs, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        bool online = inputs->connectToChannel();
        emit inputsChannelChanged(online);
        return;
    }

    QSpiceCursorChannel* _cursor =
        dynamic_cast<QSpiceCursorChannel*>(channel);
    if (_cursor) {
        cursor = _cursor;
        connect(cursor, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(cursor, SIGNAL(cursorData(int, int, int, int, void*)),
            SLOT(setClientCursor(int, int, int, int, void*)));
        connect(cursor, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        bool online = cursor->connectToChannel();
        emit cursorChannelChanged(online);
        return;
    }

    QSpiceSmartcardChannel* _smartcard =
        dynamic_cast<QSpiceSmartcardChannel*>(channel);
    if (_smartcard) {
        smartcard = _smartcard;
        connect(smartcard, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(smartcard, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        bool online = false;
#if WITH_LIBCACARD
        online = smartcard->connectToChannel();
        //qDebug()<<"WITH_LIBCACARD";
#else
        //qDebug()<<"NOT WITH_LIBCACARD";
#endif
        if ( online ) {
            smartcardManager = new QSpiceSmartcardManager(this);
            if ( smartcardManager ) {
                connect(smartcardManager, SIGNAL(cardInserted(QString&)),
                    this, SLOT(cardInserted(QString&)));
                connect(smartcardManager, SIGNAL(cardRemoved(QString&)),
                    this, SLOT(cardRemoved(QString&)));
                connect(smartcardManager, SIGNAL(readerAdded(QString&)),
                    this, SLOT(readerAdded(QString&)));
                connect(smartcardManager, SIGNAL(readerRemoved(QString&)),
                    this, SLOT(readerRemoved(QString&)));
            };
        };
        emit smartcardChannelChanged(online);
        return;
    }

    QSpiceUSBRedirChannel* _usbredir =
        dynamic_cast<QSpiceUSBRedirChannel*>(channel);
    if (_usbredir) {
        usbredir = _usbredir;
        connect(usbredir, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(usbredir, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        bool online = usbredir->connectToChannel();
        if ( online ) {
            usbDevManager = new QSpiceUsbDeviceManager(this, spiceSession);
            if ( usbDevManager ) {
                connect(usbDevManager, SIGNAL(autoConnectFailed(QString&, QString&)),
                    this, SLOT(usbDevAutoConnectFailed(QString&, QString&)));
                connect(usbDevManager, SIGNAL(deviceAdded(QString&)),
                    this, SLOT(usbDevAdded(QString&)));
                connect(usbDevManager, SIGNAL(deviceInfo(QString&, QString&)),
                    this, SLOT(usbDevError(QString&, QString&)));
                connect(usbDevManager, SIGNAL(deviceRemoved(QString&)),
                    this, SLOT(usbDevRemoved(QString&)));
            };
        };
        emit usbredirChannelChanged(online);
        return;
    }

    QSpiceWebDAVChannel* _webdav =
        dynamic_cast<QSpiceWebDAVChannel*>(channel);
    if (_webdav) {
        webdav = _webdav;
        connect(webdav, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(webdav, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        bool online = webdav->connectToChannel();
        emit webdavChannelChanged(online);
        return;
    }

    QSpicePlaybackChannel* _playback =
        dynamic_cast<QSpicePlaybackChannel*>(channel);
    if (_playback) {
        playback = _playback;
        connect(main, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(playback, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        bool online = false;
#if USE_SPICE_AUDIO
        online = playback->connectToChannel();
        //qDebug()<<"USE_SPICE_AUDIO";
#else
        //qDebug()<<"NOT USE_SPICE_AUDIO";
#endif
        if ( online && !spiceAudio ) {
            spiceAudio = new QSpiceAudio(
                this,
                static_cast<SpiceSession*>(spiceSession->gobject));
            //qDebug()<<"spiceAudio is associated:"<<spiceAudio->isAssociated();
            if ( spiceAudio->isAssociated() ) {
                // reserved for some work
            };
        };
        emit playbackChannelChanged(online);
        return;
    }

    QSpiceRecordChannel* _record =
        dynamic_cast<QSpiceRecordChannel*>(channel);
    if (_record) {
        record = _record;
        connect(main, SIGNAL(channelEvent(int)),
            this, SLOT(channelEvent(int)));
        connect(record, SIGNAL(channelMsg(SPICE_CHANNEL_MSG&)),
            this, SLOT(formatMsg(SPICE_CHANNEL_MSG&)));
        bool online = false;
#if USE_SPICE_AUDIO
        online = record->connectToChannel();
        //qDebug()<<"USE_SPICE_AUDIO";
#else
        //qDebug()<<"NOT USE_SPICE_AUDIO";
#endif
        if ( online && !spiceAudio ) {
            spiceAudio = new QSpiceAudio(
                this,
                static_cast<SpiceSession*>(spiceSession->gobject));
            //qDebug()<<"spiceAudio is associated:"<<spiceAudio->isAssociated();
            if ( spiceAudio->isAssociated() ) {
                // reserved for some work
            };
        };
        emit recordChannelChanged(online);
        return;
    }

}

void QSpiceWidget::obstructChannel(int channelType) {
    switch(channelType) {
        case SPICE_CHANNEL_MAIN:
            delete main;
            main = Q_NULLPTR;
            break;

        case SPICE_CHANNEL_DISPLAY:
            // set NULL for drop signals from
            // display-channel with changes data
            if ( display != Q_NULLPTR )
                display->setParentWidget(Q_NULLPTR);
            delete display;
            display = Q_NULLPTR;
            emit displayChannelChanged(false);
            break;

        case SPICE_CHANNEL_INPUTS:
            delete inputs;
            inputs = Q_NULLPTR;
            emit inputsChannelChanged(false);
            break;

        case SPICE_CHANNEL_CURSOR:
            setCursor(Qt::ArrowCursor);
            delete cursor;
            cursor = Q_NULLPTR;
            emit cursorChannelChanged(false);
            break;

        case SPICE_CHANNEL_SMARTCARD:
            if (smartcardManager) {
                smartcardManager->deleteLater();
            };
            delete smartcard;
            smartcard = Q_NULLPTR;
            emit smartcardChannelChanged(false);
            break;

        case SPICE_CHANNEL_USBREDIR:
            if ( usbDevManager ) {
                usbDevManager->deleteLater();
            };
            delete usbredir;
            usbredir = Q_NULLPTR;
            emit usbredirChannelChanged(false);
            break;

        case SPICE_CHANNEL_WEBDAV:
            delete webdav;
            webdav = Q_NULLPTR;
            emit webdavChannelChanged(false);
            break;

        case SPICE_CHANNEL_PLAYBACK:
            delete playback;
            playback = Q_NULLPTR;
            emit playbackChannelChanged(false);
            if ( spiceAudio ) {
                spiceAudio->deleteLater();
            };
            break;

        case SPICE_CHANNEL_RECORD:
            delete record;
            record = Q_NULLPTR;
            emit recordChannelChanged(false);
            if ( spiceAudio ) {
                spiceAudio->deleteLater();
            };
            break;

        default:
            break;
    }

}

void QSpiceWidget::mainAgentUpdate() {
    //qDebug()<<"main: AgentUpdate";
}

void QSpiceWidget::pasteClipboardSelectionFromGuest(uint type, void* _data, uint _size) {
    //qDebug()<<"main: ClipboardSelection";
    QImage _img;
    bool res;
    switch (type) {
        case VD_AGENT_CLIPBOARD_NONE:
            break;
        case VD_AGENT_CLIPBOARD_UTF8_TEXT:
            QApplication::clipboard()->setText(
                QString::fromUtf8(static_cast<char*>(_data), int(_size)));
            break;
        case VD_AGENT_CLIPBOARD_IMAGE_PNG:
            res = _img.loadFromData(static_cast<uchar*>(_data), int(_size), "PNG");
            if (res) QApplication::clipboard()->setImage(_img);
            break;
        case VD_AGENT_CLIPBOARD_IMAGE_BMP:
            res = _img.loadFromData(static_cast<uchar*>(_data), int(_size), "BMP");
            if (res) QApplication::clipboard()->setImage(_img);
            break;
        case VD_AGENT_CLIPBOARD_IMAGE_JPG:
            res = _img.loadFromData(static_cast<uchar*>(_data), int(_size), "JPG");
            if (res) QApplication::clipboard()->setImage(_img);
            break;
        case VD_AGENT_CLIPBOARD_IMAGE_TIFF:
            res = _img.loadFromData(static_cast<uchar*>(_data), int(_size), "TIFF");
            if (res) QApplication::clipboard()->setImage(_img);
            break;
        default:
            break;
    };
    emit copyPasteStateChanged(true);
}

void QSpiceWidget::clipboardSelectionGrab() {
    //qDebug()<<"main: ClipboardSelectionGrabbed";
}

void QSpiceWidget::releaseClipboardSelection(uint selection) {
    switch (selection) {
        case VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD:
            //qDebug()<<"guestClipboard_CLIPBOARDSelectionReleased";
            break;
        case VD_AGENT_CLIPBOARD_SELECTION_PRIMARY:
            //qDebug()<<"guestClipboard_PRIMARYSelectionReleased";
            break;
        case VD_AGENT_CLIPBOARD_SELECTION_SECONDARY:
            //qDebug()<<"guestClipboard_SECONDARYSelectionReleased";
            break;
        default:
            break;
    };
    emit copyPasteStateChanged(true);
}

QClipboard::Mode QSpiceWidget::selectionToClipboardMode(uint selection) {
    QClipboard::Mode mode;
    switch (selection) {
        case VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD:
            mode = QClipboard::Clipboard;
            break;
        case VD_AGENT_CLIPBOARD_SELECTION_PRIMARY:
            mode = QClipboard::Selection;
            break;
        case VD_AGENT_CLIPBOARD_SELECTION_SECONDARY:
            mode = QClipboard::FindBuffer;
            break;
        default:
            mode = QClipboard::Clipboard;
            break;
    };
    return mode;
}

uint QSpiceWidget::clipboardModeToSelection(QClipboard::Mode mode) {
    uint selection;
    switch (mode) {
        case QClipboard::Clipboard:
            selection = VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD;
            break;
        case QClipboard::Selection:
            selection = VD_AGENT_CLIPBOARD_SELECTION_PRIMARY;
            break;
        case QClipboard::FindBuffer:
            selection = VD_AGENT_CLIPBOARD_SELECTION_SECONDARY;
            break;
        default:
            selection = VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD;
            break;
    };
    return selection;
}

void QSpiceWidget::checkGuestClipboardSelectionRequest(uint selection, uint type) {
    QClipboard::Mode mode = selectionToClipboardMode(selection);
    switch (type) {
        case VD_AGENT_CLIPBOARD_UTF8_TEXT:
            sendTextClipboardDataToGuest(mode);
            break;
        case VD_AGENT_CLIPBOARD_IMAGE_PNG:
        case VD_AGENT_CLIPBOARD_IMAGE_BMP:
        case VD_AGENT_CLIPBOARD_IMAGE_JPG:
        case VD_AGENT_CLIPBOARD_IMAGE_TIFF:
            sendImageClipboardDataToGuest(mode);
            break;
        case VD_AGENT_CLIPBOARD_NONE:
        default:
            emit copyPasteStateChanged(true);
            break;
    };
}

void QSpiceWidget::sendTextClipboardDataToGuest(QClipboard::Mode mode) {
    const QString _text =
        QApplication::clipboard()->text(mode);
    if ( !_text.isEmpty() ) {
        uint selection = clipboardModeToSelection(mode);
        sendClipboardDataToGuest(
            selection,
            VD_AGENT_CLIPBOARD_UTF8_TEXT,
            reinterpret_cast<const uchar*>(_text.toUtf8().data()),
            long(_text.size()));
    } else {
        emit copyPasteStateChanged(true);
    };
}

void QSpiceWidget::sendImageClipboardDataToGuest(QClipboard::Mode mode) {
    const QImage _image =
        QApplication::clipboard()->image(mode);
    if ( !_image.isNull() ) {
        uint selection = clipboardModeToSelection(mode);
        sendClipboardDataToGuest(
            selection,
            VD_AGENT_CLIPBOARD_IMAGE_PNG,
            _image.constBits(),
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
            long(_image.sizeInBytes()));
#else
            long(_image.byteCount()));
#endif
    } else {
        emit copyPasteStateChanged(true);
    };
}

void QSpiceWidget::mainMouseUpdate() {
    //qDebug()<<"main: MouseUpdate";
    int mouseMode = main->getMouseMode();
    if (mouseMode == SPICE_MOUSE_MODE_CLIENT) {
        //We have control of the mouse now, so release the mouse
        this->releaseMouse();
    }
}

void QSpiceWidget::usbDevAutoConnectFailed(QString& dev, QString& err) {
    QString _msg = QString(
            "<font color='red'>UsbDevAutoConnectFailed</font>: %1 : %2")
        .arg(dev).arg(err);
    emit errMsg(_msg);
    //qDebug()<<"usbDevAutoConnectFailed:"<< dev<< err;
}

void QSpiceWidget::usbDevAdded(QString& dev) {
    QString _msg = QString(
            "<font color='blue'>UsbDevAdded</font>: %1")
        .arg(dev);
    emit errMsg(_msg);
    //qDebug()<<"usbDevAdded:"<< dev;
}

void QSpiceWidget::usbDevError(QString& dev, QString& err) {
    QString _msg = QString(
            "<font color='blue'>UsbDevInfo</font>: %1 : %2")
        .arg(dev).arg(err);
    emit errMsg(_msg);
    //qDebug()<<"usbDevError"<< dev<< err;
    QMessageBox::information(
        this,
        tr("Info"),
        _msg);
}

void QSpiceWidget::usbDevRemoved(QString& dev) {
    QString _msg = QString(
            "<font color='blue'>UsbDevRemoved</font>: %1")
        .arg(dev);
    emit errMsg(_msg);
    //qDebug()<<"usbDevRemoved:"<< dev;
}

void QSpiceWidget::cardInserted(QString& _vcard) {
    //qDebug()<<"cardInserted"<<_vcard;
}

void QSpiceWidget::cardRemoved(QString& _vcard) {
    //qDebug()<<"cardRemoved"<<_vcard;
}

void QSpiceWidget::readerAdded(QString& _reader) {
    //qDebug()<<"readerAdded"<<_reader;
}

void QSpiceWidget::readerRemoved(QString& _reader) {
    //qDebug()<<"readerRemoved"<<_reader;
}

void QSpiceWidget::displayPrimaryCreate(
    int                 format,
    int                 width,
    int                 height,
    int                 stride,
    int                 shmid,
    void*               imgdata) {
    Q_UNUSED(shmid)

    //qDebug() << "Display Create(" << width << ", " << height << ")";
    init_h = height;
    init_w = width;
    QImage::Format _format = QImage::Format_Invalid;

    if (img) {
        delete img;
        img = Q_NULLPTR;
    };
    switch(format) {
        case SPICE_SURFACE_FMT_32_xRGB:
            _format = QImage::Format_RGB32;
            break;

        case SPICE_SURFACE_FMT_16_555:
            _format = QImage::Format_RGB555;
            break;

        default:
            //qDebug() << "Unknown display format " << _format;
            ;;
    };
    if ( _format != QImage::Format_Invalid ) {
        img = new QImage(
            static_cast<uchar*>(imgdata),
            init_w,
            init_h,
            stride,
            _format);
    };

    if (img) {
        QSize _size(init_w + 2 * MARGIN, init_h + 2 * MARGIN);
        //qDebug()<<_size<<"emit";
        emit displayResized(_size);
        repaint(0, 0, init_w, init_h);
    }
}

void QSpiceWidget::displayInvalidate(
    int                 x,
    int                 y,
    int                 width,
    int                 height) {
    //qDebug()<<"displayInvalidate"<<x<<y<<width<<height;
    repaint(x * zoom - 1, y * zoom - 1, width * zoom + 3, height * zoom + 3);
}

void QSpiceWidget::displayPrimaryDestroy() {
    //qDebug() << "Display Destroyed";
    delete img;
    img = Q_NULLPTR;
}

void QSpiceWidget::displayMark(int mark) {
    //qDebug() << "Display Mark " << mark;
}

void QSpiceWidget::setClientCursor(
    int                width,
    int                height,
    int                hot_x,
    int                hot_y,
    void*              rgba) {
    QImage c_img(static_cast<uchar*>(rgba), width, height, QImage::Format_ARGB32);
    QPixmap pix = QPixmap::fromImage(c_img);
    QCursor c(pix, hot_x * zoom, hot_y * zoom);
    setCursor(c);
}

void QSpiceWidget::reloadUsbDevList(void* obj) {
    QSpiceUsbDeviceWidget* usbDevWdg =
        static_cast<QSpiceUsbDeviceWidget*>(obj);
    if ( usbDevWdg && usbDevManager ) {
        usbDevWdg->setEnabled(false);
        usbDevWdg->clearList();
        QStringList _devList =
            usbDevManager->spiceUsbDeviceManager_get_devices();
        foreach (QString _dev, _devList) {
            QString _name, _desc;
            QStringList _split = _dev.split("<||>");
            _name = _split.first();
            _desc = _split.last();
            QString _id = QString("%1 %2").arg(_name).arg(_desc);
            bool connected = usbDevManager->
                spiceUsbDeviceManager_is_device_connected(_id);
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(_name);
            item->setData(Qt::UserRole, _desc);
            item->setCheckState(connected ? Qt::Checked : Qt::Unchecked);
            usbDevWdg->addItem(item);
        };
        usbDevWdg->setEnabled(true);
    };
}

void QSpiceWidget::reloadSmartcardList(void* obj) {
    QSpiceSmartcardWidget* smartcardWdg =
        static_cast<QSpiceSmartcardWidget*>(obj);
    if ( smartcardWdg ) {
        smartcardWdg->setEnabled(false);
        smartcardWdg->clearList();
        QStringList _cardList = smartcardManager->
            spiceSmartcardManager_get_readers();
        foreach (QString _card, _cardList) {
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(_card);
            item->setCheckState(Qt::Checked);
            smartcardWdg->addItem(item);
        };
        smartcardWdg->setEnabled(true);
    };
}

int QtButtonToSpice(QMouseEvent* ev) {
    switch (ev->button()) {
        case Qt::LeftButton:
            return SPICE_MOUSE_BUTTON_LEFT;
        case Qt::MiddleButton:
            return SPICE_MOUSE_BUTTON_MIDDLE;
        case Qt::RightButton:
            return SPICE_MOUSE_BUTTON_RIGHT;
        default:
            return SPICE_MOUSE_BUTTON_INVALID;
    };
}

int QtButtonsMaskToSpice(Qt::MouseButtons buttons) {
    int mask = 0;
    if (buttons & Qt::LeftButton) {
        mask |= SPICE_MOUSE_BUTTON_MASK_LEFT;
    };
    if (buttons & Qt::MiddleButton) {
        mask |= SPICE_MOUSE_BUTTON_MASK_MIDDLE;
    };
    if (buttons & Qt::RightButton) {
        mask |= SPICE_MOUSE_BUTTON_MASK_RIGHT;
    };
    return mask;
}

int QtButtonsMaskToSpice(QMouseEvent* ev) {
    return QtButtonsMaskToSpice(ev->buttons());
}

bool QSpiceWidget::eventFilter(QObject* object, QEvent* event) {
    Q_UNUSED(object)

    if (! (display && inputs))
        return false;

    if ( event->type() == QEvent::MouseMove ) {
        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        if ( ev == Q_NULLPTR ) return false;
        //qDebug()<<ev->x()<<ev->y()<<":"
        //<<ev->x()*zoom<<ev->y()*zoom<<":"<<zoom;
        //if ( is_FullScreen ) {
        //    if ( (d_X==0 || d_Y==0) && img!=Q_NULLPTR ) {
        //        //d_X = (frameSize().width() -
        //        //      img->size().width())/2;
        //        //d_Y = (frameSize().height() -
        //        //       img->size().height())/2;
        //    };
        //} else {
        //    d_X = d_Y = 0;
        //};
        QPoint position = ev->pos();//-QPoint(d_X, d_Y);

        if (captured) {
            if (blockMouse) {
                blockMouse = false;
                return true;
            }

            QPoint delta = position - lastMousePos;
            inputs->inputsMotion(delta.x(), delta.y(), QtButtonsMaskToSpice(ev));

            blockMouse = true;
            QCursor::setPos(this->mapToGlobal(QPoint(this->width() / 2, this->height() / 2)));
            lastMousePos = QPoint(this->width() / 2, this->height() / 2);
        } else {
            if ( 0 <= position.y() && position.y() <= 3 ) {
                emit boarderTouched();
            };
            inputs->inputsPosition(
                //ev->x()*zoom,
                //ev->y()*zoom,
                qRound(position.x() / zoom),
                qRound(position.y() / zoom),
                display->getChannelID(),
                QtButtonsMaskToSpice(ev));

        }

        return true;
    } else if ( event->type() == QEvent::MouseButtonPress ) {
        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        if ( ev == Q_NULLPTR ) return false;

        //See if we need to capture the mouse
        int mouseMode = main->getMouseMode();
        if (mouseMode == SPICE_MOUSE_MODE_CLIENT) { //We have control of the mouse, so capturing is not neccessary
            inputs->inputsButtonPress(QtButtonToSpice(ev), QtButtonsMaskToSpice(ev));
        } else {
            if (captured) { //The mouse is already captured
                inputs->inputsButtonPress(QtButtonToSpice(ev), QtButtonsMaskToSpice(ev));
            } else {
                //Capture the mouse instead of sending the mouse click
                this->captureMouse();
            }
        }

        return true;
    } else if ( event->type() == QEvent::MouseButtonRelease ) {
        emit mouseClickedInto();
        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        if ( ev == Q_NULLPTR ) return false;
        inputs->inputsButtonRelease(
            QtButtonToSpice(ev), QtButtonsMaskToSpice(ev));
        return true;
    } else if ( event->type() == QEvent::KeyPress ) {
        QKeyEvent* ev = static_cast<QKeyEvent*>(event);
        if ( ev == Q_NULLPTR ) return false;

        Qt::KeyboardModifiers mods = ev->modifiers();
        Qt::Key key = static_cast<Qt::Key>(ev->key());
        prepareRelease = mods == (Qt::ControlModifier | Qt::AltModifier) && (ev->key() == Qt::Key_Alt || ev->key() == Qt::Key_Control);

        if ( ev->modifiers() & Qt::KeypadModifier ) {
            inputs->inputsQKeypadKeyPress(ev->key());
        } else {
            inputs->inputsQKeyPress(ev->key());
        };
        return true;
    } else if ( event->type() == QEvent::KeyRelease ) {
        QKeyEvent* ev = static_cast<QKeyEvent*>(event);
        if ( ev == Q_NULLPTR ) return false;

        Qt::KeyboardModifiers mods = ev->modifiers();
        Qt::Key key = static_cast<Qt::Key>(ev->key());
        if (((ev->modifiers() == Qt::AltModifier && ev->key() == Qt::Key_Control) || (ev->modifiers() == Qt::ControlModifier & ev->key() == Qt::Key_Alt)) && prepareRelease) {
            this->releaseMouse();
        }
        prepareRelease = false;

        if ( ev->modifiers() & Qt::KeypadModifier ) {
            inputs->inputsQKeypadKeyRelease(ev->key());
        } else {
            inputs->inputsQKeyRelease(ev->key());
        };
        return true;
    } else if ( event->type() == QEvent::MouseButtonDblClick ) {
        inputs->inputsButtonPress(
            SPICE_MOUSE_BUTTON_LEFT,
            SPICE_MOUSE_BUTTON_MASK_LEFT);
        inputs->inputsButtonRelease(
            SPICE_MOUSE_BUTTON_LEFT, 0);
        return true;
    } else if ( event->type() == QEvent::Wheel ) {
        QWheelEvent* ev = static_cast<QWheelEvent*>(event);
        if ( ev == Q_NULLPTR ) return false;
#if (QT_VERSION >= QT_VERSION_CHECK (5, 15, 0))
        if (ev->angleDelta().y() > 0) {
#else
        if (ev->delta() > 0) {
#endif
            inputs->inputsButtonPress(
                SPICE_MOUSE_BUTTON_UP,
                QtButtonsMaskToSpice(
                    QApplication::mouseButtons()));
            inputs->inputsButtonRelease(
                SPICE_MOUSE_BUTTON_UP,
                QtButtonsMaskToSpice(
                    QApplication::mouseButtons()));
        } else {
            inputs->inputsButtonPress(
                SPICE_MOUSE_BUTTON_DOWN,
                QtButtonsMaskToSpice(
                    QApplication::mouseButtons()));
            inputs->inputsButtonRelease(
                SPICE_MOUSE_BUTTON_DOWN,
                QtButtonsMaskToSpice(
                    QApplication::mouseButtons()));
        };
        ev->accept();
        return true;
    };
    return false;
}

void QSpiceWidget::resizeEvent ( QResizeEvent* event ) {
    event->accept();

    if ( main && display && display->isConnected() )
        resizeTimer.start(500);
    return QWidget::resizeEvent(event);
}

void QSpiceWidget::paintEvent(QPaintEvent* event) {
    if ( img == Q_NULLPTR ||
        img->isNull() ||
        img->format() == QImage::Format_Invalid ) {
        event->ignore();
        return;
    };

    event->accept();

    QPainter painter(this);
    QRect _rec = event->rect();
    if ( scaled ) {
        const int x = qRound(_rec.x() / zoom);
        const int y = qRound(_rec.y() / zoom);
        const int w = qRound(_rec.width() / zoom);
        const int h = qRound(_rec.height() / zoom);
        painter.drawImage(
            _rec,
            img->copy(QRect(x, y, w, h))
            .scaled(
                _rec.width(),
                _rec.height(),
                Qt::IgnoreAspectRatio,
                tr_mode));
    } else {
        QImage image = img->copy(_rec);
        QImage alphaImage(image.size(), QImage::Format_Alpha8);
        alphaImage.fill(Qt::white);
        image.setAlphaChannel(alphaImage);

//        image.reinterpretAsFormat(QImage::Format_ARGB32);
        painter.fillRect(event->rect(), Qt::black);
        painter.drawImage(_rec, image);
    };

    if ( downloadProgress ) {
        painter.setOpacity(0.10);
        painter.fillRect(
            QRect(10, 10, frameSize().width(), 50),
            Qt::blue);
        painter.setPen(Qt::yellow);
        painter.setOpacity(0.65);
        painter.setFont(QFont("Monospace", 30));
        QString text = QString(tr("Download: %1 %")).arg(downloadProgress);
        painter.drawText(
            QRect(10, 10, frameSize().width(), 50),
            Qt::AlignLeft | Qt::AlignVCenter,
            text);
    };
}

void QSpiceWidget::resizeDone() {
    if ( main && display ) {
        main->setDisplay(
            display->getChannelID(),
            0,
            0,
            _width,
            _height);
        //main->setDisplayEnabled(display->getChannelID(), true);
        //qDebug()<<"configured"<<
        main->sendMonitorConfig();
    };
    setScaledScreen(true);
}

void QSpiceWidget::setDownloadProgress(qint64 d, qint64 v) {
    downloadProgress = (qreal(d) / v) * 100;
    repaint(QRect(10, 10, frameSize().width(), 50));
    emit downloaded(downloadProgress, 100);
}

void QSpiceWidget::formatMsg(SPICE_CHANNEL_MSG& _msg) {
    QString msg = QString(
            "Domain: %1<br>Channel: %2<br>Context: %3<br>MSG: %4")
        .arg(guestName)
        .arg(_msg.channel)
        .arg(_msg.context)
        .arg(_msg.msg);
    emit errMsg(msg);
}

void QSpiceWidget::channelEvent(int _ev) {
    SpiceChannelEvent ev =
        static_cast<SpiceChannelEvent>(_ev);
    QString eventDescription, chanName, msg;
    switch (ev) {
        case SPICE_CHANNEL_NONE:
            eventDescription
            .append(tr("no event, or ignored event"));
            break;
        case SPICE_CHANNEL_OPENED:
            eventDescription
            .append(tr("connection is authentified and ready"));
            break;
        case SPICE_CHANNEL_SWITCHING:
            eventDescription
            .append(
                tr("disconnecting from the current host and connecting to the target host"));
            break;
        case SPICE_CHANNEL_CLOSED:
            eventDescription
            .append(
                tr("connection is closed normally (sent if channel was ready)"));
            break;
        case SPICE_CHANNEL_ERROR_CONNECT:
            eventDescription
            .append(tr("connection error"));
            break;
        case SPICE_CHANNEL_ERROR_TLS:
            eventDescription
            .append(tr("SSL error"));
            break;
        case SPICE_CHANNEL_ERROR_LINK:
            eventDescription
            .append(tr("error during link process"));
            break;
        case SPICE_CHANNEL_ERROR_AUTH:
            eventDescription
            .append(tr("authentication error"));
            break;
        case SPICE_CHANNEL_ERROR_IO:
            eventDescription
            .append(tr("IO error"));
            break;
        default:
            break;
    };
    if        ( sender() == main ) {
        chanName.append("Main");
    } else if ( sender() == display ) {
        chanName.append("Display");
    } else if ( sender() == inputs ) {
        chanName.append("Inputs");
    } else if ( sender() == cursor ) {
        chanName.append("Cursor");
    } else if ( sender() == usbredir ) {
        chanName.append("UsbRedir");
    } else if ( sender() == smartcard ) {
        chanName.append("Smartcard");
    } else if ( sender() == webdav ) {
        chanName.append("WebDav");
    } else if ( sender() == playback ) {
        chanName.append("Playback");
    } else if ( sender() == record ) {
        chanName.append("Record");
    };
    msg = QString(
            "Domain: %1<br>Channel: %2<br>Event: %3")
        .arg(guestName)
        .arg(chanName)
        .arg(eventDescription);
    emit errMsg(msg);
}

void QSpiceWidget::captureMouse() {
    if (captured) return;

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
    captured = true;
    blockMouse = true;
    QCursor::setPos(this->mapToGlobal(QPoint(this->width() / 2, this->height() / 2)));
    lastMousePos = QPoint(this->width() / 2, this->height() / 2);
    emit mouseCaptured();
}

void QSpiceWidget::releaseMouse() {
    if (!captured) return;

    QApplication::restoreOverrideCursor();
    captured = false;
    emit mouseReleased();
}

/* public slots */
void QSpiceWidget::setGuestAttr(QString& _name, QString& _conn) {
    guestName = _name;
    connName = _conn;
}

void QSpiceWidget::setNewSize(int _w, int _h) {
    _width  = _w - 2 * MARGIN;
    _height = _h - 2 * MARGIN - 4;
    resizeDone();
}
void QSpiceWidget::updateSize(int _w, int _h) {
    _width  = _w - 2 * MARGIN;
    _height = _h - 2 * MARGIN - 4;
    if ( main && display ) {
        // update equal TRUE for update after 1 sec
        // without further changes, because it used
        // at parent widget resizes
        if ( !display->isConnected() ) return;
        main->updateDisplay(
            display->getChannelID(),
            0,
            0,
            _width,
            _height,
            true);
    };
}

void QSpiceWidget::showUsbDevWidget() {
    QSpiceUsbDeviceWidget* usbDevWdg =
        new QSpiceUsbDeviceWidget(this);
    if ( usbDevManager ) {
        connect(usbDevManager, SIGNAL(deviceAdded(QString&)),
            usbDevWdg, SLOT(addDevice(QString&)));
        connect(usbDevManager, SIGNAL(deviceRemoved(QString&)),
            usbDevWdg, SLOT(removeDevice(QString&)));
        connect(usbDevWdg, SIGNAL(connectDevice(QString&)),
            usbDevManager, SLOT(spiceUsbDeviceManager_connect_device(QString&)));
        connect(usbDevWdg, SIGNAL(disconnectDevice(QString&)),
            usbDevManager, SLOT(spiceUsbDeviceManager_disconnect_device(QString&)));
        connect(usbDevWdg, SIGNAL(devicesChanged(void*)),
            this, SLOT(reloadUsbDevList(void*)));
        reloadUsbDevList(usbDevWdg);
    };
    usbDevWdg->exec();
    usbDevWdg->deleteLater();
}

void QSpiceWidget::showSmartCardWidget() {
    QSpiceSmartcardWidget* smartcardWdg =
        new QSpiceSmartcardWidget(this);
    if ( smartcardManager ) {
        connect(smartcardManager, SIGNAL(cardInserted(QString&)),
            smartcardWdg, SLOT(addCard(QString&)));
        connect(smartcardManager, SIGNAL(cardRemoved(QString&)),
            smartcardWdg, SLOT(removeCard(QString&)));
        //connect(smartcardWdg, SIGNAL(connectCard(QString&)),
        //        smartcardManager, SLOT(spiceSmartcardReader_insert_card(QString&)));
        //connect(smartcardWdg, SIGNAL(disconnectCard(QString&)),
        //        smartcardManager, SLOT(spiceSmartcardReader_remove_card(QString&)));
        connect(smartcardWdg, SIGNAL(cardsChanged(void*)),
            this, SLOT(reloadSmartcardList(void*)));
        reloadSmartcardList(smartcardWdg);
    };
    smartcardWdg->exec();
    smartcardWdg->deleteLater();
}

void QSpiceWidget::getScreenshot() {
    // WARNING: used %1%2%3[%4].snapshot template,
    // because filter will added to tail the template
    // after last dot.
    QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("Save Image to"),
            QString("%1%2%3[%4]_%5_%6.snapshot")
            .arg(QDir::homePath())
            .arg(QDir::separator())
            .arg(guestName)
            .arg(connName)
            .arg(QDate::currentDate().toString("dd.MM.yyyy"))
            .arg(QTime::currentTime().toString()),
            "Images (*.png)");
    if ( !fileName.isNull() && img != Q_NULLPTR ) {
        img->save(fileName, "png");
    };
}

void QSpiceWidget::setTransformationMode(Qt::TransformationMode _mode) {
    tr_mode = _mode;
}

void QSpiceWidget::setFullScreen(bool enable) {
    is_FullScreen = enable;
    if( enable ) {
        //this->setWindowFlags( Qt::Window );
        //this->showFullScreen();
        //QPalette p;
        //p.setColor( QPalette::Background, QColor(22,22,22) );
        //this->setPalette( p );
    } else {
        //this->setWindowFlags( Qt::Widget );
        //this->showNormal();
        //this->setPalette( QPalette() );
    };
}

void QSpiceWidget::setScaledScreen(bool state) {
    if ( img == Q_NULLPTR ) return;
    scaled = state;
    if( scaled ) {
        qreal h_zoom = qreal(frameSize().height()) /
            img->height();
        qreal w_zoom = qreal(frameSize().width()) /
            img->width();
        zoom = qMin(w_zoom, h_zoom);
        if ( zoom == 1.0 ) scaled = false;
    } else {
        zoom = 1.0;
    };
    if ( display != Q_NULLPTR )
        display->setScaled(scaled);
    if ( !is_FullScreen ) {
        emit displayResized(
            QSize(img->width()*zoom,
                img->height()*zoom));
    } else {
    };
    repaint();
}


void QSpiceWidget::focusInEvent(QFocusEvent* event) {
    this->grabKeyboard();
}

void QSpiceWidget::focusOutEvent(QFocusEvent* event) {
    this->releaseKeyboard();
}
