#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMessageBox>

#include <chrono>
#include <limits>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->tableWidget_inputMatrix, &QTableWidget::itemChanged,
            this, &MainWindow::checkInsertedItem);
    QFont f("unexistent");
    f.setStyleHint(QFont::Monospace);
    ui->textBrowser_log->setFont(f);
    f.setPixelSize(20);
    ui->label_Answer->setFont(f);
    ui->frame_Answer->hide();

    menuBar()->addAction("Load test data", this, &MainWindow::loadTestData);
    menuBar()->addAction("Random input", this, &MainWindow::randomInput);
    std::srand(std::time(nullptr));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_spinBox_nCities_valueChanged(int arg1)
{
    if (arg1 > m_nCities) {
        ui->tableWidget_inputMatrix->setColumnCount(arg1);
        ui->tableWidget_inputMatrix->setRowCount(arg1);
        QStringList list;
        for (int i = 0; i < arg1; ++i)
            list.push_back("c_" + QString::number(i));
        ui->tableWidget_inputMatrix->setHorizontalHeaderLabels(list);
        ui->tableWidget_inputMatrix->setVerticalHeaderLabels(list);
        for (int i = m_nCities - 1; i < arg1; ++i)
            ui->tableWidget_inputMatrix->setItem(i, i, new QTableWidgetItem("X"));
        for (int col = 0; col < arg1; ++col)
            for (int row = 0; row < arg1; ++row) {
                if (ui->tableWidget_inputMatrix->item(row, col) == nullptr) {
                    ui->tableWidget_inputMatrix->setItem(row, col, new QTableWidgetItem("0"));
                }
            }
    }
    else if (arg1 < m_nCities) {
        ui->tableWidget_inputMatrix->setColumnCount(arg1);
        ui->tableWidget_inputMatrix->setRowCount(arg1);
    }
    m_nCities = arg1;
}

void MainWindow::checkInsertedItem(QTableWidgetItem *item)
{
    if (item->row() == item->column())
    {
        item->setText("X");
        return;
    }
    bool isOk = true;
    const float value = item->text().toFloat(&isOk);
    isOk = (value > 0.f) && isOk;
    if (!isOk)
        item->setText("0");
    else
        item->setText(QString::number(value));
}

QString MainWindow::getConvertedTime(const size_t timeInNs)
{
    const size_t timeInMs = timeInNs / 1000000ull;
    const size_t timeInSeconds = timeInMs / 1000;
    const size_t timeInMinutes = timeInSeconds / 60;
    const size_t timeInHours = timeInMinutes / 60;
    const size_t ns = timeInNs % 1000000ull;
    const size_t ms = timeInMs % 1000;
    const size_t sec = timeInSeconds % 60;
    const size_t min = timeInMinutes % 60;
    const size_t hrs = timeInHours;

    QString result;
    if (hrs > 0)
        result += QString::number(hrs) + "h, ";
    if (min > 0)
        result += QString::number(min) + "min, ";
    if (sec > 0)
        result += QString::number(sec) + "sec, ";
    if (ms > 0)
        result += QString::number(ms) + "ms, ";
    result += QString::number(ns) + "ns";

    return result;
}

void MainWindow::fillMatrix(QVector<float> &mat, QTableWidget *table)
{
    const int size = table->columnCount();
    for (int col = 0; col < size; ++col)
        for (int row = 0; row < size; ++row) {
            if (row == col) {
                get(mat, size, row, col) = -1.f;
                continue;
            }

            get(mat, size, row, col) = table->item(row, col)->text().toFloat();
        }
}

QString MainWindow::getMatrixString(const QVector<float> &mat, const int size)
{
    const int floatPrecision = 3;
    const int valueWith = 10;
    QString result;
    result += QString("   ");
    for (int col = 0; col < size; ++col)
        result += QString("%1 ").arg(col, valueWith);
    result += "\n";
    for (int row = 0; row < size; ++row) {
        result += QString("%1|").arg(row, 3);
        for (int col = 0; col < size; ++col) {
            if (get(mat, size, row, col) < 0.f) {
                result += QString("%1 ").arg("X", valueWith);
                continue;
            }

            result += QString("%1 ").arg(get(mat, size, row, col), valueWith, 'g', floatPrecision);
        }
        result += QString("|\n");
    }
    return result;
}

