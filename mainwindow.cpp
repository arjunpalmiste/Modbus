#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("ModbusClient");
    setWindowIcon(QIcon(":/images/modbus.png"));
    ui->ipEdit->setText("127.0.0.1");
    ui->portEdit->setText("502");
    setBaseSize(500, 350);

    ui->readAddress->setMaximum(m_max_num);
    ui->writeAddress->setMaximum(m_max_num);
    ui->readSize->setEditable(true);
    ui->writeSize->setEditable(true);

    m_max_num = ui->readSize->maxVisibleItems();
    m_wsta_addr = ui->readAddress->minimum();;
    m_item = new QTreeWidgetItem[m_max_num]();
    m_base_num = 10;
    modbus_client = new modbus(m_max_num);
    setup = new Setting(this);
    setup->hide();

    key_esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    key_ret = new QShortcut(QKeySequence(Qt::Key_Return), this);
    client_socket = new QTcpSocket(this);


    initActions();
    initWriteBox();


    connect(key_ret, SIGNAL(activated()), this, SLOT(on_connectButton_clicked()));
    connect(key_esc, &QShortcut::activated, this, &QMainWindow::close);

    connect(client_socket, &QTcpSocket::readyRead, this, &MainWindow::parse_Recv_Data, Qt::UniqueConnection);
}

MainWindow::~MainWindow()
{
    if(m_item)
        delete [] m_item;
    if(modbus_client)
        delete modbus_client;
    delete ui;
}


void MainWindow::parse_Recv_Data()
{
//    for (int i = 0; i < ui->readValue->count(); ++i) {
//        ui->readValue->takeItem(i);
//    }
    ui->readValue->clear();
    int ret;
    memset(modbus_client->mb_rsp_msg, 0, MB_DATA_MAX);
    client_socket->read(modbus_client->mb_rsp_msg, MB_DATA_MAX);

    uint16_t len = ((uint8_t)modbus_client->mb_rsp_msg[4] << 8) + (uint8_t)modbus_client->mb_rsp_msg[5] + 6;
    display_info(modbus_client->mb_rsp_msg, len, false);

    ret = modbus_client->Param_Response(ui->readValue);

    switch (ret) {
    case MB_ERR:
        statusBar()->showMessage("There is no response for now, waiting for update....", 1000);
        break;
    case 1:
        statusBar()->showMessage("Illegal function code!!!", 1000);
        break;
    case 2:
        statusBar()->showMessage("Illegal data address!!!", 1000);
        break;
    case 3:
        statusBar()->showMessage("Illegal data value!!!", 1000);
        break;
    case 4:
        statusBar()->showMessage("Server failure!!!", 1000);
        break;
    default:
        statusBar()->showMessage("Sent successfully!!!", 1000);
        break;
    }
}

void MainWindow::initActions()
{
    client_status = false;
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionOptions->setEnabled(true);

    connect(ui->actionConnect, &QAction::triggered,
            this, &MainWindow::on_connectButton_clicked);
    connect(ui->actionDisconnect, &QAction::triggered,
            this, &MainWindow::on_connectButton_clicked);
    connect(ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionOptions, &QAction::triggered, setup, &Setting::show);
    connect(setup, &Setting::apply_signal, this, [this](){
        m_max_num = setup->m_totalnum;
        m_base_num = setup->m_base;
        modbus_client->m_base_num = m_base_num;
        if(m_item)
        {
            delete [] m_item;
            m_item = new QTreeWidgetItem[m_max_num]();
        }
        showWriteTable();
        if(modbus_client->m_Value)
        {
            delete [] modbus_client->m_Value;
            modbus_client->m_Value = new int16_t[m_max_num]();
        }
        ui->readAddress->setMaximum(m_max_num);
        ui->writeAddress->setMaximum(m_max_num);
        setup->hide();
    });
    connect(ui->actionDisplay_Communication, &QAction::triggered, this,[this](){
        dis_mess = new QDialog(this);
        dis_mess->setModal(false);
        dis_mess->setAttribute(Qt::WA_DeleteOnClose);
//        dis_mess->setStyleSheet("QLabel{"
//                                "min-width:400px;"
//                                "min-height:200px; "
//                                "font-size:16px;"
//                                "}");
        dis_mess->setBaseSize(400,200);
        dis_mess->resize(400,200);

        dis_mess->setWindowFlags(dis_mess->windowFlags() | Qt::WindowMinMaxButtonsHint );
        dis_mess->setWindowTitle("Communication");



        commu_info = new QListWidget(dis_mess);
        commu_info->resize(dis_mess->size());
        commu_info->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        commu_info->setFont(QFont("微软雅黑", 14));
        commu_info->show();

        QGridLayout *grid = new QGridLayout(dis_mess);
        grid->addWidget(commu_info);
        dis_mess->setLayout(grid);
        dis_mess->show();
    });
//    connect()
}

