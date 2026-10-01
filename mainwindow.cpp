#include "mainwindow.h"
#include "common.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTabWidget>
#include <QTime>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("FileSend"));
    resize(720, 600);

    auto *central = new QWidget;
    auto *root = new QVBoxLayout(central);

    // --- device name row (replaces menu option 3: Name Change) ---
    auto *nameRow = new QHBoxLayout;
    nameRow->addWidget(new QLabel(tr("Device name:")));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setMaxLength(32);
    auto *saveBtn = new QPushButton(tr("Save"));
    nameRow->addWidget(m_nameEdit, 1);
    nameRow->addWidget(saveBtn);

    m_tabs = new QTabWidget;
    m_tabs->addTab(buildSendTab(), tr("Send"));
    m_tabs->addTab(buildReceiveTab(), tr("Receive"));

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(1000);

    root->addLayout(nameRow);
    root->addWidget(m_tabs, 3);
    root->addWidget(new QLabel(tr("Log")));
    root->addWidget(m_log, 1);
    setCentralWidget(central);

    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveDeviceName);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &MainWindow::saveDeviceName);

    // discovery
    connect(&m_discovery, &Discovery::deviceFound, this, [this](const Device &d) {
        auto *item = new QListWidgetItem(QString("%1  (%2)").arg(d.name, d.address.toString()));
        item->setData(Qt::UserRole, d.address.toString());
        m_deviceList->addItem(item);
    });
    connect(&m_discovery, &Discovery::scanFinished, this, [this](int n) {
        m_refreshBtn->setEnabled(true);
        log(n ? tr("Found %1 device(s).").arg(n) : tr("No devices found."));
    });

    // sender
    connect(&m_sender, &FileSender::progress, this, [this](qint64 d, qint64 t) {
        setPercent(m_sendBar, d, t);
    });
    connect(&m_sender, &FileSender::finished, this, [this](bool ok, const QString &msg) {
        m_sendBtn->setEnabled(true);
        if (ok) m_sendBar->setValue(100);
        log(ok ? msg : tr("Send failed: %1").arg(msg));
    });

    // receiver
    connect(&m_receiver, &FileReceiver::transferStarted, this,
            [this](const QString &name, qint64 size, const QString &peer) {
        m_recvBar->setValue(0);
        m_recvStatus->setText(tr("Receiving %1 from %2").arg(name, peer));
        log(tr("Incoming: %1 (%2 bytes) from %3").arg(name).arg(size).arg(peer));
    });
    connect(&m_receiver, &FileReceiver::progress, this, [this](qint64 d, qint64 t) {
        setPercent(m_recvBar, d, t);
    });
    connect(&m_receiver, &FileReceiver::transferFinished, this,
            [this](bool ok, const QString &msg) {
        m_recvStatus->setText(tr("Listening… waiting for files"));
        if (ok) m_recvBar->setValue(100);
        log(ok ? tr("Saved to %1").arg(msg) : tr("Receive failed: %1").arg(msg));
    });

    loadDeviceName();
    refreshDevices();
}

QWidget *MainWindow::buildSendTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Devices on your network:")), 1);
    m_refreshBtn = new QPushButton(tr("Refresh"));
    top->addWidget(m_refreshBtn);

    m_deviceList = new QListWidget;

    auto *fileRow = new QHBoxLayout;
    m_fileEdit = new QLineEdit;
    m_fileEdit->setPlaceholderText(tr("File to send…"));
    auto *browse = new QPushButton(tr("Browse…"));
    fileRow->addWidget(m_fileEdit, 1);
    fileRow->addWidget(browse);

    m_sendBtn = new QPushButton(tr("Send File"));
    m_sendBar = new QProgressBar;
    m_sendBar->setRange(0, 100);

    v->addLayout(top);
    v->addWidget(m_deviceList, 1);

    auto *ipRow = new QHBoxLayout;
    m_ipEdit = new QLineEdit;
    m_ipEdit->setPlaceholderText(tr("Or enter the receiver's IP address, e.g. 192.168.43.12"));
    auto *addIp = new QPushButton(tr("Add"));
    ipRow->addWidget(m_ipEdit, 1);
    ipRow->addWidget(addIp);
    v->addLayout(ipRow);
    connect(addIp, &QPushButton::clicked, this, &MainWindow::addManualDevice);
    connect(m_ipEdit, &QLineEdit::returnPressed, this, &MainWindow::addManualDevice);
    v->addLayout(fileRow);
    v->addWidget(m_sendBtn);
    v->addWidget(m_sendBar);

    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshDevices);
    connect(browse, &QPushButton::clicked, this, &MainWindow::browseFile);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::sendFile);
    return w;
}

QWidget *MainWindow::buildReceiveTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);

    auto *dirRow = new QHBoxLayout;
    dirRow->addWidget(new QLabel(tr("Save to:")));
    m_dirEdit = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    auto *browse = new QPushButton(tr("Browse…"));
    dirRow->addWidget(m_dirEdit, 1);
    dirRow->addWidget(browse);

    m_listenBtn = new QPushButton(tr("Start Receiving"));
    m_listenBtn->setCheckable(true);
    m_recvStatus = new QLabel(tr("Not listening"));
    m_recvBar = new QProgressBar;
    m_recvBar->setRange(0, 100);

    m_localIpLabel = new QLabel(tr("Your IP address: %1").arg(localAddresses()));
    m_localIpLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    v->addWidget(m_localIpLabel);
    v->addLayout(dirRow);
    v->addWidget(m_listenBtn);
    v->addWidget(m_recvStatus);
    v->addWidget(m_recvBar);
    v->addStretch(1);

    connect(browse, &QPushButton::clicked, this, &MainWindow::browseSaveDir);
    connect(m_listenBtn, &QPushButton::toggled, this, &MainWindow::toggleReceiver);
    return w;
}

