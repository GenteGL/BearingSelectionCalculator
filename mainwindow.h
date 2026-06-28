#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <vector>
#include <string>

class QLabel;
class QLineEdit;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct Bearing {
    std::string number;
    double d;
    double D;
    double B;
    double C;
    double C0;
    double f0;
    double p;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onCalculateClicked();

private:
    Ui::MainWindow *ui;
    std::vector<Bearing> bearings;

    QLineEdit *inputFr = nullptr;
    QLineEdit *inputFa = nullptr;
    QLineEdit *inputN = nullptr;
    QLineEdit *inputL10h = nullptr;
    QPushButton *calculateButton = nullptr;

    QLabel *resultTitle = nullptr;
    QLabel *resultBadge = nullptr;
    QLabel *dimensionsLabel = nullptr;
    QLabel *capacityValueLabel = nullptr;
    QLabel *capacityNeedLabel = nullptr;
    QLabel *resourceValueLabel = nullptr;
    QLabel *noteTextLabel = nullptr;
    QLabel *statusIconLabel = nullptr;

    void setupModernUi();
    void loadBearings();
    void showError(const QString &message);
    void showNoResult();
    void showResult(const Bearing &bearing, double neededCapacity, double realResourceHours);
    bool parsePositiveDouble(const QString &text, double &outValue, const QString &fieldName, QString &errorMessage);
    void getEandY(double t, double &e, double &Y);
};

#endif // MAINWINDOW_H