void MainWindow::initWriteBox()
{
    ui->writeTable->addItem(tr("Coils"), 0);
    ui->writeTable->addItem(tr("Discrete Inputs"), 1);
    ui->writeTable->addItem(tr("Holding Registers"), 2);
    ui->writeTable->addItem(tr("Input Registers"), 3);

    //    ui->writeValueTable->setEditTriggers(QTreeView::DoubleClicked | QTreeView::EditTriggers());

    ui->writeValueTable->setAutoScroll(true);
    ui->writeValueTable->setFrameStyle(QFrame::Plain | QFrame::Box);
    ui->readValue->setFrameStyle(QFrame::Plain | QFrame::Box);
    ui->writeValueTable->setColumnWidth(0, 100);
    ui->writeValueTable->setIndentation(0);
    ui->writeValueTable->setAlternatingRowColors(true);
    // 设置读框间隔颜色
    ui->readValue->setAlternatingRowColors(true);

    showWriteTable();
    ui->writeValueTable->setColumnHidden(1, false);
    ui->writeValueTable->setColumnHidden(2, true);
    ui->writeValueTable->resizeColumnToContents(0);

    connect(ui->writeValueTable, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int column){
        if(column == 2)
        {
            ui->writeValueTable->openPersistentEditor(item, column);
        }
    });
    connect(ui->writeAddress, &QSpinBox::valueChanged, this, &MainWindow::write_AddrSize_changed);
    connect(ui->writeSize, &QComboBox::currentTextChanged, this, &MainWindow::write_AddrSize_changed);

    connect(ui->readButton, SIGNAL(clicked), this, SLOT(on_readButton_clicked), Qt::UniqueConnection);
    connect(ui->writeButton, SIGNAL(clicked), this, SLOT(on_writeButton_clicked), Qt::UniqueConnection);
    connect(ui->writeTable, &QComboBox::currentIndexChanged,
            this, &MainWindow::onWriteTableChanged);
}

void MainWindow::write_AddrSize_changed()
{
    QTreeWidgetItem *item = m_item;
    int num = ui->writeSize->currentText().toInt();
    int addr = ui->writeAddress->value();
    for (int i = m_wsta_addr; i < addr; ++i) {
        item = m_item + i;
        item->setHidden(true);
    }
    for (int i = addr; i < addr + num && i < m_max_num; ++i) {
        item =m_item + i;
        item->setHidden(false);
    }
    for(int i = addr + num; i < m_max_num; ++i)
    {
        item =m_item + i;
        item->setHidden(true);
    }
}


void MainWindow::showWriteTable()
{
    QTreeWidgetItem *item = NULL;
    if (m_base_num == 10)
        ui->writeValueTable->setHeaderLabels(QStringList() << "Addr" << "Coils" << "Registers");
    else if(m_base_num == 16)
        ui->writeValueTable->setHeaderLabels(QStringList() << "Addr(hex)" << "Coils" << "Registers(hex)");
    ui->writeValueTable->resizeColumnToContents(0);

    for (int i = m_wsta_addr; i < m_max_num; ++i) {
        item = m_item + i;
        item->setText(0, QString("%1").arg(i, 4, m_base_num, QLatin1Char('0')));
        item->setCheckState(1,Qt::Unchecked);
        item->setData(2, Qt::EditRole, QVariant(0));
//        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->writeValueTable->addTopLevelItem(item);
    }
    write_AddrSize_changed();
}

void MainWindow::on_connectButton_clicked()
{
    if(!client_status)
    {
        QString ip_addr = ui->ipEdit->text();
        QString port = ui->portEdit->text();
        // 连接服务器F
        ui->connectButton->setText("Disconnect");
        ui->actionDisconnect->setEnabled(true);
        ui->actionConnect->setEnabled(false);
        client_socket->connectToHost(QHostAddress(ip_addr), port.toShort());

        connect(client_socket, SIGNAL(connected), this, SLOT(Connect_info), Qt::UniqueConnection);

        client_status = true;
    }
    else
    {

        ui->connectButton->setText("Connect");
        ui->actionDisconnect->setEnabled(false);
        ui->actionConnect->setEnabled(true);
        client_socket->disconnectFromHost();
        if(client_socket->state() == QTcpSocket::UnconnectedState)
        {
            statusBar()->showMessage("Disconnected", 5000);
        }

        client_status = false;
    }
    connect(client_socket, &QTcpSocket::errorOccurred, this, &MainWindow::Connecterr_info, Qt::UniqueConnection);
}

