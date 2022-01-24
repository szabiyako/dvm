#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QTableWidget>
#include <QVector>
#include <QPoint>
#include <QString>

#include <cstdlib>
#include <ctime>

enum class AnswerType : int {
    FIRST,
    ALL
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_spinBox_nCities_valueChanged(int arg1);

    // Start branch and bound compute
    void on_pushButton_compute_clicked();

    void on_spinBox_logPage_valueChanged(int arg1);

    // Start bruteforce compute
    void on_pushButton_clicked();

    void loadTestData();
    void randomInput();
    void on_pushButton_clearInput_clicked();
    void on_comboBox_AnswerType_currentIndexChanged(int index);

private:
    // Check for user input in table
    void checkInsertedItem(QTableWidgetItem *item);

    // Routine functions
    template<class T>
    static inline T &get(QVector<T>& mat, const int size, const int row, const int col) { return mat[row + col * size]; }
    template<class T>
    static inline float get(const QVector<T>& mat, const int size, const int row, const int col) { return mat[row + col * size]; }
    static QString getConvertedTime(const size_t timeInNs);
    static void fillMatrix(QVector<float>& mat, QTableWidget *table);
    static QString getMatrixString(const QVector<float>& mat, const int size);
    static QString getRouteString(const QVector<QPoint>& route);
    static void sortRoute(QVector<QPoint>& route);
    static int randomInt(const int max);
    bool findPivotZero(const QVector<float>& mat, const int size, QPoint &zeroPos, float &score);
    QVector<float> matWithIncludedPath(
            const QVector<float>& oldMat,
            const int size,
            const QPoint &newPath,
            const QVector<QPoint> &currentRoute);
    float simplifyMatrix(QVector<float>& mat, const int size);


    void clearLog();
    void addLog(const QString &string);
    void showLog();

    // Recursive branch and bound
    void calcNode(
            const QVector<float> &mat,
            const int size,
            const QVector<QPoint> &currentRoute,
            const float topNodeRating,
            float &bestRating,
            QVector<QVector<QPoint>> &bestRoute,
            const bool needToSimplify,
            const AnswerType answerType);

    // Recursive bruteforce
    void bruteForceCalc(
            const QVector<float> &mat,
            const int size,
            const QVector<int> &route,
            const float prevScore,
            float &bestRating,
            QVector<QVector<QPoint>> &bestRoutes,
            const AnswerType answerType);

private:
    int m_nCities = 2;
    AnswerType m_answerType = AnswerType(0);

    QVector<QString> m_logText;
    int m_lineCounter = 0;
    const int m_maxLines = 200;
    int m_logIndex = 0;

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
