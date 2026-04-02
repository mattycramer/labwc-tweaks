#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "appearance.h"
#include "defaultapps.h"
#include "display.h"
#include "behaviour.h"
#include "mouse.h"
#include "keyboard.h"
#include "touchscreen.h"
#include "loginscreen.h"
#include "about.h"
#include "template.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QStandardPaths>
#include <QTextStream>
#include <string>
#include <unistd.h>
#include "environment.h"
#include "find-themes.h"
#include "log.h"
#include "macros.h"
#include "maindialog.h"
#include "xml.h"

MainDialog::MainDialog(QWidget *parent) : QDialog(parent)
{
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(6, 6, 6, 6);

    QWidget *widget = new QWidget(this);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(widget);
    horizontalLayout->setContentsMargins(6, 6, 6, 6);

    // List Widget on the Left
    QListWidget *list = new QListWidget(widget);

    QListWidgetItem *item0 = new QListWidgetItem(list);
    item0->setIcon(QIcon::fromTheme("applications-graphics"));
    item0->setText(tr("Appearance"));

    QListWidgetItem *item1 = new QListWidgetItem(list);
    item1->setIcon(QIcon::fromTheme("preferences-desktop-default-applications"));
    item1->setText(tr("Default Apps"));

    QListWidgetItem *item2 = new QListWidgetItem(list);
    item2->setIcon(QIcon::fromTheme("video-display"));
    item2->setText(tr("Display"));

    QListWidgetItem *item3 = new QListWidgetItem(list);
    item3->setIcon(QIcon::fromTheme("preferences-desktop"));
    item3->setText(tr("Behaviour"));

    QListWidgetItem *item4 = new QListWidgetItem(list);
    item4->setIcon(QIcon::fromTheme("input-mouse"));
    item4->setText(tr("Mouse & Touchpad"));

    QListWidgetItem *item5 = new QListWidgetItem(list);
    item5->setIcon(QIcon::fromTheme("preferences-desktop-keyboard"));
    item5->setText(tr("Keyboard"));

    QListWidgetItem *item6 = new QListWidgetItem(list);
    item6->setIcon(QIcon::fromTheme("preferences-desktop-touchscreen"));
    item6->setText(tr("Touchscreen"));

    QListWidgetItem *item7 = new QListWidgetItem(list);
    item7->setIcon(QIcon::fromTheme("system-switch-user"));
    item7->setText(tr("Login Screen"));

    QListWidgetItem *item8 = new QListWidgetItem(list);
    item8->setIcon(QIcon::fromTheme("help-about"));
    item8->setText(tr("About"));

    if (!qgetenv("LABWC_TWEAKS_SHOW_TEMPLATE").isEmpty()) {
        QListWidgetItem *item99 = new QListWidgetItem(list);
        item99->setIcon(QIcon::fromTheme("preferences-system"));
        item99->setText("Template");
    }

    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(list->sizePolicy().hasHeightForWidth());
    list->setSizePolicy(sizePolicy);
    list->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
    list->setCurrentRow(0);
    list->setFixedWidth(list->sizeHintForColumn(0) + 2 * list->frameWidth());

    horizontalLayout->addWidget(list);

    // The stack containing all the pages
    QStackedWidget *stack = new QStackedWidget(widget);

    m_pageAppearance = new Appearance();
    stack->addWidget(m_pageAppearance);

    m_pageDefaultApps = new DefaultApps();
    stack->addWidget(m_pageDefaultApps);

    m_pageDisplay = new DisplayPage();
    stack->addWidget(m_pageDisplay);

    m_pageBehaviour = new Behaviour();
    stack->addWidget(m_pageBehaviour);

    m_pageMouse = new Mouse();
    stack->addWidget(m_pageMouse);

    m_pageKeyboard = new Keyboard();
    stack->addWidget(m_pageKeyboard);

    m_pageTouchscreen = new Touchscreen();
    stack->addWidget(m_pageTouchscreen);

    m_pageLoginScreen = new LoginScreen();
    stack->addWidget(m_pageLoginScreen);

    m_pageAbout = new About();
    stack->addWidget(m_pageAbout);

    if (!qgetenv("LABWC_TWEAKS_SHOW_TEMPLATE").isEmpty()) {
        m_pageTemplate = new Template();
        stack->addWidget(m_pageTemplate);
    }

    horizontalLayout->addWidget(stack);

    verticalLayout->addWidget(widget);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Orientation::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Apply
                                    | QDialogButtonBox::StandardButton::Close);
    m_buttonBox->setCenterButtons(false);

    verticalLayout->addWidget(m_buttonBox);

    // Change pages when list items are clicked
    QObject::connect(list, SIGNAL(currentRowChanged(int)), stack, SLOT(setCurrentIndex(int)));

    // Close Button
    QObject::connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Apply Button
    QObject::connect(m_buttonBox, &QDialogButtonBox::clicked, [&](QAbstractButton *button) {
        if (m_buttonBox->standardButton(button) == QDialogButtonBox::Apply) {
            onApply();
        }
    });

    activate();
}

MainDialog::~MainDialog()
{
    xml_finish();
}

// Init settings and setup UI widgets
void MainDialog::activate()
{
    m_pageAppearance->activate();
    m_pageDefaultApps->activate();
    m_pageDisplay->activate();
    m_pageBehaviour->activate();
    m_pageMouse->activate();
    m_pageKeyboard->activate();
    m_pageTouchscreen->activate();
    m_pageLoginScreen->activate();
    m_pageAbout->loadInfo();
    m_pageAbout->getEnv();
    if (!qgetenv("LABWC_TWEAKS_SHOW_TEMPLATE").isEmpty()) {
        m_pageTemplate->activate();
    }
}

void MainDialog::onApply()
{
    m_pageAppearance->onApply();
    m_pageDefaultApps->onApply();
    m_pageDisplay->onApply();
    m_pageBehaviour->onApply();
    m_pageMouse->onApply();
    m_pageKeyboard->onApply();
    m_pageTouchscreen->onApply();
    m_pageLoginScreen->onApply();

    xml_save();
    environmentSave();

    /* reconfigure labwc */
    if (!fork()) {
        execl("/bin/sh", "/bin/sh", "-c", "labwc -r", (void *)NULL);
    }
}
