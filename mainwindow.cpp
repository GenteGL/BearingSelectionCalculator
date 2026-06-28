#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <QDebug>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

QFrame *makeFrame(const QString &objectName)
{
    auto *frame = new QFrame;
    frame->setObjectName(objectName);
    frame->setFrameShape(QFrame::NoFrame);
    return frame;
}

QLabel *makeLabel(const QString &text, const QString &objectName)
{
    auto *label = new QLabel(text);
    label->setObjectName(objectName);
    label->setWordWrap(true);
    return label;
}

void addShadow(QWidget *widget, int blur = 24, int alpha = 24)
{
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blur);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(26, 48, 86, alpha));
    widget->setGraphicsEffect(shadow);
}

QFrame *makeInputCard(const QString &icon, const QString &labelText, const QString &unit, QLineEdit *input)
{
    auto *card = makeFrame("inputCard");
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(18);

    auto *iconBox = makeLabel(icon, "inputIcon");
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setFixedSize(58, 58);
    layout->addWidget(iconBox);

    auto *fieldLayout = new QVBoxLayout;
    fieldLayout->setSpacing(10);

    auto *captionRow = new QHBoxLayout;
    captionRow->setSpacing(8);
    auto *caption = makeLabel(labelText, "fieldCaption");
    caption->setWordWrap(false);
    auto *unitLabel = makeLabel(unit, "fieldUnit");
    unitLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    captionRow->addWidget(caption, 1);
    captionRow->addWidget(unitLabel);

    input->setObjectName("parameterInput");
    input->setMinimumHeight(48);
    input->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    fieldLayout->addLayout(captionRow);
    fieldLayout->addWidget(input);
    layout->addLayout(fieldLayout, 1);

    return card;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupModernUi();
    loadBearings();
    connect(calculateButton, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
    onCalculateClicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupModernUi()
{
    setWindowTitle("Подбор подшипника");
    resize(1180, 860);
    setMinimumSize(980, 720);
    QApplication::setFont(QFont("Segoe UI", 10));

    auto *root = new QWidget;
    root->setObjectName("appRoot");
    setCentralWidget(root);

    auto *mainLayout = new QHBoxLayout(root);
    mainLayout->setContentsMargins(26, 26, 26, 26);
    mainLayout->setSpacing(22);

    auto *inputPanel = makeFrame("panel");
    auto *resultPanel = makeFrame("panel");
    addShadow(inputPanel);
    addShadow(resultPanel);
    mainLayout->addWidget(inputPanel, 1);
    mainLayout->addWidget(resultPanel, 1);

    auto *inputLayout = new QVBoxLayout(inputPanel);
    inputLayout->setContentsMargins(18, 20, 18, 24);
    inputLayout->setSpacing(18);

    auto *inputHeader = new QHBoxLayout;
    inputHeader->setContentsMargins(0, 0, 0, 12);
    inputHeader->setSpacing(18);
    auto *calculatorIcon = makeLabel("▦", "headerIconBlue");
    calculatorIcon->setAlignment(Qt::AlignCenter);
    calculatorIcon->setFixedSize(76, 76);
    auto *inputTitleBox = new QVBoxLayout;
    inputTitleBox->setSpacing(6);
    inputTitleBox->addWidget(makeLabel("Исходные данные", "panelTitle"));
    inputTitleBox->addWidget(makeLabel("Введите параметры работы узла", "panelSubtitle"));
    inputHeader->addWidget(calculatorIcon);
    inputHeader->addLayout(inputTitleBox, 1);
    inputLayout->addLayout(inputHeader);

    inputFr = new QLineEdit("1000");
    inputFa = new QLineEdit("0");
    inputN = new QLineEdit("1000");
    inputL10h = new QLineEdit("5000");

    inputLayout->addWidget(makeInputCard("◎", "Радиальная нагрузка Fr", "Н", inputFr));
    inputLayout->addWidget(makeInputCard("↔", "Осевая нагрузка Fa", "Н", inputFa));
    inputLayout->addWidget(makeInputCard("↻", "Частота вращения n", "об/мин", inputN));
    inputLayout->addWidget(makeInputCard("◷", "Требуемый ресурс L10h", "ч", inputL10h));
    inputLayout->addStretch(1);

    calculateButton = new QPushButton("▦  Рассчитать");
    calculateButton->setObjectName("calculateButton");
    calculateButton->setMinimumHeight(70);
    inputLayout->addWidget(calculateButton);

    auto *resultLayout = new QVBoxLayout(resultPanel);
    resultLayout->setContentsMargins(18, 20, 18, 24);
    resultLayout->setSpacing(18);

    auto *resultHeader = new QHBoxLayout;
    resultHeader->setContentsMargins(0, 0, 0, 12);
    resultHeader->setSpacing(18);
    auto *bearingIcon = makeLabel("◉", "headerIconGreen");
    bearingIcon->setAlignment(Qt::AlignCenter);
    bearingIcon->setFixedSize(76, 76);
    auto *resultTitleBox = new QVBoxLayout;
    resultTitleBox->setSpacing(6);
    resultTitleBox->addWidget(makeLabel("Результаты подбора", "panelTitle"));
    resultTitleBox->addWidget(makeLabel("Рекомендованный подшипник", "panelSubtitle"));
    resultHeader->addWidget(bearingIcon);
    resultHeader->addLayout(resultTitleBox, 1);
    resultLayout->addLayout(resultHeader);

    auto *resultBody = makeFrame("resultBody");
    auto *bodyLayout = new QVBoxLayout(resultBody);
    bodyLayout->setContentsMargins(16, 18, 16, 18);
    bodyLayout->setSpacing(18);
    resultLayout->addWidget(resultBody, 1);

    auto *selectedCard = makeFrame("selectedCard");
    auto *selectedLayout = new QVBoxLayout(selectedCard);
    selectedLayout->setContentsMargins(24, 22, 24, 22);
    selectedLayout->setSpacing(22);

    auto *selectedTop = new QHBoxLayout;
    selectedTop->setSpacing(16);
    statusIconLabel = makeLabel("✓", "statusIcon");
    statusIconLabel->setAlignment(Qt::AlignCenter);
    statusIconLabel->setFixedSize(58, 58);
    resultTitle = makeLabel("Подшипник не выбран", "resultTitle");
    resultTitle->setWordWrap(false);
    resultBadge = makeLabel("Ожидание", "resultBadge");
    resultBadge->setAlignment(Qt::AlignCenter);
    selectedTop->addWidget(statusIconLabel);
    selectedTop->addWidget(resultTitle, 1);
    selectedTop->addWidget(resultBadge);
    selectedLayout->addLayout(selectedTop);

    dimensionsLabel = makeLabel("d = — мм    |    D = — мм    |    B = — мм", "dimensionsLabel");
    dimensionsLabel->setAlignment(Qt::AlignCenter);
    selectedLayout->addWidget(dimensionsLabel);
    bodyLayout->addWidget(selectedCard);

    auto *capacityCard = makeFrame("metricCard");
    auto *capacityLayout = new QHBoxLayout(capacityCard);
    capacityLayout->setContentsMargins(16, 16, 16, 16);
    capacityLayout->setSpacing(18);
    auto *capacityIcon = makeLabel("◆", "metricIconBlue");
    capacityIcon->setAlignment(Qt::AlignCenter);
    capacityIcon->setFixedSize(54, 54);
    auto *capacityText = new QVBoxLayout;
    capacityText->setSpacing(4);
    capacityText->addWidget(makeLabel("Динамическая грузоподъёмность C", "metricCaption"));
    capacityValueLabel = makeLabel("— Н", "metricValueGreen");
    capacityText->addWidget(capacityValueLabel);
    capacityNeedLabel = makeLabel("(требовалось ≥ — Н)", "metricHint");
    capacityNeedLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    capacityLayout->addWidget(capacityIcon);
    capacityLayout->addLayout(capacityText, 1);
    capacityLayout->addWidget(capacityNeedLabel);
    bodyLayout->addWidget(capacityCard);

    auto *resourceCard = makeFrame("metricCard");
    auto *resourceLayout = new QHBoxLayout(resourceCard);
    resourceLayout->setContentsMargins(16, 16, 16, 16);
    resourceLayout->setSpacing(18);
    auto *resourceIcon = makeLabel("⌛", "metricIconPurple");
    resourceIcon->setAlignment(Qt::AlignCenter);
    resourceIcon->setFixedSize(54, 54);
    auto *resourceText = new QVBoxLayout;
    resourceText->setSpacing(4);
    resourceText->addWidget(makeLabel("Расчётный ресурс", "metricCaption"));
    resourceValueLabel = makeLabel("— часов", "metricValuePurple");
    resourceText->addWidget(resourceValueLabel);
    resourceLayout->addWidget(resourceIcon);
    resourceLayout->addLayout(resourceText, 1);
    bodyLayout->addWidget(resourceCard);

    auto *noteCard = makeFrame("noteCard");
    auto *noteLayout = new QVBoxLayout(noteCard);
    noteLayout->setContentsMargins(18, 16, 18, 16);
    noteLayout->setSpacing(12);
    noteLayout->addWidget(makeLabel("ⓘ  Примечание", "noteTitle"));
    noteTextLabel = makeLabel("Подшипник выбран по динамической грузоподъёмности.\n\nУбедитесь, что условия эксплуатации соответствуют каталогным данным.", "noteText");
    noteLayout->addWidget(noteTextLabel);
    bodyLayout->addWidget(noteCard);
    bodyLayout->addStretch(1);

    setStyleSheet(R"(
        #appRoot {
            background: #f6f9fd;
        }
        #panel {
            background: #ffffff;
            border: 1px solid #dce5f0;
            border-radius: 14px;
        }
        #panelTitle {
            color: #102552;
            font-size: 24px;
            font-weight: 700;
        }
        #panelSubtitle {
            color: #738097;
            font-size: 16px;
        }
        #headerIconBlue {
            background: #eaf2ff;
            border-radius: 18px;
            color: #1f70d8;
            font-size: 36px;
            font-weight: 700;
        }
        #headerIconGreen {
            background: #e5f7ee;
            border-radius: 18px;
            color: #2dae68;
            font-size: 36px;
            font-weight: 700;
        }
        #inputCard, #metricCard {
            background: #ffffff;
            border: 1px solid #dbe4ee;
            border-radius: 12px;
        }
        #inputIcon {
            background: #edf4ff;
            border-radius: 12px;
            color: #216fd4;
            font-size: 30px;
            font-weight: 700;
        }
        #fieldCaption, #metricCaption {
            color: #4b5870;
            font-size: 16px;
        }
        #fieldUnit, #metricHint {
            color: #54627a;
            font-size: 15px;
        }
        #parameterInput {
            background: #ffffff;
            border: 1px solid #ccd7e4;
            border-radius: 8px;
            color: #061431;
            font-size: 17px;
            padding: 0 14px;
            selection-background-color: #1d73dd;
        }
        #parameterInput:focus {
            border: 1px solid #1d73dd;
        }
        #calculateButton {
            background: #1f73da;
            border: none;
            border-radius: 9px;
            color: #ffffff;
            font-size: 20px;
            font-weight: 700;
        }
        #calculateButton:hover {
            background: #1768c9;
        }
        #calculateButton:pressed {
            background: #1159af;
        }
        #resultBody {
            background: #ffffff;
            border: 1px solid #dbe4ee;
            border-radius: 12px;
        }
        #selectedCard {
            background: #fbfffc;
            border: 1px solid #ace7c6;
            border-radius: 12px;
        }
        #statusIcon {
            background: #2fac65;
            border-radius: 29px;
            color: #ffffff;
            font-size: 30px;
            font-weight: 800;
        }
        #resultTitle {
            color: #08142f;
            font-size: 21px;
            font-weight: 800;
        }
        #resultBadge {
            background: #eef2f7;
            border-radius: 16px;
            color: #425069;
            font-size: 15px;
            font-weight: 700;
            padding: 7px 12px;
        }
        #dimensionsLabel {
            color: #202c42;
            font-size: 17px;
        }
        #metricIconBlue {
            background: #edf5ff;
            border: 1px solid #8fb7ff;
            border-radius: 27px;
            color: #236bd1;
            font-size: 24px;
            font-weight: 800;
        }
        #metricIconPurple {
            background: #f4edff;
            border: 1px solid #c9a7ff;
            border-radius: 27px;
            color: #884be4;
            font-size: 22px;
            font-weight: 800;
        }
        #metricValueGreen {
            color: #28a85d;
            font-size: 22px;
            font-weight: 800;
        }
        #metricValuePurple {
            color: #8649df;
            font-size: 22px;
            font-weight: 800;
        }
        #noteCard {
            background: #f8fbff;
            border: 1px solid #cfe2ff;
            border-radius: 10px;
        }
        #noteTitle {
            color: #145fc3;
            font-size: 16px;
            font-weight: 800;
        }
        #noteText {
            color: #263653;
            font-size: 15px;
            line-height: 145%;
        }
    )");
}

