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

// Структура подшипника.
// e, X, Y, p берутся прямо из каталога производителя (хранятся в CSV),
// а не вычисляются по общей таблице — так точнее и проще поддерживать.
struct Bearing {
    std::string number;
    double d, D, B;   // геометрия, мм
    double C, C0;      // динамическая и статическая грузоподъёмность, Н
    double e;           // предельное отношение Fa/Fr
    double X;           // коэффициент радиальной нагрузки (при Fa/Fr > e)
    double Y;           // коэффициент осевой нагрузки (при Fa/Fr > e)
    double p;           // показатель степени кривой усталости (3 — шариковые, 10/3 — роликовые)
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

    void loadBearings(); // функция загрузки CSV

    // Возвращает true и заполняет outValue, если текст — корректное
    // неотрицательное число. Используется для валидации полей ввода.
    bool parsePositiveDouble(const QString &text, double &outValue, const QString &fieldName, QString &errorMessage);
};

#endif // MAINWINDOW_H