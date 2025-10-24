#include "addcatalog.h"
#include "ui_addcatalog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QListView>
#include <QStandardItemModel>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFontMetrics>
#include <QSortFilterProxyModel>
#include <QApplication>

#include "databaseutils.h"
#include "utils.h"


AddCatalog::AddCatalog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Catalog)
    , jewelryMenu(new JewelryMenu(this))   // parented, auto-cleanup
{
    ui->setupUi(this);
    setWindowSize(this);

    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowTitle("Add Catalog");
    setWindowIcon(QIcon(":/icon/addcatalog.png"));

    setupGoldTable();
    ui->companyName_lineEdit->setText("SHREE LAXMINARAYAN EXPORT");

    connect(ui->jewelryButton, &QPushButton::clicked, this, [this]() {
        jewelryMenu->getMenu()->popup(ui->jewelryButton->mapToGlobal(QPoint(0, ui->jewelryButton->height())));
    });

    connect(jewelryMenu, &JewelryMenu::itemSelected, this, &AddCatalog::onJewelryItemSelected);

    this->setStyleSheet(R"(
        QWidget {
            background-color: #F9FAFB;
            color: #2B2B2B;
            font-family: "Segoe UI", "Arial";
            font-size: 14px;
        }

        QLineEdit {
            background-color: #FFFFFF;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
            padding: 4px 6px;
            selection-background-color: #4A90E2;
        }

        QLineEdit:focus {
            border: 1px solid #4A90E2;
            background-color: #FDFEFF;
        }

        QPushButton {
            background-color: #E7E9EC;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
            padding: 6px 10px;
            font-weight: 500;
        }

        QPushButton:hover {
            background-color: #DDE4F2;
            border: 1px solid #4A90E2;
        }

        QPushButton:pressed {
            background-color: #C7D8F0;
            border: 1px solid #4A90E2;
        }

        QStackedWidget {
            background-color: #FFFFFF;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
        }

        QListView {
            background-color: #F9FAFB;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
            color: #2B2B2B;
            outline: none;
            padding: 6px;
        }

        QListView::item {
            background-color: #FFFFFF;
            border: 1px solid #D4D4D4;
            border-radius: 0px;
            margin: 8px;
            padding: 8px 6px;
        }

        QListView::item:hover {
            background-color: #EEF3FA;
            border: 1px solid #9EB9E2;
            color: #1A1A1A;
        }

        QListView::item:selected {
            background-color: #D9E8FC;
            border: 1px solid #4A90E2;
            color: #000000;
            font-weight: 500;
        }

        QScrollBar:vertical, QScrollBar:horizontal {
            background: #F2F2F2;
            border: none;
            width: 12px;
            height: 12px;
        }

        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: #BDBDBD;
            border: 1px solid #A5A5A5;
            border-radius: 0px;
        }

        QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
            background: #9E9E9E;
        }

        QScrollBar::add-line, QScrollBar::sub-line {
            background: none;
            border: none;
            width: 0;
            height: 0;
        }
    )");



    ui->catalog_stacked->setCurrentIndex(0);

}

AddCatalog::~AddCatalog()
{
    QSqlDatabase::removeDatabase("modify_catalog_conn") ;
    delete ui;
    // jewelryMenu auto-deleted since it has "this" as parent
}

void AddCatalog::setRequestedPage(int index) {
    requestedPageIndex = index;

    if (ui->catalog_stacked->currentIndex() == 0) {
        switch (requestedPageIndex) {
        case 0: on_add_catalog_button_released(); ; break;
        case 1: on_modify_catalog_button_released(); break;
        case 2: on_delete_catalog_button_released(); break;
        default: on_add_catalog_button_released(); break;
        }
        requestedPageIndex = -1; // reset
    }
}

void AddCatalog::onJewelryItemSelected(const QString &item)
{
    selectedImageType = item; // e.g., "Ring (Men's Party Wear)"
    ui->jewelryButton->setText(item); // Update the button text to show the selection
}