QString MainWindow::getRouteString(const QVector<QPoint> &route)
{
    QString result;
    const int size = route.size();
    result += "{";
    if (size == 0) {
        result += "Empty}\n";
        return result;
    }
    for (int i = 0; i < size - 1; ++i)
        result += QString("%1->%2, ").arg(route[i].x()).arg(route[i].y());
    result += QString("%1->%2}\n").arg(route[size-1].x()).arg(route[size-1].y());
    return result;
}

void MainWindow::sortRoute(QVector<QPoint> &route) // TODO
{
    const int size = route.size();
    for (int i = 0; i < size; ++i) { // Start from 0 city
        if (route[i].x() == 0) {
            std::swap(route[0], route[i]);
            break;
        }
    }
    for (int left = 0; left < size - 1; ++left) {
        for (int i = left + 1; i < size; ++i) {
            if (route[left].y() == route[i].x()) {
                std::swap(route[left + 1], route[i]);
                break;
            }
        }
    }
}

int MainWindow::randomInt(const int max)
{
    return std::rand() % max + 1;
}

bool MainWindow::findPivotZero(
        const QVector<float> &mat,
        const int size,
        QPoint &zeroPos,
        float &score)
{
    QVector<float> rowScore(size, 0);
    QVector<float> colScore(size, 0);

    // fillRowScore
    for (int row = 0; row < size; ++row) {
        float minValue = std::numeric_limits<float>::max();
        bool hasZero = false;
        for (int col = 0; col < size; ++col) {
            const float value = get(mat, size, row, col);
            if (value < 0.f)
                continue;

            if (!hasZero && qFuzzyIsNull(value)) {
                hasZero = true;
                continue;
            }

            if (value < minValue)
                minValue = value;
        }
        if (minValue == std::numeric_limits<float>::max())
            minValue = 0.f;
        rowScore[row] = minValue;
    }

    // fillColScore
    for (int col = 0; col < size; ++col) {
        float minValue = std::numeric_limits<float>::max();
        bool hasZero = false;
        for (int row = 0; row < size; ++row) {
            const float value = get(mat, size, row, col);
            if (value < 0.f)
                continue;

            if (!hasZero && qFuzzyIsNull(value)) {
                hasZero = true;
                continue;
            }

            if (value < minValue)
                minValue = value;
        }
        if (minValue == std::numeric_limits<float>::max())
            minValue = 0.f;
        colScore[col] = minValue;
    }

    float bestScore = -1.f;
    QPoint resPos = QPoint(0, 0);
    for (int col = 0; col < size; ++col)
        for (int row = 0; row < size; ++row) {
            const float value = get(mat, size, row, col);
            if (qFuzzyIsNull(value)) {
                const float score = rowScore[row] + colScore[col];
                if (score > bestScore) {
                    bestScore = score;
                    resPos = QPoint(row, col);
                }
            }
        }

    zeroPos = resPos;
    score = bestScore;

    if (bestScore < 0.f)
        return false;

    return true;
}

