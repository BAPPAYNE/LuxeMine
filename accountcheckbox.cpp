#include "accountcheckbox.h"
#include "ui_accountcheckbox.h"

AccountCheckbox::AccountCheckbox(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AccountCheckbox)
{
    ui->setupUi(this);

    ui->btn_opening_balance_info->installEventFilter(this) ;
    ui->text_opening_balance_info->setVisible(false) ;
    ui->text_opening_balance_info->setReadOnly(true);
    ui->text_opening_balance_info->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->text_opening_balance_info->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->text_opening_balance_info->setFrameShape(QFrame::Panel);       // classic panel frame
    ui->text_opening_balance_info->setFrameShadow(QFrame::Sunken);

}

bool AccountCheckbox::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->btn_opening_balance_info) {
        if(event->type() == QEvent::Enter) {
            ui->text_opening_balance_info->setVisible(true) ;
        }

        if (event->type() == QEvent::Leave) {
            ui->text_opening_balance_info->setVisible(false) ;
        }
    }
    return QWidget::eventFilter(watched, event);
}

AccountCheckbox::~AccountCheckbox()
{
    delete ui;
}