void AddCatalog::addTableRow(QTableWidget *table, const QString &tableType)
{
    int newRow = table->rowCount();
    table->insertRow(newRow);

    // parented to table → no leaks if row is removed
    QComboBox *shapeCombo = new QComboBox(table);
    QComboBox *sizeCombo = new QComboBox(table);

    QStringList shapes = DatabaseUtils::fetchShapes(tableType);
    if (shapes.isEmpty()) {
        QMessageBox::critical(this, "Database Error", "Failed to fetch shapes for " + tableType);
        table->removeRow(newRow);
        return;
    }

    shapeCombo->addItems(shapes);

    auto populateSizeCombo = [this, sizeCombo, tableType](const QString &selectedShape) {
        sizeCombo->clear();
        QStringList sizes = DatabaseUtils::fetchSizes(tableType, selectedShape);
        if (sizes.isEmpty()) {
            QMessageBox::critical(this, "Database Error", "Failed to fetch sizes for " + selectedShape);
            return;
        }
        sizeCombo->addItems(sizes);
    };

    populateSizeCombo(shapes.first());
    connect(shapeCombo, &QComboBox::currentTextChanged, populateSizeCombo);

    table->setCellWidget(newRow, 0, shapeCombo);
    table->setCellWidget(newRow, 1, sizeCombo);
}

void AddCatalog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (ui->diaTable->hasFocus()) {
            addTableRow(ui->diaTable, "diamond");
        } else if (ui->stoneTable->hasFocus()) {
            addTableRow(ui->stoneTable, "stone");
        }
    } else if (event->key() == Qt::Key_Delete) {
        QTableWidget *focusedTable =
            ui->diaTable->hasFocus() ? ui->diaTable :
                ui->stoneTable->hasFocus() ? ui->stoneTable : nullptr;

        if (focusedTable && focusedTable->currentRow() >= 0) {
            if (QMessageBox::question(this, "Confirm Deletion", "Delete this row?") == QMessageBox::Yes) {
                int row = focusedTable->currentRow();

                // Explicitly delete widgets in the row to prevent leaks
                for (int col = 0; col < focusedTable->columnCount(); ++col) {
                    QWidget *cellWidget = focusedTable->cellWidget(row, col);
                    if (cellWidget) {
                        delete cellWidget;
                    }
                }

                focusedTable->removeRow(row);
            }
        }
    } else {
        QDialog::keyPressEvent(event);
    }
}

void AddCatalog::setupGoldTable()
{
    QList<int> karats = {24, 22, 20, 18, 14, 10};
    ui->goldTable->setRowCount(karats.size());
    ui->goldTable->setColumnCount(2); // ensure at least 2 columns

    for (int i = 0; i < karats.size(); ++i) {
        // Column 0: karat (read-only)
        QTableWidgetItem *karatItem = new QTableWidgetItem(QString::number(karats[i]) + "kt");
        karatItem->setFlags(karatItem->flags() & ~Qt::ItemIsEditable);
        ui->goldTable->setItem(i, 0, karatItem);

        // Column 1: editable weight
        ui->goldTable->setItem(i, 1, new QTableWidgetItem());
    }

    // Connect weight editing → triggers only when 24kt weight is modified
    connect(ui->goldTable, &QTableWidget::itemChanged,
            this, &AddCatalog::calculateGoldWeights);
}

void AddCatalog::calculateGoldWeights(QTableWidgetItem *item)
{
    // Only recalc when 24kt weight is changed (row 0, column 1)
    if (!item || item->column() != 1 || item->row() != 0) return;

    bool ok;
    double weight24kt = item->text().toDouble(&ok);
    if (!ok || weight24kt <= 0) return;

    QList<int> karats = {24, 22, 20, 18, 14, 10};

    if (!ui->goldTable) return; // Safety check

    // Prevent recursive signals when updating table programmatically
    ui->goldTable->blockSignals(true);

    for (int i = 1; i < karats.size(); ++i) {
        // Check that item exists before accessing
        QTableWidgetItem *w = ui->goldTable->item(i, 1);
        if (!w) {
            w = new QTableWidgetItem();
            ui->goldTable->setItem(i, 1, w); // create if missing
        }

        double newWeight = (karats[i] / 24.0) * weight24kt;
        w->setText(QString::number(newWeight, 'f', 3));
    }

    ui->goldTable->blockSignals(false);
}

