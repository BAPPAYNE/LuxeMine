#ifndef MEGAMINEMAINWINDOW_H
#define MEGAMINEMAINWINDOW_H

#include <QMainWindow>

#include "admin.h"
#include "addcatalog.h"
#include "loginwindow.h"
#include "orderlist.h"
#include "ordermenu.h"
#include "user.h"

namespace Ui {
class MegaMineMainWindow;
}

class MegaMineMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MegaMineMainWindow(QWidget *parent = nullptr);
    ~MegaMineMainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void openAdminPage(int index);
    void openCatalogPage(int pageIndex);
    void openOrderBook();
    void openUser();

private:

    void updateMdiBackground();

    Ui::MegaMineMainWindow *ui;

    Admin *newAdmin = nullptr;
    AddCatalog *newAddCatalog = nullptr;
    User *newUser = nullptr;



};

#endif // MEGAMINEMAINWINDOW_H