QVector<float> MainWindow::matWithIncludedPath(
        const QVector<float> &oldMat,
        const int size,
        const QPoint &newPath,
        const QVector<QPoint> &currentRoute)
{
    QVector<float> mat = oldMat;
    for (int row = 0; row < size; ++row){
        get(mat, size, row, newPath.y()) = -1;
    }
    for (int col = 0; col < size; ++col){
        get(mat, size, newPath.x(), col) = -1;
    }

    const int nRoutes = currentRoute.size();
    if (nRoutes == size - 2) // Не удаляем подцикл если следующтй путь последний
        return mat;
    int beginCurrentPath = newPath.x();
    int endCurrentPath = newPath.y();
    bool done = false;
    while (!done) {// Удаляем подциклы
        done = true;
        for (int r = 0; r < nRoutes; ++r) {
            const int endPartPath = currentRoute[r].y();
            if (beginCurrentPath == endPartPath) {
                beginCurrentPath = currentRoute[r].x();
                done = false;
                break;
            }
        }
        for (int r = 0; r < nRoutes; ++r) {
            const int beginPartPath = currentRoute[r].x();
            if (endCurrentPath == beginPartPath) {
                endCurrentPath = currentRoute[r].y();
                done = false;
                break;
            }
        }
    }
    get(mat, size, endCurrentPath, beginCurrentPath) = -1;
    return mat;
}

float MainWindow::simplifyMatrix(QVector<float> &mat, const int size)
{
    float result = 0.f;
    for (int row = 0; row < size; ++row) {
        float minValue = std::numeric_limits<float>::max();
        for (int col = 0; col < size; ++col) {
            float &value = get(mat, size, row, col);
            if (value < 0.f)
                continue;
            if (value < minValue)
                minValue = value;
        }

        if (minValue == std::numeric_limits<float>::max())
            continue;
        for (int col = 0; col < size; ++col) {
            float &value = get(mat, size, row, col);
            if (value < 0.f)
                continue;

            value -= minValue;
        }
        result += minValue;
    }

    for (int col = 0; col < size; ++col) {
        float minValue = std::numeric_limits<float>::max();
        for (int row = 0; row < size; ++row) {
            float &value = get(mat, size, row, col);
            if (value < 0.f)
                continue;
            if (value < minValue)
                minValue = value;
        }

        if (minValue == std::numeric_limits<float>::max())
            continue;
        for (int row = 0; row < size; ++row) {
            float &value = get(mat, size, row, col);
            if (value < 0.f)
                continue;

            value -= minValue;
        }
        result += minValue;
    }
    return result;
}

void MainWindow::clearLog()
{
    m_logText.clear();
    m_logText.push_back("");
    m_logIndex = 0;
    m_lineCounter = 0;
    ui->textBrowser_log->clear();
    ui->spinBox_logPage->setMaximum(1);
    ui->label_logPagesCount->setText("1");
}

void MainWindow::addLog(const QString &string)
{
    m_logText[m_logIndex] += string;
    m_lineCounter += string.count(QChar('\n'));
    if (m_lineCounter > m_maxLines) {
        m_lineCounter = 0;
        m_logIndex++;
        m_logText.push_back("");
        const int count = m_logIndex + 1;
        ui->spinBox_logPage->setMaximum(count);
        ui->label_logPagesCount->setText(QString::number(count));
    }
}

void MainWindow::showLog()
{
    const int index = ui->spinBox_logPage->value() - 1;
    ui->textBrowser_log->setText(m_logText[index]);
}