void AddCatalog::on_save_insert_clicked()
{
    QString imagePath = ui->imagPath_lineEdit->text();
    QString designNo = ui->designNO_lineEdit->text();
    QString companyName = ui->companyName_lineEdit->text();
    QString note = ui->note->toPlainText();

    // if (imagePath.isEmpty() || designNo.isEmpty() || selectedImageType.isEmpty() || companyName.isEmpty()) {
    //     QMessageBox::warning(this, "Input Error", "All fields must be filled!");
    //     return;
    // }

    if (designNo.isEmpty()){
        QMessageBox::warning(this, "Input Error", "Design Number must be filled!") ;
    }

    // Build diamond JSON
    QJsonArray diamondArray;
    for (int row = 0; row < ui->diaTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *combo = qobject_cast<QComboBox*>(ui->diaTable->cellWidget(row, 0))) rowObject["type"] = combo->currentText();
        if (auto *combo = qobject_cast<QComboBox*>(ui->diaTable->cellWidget(row, 1))) rowObject["sizeMM"] = combo->currentText();
        if (auto *item = ui->diaTable->item(row, 2)) rowObject["quantity"] = item->text();
        diamondArray.append(rowObject);
    }

    // Build stone JSON
    QJsonArray stoneArray;
    for (int row = 0; row < ui->stoneTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *combo = qobject_cast<QComboBox*>(ui->stoneTable->cellWidget(row, 0))) rowObject["type"] = combo->currentText();
        if (auto *combo = qobject_cast<QComboBox*>(ui->stoneTable->cellWidget(row, 1))) rowObject["sizeMM"] = combo->currentText();
        if (auto *item = ui->stoneTable->item(row, 2)) rowObject["quantity"] = item->text();
        stoneArray.append(rowObject);
    }

    // Build gold JSON
    QJsonArray goldArray;
    for (int row = 1; row < ui->goldTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *item = ui->goldTable->item(row, 0)) rowObject["karat"] = item->text();
        if (auto *item = ui->goldTable->item(row, 1)) rowObject["weight(g)"] = item->text();
        goldArray.append(rowObject);
    }

    // Save image
    QString newImagePath = DatabaseUtils::saveImage(imagePath);
    if (newImagePath.isEmpty()) {
        QMessageBox::warning(this, "File Error", "Failed to save the image!");
        return;
    }

    // Insert DB record
    QString successReturn = DatabaseUtils::insertCatalogData(newImagePath, selectedImageType, designNo,
                                                             companyName, goldArray, diamondArray, stoneArray, note) ;
    if (successReturn == "error") {
        QMessageBox::critical(this, "Insert Error", "Failed to insert data into database!");
        return;
    }else if (successReturn == "insert") {
        QMessageBox::information(this, "Success", "Data inserted successfully!");
    } else if (successReturn == "modify") {
        QMessageBox::information(this, "Success", "Data updated successfully!");
        ui->designNO_lineEdit->setEnabled(true) ;
        ui->catalog_stacked->setCurrentIndex(2) ;
        isModifyMode = false;
    } else {
        QMessageBox::warning(this, "Error", "Error");
    }


    // Clear fields safely
    ui->imagPath_lineEdit->clear();
    ui->designNO_lineEdit->clear();
    selectedImageType.clear();
    ui->jewelryButton->setText("select jewelry type");
    ui->imageView_label_at_addImage->clear();

    // Safely clear diamond + stone tables (delete widgets first)
    auto clearTable = [](QTableWidget *table) {
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QWidget *w = table->cellWidget(row, col);
                if (w) delete w;
            }
        }
        table->setRowCount(0);
    };
    clearTable(ui->diaTable);
    clearTable(ui->stoneTable);

    // Reset gold weights
    for (int row = 0; row < ui->goldTable->rowCount(); ++row) {
        if (auto *item = ui->goldTable->item(row, 1)) item->setText("");
    }

    ui->note->clear();
}

