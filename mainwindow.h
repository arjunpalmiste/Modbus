#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QTreeWidget>
#include <QShortcut>
#include "modbus.h"
#include "setting.h"
#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();
    void onWriteTableChanged(int index);
    void on_readButton_clicked();
    void on_writeButton_clicked();

private:
    Ui::MainWindow *ui;
    QTcpSocket *client_socket;
    QTreeWidgetItem *m_item;
    QShortcut *key_esc;
    QShortcut *key_ret;
    QDialog *dis_mess;
    QListWidget *commu_info;
    Setting *setup;
    modbus * modbus_client;
    int m_base_num;
    int m_max_num;
    int m_wsta_addr;
    bool client_status;

    void Connect_info();
    void Connecterr_info();
    void initActions();
    void initWriteBox();
    void showWriteTable();
    void write_AddrSize_changed();
    void parse_Recv_Data();
    void display_info(char *message, uint16_t len, bool is_tx);
};
#endif // MAINWINDOW_H
