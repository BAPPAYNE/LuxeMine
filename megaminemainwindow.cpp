#include "megaminemainwindow.h"
#include "ui_megaminemainwindow.h"
#include <QPixmap>
#include <QBrush>
#include <QResizeEvent>
#include <QDebug>
#include <QMdiSubWindow>

MegaMineMainWindow::MegaMineMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MegaMineMainWindow)
{
    ui->setupUi(this);

    // Set background once after UI loads
    QTimer::singleShot(0, this, [this]() {
        updateMdiBackground();
    });

    connect(ui->actionShow_Image, &QAction::triggered, this, [this]() {
        openAdminPage(0); // index for show_images
    });
    connect(ui->actionUpdate_Price, &QAction::triggered, this, [this]() {
        openAdminPage(1);
    });

    connect(ui->actionAdd_Diamond, &QAction::triggered, this, [this]() {
        openAdminPage(2);
    });

    connect(ui->actionShow_Users, &QAction::triggered, this, [this]() {
        openAdminPage(3);
    });

    connect(ui->actionJewellry_Menu, &QAction::triggered, this, [this]() {
        openAdminPage(4);
    });

    connect(ui->actionOrder_Book_Users, &QAction::triggered, this, [this]() {
        openAdminPage(5);
    });

    connect(ui->actionOrder_List, &QAction::triggered, this, [this]() {
        openAdminPage(6);
    });

    connect(ui->actionAdd_Catalog, &QAction::triggered, this, [this]() {
        openCatalogPage(0);
    });

    connect(ui->actionModify_Catalog, &QAction::triggered, this, [this]() {
        openCatalogPage(1);
    });

    connect(ui->actionDelete_Catalog, &QAction::triggered, this, [this]() {
        openCatalogPage(2);
    });
    connect(ui->actionOpen, &QAction::triggered, this, [this]() {
        openOrderBook();
    });
    connect(ui->actionOpen_2, &QAction::triggered, this, [this]() {
       openUser();
    });
}

void MegaMineMainWindow::openAdminPage(int pageIndex)
{
    if (!newAdmin || newAdmin->isHidden()) {
        newAdmin = new Admin();
        newAdmin->setAttribute(Qt::WA_DeleteOnClose);
        connect(newAdmin, &QObject::destroyed, this, [this]() { newAdmin = nullptr; });

        QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(newAdmin);
        subWindow->setWindowTitle("Admin Panel");
        subWindow->setAttribute(Qt::WA_DeleteOnClose);

        ui->mdiArea->setActiveSubWindow(subWindow);
        subWindow->showMaximized();  // ✅ maximize the subwindow itself
    }

    newAdmin->raise();
    newAdmin->activateWindow();
    newAdmin->setRequestedPage(pageIndex);
}

void MegaMineMainWindow::openCatalogPage(int pageIndex)
{
    if (!newAddCatalog || newAddCatalog->isHidden()) {
        // Create AddCatalog window (child widget)
        newAddCatalog = new AddCatalog();

        // Set this as parent to auto-handle memory correctly
        newAddCatalog->setAttribute(Qt::WA_DeleteOnClose);

        // Reset pointer when window is closed
        connect(newAddCatalog, &QObject::destroyed, this, [this]() {
            newAddCatalog = nullptr;
        });

        // Add to MDI area
        QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(newAddCatalog);
        subWindow->setWindowTitle("Catalog Page");
        subWindow->setAttribute(Qt::WA_DeleteOnClose);

        // Activate and show inside MDI
        ui->mdiArea->setActiveSubWindow(subWindow);

        // ✅ Maximize the subwindow (not the child)
        subWindow->showMaximized();
    }

    // Bring it to the front
    newAddCatalog->raise();
    newAddCatalog->activateWindow();

    // Open requested page directly
    newAddCatalog->setRequestedPage(pageIndex);
}