void AddCatalog::on_brows_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif)");

    if (filePath.isEmpty())
        return;

    ui->imagPath_lineEdit->setText(filePath);

    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Image Error", "Failed to load the selected image.");
        return;
    }

    int labelWidth = ui->imageView_label_at_addImage->width();
    int labelHeight = ui->imageView_label_at_addImage->height();

    if (labelWidth > 0 && labelHeight > 0) {
        ui->imageView_label_at_addImage->setPixmap(
            pixmap.scaled(labelWidth, labelHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void AddCatalog::on_goldTable_cellChanged(int row, int column)
{
    if (column != 1) return; // Only format weight column

    if (auto *item = ui->goldTable->item(row, column)) {
        bool ok;
        double value = item->text().toDouble(&ok);
        if (ok) {
            ui->goldTable->blockSignals(true);
            item->setText(QString::number(value, 'f', 3));
            ui->goldTable->blockSignals(false);
        }
    }
}

void AddCatalog::on_addCatalog_cancel_button_clicked()
{
    if (isModifyMode == true) {
        ui->catalog_stacked->setCurrentIndex(2) ;
        isModifyMode = false ;
        return ;
    }
    if (parentWidget() && !parentWidget()->isVisible()) {
        parentWidget()->show();
    }
    this->close();
}

void AddCatalog::resetAddCatalogUI()
{
    // Clear all input fields
    ui->imagPath_lineEdit->clear();
    ui->designNO_lineEdit->clear();
    ui->designNO_lineEdit->setEnabled(true);
    ui->companyName_lineEdit->clear();
    ui->note->clear();

    // Reset jewelry button
    ui->jewelryButton->setText("Select Jewelry Type");
    selectedImageType.clear();

    // Clear image view
    ui->imageView_label_at_addImage->clear();

    // Clear tables
    ui->goldTable->setRowCount(0);
    ui->diaTable->setRowCount(0);
    ui->stoneTable->setRowCount(0);

    // Reinitialize gold table
    setupGoldTable();

    // Exit modify mode
    isModifyMode = false;
}

void AddCatalog::on_add_catalog_button_released()
{
    if (isModifyMode) {
        auto result = QMessageBox::warning(
            this,
            "Cancel Modification",
            "You are currently modifying an item.\n"
            "Do you want to cancel modification and switch to Add Catalog?",
            QMessageBox::Ok | QMessageBox::Cancel
            );

        if (result == QMessageBox::Ok) {
            resetAddCatalogUI(); // Clear everything
            ui->catalog_stacked->setCurrentIndex(0);
        } else {
            return;
        }


        // Cancel modify mode
        isModifyMode = false;

        // Re-enable designNo editing since we are switching back
        ui->designNO_lineEdit->setEnabled(true);
    }

    ui->catalog_stacked->setCurrentIndex(0);
}

void AddCatalog::loadCatalogForModify()
{
    qDebug() << "[ModifyView] Loading catalog data for modify/delete...";

    modifyCatalogModel->removeRows(0, modifyCatalogModel->rowCount());

    QSqlDatabase db = QSqlDatabase::database("modify_catalog_conn");
    if (!db.isOpen()) {
        qWarning() << "[DB ERROR] Database not open!";
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT image_path, image_type, design_no, company_name FROM image_data WHERE \"delete\" = 0")) {
        qWarning() << "[DB ERROR]" << query.lastError().text();
        return;
    }

    const QSize iconSize(160, 160);

    auto resolveImagePath = [](const QString &path) {
        QString fullPath = path;
        if (!QFile::exists(fullPath)) {
            QString alt = QDir(QCoreApplication::applicationDirPath()).filePath(path);
            if (QFile::exists(alt))
                fullPath = alt;
        }
        return fullPath;
    };

    while (query.next()) {
        QString designNo = query.value("design_no").toString();
        QString company = query.value("company_name").toString();
        QString fullPath = resolveImagePath(query.value("image_path").toString());

        QPixmap pix = QFile::exists(fullPath)
                          ? QPixmap(fullPath)
                          : QPixmap(":/icon/no_image_1.png");

        if (pix.isNull()) {
            qWarning() << "Failed to load image:" << fullPath;
            continue;
        }

        auto *item = new QStandardItem(QIcon(pix.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)),
                                       QString("%1\n%2").arg(designNo, company));
        item->setEditable(false);
        item->setData(fullPath, Qt::UserRole + 1);
        item->setData(designNo, Qt::UserRole + 2);
        item->setData(company, Qt::UserRole + 3);
        modifyCatalogModel->appendRow(item);
    }

    qDebug() << "[ModifyView] Loaded" << modifyCatalogModel->rowCount() << "items.";
}

void AddCatalog::modifyClickedAction(const QString &designNo)
{
    // ---------- Database ----------
    QSqlDatabase db;
    if (QSqlDatabase::contains("modify_catalog_conn"))
        db = QSqlDatabase::database("modify_catalog_conn");
    else {
        db = QSqlDatabase::addDatabase("QSQLITE", "modify_catalog_conn");
        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                               .filePath("database/mega_mine_image.db"));
    }

    if (!db.isOpen() && !db.open()) {
        qWarning() << "[DB ERROR]" << db.lastError().text();
        return;
    }

    // ---------- Query ----------
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT image_path, image_type, company_name, note,
               gold_weight, diamond, stone
        FROM image_data
        WHERE design_no = :design_no AND "delete" = 0
    )");
    query.bindValue(":design_no", designNo);

    if (!query.exec() || !query.next()) {
        QMessageBox::warning(this, "Not Found", "No record found for design: " + designNo);
        return;
    }

    // ---------- Extract Values ----------
    QString relImagePath = query.value("image_path").toString();
    QString absImagePath = QDir(QCoreApplication::applicationDirPath()).filePath(relImagePath);
    QString imageType = query.value("image_type").toString();
    QString companyName = query.value("company_name").toString();
    QString note = query.value("note").toString();
    QString goldJson = query.value("gold_weight").toString();
    QString diamondJson = query.value("diamond").toString();
    QString stoneJson = query.value("stone").toString();

    // ---------- UI Reset ----------
    ui->imagPath_lineEdit->setText(absImagePath);
    ui->designNO_lineEdit->setText(designNo);
    ui->companyName_lineEdit->setText(companyName);
    ui->note->setText(note);
    ui->jewelryButton->setText(imageType);
    ui->designNO_lineEdit->setEnabled(false);
    selectedImageType = imageType;

    // ---------- Load Image ----------
    auto loadImageToLabel = [this](const QString &path) {
        QString realPath = QFile::exists(path)
        ? path
        : QDir(QCoreApplication::applicationDirPath()).filePath(path);
        QPixmap pixmap = QFile::exists(realPath)
                             ? QPixmap(realPath)
                             : QPixmap(":/icon/no_image_1.png");

        if (pixmap.isNull()) {
            qWarning() << "[UI] Image missing:" << realPath;
            return;
        }

        const int w = ui->imageView_label_at_addImage->width();
        const int h = ui->imageView_label_at_addImage->height();
        ui->imageView_label_at_addImage->setPixmap(
            pixmap.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    };
    loadImageToLabel(absImagePath);

    // ---------- Load JSON Tables ----------
    ui->goldTable->setRowCount(0);
    ui->diaTable->setRowCount(0);
    ui->stoneTable->setRowCount(0);

    // --- Helper to populate QTableWidget from JSON ---
    auto populateTable = [](QTableWidget *table, const QJsonArray &array,
                            const QStringList &shapes = {}, const QString &type = QString()) {
        for (const QJsonValue &val : array) {
            const QJsonObject obj = val.toObject();
            const int row = table->rowCount();
            table->insertRow(row);

            if (type == "gold") {
                table->setItem(row, 0, new QTableWidgetItem(obj["karat"].toString()));
                table->setItem(row, 1, new QTableWidgetItem(obj["weight(g)"].toString()));
            } else {
                QComboBox *shapeBox = new QComboBox(table);
                shapeBox->addItems(shapes);
                shapeBox->setCurrentText(obj["type"].toString());

                QComboBox *sizeBox = new QComboBox(table);
                sizeBox->addItems(DatabaseUtils::fetchSizes(type, obj["type"].toString()));
                sizeBox->setCurrentText(obj["sizeMM"].toString());

                table->setCellWidget(row, 0, shapeBox);
                table->setCellWidget(row, 1, sizeBox);
                table->setItem(row, 2, new QTableWidgetItem(obj["quantity"].toString()));
            }
        }
    };

    // --- Parse & Fill Gold ---
    QJsonDocument goldDoc = QJsonDocument::fromJson(goldJson.toUtf8());
    if (goldDoc.isArray()) {
        populateTable(ui->goldTable, goldDoc.array(), {}, "gold");
    } else {
        qWarning() << "[WARN] Invalid gold JSON for design:" << designNo;
    }

    // Cache shapes to reduce DB hits
    const QStringList diamondShapes = DatabaseUtils::fetchShapes("diamond");
    const QStringList stoneShapes = DatabaseUtils::fetchShapes("stone");

    // --- Parse & Fill Diamonds ---
    QJsonDocument diaDoc = QJsonDocument::fromJson(diamondJson.toUtf8());
    if (diaDoc.isArray()) {
        populateTable(ui->diaTable, diaDoc.array(), diamondShapes, "diamond");
    } else {
        qWarning() << "[WARN] Invalid diamond JSON for design:" << designNo;
    }

    // --- Parse & Fill Stones ---
    QJsonDocument stoneDoc = QJsonDocument::fromJson(stoneJson.toUtf8());
    if (stoneDoc.isArray()) {
        populateTable(ui->stoneTable, stoneDoc.array(), stoneShapes, "stone");
    } else {
        qWarning() << "[WARN] Invalid stone JSON for design:" << designNo;
    }

    // ---------- Finalize ----------
    ui->catalog_stacked->setCurrentIndex(0);
    isModifyMode = true;

    qDebug() << "[Modify Mode] Loaded design:" << designNo << "Type:" << imageType;
}

