#include "setting.h"
#include "ui_setting.h"

Setting::Setting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Setting)
{
    ui->setupUi(this);
    //    setAttribute(Qt::WA_Hover);
    ui->numCombo->setCurrentText("10");
    ui->numCombo->setEditable(true);
    connect(ui->applyButton, &QPushButton::clicked, this, [this](){
        if (ui->baseBox->currentText() == "Dec")
            m_base = 10;
        else if(ui->baseBox->currentText() == "Hex")
            m_base = 16;
        m_totalnum = ui->numCombo->currentText().toInt();
        emit apply_signal();
    });
    hide();
}

Setting::~Setting()
{
    delete ui;
}
