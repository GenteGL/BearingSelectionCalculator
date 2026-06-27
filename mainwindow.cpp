#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <cmath>
#include <limits>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadBearings(); // загружаем CSV при запуске
    connect(ui->calculateButton, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadBearings()
{
    QFile file(":/data/bearings.csv");   // путь внутри ресурсов

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Не удалось открыть bearings.csv:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    bool firstLine = true;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // пропускаем строку заголовка, если она есть в csv
        if (firstLine) {
            firstLine = false;
            if (line.startsWith("number", Qt::CaseInsensitive)) {
                continue;
            }
        }

        QStringList parts = line.split(",");
        // Новый формат: number,d,D,B,C,C0,e,X,Y,p — всего 10 полей
        if (parts.size() >= 10) {
            Bearing b;
            b.number = parts[0].toStdString();
            b.d  = parts[1].toDouble();
            b.D  = parts[2].toDouble();
            b.B  = parts[3].toDouble();
            b.C  = parts[4].toDouble();
            b.C0 = parts[5].toDouble();
            b.e  = parts[6].toDouble();
            b.X  = parts[7].toDouble();
            b.Y  = parts[8].toDouble();
            b.p  = parts[9].toDouble();
            bearings.push_back(b);
        }
    }
    file.close();

    qDebug() << "Загружено подшипников:" << bearings.size();
}

bool MainWindow::parsePositiveDouble(const QString &text, double &outValue, const QString &fieldName, QString &errorMessage)
{
    bool ok = false;
    double value = text.toDouble(&ok);

    if (!ok || text.trimmed().isEmpty()) {
        errorMessage = QString("Поле «%1» должно содержать число.").arg(fieldName);
        return false;
    }
    if (value < 0) {
        errorMessage = QString("Поле «%1» не может быть отрицательным.").arg(fieldName);
        return false;
    }

    outValue = value;
    return true;
}

void MainWindow::onCalculateClicked()
{
    double Fr = 0, Fa = 0, n = 0, L10h_desired = 0;
    QString error;

    // --- Валидация ввода ---
    if (!parsePositiveDouble(ui->inputFr->text(), Fr, "Радиальная нагрузка Fr", error) ||
        !parsePositiveDouble(ui->inputFa->text(), Fa, "Осевая нагрузка Fa", error) ||
        !parsePositiveDouble(ui->inputN->text(), n, "Частота вращения n", error) ||
        !parsePositiveDouble(ui->inputL10h->text(), L10h_desired, "Требуемый ресурс L10h", error)) {
        ui->resultLabel->setText("Ошибка ввода: " + error);
        return;
    }

    if (n == 0) {
        ui->resultLabel->setText("Ошибка ввода: частота вращения n не может быть равна нулю.");
        return;
    }
    if (Fr == 0 && Fa == 0) {
        ui->resultLabel->setText("Ошибка ввода: укажите хотя бы одну ненулевую нагрузку (Fr или Fa).");
        return;
    }
    if (L10h_desired == 0) {
        ui->resultLabel->setText("Ошибка ввода: требуемый ресурс должен быть больше нуля.");
        return;
    }

    const double V = 1.0; // вращается внутреннее кольцо (стандартный случай)
    const double L10 = (L10h_desired * 60.0 * n) / 1000000.0; // ресурс в млн. оборотов

    // --- Поиск подходящего подшипника ---
    // Среди ВСЕХ подходящих выбираем подшипник с минимальным C,
    // удовлетворяющим требованию — то есть самый компактный/лёгкий вариант,
    // а не первый по списку.
    const Bearing* best = nullptr;
    double bestC_needed = 0;
    double bestL10h_real = 0;

    for (const Bearing& b : bearings) {
        // Эквивалентная динамическая нагрузка P по ГОСТ 18855-94 / ISO 281
        double ratio = (Fr > 0) ? (Fa / (V * Fr)) : std::numeric_limits<double>::infinity();

        double P;
        if (ratio <= b.e) {
            P = V * Fr;            // осевая нагрузка пренебрежимо мала
        } else {
            P = b.X * V * Fr + b.Y * Fa;
        }

        if (P <= 0)
            continue; // защита от деления на ноль ниже

        double C_needed = P * std::pow(L10, 1.0 / b.p);

        if (b.C >= C_needed) {
            if (best == nullptr || b.C < best->C) {
                best = &b;
                bestC_needed = C_needed;
                bestL10h_real = (std::pow(b.C / P, b.p) * 1000000.0) / (60.0 * n);
            }
        }
    }

    if (best != nullptr) {
        QString result = QString(
                             "Подшипник: %1\n"
                             "d = %2 мм | D = %3 мм | B = %4 мм\n"
                             "C = %5 Н (требовалось ≥ %6 Н)\n"
                             "Ресурс: %7 часов"
                             )
                             .arg(QString::fromStdString(best->number))
                             .arg(best->d).arg(best->D).arg(best->B)
                             .arg(best->C)
                             .arg(QString::number(bestC_needed, 'f', 0))
                             .arg((qint64)bestL10h_real);
        ui->resultLabel->setText(result);
        return;
    }

    ui->resultLabel->setText("Подходящий подшипник не найден.\nУвеличьте диаметр или уменьшите нагрузку.");
}