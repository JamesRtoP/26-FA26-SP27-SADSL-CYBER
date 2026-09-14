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
#include <QtWidgets/QHBoxLayout>
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
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *lnIPAddress;
    QLabel *label_2;
    QSpinBox *spnPort;
    QPushButton *btnConnect;
    QGroupBox *grpSendData;
    QFormLayout *formLayout_2;
    QLabel *label_3;
    QLineEdit *lnMessage;
    QPushButton *btnSend;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout;
    QListWidget *lstConsole;
    QPushButton *btnClear;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->setEnabled(true);
        MainWindow->resize(1048, 660);
        MainWindow->setStyleSheet(QString::fromUtf8("#lnIPAddress[state = \"0\"]{\n"
"	background-color: rgb(234, 0, 0);\n"
"}\n"
"\n"
"#lnIPAddress[state = \"1\"]{\n"
"	background-color: rgb(67, 135, 100);\n"
"}"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName("horizontalLayout");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        formLayout = new QFormLayout(groupBox);
        formLayout->setObjectName("formLayout");
        label = new QLabel(groupBox);
        label->setObjectName("label");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, label);

        lnIPAddress = new QLineEdit(groupBox);
        lnIPAddress->setObjectName("lnIPAddress");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, lnIPAddress);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, label_2);

        spnPort = new QSpinBox(groupBox);
        spnPort->setObjectName("spnPort");
        spnPort->setMinimum(1024);
        spnPort->setMaximum(65535);

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, spnPort);

        btnConnect = new QPushButton(groupBox);
        btnConnect->setObjectName("btnConnect");

        formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, btnConnect);


        verticalLayout_2->addWidget(groupBox);

        grpSendData = new QGroupBox(centralwidget);
        grpSendData->setObjectName("grpSendData");
        grpSendData->setEnabled(true);
        formLayout_2 = new QFormLayout(grpSendData);
        formLayout_2->setObjectName("formLayout_2");
        label_3 = new QLabel(grpSendData);
        label_3->setObjectName("label_3");

        formLayout_2->setWidget(0, QFormLayout::ItemRole::LabelRole, label_3);

        lnMessage = new QLineEdit(grpSendData);
        lnMessage->setObjectName("lnMessage");

        formLayout_2->setWidget(0, QFormLayout::ItemRole::FieldRole, lnMessage);

        btnSend = new QPushButton(grpSendData);
        btnSend->setObjectName("btnSend");
        btnSend->setEnabled(true);

        formLayout_2->setWidget(1, QFormLayout::ItemRole::FieldRole, btnSend);


        verticalLayout_2->addWidget(grpSendData);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);


        horizontalLayout->addLayout(verticalLayout_2);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        lstConsole = new QListWidget(centralwidget);
        lstConsole->setObjectName("lstConsole");

        verticalLayout->addWidget(lstConsole);

        btnClear = new QPushButton(centralwidget);
        btnClear->setObjectName("btnClear");

        verticalLayout->addWidget(btnClear);


        horizontalLayout->addLayout(verticalLayout);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1048, 25));
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
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "Connection information", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "IP Address", nullptr));
        lnIPAddress->setInputMask(QCoreApplication::translate("MainWindow", "000.000.000.000;_", nullptr));
        lnIPAddress->setProperty("state", QVariant(QString()));
        label_2->setText(QCoreApplication::translate("MainWindow", "Port", nullptr));
        btnConnect->setText(QCoreApplication::translate("MainWindow", "Connect", nullptr));
        grpSendData->setTitle(QCoreApplication::translate("MainWindow", "Send Data", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Message", nullptr));
        lnMessage->setText(QString());
        lnMessage->setProperty("state", QVariant(QString()));
        btnSend->setText(QCoreApplication::translate("MainWindow", "Send", nullptr));
        btnClear->setText(QCoreApplication::translate("MainWindow", "Clear", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