void MainWindow::loadBearings()
{
    QFile file(":/data/bearings.csv");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Не удалось открыть bearings.csv:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    bool firstLine = true;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        if (firstLine) {
            firstLine = false;
            if (line.startsWith("number", Qt::CaseInsensitive)) {
                continue;
            }
        }

        const QStringList parts = line.split(",");
        if (parts.size() >= 8) {
            Bearing b;
            b.number = parts[0].toStdString();
            b.d = parts[1].toDouble();
            b.D = parts[2].toDouble();
            b.B = parts[3].toDouble();
            b.C = parts[4].toDouble();
            b.C0 = parts[5].toDouble();
            b.f0 = parts[6].toDouble();
            b.p = parts[7].toDouble();
            bearings.push_back(b);
        }
    }

    qDebug() << "Загружено подшипников:" << bearings.size();
}

bool MainWindow::parsePositiveDouble(const QString &text, double &outValue, const QString &fieldName, QString &errorMessage)
{
    bool ok = false;
    const double value = text.toDouble(&ok);

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

void MainWindow::getEandY(double t, double &e, double &Y)
{
    struct Point {
        double t;
        double e;
        double Y;
    };

    static const Point table[] = {
        {0.172, 0.19, 2.30},
        {0.345, 0.22, 1.99},
        {0.689, 0.26, 1.71},
        {1.03, 0.28, 1.55},
        {1.38, 0.30, 1.45},
        {2.07, 0.34, 1.31},
        {3.45, 0.38, 1.15},
        {5.17, 0.42, 1.04},
        {6.89, 0.44, 1.00},
    };
    const int n = sizeof(table) / sizeof(Point);

    if (t <= table[0].t) {
        e = table[0].e;
        Y = table[0].Y;
        return;
    }
    if (t >= table[n - 1].t) {
        e = table[n - 1].e;
        Y = table[n - 1].Y;
        return;
    }

    for (int i = 0; i < n - 1; i++) {
        if (t >= table[i].t && t <= table[i + 1].t) {
            const double frac = (t - table[i].t) / (table[i + 1].t - table[i].t);
            e = table[i].e + frac * (table[i + 1].e - table[i].e);
            Y = table[i].Y + frac * (table[i + 1].Y - table[i].Y);
            return;
        }
    }

    e = table[n - 1].e;
    Y = table[n - 1].Y;
}

void MainWindow::showError(const QString &message)
{
    statusIconLabel->setText("!");
    resultTitle->setText("Ошибка ввода");
    resultBadge->setText("Проверьте");
    dimensionsLabel->setText("d = — мм    |    D = — мм    |    B = — мм");
    capacityValueLabel->setText("— Н");
    capacityNeedLabel->setText("(требовалось ≥ — Н)");
    resourceValueLabel->setText("— часов");
    noteTextLabel->setText(message);
}

void MainWindow::showNoResult()
{
    statusIconLabel->setText("!");
    resultTitle->setText("Подшипник не найден");
    resultBadge->setText("Нет выбора");
    dimensionsLabel->setText("d = — мм    |    D = — мм    |    B = — мм");
    capacityValueLabel->setText("— Н");
    capacityNeedLabel->setText("(требовалось ≥ — Н)");
    resourceValueLabel->setText("— часов");
    noteTextLabel->setText("Подходящий подшипник не найден. Увеличьте диаметр или уменьшите нагрузку.");
}

void MainWindow::showResult(const Bearing &bearing, double neededCapacity, double realResourceHours)
{
    statusIconLabel->setText("✓");
    resultTitle->setText(QString("Подшипник %1").arg(QString::fromStdString(bearing.number)));
    resultBadge->setText("Подходит");
    dimensionsLabel->setText(QString("d = %1 мм    |    D = %2 мм    |    B = %3 мм")
                                 .arg(bearing.d)
                                 .arg(bearing.D)
                                 .arg(bearing.B));
    capacityValueLabel->setText(QString("%1 Н").arg(QString::number(bearing.C, 'f', 0)));
    capacityNeedLabel->setText(QString("(требовалось ≥ %1 Н)").arg(QString::number(neededCapacity, 'f', 0)));
    resourceValueLabel->setText(QString("%1 часов").arg(QString::number(static_cast<qint64>(realResourceHours))));
    noteTextLabel->setText("Подшипник выбран по динамической грузоподъёмности.\n\nУбедитесь, что условия эксплуатации соответствуют каталогным данным.");
}

void MainWindow::onCalculateClicked()
{
    double Fr = 0;
    double Fa = 0;
    double n = 0;
    double L10hDesired = 0;
    QString error;

    if (!parsePositiveDouble(inputFr->text(), Fr, "Радиальная нагрузка Fr", error) ||
        !parsePositiveDouble(inputFa->text(), Fa, "Осевая нагрузка Fa", error) ||
        !parsePositiveDouble(inputN->text(), n, "Частота вращения n", error) ||
        !parsePositiveDouble(inputL10h->text(), L10hDesired, "Требуемый ресурс L10h", error)) {
        showError("Ошибка ввода: " + error);
        return;
    }

    if (n == 0) {
        showError("Ошибка ввода: частота вращения n не может быть равна нулю.");
        return;
    }
    if (Fr == 0 && Fa == 0) {
        showError("Ошибка ввода: укажите хотя бы одну ненулевую нагрузку (Fr или Fa).");
        return;
    }
    if (L10hDesired == 0) {
        showError("Ошибка ввода: требуемый ресурс должен быть больше нуля.");
        return;
    }

    const double V = 1.0;
    const double XHigh = 0.56;
    const double L10 = (L10hDesired * 60.0 * n) / 1000000.0;

    const Bearing *best = nullptr;
    double bestCNeeded = 0;
    double bestL10hReal = 0;

    for (const Bearing &b : bearings) {
        const double t = (b.C0 > 0) ? (b.f0 * Fa / b.C0) : 0.0;
        double e = 0;
        double Y = 0;
        getEandY(t, e, Y);

        const double ratio = (Fr > 0) ? (Fa / (V * Fr)) : std::numeric_limits<double>::infinity();
        const double P = (ratio <= e) ? (V * Fr) : (XHigh * V * Fr + Y * Fa);

        if (P <= 0) {
            continue;
        }

        const double CNeeded = P * std::pow(L10, 1.0 / b.p);
        if (b.C >= CNeeded && (best == nullptr || b.C < best->C)) {
            best = &b;
            bestCNeeded = CNeeded;
            bestL10hReal = (std::pow(b.C / P, b.p) * 1000000.0) / (60.0 * n);
        }
    }

    if (best != nullptr) {
        showResult(*best, bestCNeeded, bestL10hReal);
        return;
    }

    showNoResult();
}
