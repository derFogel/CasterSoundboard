#include "keyboardwatcher.h"

#include <QLayout>
#include <QComboBox>
#include <QDir>
#include <QPushButton>
#include <QSocketNotifier>
#include <QDebug>
#include <QKeyEvent>
#include <QApplication>

#include <sys/ioctl.h>
#include <linux/input.h>
#include <fcntl.h>
#include <cstdio>
#include <unistd.h>

KeyboardWatcher::KeyboardWatcher(QWidget* parent)
    : QWidget(parent),
      target(nullptr),
      capturing(false),
      inputfile(0)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);

    keyboardSelector = new QComboBox(this);
    layout->addWidget(keyboardSelector);

    capture = new QPushButton(this);
    capture->setText("Start Capture");
    connect(capture, &QPushButton::clicked, this, &KeyboardWatcher::toggleCapture);
    layout->addWidget(capture);

    scanKeyboards = new QPushButton(this);
    scanKeyboards->setText("Find Keyboards");
    connect(scanKeyboards, &QPushButton::clicked, this, &KeyboardWatcher::findKeyboards);
    layout->addWidget(scanKeyboards);

    findKeyboards();
}

void KeyboardWatcher::setTarget(QObject *target)
{
    this->target = target;
}

void KeyboardWatcher::findKeyboards()
{
    keyboardSelector->clear();

    QDir dir("/dev/input/by-id");
    const QStringList entryList = dir.entryList(QDir::Filter::NoDotAndDotDot);
    for (int i = 0; i < entryList.length(); i++) {
        if (!entryList[i].endsWith("kbd")) continue;
        QString file = "/dev/input/by-id/" + entryList[i];
        QFileInfo info(file);
        int fd = open(info.canonicalFilePath().toStdString().c_str(), O_RDONLY);
        if (fd == -1) continue;
        char name[256];
        ioctl(fd, EVIOCGNAME(256), name);
        ::close(fd);
        keyboardSelector->insertItem(i, QString(name), QVariant(info.canonicalFilePath()));
    }
}

void KeyboardWatcher::toggleCapture()
{
    capturing = !capturing;
    if (capturing) {
        capture->setText("Stop Capture");

        inputfile = open(keyboardSelector->currentData().toByteArray(), O_RDONLY);
        ioctl(inputfile, EVIOCGRAB, 1);

        watcher = new QSocketNotifier(inputfile, QSocketNotifier::Read, this);
        connect(watcher, &QSocketNotifier::activated, this, &KeyboardWatcher::inputChanged);
    } else {
        delete watcher;
        capture->setText("Start Capture");
        ioctl(inputfile, EVIOCGRAB, 0);
        ::close(inputfile);
        inputfile = 0;
    }
}

void KeyboardWatcher::inputChanged()
{
    struct keyevent {
        struct timeval time;
        unsigned short type;
        unsigned short code;
        unsigned int value;
    };

    const static QMap<int, int> lookup = {
        {2, Qt::Key_1}, {3, Qt::Key_2}, {4, Qt::Key_3}, {5, Qt::Key_4}, {6, Qt::Key_5}, {7, Qt::Key_6}, {8, Qt::Key_7}, {9, Qt::Key_8},
        {16, Qt::Key_Q}, {17, Qt::Key_W}, {18, Qt::Key_E}, {19, Qt::Key_R}, {20, Qt::Key_T}, {21, Qt::Key_Y}, {22, Qt::Key_U}, {23, Qt::Key_I},
        {30, Qt::Key_A}, {31, Qt::Key_S}, {32, Qt::Key_D}, {33, Qt::Key_F}, {34, Qt::Key_G}, {35, Qt::Key_H}, {36, Qt::Key_J}, {37, Qt::Key_K},
        {44, Qt::Key_Z}, {45, Qt::Key_X}, {46, Qt::Key_C}, {47, Qt::Key_V}, {48, Qt::Key_B}, {49, Qt::Key_N}, {50, Qt::Key_M}, {51, Qt::Key_Comma},
    };

    if (!inputfile) return;

    keyevent event;
    read(inputfile, &event, sizeof(keyevent));

    if (target) {
        if (lookup.contains(event.code) && event.value == 0) {
            qDebug() << event.value;
            QKeyEvent* release = new QKeyEvent(QEvent::KeyRelease, lookup[event.code], Qt::NoModifier);
            QCoreApplication::postEvent(target, release);
        }
    }
}