void MainWindow::calcNode(
        const QVector<float> &inputMat,
        const int size,
        const QVector<QPoint> &currentRoute,
        const float beforeSimplifyRating, // Оценка текущей ноды до приведения
        float &bestRating,
        QVector<QVector<QPoint>> &bestRoute,
        const bool needToSimplify,
        const AnswerType answerType)
{
    addLog("\n");
    addLog("\n");
    addLog("Текущий маршрут:");
    addLog(getRouteString(currentRoute));
    //qDebug() << getRouteString(currentRoute);
    addLog("Текущая матрица:\n");
    addLog(getMatrixString(inputMat, size));
    QVector<float> mat = inputMat;
    float simplifyRating = simplifyMatrix(mat, size);
    if (!needToSimplify)
        simplifyRating = 0.f;
    const float currentRating = simplifyRating + beforeSimplifyRating;
    if (simplifyRating > 0.f) {
        addLog("Приведёная матрица:\n");
        addLog(getMatrixString(mat, size));
        addLog(QString("Оценка после приведения: %1 + %2 = %3\n").arg(beforeSimplifyRating).arg(simplifyRating).arg(currentRating));
    }
    else {
        if (needToSimplify)
            addLog(QString("Матрица уже приведёная, оценка не изменилась: %1\n").arg(currentRating));
        else
            addLog(QString("При исключении пути приведение не требуется, оценка не изменилась: %1\n").arg(currentRating));
    }
    const bool hasRecord = (bestRating != std::numeric_limits<float>::max());
    if (hasRecord) { // Если уже есть рекорд
        if (answerType == AnswerType::FIRST && bestRating <= currentRating) {
            addLog(QString("Оценка хуже или равна текущему рекорду: %1 <= %2; Закрытие ветки.\n").arg(bestRating).arg(currentRating));
            return;
        }
        else if (answerType == AnswerType::ALL && bestRating < currentRating) {
            addLog(QString("Оценка хуже текущего рекорда: %1 < %2; Закрытие ветки.\n").arg(bestRating).arg(currentRating));
            return;
        }
    }

    QPoint zeroPos;
    float score = 0.f;
    const bool isFounded = findPivotZero(mat, size, zeroPos, score);
    if (!isFounded) {
        const bool isAnswer = currentRoute.size() == size;
        if (!isAnswer) {
            addLog("Доступные пути кончились, решение не получено; Закрытие ветки.\n");
            return;
        }
        if (answerType == AnswerType::FIRST && bestRating > currentRating) {
            addLog(QString("Новый рекорд: %1; Рекордный путь: %2").arg(currentRating).arg(getRouteString(currentRoute)));
            bestRating = currentRating;
            bestRoute = {currentRoute};
        }
        else if (answerType == AnswerType::ALL && bestRating >= currentRating) {
            const bool newRecord = bestRating > currentRating;
            if (newRecord) {
                addLog(QString("Новый рекорд: %1; Рекордный путь: %2").arg(currentRating).arg(getRouteString(currentRoute)));
                bestRating = currentRating;
                bestRoute = {currentRoute};
            }
            else {
                addLog(QString("Получен старый рекорд: %1; Добавлен путь: %2").arg(currentRating).arg(getRouteString(currentRoute)));
                bestRating = currentRating;
                bestRoute.push_back(currentRoute);
            }
        }
        else {
            addLog(QString("Получено решение: %1; Полученый путь: %2").arg(currentRating).arg(getRouteString(currentRoute)));
            addLog(QString("Полученное решение хуже или равно текущему рекорду: %1 <= %2; Закрытие ветки.\n").arg(bestRating).arg(currentRating));
        }
        return;
    }
    addLog(QString("Включаем в маршрут путь %1->%2\n").arg(zeroPos.x()).arg(zeroPos.y()));
    QVector<QPoint> newRoute = currentRoute;
    newRoute.push_back(zeroPos);
    const QVector<float> newMat = matWithIncludedPath(
                mat,
                size,
                zeroPos,
                currentRoute);
    calcNode(newMat, size, newRoute, currentRating, bestRating, bestRoute, true, answerType);
    addLog("\n");
    addLog("\n");
    addLog("Возврат к маршруту:");
    addLog(getRouteString(currentRoute));
    const float secondRating = currentRating + score;
    addLog(QString("Исключаем из маршрута путь %1->%2; Оценка после исключения: %3 + %4 = %5\n").arg(zeroPos.x()).arg(zeroPos.y()).arg(currentRating).arg(score).arg(secondRating));
    const bool hasRecordNow = (bestRating != std::numeric_limits<float>::max());
    if (hasRecordNow) { // Если уже есть рекорд
        if (answerType == AnswerType::FIRST && bestRating <= secondRating) {
            addLog(QString("Оценка хуже или равна текущему рекорду: %1 <= %2; Закрытие ветки.\n").arg(bestRating).arg(secondRating));
            return;
        }
        else if (answerType == AnswerType::ALL && bestRating < secondRating) {
            addLog(QString("Оценка хуже текущего рекорда: %1 < %2; Закрытие ветки.\n").arg(bestRating).arg(secondRating));
            return;
        }
    }
    get(mat, size, zeroPos.x(), zeroPos.y()) = -1;
    calcNode(mat, size, currentRoute, secondRating, bestRating, bestRoute, false, answerType);
}

