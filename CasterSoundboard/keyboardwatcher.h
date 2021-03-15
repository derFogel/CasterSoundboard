#ifndef KEYBOARDWATCHER_H
#define KEYBOARDWATCHER_H

#include <QWidget>

class QComboBox;
class QPushButton;
class QSocketNotifier;

class KeyboardWatcher : public QWidget
{
    Q_OBJECT
public:
    KeyboardWatcher(QWidget* parent = nullptr);

    void setTarget(QObject* target);

private:
    QComboBox* keyboardSelector;
    QPushButton* capture;
    QPushButton* scanKeyboards;
    QSocketNotifier* watcher;
    QObject* target;
    bool capturing;
    int inputfile;

public slots:
    void findKeyboards();
    void toggleCapture();
    void inputChanged();
};

#endif // KEYBOARDWATCHER_H