void AddCatalog::deleteClickedAction(const QString &designNo) {
    QMessageBox::StandardButton confirmDelete = QMessageBox::question(this, "Delete Confirmation", "Are you sure you want to delete design " + designNo + "?", QMessageBox::Yes | QMessageBox::No) ;

    if (confirmDelete == QMessageBox::Yes) {
        QSqlDatabase db;
        if (QSqlDatabase::contains("modify_catalog_conn")){
            db = QSqlDatabase::database("modify_catalog_conn");
            qDebug() << "in if " ;
        } else {
            db = QSqlDatabase::addDatabase("QSQLITE", "modify_catalog_conn");
            db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                                   .filePath("database/mega_mine_image.db"));
            qDebug() << "in else " ;
        }


        if (!db.open()){
            db.open() ;
        }
        QSqlQuery query(db) ;
        query.prepare(R"(UPDATE image_data SET "delete" = 1 WHERE design_no = :design_no)") ;
        query.bindValue(":design_no", designNo) ;
        query.exec() ;

        db.close() ;

    }

}

void AddCatalog::onModifyCatalogContextMenuRightClicked(const QPoint &pos) {
    QModelIndex index = modifyCatalogView->indexAt(pos) ;
    if(!index.isValid()){
        qDebug() << "Index is not valid." << index ;
        return ;
    }

    qDebug() << "Index : " << index ;

    QString designNo = index.data(Qt::DisplayRole).toString().section('\n', 0, 0).trimmed();

    QMenu modifyRightClickMenu ;
    QAction *modifyDesignAct = modifyRightClickMenu.addAction("Modify") ;
    QAction *deleteDesignAct = modifyRightClickMenu.addAction("Delete") ;

    QAction *selectedAct = modifyRightClickMenu.exec(modifyCatalogView->viewport()->mapToGlobal(pos)) ;

    if (!selectedAct){
        return ;
    }
    if( selectedAct == modifyDesignAct) {
        AddCatalog::modifyClickedAction(designNo) ;
    } else if (selectedAct == deleteDesignAct) {
        qDebug() << "delete clicked for design. " ;
        AddCatalog::deleteClickedAction(designNo) ;
        modifyCatalogModel->removeRow(index.row()) ;

    }


}