void MainWindow::loadDeviceName()
{
    QSettings s;
    QString name = s.value("deviceName").toString();

    if (name.isEmpty()) {   // first-time setup
        bool ok = false;
        name = QInputDialog::getText(this, tr("First Time Setup"), tr("Enter device name:"),
                                     QLineEdit::Normal, QSysInfo::machineHostName(), &ok);
        name = normalizeDeviceName(ok ? name : QString());
        s.setValue("deviceName", name);
    }
    m_nameEdit->setText(name);
    m_discovery.setDeviceName(name);
}

void MainWindow::saveDeviceName()
{
    const QString name = normalizeDeviceName(m_nameEdit->text());
    m_nameEdit->setText(name);
    QSettings().setValue("deviceName", name);
    m_discovery.setDeviceName(name);
    log(tr("Device name set to \"%1\".").arg(name));
}

void MainWindow::refreshDevices()
{
    for (int i = m_deviceList->count() - 1; i >= 0; --i)   // keep manually added entries
        if (!m_deviceList->item(i)->data(Qt::UserRole + 1).toBool())
            delete m_deviceList->takeItem(i);
    m_refreshBtn->setEnabled(false);
    log(tr("Searching for devices…"));
    m_discovery.scan(2000);
}

void MainWindow::browseFile()
{
    const QString f = QFileDialog::getOpenFileName(this, tr("Choose file to send"));
    if (!f.isEmpty()) m_fileEdit->setText(f);
}

void MainWindow::browseSaveDir()
{
    const QString d = QFileDialog::getExistingDirectory(this, tr("Choose save folder"), m_dirEdit->text());
    if (!d.isEmpty()) m_dirEdit->setText(d);
}

void MainWindow::sendFile()
{
    auto *item = m_deviceList->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("FileSend"), tr("Select a device first."));
        return;
    }
    const QString path = m_fileEdit->text();
    if (!QFileInfo(path).isFile()) {
        QMessageBox::information(this, tr("FileSend"), tr("Choose a valid file to send."));
        return;
    }
    if (m_sender.isBusy()) return;

    const QString ip = item->data(Qt::UserRole).toString();
    log(tr("Sending %1 to %2…").arg(QFileInfo(path).fileName(), ip));
    m_sendBar->setValue(0);
    m_sendBtn->setEnabled(false);
    m_sender.send(QHostAddress(ip), path);
}

void MainWindow::toggleReceiver(bool on)
{
    if (on) {
        if (!m_receiver.start(m_dirEdit->text())) {
            QMessageBox::warning(this, tr("FileSend"),
                                 tr("Cannot listen on port %1: %2")
                                     .arg(Protocol::TransferPort).arg(m_receiver.errorString()));
            m_listenBtn->setChecked(false);
            return;
        }
        if (!m_discovery.startListening())
            log(tr("Warning: discovery listener failed; other devices won't see you."));

        m_listenBtn->setText(tr("Stop Receiving"));
        m_localIpLabel->setText(tr("Your IP address: %1").arg(localAddresses()));
        m_recvStatus->setText(tr("Listening… waiting for files"));
        m_dirEdit->setEnabled(false);
        log(tr("Receiver started."));
    } else {
        m_receiver.stop();
        m_discovery.stopListening();
        m_listenBtn->setText(tr("Start Receiving"));
        m_recvStatus->setText(tr("Not listening"));
        m_dirEdit->setEnabled(true);
        log(tr("Receiver stopped."));
    }
}

void MainWindow::log(const QString &text)
{
    m_log->appendPlainText(QTime::currentTime().toString("HH:mm:ss  ") + text);
}

void MainWindow::setPercent(QProgressBar *bar, qint64 done, qint64 total)
{
    bar->setValue(total > 0 ? int(done * 100 / total) : 100);
}

QString MainWindow::localAddresses()
{
    QStringList ips;
    const auto all = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &ifc : all) {
        const auto f = ifc.flags();
        if (!(f & QNetworkInterface::IsUp) || !(f & QNetworkInterface::IsRunning)
            || (f & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &e : ifc.addressEntries()) {
            if (e.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            const QString ip = e.ip().toString();
            if (ip.startsWith("169.254.")) continue;   // link-local, not useful
            ips << ip;
        }
    }
    return ips.isEmpty() ? QStringLiteral("unknown") : ips.join(", ");
}

void MainWindow::addManualDevice()
{
    QHostAddress addr;
    if (!addr.setAddress(m_ipEdit->text().trimmed()) ||
        addr.protocol() != QAbstractSocket::IPv4Protocol) {
        QMessageBox::information(this, tr("FileSend"), tr("Enter a valid IPv4 address."));
        return;
    }
    auto *item = new QListWidgetItem(tr("Manual  (%1)").arg(addr.toString()));
    item->setData(Qt::UserRole, addr.toString());
    item->setData(Qt::UserRole + 1, true);   // manual entry: survives Refresh
    m_deviceList->addItem(item);
    m_deviceList->setCurrentItem(item);
    m_ipEdit->clear();
}