void MainWindow::bruteForceCalc(
        const QVector<float> &mat,
        const int size,
        const QVector<int> &route,
        const float prevScore,
        float &bestRating,
        QVector<QVector<QPoint>> &bestRoutes,
        const AnswerType answerType)
{
    const int iter = route.size();
    if (iter == size) {
        if (answerType == AnswerType::FIRST && prevScore < bestRating) {
            QVector<QPoint> checkRoute(size);
            for (int i = 0; i < size; ++i)
                checkRoute[i] = QPoint(i, route[i]);
            int hop = 0;
            int nHops = 0;
            bool endLoop = false;
            while (!endLoop){
                hop = route[hop];
                ++nHops;
                if (hop == 0)
                    endLoop = true;
            }
            bool isRealRecord = (nHops == size);
            if (isRealRecord) {
                bestRating = prevScore;
                bestRoutes = {checkRoute};

                addLog(QString("Новый рекорд: %1; Рекордный путь: %2").arg(bestRating).arg(getRouteString(bestRoutes[0])));
            }
            else
                addLog(QString("Полученный путь не является замкнутым; Закрытие ветки.\n"));
        }
        else if (answerType == AnswerType::ALL && prevScore <= bestRating) {
            QVector<QPoint> checkRoute(size);
            for (int i = 0; i < size; ++i)
                checkRoute[i] = QPoint(i, route[i]);
            int hop = 0;
            int nHops = 0;
            bool endLoop = false;
            while (!endLoop){
                hop = route[hop];
                ++nHops;
                if (hop == 0)
                    endLoop = true;
            }
            bool isRealRecord = (nHops == size);
            if (isRealRecord) {
                const bool newRecord = prevScore < bestRating;
                if (newRecord) {
                    bestRating = prevScore;
                    bestRoutes = {checkRoute};
                    addLog(QString("Новый рекорд: %1; Рекордный путь: %2").arg(bestRating).arg(getRouteString(bestRoutes[0])));
                }
                else {
                    bestRating = prevScore;
                    bestRoutes.push_back(checkRoute);
                    addLog(QString("Получен старый рекорд: %1; Добавлен путь: %2").arg(bestRating).arg(getRouteString(bestRoutes[0])));
                }
            }
            else
                addLog(QString("Полученный путь не является замкнутым; Закрытие ветки.\n"));
        }
        return;
    }

    for (int i = 0; i < size; ++i) {
        if (iter == i)
            continue;
        const bool isVisited = std::find(route.begin(), route.end(), i) != route.end();
        if (isVisited)
            continue;

        const float newScore = prevScore + get(mat, size, iter, i);
        const bool hasRecord = (bestRating != std::numeric_limits<float>::max());

        QVector<int> newRoute = route;
        newRoute.push_back(i);
        QVector<QPoint> currRoute(newRoute.size());
        for (int i = 0; i < newRoute.size(); ++i)
            currRoute[i] = QPoint(i, newRoute[i]);
        addLog(QString("Текущая длина: %1; Текущий путь: %2").arg(newScore).arg(getRouteString(currRoute)));
        if (hasRecord) {
            if (answerType == AnswerType::FIRST && bestRating <= newScore) {
                addLog(QString("Выбранный путь хуже или равен текущему рекорду: %1 <= %2; Закрытие ветки.\n").arg(bestRating).arg(newScore));
                continue;
            }
            else if (answerType == AnswerType::ALL && bestRating < newScore) {
                addLog(QString("Выбранный путь хуже текущего рекорда: %1 < %2; Закрытие ветки.\n").arg(bestRating).arg(newScore));
                continue;
            }
        }
        bruteForceCalc(mat, size, newRoute, newScore, bestRating, bestRoutes, answerType);
    }
}