void AddCatalog::setupModifyCatalogView()
{
    QWidget *modifyPage = ui->catalog_stacked->widget(2);
    if (!modifyPage) return;

    // --- Ensure layout ---
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(modifyPage->layout());
    if (!layout) {
        layout = new QVBoxLayout(modifyPage);
        modifyPage->setLayout(layout);
    }

    // --- Ensure view exists ---
    if (!modifyCatalogView) {
        modifyCatalogView = new QListView(modifyPage);
        modifyCatalogView->setViewMode(QListView::IconMode);
        modifyCatalogView->setIconSize(QSize(120, 120));
        modifyCatalogView->setGridSize(QSize(160, 160));
        modifyCatalogView->setResizeMode(QListView::Adjust);
        modifyCatalogView->setUniformItemSizes(true);
        modifyCatalogView->setSelectionMode(QAbstractItemView::SingleSelection);
        modifyCatalogView->setSpacing(12);
        modifyCatalogView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        modifyCatalogView->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(modifyCatalogView, &QListView::customContextMenuRequested,
                this, &AddCatalog::onModifyCatalogContextMenuRightClicked);

        layout->addWidget(modifyCatalogView);
    }

    // --- Ensure models exist ---
    if (!modifyCatalogModel)
        modifyCatalogModel = new QStandardItemModel(this);

    if (!filterModel) {
        filterModel = new QSortFilterProxyModel(this);
        filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        filterModel->setFilterRole(Qt::UserRole);
        filterModel->setDynamicSortFilter(true);
        filterModel->setSourceModel(modifyCatalogModel);
    }

    // ✅ Always attach the proxy model to the view
    modifyCatalogView->setModel(filterModel);

    // --- Ensure single connection for double-click ---
    static bool connected = false;
    if (!connected) {
        connect(modifyCatalogView, &QListView::doubleClicked, this, [this](const QModelIndex &proxyIndex) {
            qDebug() << "[ModifyView] Double-click detected.";

            if (!proxyIndex.isValid()) return;

            QModelIndex sourceIndex = filterModel ? filterModel->mapToSource(proxyIndex) : proxyIndex;
            if (!sourceIndex.isValid()) {
                qWarning() << "[ModifyView] Invalid source index after mapping!";
                return;
            }

            QStandardItem *item = modifyCatalogModel->itemFromIndex(sourceIndex);
            if (!item) {
                qWarning() << "[ModifyView] itemFromIndex returned nullptr!";
                return;
            }

            QString designNo = item->data(Qt::UserRole + 2).toString();
            qDebug() << item->data().toString();
            if (designNo.isEmpty()) {
                qWarning() << "[ModifyView] Empty design number!";
                return;
            }

            // --- Delete / Modify handling ---
            if (deleteIsSet) {
                deleteClickedAction(designNo);
                modifyCatalogModel->removeRow(sourceIndex.row());
                QMessageBox::information(this, "Deleted", "Design " + designNo + " has been deleted.");
            } else {
                modifyClickedAction(designNo);
            }
        });
        connected = true;
    }
}

