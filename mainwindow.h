#pragma once
#include <QMainWindow>
#include "discovery.h"
#include "filesender.h"
#include "filereceiver.h"

class QLineEdit;
class QListWidget;
class QPushButton;
class QProgressBar;
class QPlainTextEdit;
class QTabWidget;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void saveDeviceName();
    void refreshDevices();
    void addManualDevice();
    void browseFile();
    void browseSaveDir();
    void sendFile();
    void toggleReceiver(bool on);

private:
    QWidget *buildSendTab();
    QWidget *buildReceiveTab();
    void loadDeviceName();
    void log(const QString &text);
    static void setPercent(QProgressBar *bar, qint64 done, qint64 total);
    static QString localAddresses();

    Discovery m_discovery;
    FileSender m_sender;
    FileReceiver m_receiver;

    QLineEdit *m_nameEdit;
    QTabWidget *m_tabs;
    QPlainTextEdit *m_log;

    // send tab
    QListWidget *m_deviceList;
    QPushButton *m_refreshBtn;
    QLineEdit *m_ipEdit;
    QLineEdit *m_fileEdit;
    QPushButton *m_sendBtn;
    QProgressBar *m_sendBar;

    // receive tab
    QLineEdit *m_dirEdit;
    QPushButton *m_listenBtn;
    QLabel *m_localIpLabel;
    QLabel *m_recvStatus;
    QProgressBar *m_recvBar;
};