void MainWindow::on_pushButton_compute_clicked()
{
    clearLog();
    ui->label_Answer->setText("Computing...");

    QVector<float> mat(m_nCities * m_nCities);
    fillMatrix(mat, ui->tableWidget_inputMatrix);

    addLog("Входная матрица:\n");
    addLog(getMatrixString(mat, m_nCities));
    float bestRating = std::numeric_limits<float>::max();
    QVector<QVector<QPoint>> bestRoutes;
    const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    calcNode(mat, m_nCities, QVector<QPoint>(), 0.f, bestRating, bestRoutes, true, m_answerType);
    const std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
    const size_t timeInNs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    addLog("\n");
    addLog("\n");
    addLog("Обход дерева окончен\n");
    const int nRoutes = bestRoutes.size();
    for (int r = 0; r < nRoutes; ++r)
        sortRoute(bestRoutes[r]);
    if (nRoutes == 1)
        addLog(QString("Лучший маршрут: " + getRouteString(bestRoutes[0])));
    else {
        addLog(QString("Лучшие маршруты (%1):\n").arg(nRoutes));
        for (int r = 0; r < nRoutes; ++r)
            addLog(QString("  " + getRouteString(bestRoutes[r])));
    }
    addLog(QString("Длина маршрута: %1\n").arg(bestRating));
    showLog();
    QString answer = "";
    if (nRoutes == 1)
        answer += "Best route = " + getRouteString(bestRoutes[0]);
    else {
        answer += "Best routes (" + QString::number(nRoutes) + "):\n";
        for (int r = 0; r < nRoutes; ++r) {
            //TEST
            //float sum = 0.f;
            //for (int i = 0; i < bestRoutes[r].size(); ++i) {
            //    const QPoint &node = bestRoutes[r][i];
            //    sum += get(mat, m_nCities, node.x(), node.y());
            //}
            //answer += QString::number(sum) + ":";
            //
            answer += getRouteString(bestRoutes[r]);
        }
    }
    answer += QString("Length = %1\n").arg(bestRating);
    answer += QString("Time = %1").arg(getConvertedTime(timeInNs));
    ui->label_Answer->setText(answer);
    ui->label_AnswerTitle->setText("Answer (branch and bound)");
    ui->frame_Answer->show();
}

void MainWindow::on_spinBox_logPage_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    showLog();
}

void MainWindow::on_pushButton_clicked()
{
    if (m_nCities > 10) {
        if (QMessageBox::No == QMessageBox::warning(this, "Are you sure?", "Brute force at (nCities > 10)\nis not recommended,\ncontinue anyway?", QMessageBox::Yes | QMessageBox::No))
            return;
    }
    clearLog();
    ui->label_Answer->setText("Computing...");

    QVector<float> mat(m_nCities * m_nCities);
    fillMatrix(mat, ui->tableWidget_inputMatrix);

    addLog("Входная матрица:\n");
    addLog(getMatrixString(mat, m_nCities));
    float bestRating = std::numeric_limits<float>::max();
    QVector<QVector<QPoint>> bestRoutes;
    const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    bruteForceCalc(mat, m_nCities, {}, 0.f, bestRating, bestRoutes, m_answerType);
    const std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
    const size_t timeInNs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    addLog("\n");
    addLog("\n");
    addLog("Полный перебор окончен\n");
    const int nRoutes = bestRoutes.size();
    for (int r = 0; r < nRoutes; ++r)
        sortRoute(bestRoutes[r]);
    if (nRoutes == 1)
        addLog(QString("Лучший маршрут: " + getRouteString(bestRoutes[0])));
    else {
        addLog(QString("Лучшие маршруты (%1):\n").arg(nRoutes));
        for (int r = 0; r < nRoutes; ++r)
            addLog(QString("  " + getRouteString(bestRoutes[r])));
    }
    addLog(QString("Длина маршрута: %1\n").arg(bestRating));
    showLog();
    QString answer = "";
    if (nRoutes == 1)
        answer += "Best route = " + getRouteString(bestRoutes[0]);
    else {
        answer += "Best routes (" + QString::number(nRoutes) + "):\n";
        for (int r = 0; r < nRoutes; ++r) {
            //TEST
            //float sum = 0.f;
            //for (int i = 0; i < bestRoutes[r].size(); ++i) {
            //    const QPoint &node = bestRoutes[r][i];
            //    sum += get(mat, m_nCities, node.x(), node.y());
            //}
            //answer += QString::number(sum) + ":";
            //
            answer += getRouteString(bestRoutes[r]);
        }
    }
    answer += QString("Length = %1\n").arg(bestRating);
    answer += QString("Time = %1").arg(getConvertedTime(timeInNs));
    ui->label_Answer->setText(answer);
    ui->label_AnswerTitle->setText("Answer (brute force)");
    ui->frame_Answer->show();
}

