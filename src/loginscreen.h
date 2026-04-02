#ifndef LOGINSCREEN_H
#define LOGINSCREEN_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class pageLoginScreen;
}
QT_END_NAMESPACE

class LoginScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoginScreen(QWidget *parent = nullptr);
    ~LoginScreen();

    void activate();
    void onApply();

private:
    Ui::pageLoginScreen *ui;
};

#endif // LOGINSCREEN_H