void MegaMineMainWindow::openOrderBook()
{
    // Open login window (modal)
    LoginWindow loginWindow(this);

    connect(&loginWindow, &LoginWindow::loginAccepted, this,
            [this, &loginWindow](const QString &action)
            {
                QString role = loginWindow.getRole().toLower();
                QString userName = loginWindow.getUserName();
                QString userId = loginWindow.getUserId();

                // --- Seller Role: Open OrderMenu ---
                if (action == "orderMenu" && role == "seller") {
                    QString partyId = loginWindow.getPartyId();
                    QString partyName = loginWindow.getPartyName();
                    QString partyAddress = loginWindow.getPartyAddress();
                    QString partyCity = loginWindow.getPartyCity();
                    QString partyState = loginWindow.getPartyState();
                    QString partyCountry = loginWindow.getPartyCountry();

                    // Create the OrderMenu widget
                    auto *newOrderMenu = new OrderMenu();
                    newOrderMenu->setWindowFlags(Qt::Widget); // Important if it's a QMainWindow subclass
                    newOrderMenu->setAttribute(Qt::WA_DeleteOnClose);
                    newOrderMenu->setInitialInfo(userName, userId,
                                                 partyName, partyId,
                                                 partyAddress, partyCity,
                                                 partyState, partyCountry);
                    newOrderMenu->insertDummyOrder();

                    // Add it to the MDI area
                    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(newOrderMenu);
                    subWindow->setWindowTitle("Order Menu - " + userName);
                    subWindow->setAttribute(Qt::WA_DeleteOnClose);

                    // Activate and show
                    ui->mdiArea->setActiveSubWindow(subWindow);
                    subWindow->showMaximized();

                }
                // --- Other Roles: Open OrderList ---
                else if (action == "orderList" &&
                         (role == "designer" || role == "manufacturer" ||
                          role == "accountant" || role == "manager" ||
                          role == "seller"))
                {
                    auto *newOrderList = new OrderList(nullptr, role);
                    newOrderList->setWindowFlags(Qt::Widget);
                    newOrderList->setAttribute(Qt::WA_DeleteOnClose);
                    newOrderList->setRoleAndUserInfo(role, userId, userName);

                    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(newOrderList);
                    subWindow->setWindowTitle("Order List - " + userName);
                    subWindow->setAttribute(Qt::WA_DeleteOnClose);

                    ui->mdiArea->setActiveSubWindow(subWindow);
                    subWindow->showMaximized();
                }
                // --- Unknown Role ---
                else {
                    QMessageBox::warning(this, "Unknown Role", "This role is not supported.");
                }
            });

    // Run login window modally
    loginWindow.exec();
}

void MegaMineMainWindow::openUser()
{
    // Check if User window already exists
    if (newUser && !newUser->isHidden()) {
        newUser->raise();
        newUser->activateWindow();
        return;
    }

    // Create new User window
    newUser = new User();
    newUser->setAttribute(Qt::WA_DeleteOnClose);  // auto-delete when closed

    // Reset pointer when closed
    connect(newUser, &QObject::destroyed, this, [this]() { newUser = nullptr; });

    // Add to MDI Area
    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(newUser);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setWindowTitle("User Management");

    // Resize to fit inside mdiArea
    newUser->resize(ui->mdiArea->size());

    // Show and maximize it
    subWindow->showMaximized();
}


void MegaMineMainWindow::updateMdiBackground()
{
    QSize mdiAreaSize = ui->mdiArea->size();

    QPixmap background(":/Backgrounds/1.jpg");
    if (background.isNull()) {
        qDebug() << "Background image not found!";
        return;
    }

    QPixmap scaledBg = background.scaled(
        mdiAreaSize,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
        );

    qDebug() << mdiAreaSize;

    ui->mdiArea->setBackground(QBrush(scaledBg));
}

void MegaMineMainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateMdiBackground(); // automatically resize image when window resizes
}

MegaMineMainWindow::~MegaMineMainWindow()
{
    delete ui;
}
