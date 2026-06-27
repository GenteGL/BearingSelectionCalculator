#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// Структура подшипника — переезжает сюда из старого main.cpp
struct Bearing {
    std::string number;
    double d, D, B, C, C0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onCalculateClicked(); // функция которая сработает при нажатии кнопки

private:
    Ui::MainWindow *ui;
    std::vector<Bearing> bearings; // список всех подшипников из CSV
    void loadBearings();           // функция загрузки CSV
};

#endif // MAINWINDOW_H