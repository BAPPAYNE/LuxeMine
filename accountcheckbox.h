#ifndef ACCOUNTCHECKBOX_H
#define ACCOUNTCHECKBOX_H

#include <QDialog>

namespace Ui {
class AccountCheckbox;
}

class AccountCheckbox : public QDialog
{
    Q_OBJECT

public:
    explicit AccountCheckbox(QWidget *parent = nullptr);
    ~AccountCheckbox();

private:
    Ui::AccountCheckbox *ui;


protected :
    bool eventFilter(QObject *watched, QEvent *event) override ;
};

#endif // ACCOUNTCHECKBOX_H
