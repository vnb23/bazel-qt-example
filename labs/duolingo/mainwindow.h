#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStatusBar>
#include <QStackedWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QProgressBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QStringList>
#include <QTimer>
#include <QKeyEvent>
#include <QIcon>
#include <vector>
#include <random>

class DifficultyDialog : public QDialog {
    Q_OBJECT
public:
    DifficultyDialog(QWidget *parent = nullptr);
    QString getDifficulty() const;

private:
    QComboBox *difficultyCombo;
};

class LanguageApp : public QMainWindow {
    Q_OBJECT

public:
    LanguageApp(QWidget *parent = nullptr);
    struct TranslationExercise {
        QString english;
        QString russian;
        QString difficulty;
    };

    struct GrammarExercise {
        QString sentence;
        QString correctAnswer;
        std::vector<QString> options;
        QString difficulty;
    };

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUI();
    void setupExercises();
    void setupMenu();
    void startTranslationExercise();
    void startGrammarExercise();
    void setupTranslationUI();
    void setupGrammarUI();
    void showNextTranslationExercise();
    void showNextGrammarExercise();
    void checkTranslationAnswer();
    void checkGrammarAnswer();
    void finishExercise(bool success, bool timeExpired = false, bool manualExit = false);
    void changeDifficulty();
    void clearExerciseWidgets();
    void updateTimeDisplay();
    void initDatabase();
    void addTranslationExercise(const QString &english, const QString &russian, const QString &difficulty);
    void addGrammarExercise(const QString &sentence, const QString &correctAnswer,
                          const std::vector<QString> &options, const QString &difficulty);
    QStackedWidget *stackedWidget;
    QWidget *menuWidget;
    QWidget *exerciseWidget;
    QVBoxLayout *exerciseLayout;
    QProgressBar *progressBar;
    QLabel *scoreLabel;
    QTimer *exerciseTimer;

    QLabel *exerciseLabel;
    QTextEdit *answerEdit;
    QPushButton *submitBtn;
    QButtonGroup *radioGroup;

    QString currentExerciseType;
    int currentExerciseIndex;
    int attemptsLeft;
    int maxAttempts;
    int timeLimit;
    int score;
    QString currentDifficulty;
    int timeLeft;
    QLabel *timeLabel;

    std::vector<TranslationExercise> translationExercisesEasy;
    std::vector<TranslationExercise> translationExercisesMedium;
    std::vector<TranslationExercise> translationExercisesHard;

    std::vector<GrammarExercise> grammarExercisesEasy;
    std::vector<GrammarExercise> grammarExercisesMedium;
    std::vector<GrammarExercise> grammarExercisesHard;

    std::vector<TranslationExercise> currentTranslationExercises;
    std::vector<GrammarExercise> currentGrammarExercises;
};

#endif // MAINWINDOW_H