#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <cmath>

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
    QFile file("bearings.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(",");
        if (parts.size() >= 6) {
            Bearing b;
            b.number = parts[0].toStdString();
            b.d  = parts[1].toDouble();
            b.D  = parts[2].toDouble();
            b.B  = parts[3].toDouble();
            b.C  = parts[4].toDouble();
            b.C0 = parts[5].toDouble();
            bearings.push_back(b);
        }
    }
    file.close();
}

void MainWindow::onCalculateClicked()
{
    double Fr = ui->inputFr->text().toDouble();
    double Fa = ui->inputFa->text().toDouble();
    double n  = ui->inputN->text().toDouble();
    double L10h_desired = ui->inputL10h->text().toDouble();

    double P = Fr + 0.5 * Fa;
    double L10 = (L10h_desired * 60 * n) / 1000000.0;
    double C_needed = P * pow(L10, 1.0 / 3.0);

    for (const Bearing& b : bearings) {
        if (b.C >= C_needed) {
            double L10h_real = (pow(b.C / P, 3.0) * 1000000.0) / (60.0 * n);
            QString result = QString("Подшипник: %1\nd = %2 мм | D = %3 мм | B = %4 мм\nC = %5 Н\nРесурс: %6 часов")
                                 .arg(QString::fromStdString(b.number))
                                 .arg(b.d).arg(b.D).arg(b.B).arg(b.C)
                                 .arg((int)L10h_real);
            ui->resultLabel->setText(result);
            return;
        }
    }

    ui->resultLabel->setText("Подходящий подшипник не найден.\nУвеличьте диаметр или уменьшите нагрузку.");
}