void AddCatalog::closeEvent(QCloseEvent *event)
{
    qDebug() << "[AddCatalog] Closing window...";

    // Ensure model pointers are safely deleted before removing DB
    if (modifyCatalogModel) {
        modifyCatalogModel->clear();
        delete modifyCatalogModel;
        modifyCatalogModel = nullptr;
    }

    if (modifyCatalogView) {
        modifyCatalogView->setModel(nullptr);
        delete modifyCatalogView;
        modifyCatalogView = nullptr;
    }

    // Close and safely remove the modify_catalog_conn
    if (QSqlDatabase::contains("modify_catalog_conn")) {
        {
            QSqlDatabase db = QSqlDatabase::database("modify_catalog_conn", false);
            if (db.isOpen()) {
                db.close();
                qDebug() << "[DB] modify_catalog_conn closed.";
            }
        }
        QSqlDatabase::removeDatabase("modify_catalog_conn");
        qDebug() << "[DB] modify_catalog_conn removed.";
    }

    QWidget::closeEvent(event);  // Call base implementation
}

void AddCatalog::loadModifyCatalogData()
{
    // --- Ensure model exists ---
    if (!modifyCatalogModel) {
        qWarning() << "[Catalog] modifyCatalogModel is null — initializing...";
        modifyCatalogModel = new QStandardItemModel(this);
        if (modifyCatalogView)
            modifyCatalogView->setModel(modifyCatalogModel);
    }
    modifyCatalogModel->clear();

    // --- Database setup ---
    QSqlDatabase db;
    if (QSqlDatabase::contains("modify_catalog_conn")) {
        db = QSqlDatabase::database("modify_catalog_conn");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", "modify_catalog_conn");
        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                               .filePath("database/mega_mine_image.db"));
    }

    if (!db.isOpen() && !db.open()) {
        qWarning() << "[DB ERROR] Unable to open database:" << db.lastError().text();
        return;
    }

    // --- Fetch catalog data ---
    QSqlQuery query(db);
    if (!query.exec(R"(SELECT design_no, company_name, image_path
                        FROM image_data
                        WHERE "delete" = 0)")) {
        qWarning() << "[DB ERROR] Query failed:" << query.lastError().text();
        return;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    int itemCount = 0;

    // --- Iterate results ---
    while (query.next()) {
        const QString designNo = query.value("design_no").toString();
        QString company = query.value("company_name").toString();
        QString imageRelPath = query.value("image_path").toString();

        // --- Resolve absolute path ---
        QString absImagePath = QDir(appDir).filePath(imageRelPath);
        if (!QFile::exists(absImagePath)) {
            qWarning() << "[WARN] Missing image for design:" << designNo << "->" << absImagePath;
            absImagePath = ":/icon/no_image_1.png";
        }

        // --- Load & scale preview ---
        QPixmap pix(absImagePath);
        if (pix.isNull())
            pix.load(":/icon/no_image_1.png");
        QIcon icon(pix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // --- Trim company name for UI ---
        if (company.length() > 25)
            company = company.left(25) + "…";

        // --- Create model item ---
        auto *item = new QStandardItem(icon, designNo + "\n" + company);
        item->setEditable(false);
        item->setTextAlignment(Qt::AlignCenter);
        item->setSizeHint(QSize(160, 160));
        item->setData(designNo, Qt::UserRole);

        modifyCatalogModel->appendRow(item);
        ++itemCount;
    }

    qInfo() << "[Catalog] Loaded" << itemCount << "designs into modify view.";
}

void AddCatalog::on_modify_catalog_button_released()
{
    deleteIsSet = false;
    ui->catalog_stacked->setCurrentIndex(2);

    QWidget *modifyPage = ui->catalog_stacked->widget(2);
    if (!modifyPage)
        return;

    // ✅ Ensure layout
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(modifyPage->layout());
    if (!layout) {
        layout = new QVBoxLayout(modifyPage);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(6);
        modifyPage->setLayout(layout);
    }

    // ✅ Reset models
    delete modifyCatalogModel;
    modifyCatalogModel = new QStandardItemModel(this);

    delete filterModel;
    filterModel = new QSortFilterProxyModel(this);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    filterModel->setFilterRole(Qt::UserRole);
    filterModel->setDynamicSortFilter(true);
    filterModel->setSourceModel(modifyCatalogModel);

    // ✅ Setup view
    setupModifyCatalogView();
    modifyCatalogView->setModel(filterModel);

    // ✅ Search bar
    static QLineEdit *modifySearchBar = nullptr;
    if (!modifySearchBar) {
        modifySearchBar = new QLineEdit(modifyPage);
        modifySearchBar->setPlaceholderText("Search by design number...");
        modifySearchBar->setClearButtonEnabled(true);
        modifySearchBar->setFixedHeight(32);
        layout->insertWidget(0, modifySearchBar);

        connect(modifySearchBar, &QLineEdit::textChanged, this, [this](const QString &text) {
            if (filterModel)
                filterModel->setFilterFixedString(text.trimmed());
        });
    }

    // ✅ Database
    if (!QSqlDatabase::contains("modify_catalog_conn")) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "modify_catalog_conn");
        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                               .filePath("database/mega_mine_image.db"));
        if (!db.open()) {
            QMessageBox::critical(this, "DB Error", db.lastError().text());
            return;
        }
    }

    // loadModifyCatalogData();
    setupModifyCatalogView();
    loadCatalogForModify();

}