void MainWindow::loadTestData()
{
    ui->spinBox_nCities->setValue(6);
    QVector<float> mat(6 * 6);
    get(mat, 6, 0, 0) = -1;
    get(mat, 6, 1, 0) = 4;
    get(mat, 6, 2, 0) = 5;
    get(mat, 6, 3, 0) = 2;
    get(mat, 6, 4, 0) = 4;
    get(mat, 6, 5, 0) = 3;

    get(mat, 6, 0, 1) = 6;
    get(mat, 6, 1, 1) = -1;
    get(mat, 6, 2, 1) = 4;
    get(mat, 6, 3, 1) = 3;
    get(mat, 6, 4, 1) = 6;
    get(mat, 6, 5, 1) = 2;

    get(mat, 6, 0, 2) = 3;
    get(mat, 6, 1, 2) = 3;
    get(mat, 6, 2, 2) = -1;
    get(mat, 6, 3, 2) = 4;
    get(mat, 6, 4, 2) = 6;
    get(mat, 6, 5, 2) = 4;

    get(mat, 6, 0, 3) = 5;
    get(mat, 6, 1, 3) = 4;
    get(mat, 6, 2, 3) = 4;
    get(mat, 6, 3, 3) = -1;
    get(mat, 6, 4, 3) = 7;
    get(mat, 6, 5, 3) = 3;

    get(mat, 6, 0, 4) = 4;
    get(mat, 6, 1, 4) = 2;
    get(mat, 6, 2, 4) = 3;
    get(mat, 6, 3, 4) = 5;
    get(mat, 6, 4, 4) = -1;
    get(mat, 6, 5, 4) = 5;

    get(mat, 6, 0, 5) = 2;
    get(mat, 6, 1, 5) = 3;
    get(mat, 6, 2, 5) = 2;
    get(mat, 6, 3, 5) = 6;
    get(mat, 6, 4, 5) = 5;
    get(mat, 6, 5, 5) = -1;

    for (int col = 0; col < 6; ++col)
        for (int row = 0; row < 6; ++row) {
            if (row == col)
                continue;

            ui->tableWidget_inputMatrix->setItem(row, col, new QTableWidgetItem(QString::number(get(mat, 6, row, col))));
        }
}

void MainWindow::randomInput()
{
    for (int col = 0; col < m_nCities; ++col)
        for (int row = 0; row < m_nCities; ++row) {
            if (row == col)
                continue;

            ui->tableWidget_inputMatrix->setItem(row, col, new QTableWidgetItem(QString::number(randomInt(10))));
        }
}

void MainWindow::on_pushButton_clearInput_clicked()
{
    for (int col = 0; col < m_nCities; ++col)
        for (int row = 0; row < m_nCities; ++row) {
            if (row == col)
                continue;

            ui->tableWidget_inputMatrix->setItem(row, col, new QTableWidgetItem("0"));
        }
}

void MainWindow::on_comboBox_AnswerType_currentIndexChanged(int index)
{
    m_answerType = AnswerType(index);
}
