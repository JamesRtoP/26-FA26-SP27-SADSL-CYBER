/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QListWidget *lstConsole;
    QPushButton *btnClear;
    QWidget *widget1;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox;
    QFormLayout *formLayout;
    QLabel *label;
    QSpinBox *spnServerPort;
    QPushButton *btnStartServer;
    QLabel *lblConnectionStatus;
    QGroupBox *groupBox_2;
    QFormLayout *formLayout_2;
    QLabel *label_2;
    QLineEdit *lnMessage;
    QPushButton *btnSendToAll;
    QSpacerItem *verticalSpacer;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(750, 524);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        widget = new QWidget(centralwidget);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(440, 50, 258, 351));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        lstConsole = new QListWidget(widget);
        lstConsole->setObjectName("lstConsole");

        verticalLayout->addWidget(lstConsole);

        btnClear = new QPushButton(widget);
        btnClear->setObjectName("btnClear");

        verticalLayout->addWidget(btnClear);

        widget1 = new QWidget(centralwidget);
        widget1->setObjectName("widget1");
        widget1->setGeometry(QRect(70, 50, 321, 311));
        verticalLayout_2 = new QVBoxLayout(widget1);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(widget1);
        groupBox->setObjectName("groupBox");
        formLayout = new QFormLayout(groupBox);
        formLayout->setObjectName("formLayout");
        label = new QLabel(groupBox);
        label->setObjectName("label");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, label);

        spnServerPort = new QSpinBox(groupBox);
        spnServerPort->setObjectName("spnServerPort");
        spnServerPort->setMinimum(1024);
        spnServerPort->setMaximum(65535);

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, spnServerPort);

        btnStartServer = new QPushButton(groupBox);
        btnStartServer->setObjectName("btnStartServer");

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, btnStartServer);

        lblConnectionStatus = new QLabel(groupBox);
        lblConnectionStatus->setObjectName("lblConnectionStatus");
        lblConnectionStatus->setMinimumSize(QSize(25, 25));
        lblConnectionStatus->setMaximumSize(QSize(25, 25));
        lblConnectionStatus->setStyleSheet(QString::fromUtf8("#lblConnectionStatus[state = \"0\"]{\n"
"	background-color: rgb(234, 0, 0);\n"
"}\n"
"\n"
"#lblConnectionStatus[state = \"1\"]{\n"
"	background-color: rgb(67, 135, 100);\n"
"}"));

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, lblConnectionStatus);


        verticalLayout_2->addWidget(groupBox);

        groupBox_2 = new QGroupBox(widget1);
        groupBox_2->setObjectName("groupBox_2");
        formLayout_2 = new QFormLayout(groupBox_2);
        formLayout_2->setObjectName("formLayout_2");
        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");

        formLayout_2->setWidget(0, QFormLayout::ItemRole::LabelRole, label_2);

        lnMessage = new QLineEdit(groupBox_2);
        lnMessage->setObjectName("lnMessage");

        formLayout_2->setWidget(0, QFormLayout::ItemRole::FieldRole, lnMessage);

        btnSendToAll = new QPushButton(groupBox_2);
        btnSendToAll->setObjectName("btnSendToAll");

        formLayout_2->setWidget(1, QFormLayout::ItemRole::FieldRole, btnSendToAll);


        verticalLayout_2->addWidget(groupBox_2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 750, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);
        QObject::connect(btnClear, &QPushButton::clicked, lstConsole, qOverload<>(&QListWidget::clear));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        btnClear->setText(QCoreApplication::translate("MainWindow", "Clear", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "Server Information", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Port:", nullptr));
        btnStartServer->setText(QCoreApplication::translate("MainWindow", "Start Server", nullptr));
        lblConnectionStatus->setText(QString());
        lblConnectionStatus->setProperty("state", QVariant(QCoreApplication::translate("MainWindow", "0", nullptr)));
        groupBox_2->setTitle(QCoreApplication::translate("MainWindow", "Message Information", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Message: ", nullptr));
        btnSendToAll->setText(QCoreApplication::translate("MainWindow", "Send To All", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