void AddCatalog::on_delete_catalog_button_released()
{
    on_modify_catalog_button_released();
    AddCatalog::deleteIsSet = true ;
    ui->catalog_stacked->setCurrentIndex(2);
}

void AddCatalog::on_demoDownloadPushButton_clicked()
{
    // 1️⃣ Get the path of the demo file in your application
    QString appDir = QCoreApplication::applicationDirPath(); // path of .exe
    QString demoFilePath = appDir + "/excel/demo_catalog.xlsx";

    // 2️⃣ Ask user where to save it
    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Save Demo Catalog",
        QDir::homePath() + "/demo_catalog.xlsx", // default file name
        "Excel Files (*.xlsx)"
        );

    if (savePath.isEmpty()) {
        return; // user cancelled
    }

    // 3️⃣ Copy demo file to user's selected location
    if (QFile::copy(demoFilePath, savePath)) {
        QMessageBox::information(this, "Success", "Demo catalog downloaded successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to download demo catalog. File may already exist or path is invalid.");
    }
}


void AddCatalog::on_bulk_import_button_clicked()
{
    QString excelPath = QFileDialog::getOpenFileName(this, "Select Excel File","","Excel Files (*.xlsx)");
    if(excelPath.isEmpty())
        return ;

    if (DatabaseUtils::excelBulkInsertCatalog(excelPath)){
        QMessageBox::information(this, "Success", "Bulk import completed successfully") ;
    } else {
        QMessageBox::critical(this, "Error", "Bulk import failed!") ;
    }

}