void MainWindow::display_info(char *message, uint16_t len, bool istx)
{
    if (dis_mess && commu_info)
    {
        QString temp = "";
        for (int i = 0; i < len; ++i) {
            temp.append(QString(message[i]));
        }
        if(istx)
            commu_info->addItem("TX: " + temp.toLatin1().toHex(' '));
        //        qDebug() << "TX: " + temp.toLatin1().toHex(' ');
        else
            commu_info->addItem("RX: " + temp.toLatin1().toHex(' '));

    }
}

void MainWindow::on_readButton_clicked()
{
    if(client_socket->state() != QAbstractSocket::ConnectedState)
    {
        statusBar()->showMessage("Please Connect Server!!!", 1000);
        return;
    }
    uint16_t addr = ui->readAddress->value();
    uint16_t num = ui->readSize->currentText().toInt();
    uint8_t func_code = 1;
    switch (ui->writeTable->currentIndex()) {
    case 0:
        func_code = 0x01;
        break;
    case 1:
        func_code = 0x02;
        break;
    case 2:
        func_code = 0x03;
        break;
    case 3:
        func_code = 0x04;
        break;
    default:
        break;
    }
    modbus_client->Create_Req_Msg(func_code, addr, num);

    display_info(modbus_client->mb_req_msg, modbus_client->mb_req_len, true);

    client_socket->write(modbus_client->mb_req_msg, modbus_client->mb_req_len);
}

void MainWindow::on_writeButton_clicked()
{
    if(client_socket->state() != QAbstractSocket::ConnectedState)
    {
        statusBar()->showMessage("Please Connect Server!!!", 1000);
        return;
    }
    uint16_t addr = ui->writeAddress->value();
    uint16_t num = ui->writeSize->currentText().toInt();
    uint16_t cnt = 0, i = 0;
    QTreeWidgetItem *item = NULL;
    uint8_t func_code = 5;
    int16_t *value = modbus_client->m_Value;
    int16_t flag = 0x0001;
    memset(value, 0, sizeof(modbus_client->m_Value));
    switch (ui->writeTable->currentIndex()) {
    case 0:
    {
        for (i = 0; i <  num && i < m_max_num - addr; ++i) {
//            if (i % 16 == 8)
//                flag = 0x0001;
//            else
            if(i != 0 && i % 16 == 0)
            {
                flag = 0x0001;
                cnt++;
            }
            item = m_item + i + addr;
            if (item->checkState(1))
                value[cnt] |= flag;
            flag = flag << 1;
        }
        func_code = i > 1 ? 0x0F : 0x05;
        break;
    }
    case 2:
    {
        for (i = addr; i < addr + num && i < m_max_num; ++i) {
            item = m_item + i;
            value[cnt++] = item->data(2,0).toInt();
//            qDebug() << value[cnt];
        }
        func_code = i > 1 ? 0x10 : 0x06;
        break;
    }
    default:
        break;
    }

    modbus_client->Create_Req_Msg(func_code, addr, num, value);
    display_info(modbus_client->mb_req_msg, modbus_client->mb_req_len, true);
    client_socket->write(modbus_client->mb_req_msg, modbus_client->mb_req_len);
}

void MainWindow::onWriteTableChanged(int index)
{
    const bool coilsOrHolding = index == 0 || index == 2;
    if (coilsOrHolding) {
        ui->writeValueTable->setColumnHidden(1, index != 0);
        ui->writeValueTable->setColumnHidden(2, index != 2);
        ui->writeValueTable->resizeColumnToContents(0);
    }
    ui->writeButton->setEnabled(coilsOrHolding);
    ui->writeGroupBox->setEnabled(coilsOrHolding);
}


void MainWindow::Connect_info()
{
    statusBar()->showMessage(tr("Connect success"), 5000);
    ui->connectButton->setText("Disconnect");
    ui->actionDisconnect->setEnabled(true);
    ui->actionConnect->setEnabled(false);
    client_status = true;
}

void MainWindow::Connecterr_info()
{
    statusBar()->showMessage(tr("Connect failed"), 5000);
    ui->connectButton->setText("Connect");
    ui->actionDisconnect->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    client_status = false;
}

