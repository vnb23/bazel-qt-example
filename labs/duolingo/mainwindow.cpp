#include "mainwindow.h"
#include <algorithm>
#include <random>
void LanguageApp::updateTimeDisplay() {
    int minutes = timeLeft / 60;
    int seconds = timeLeft % 60;
    timeLabel->setText(QString("Время: %1:%2")
                      .arg(minutes, 2, 10, QLatin1Char('0'))
                      .arg(seconds, 2, 10, QLatin1Char('0')));
}
DifficultyDialog::DifficultyDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Выбор уровня сложности");

    QFormLayout *layout = new QFormLayout(this);

    difficultyCombo = new QComboBox(this);
    difficultyCombo->addItems({"Легкий", "Средний", "Сложный"});

    layout->addRow("Уровень сложности:", difficultyCombo);

    QPushButton *okButton = new QPushButton("OK", this);
    okButton->setStyleSheet("QPushButton { min-width: 80px; }");
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addRow(okButton);

    setLayout(layout);
}

QString DifficultyDialog::getDifficulty() const {
    return difficultyCombo->currentText();
}

LanguageApp::LanguageApp(QWidget *parent) : QMainWindow(parent), timeLeft(0),
    stackedWidget(nullptr),
    menuWidget(nullptr),
    exerciseWidget(nullptr),
    exerciseLayout(nullptr),
    progressBar(nullptr),
    scoreLabel(nullptr),
    exerciseLabel(nullptr),
    answerEdit(nullptr),
    submitBtn(nullptr),
    radioGroup(nullptr),
    currentExerciseIndex(0),
    attemptsLeft(0),
    maxAttempts(3),
    timeLimit(120),
    score(0),
    currentDifficulty("Средний")
{
    setWindowTitle("Language Learning App");
    initDatabase();
    setupUI();
    setupExercises();
    setupMenu();
}
void LanguageApp::initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("exercises.db");

    if (!db.open()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть базу данных: " + db.lastError().text());
        return;
    }

    QSqlQuery query;

    query.exec("CREATE TABLE IF NOT EXISTS translation_exercises ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "english TEXT NOT NULL, "
               "russian TEXT NOT NULL, "
               "difficulty TEXT NOT NULL)");

    query.exec("CREATE TABLE IF NOT EXISTS grammar_exercises ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "sentence TEXT NOT NULL, "
               "correct_answer TEXT NOT NULL, "
               "option1 TEXT NOT NULL, "
               "option2 TEXT NOT NULL, "
               "option3 TEXT NOT NULL, "
               "difficulty TEXT NOT NULL)");
}
void LanguageApp::setupUI() {
    QWidget *mainWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(mainWidget);

    QWidget *leftPanel = new QWidget(mainWidget);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(15);

    QLabel *titleLabel = new QLabel("Выберите тип упражнения:", leftPanel);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; }");

    QPushButton *translationBtn = new QPushButton("Translation", leftPanel);
    QPushButton *grammarBtn = new QPushButton("Grammar", leftPanel);

    translationBtn->setMinimumSize(200, 50);
    grammarBtn->setMinimumSize(200, 50);

    leftLayout->addWidget(titleLabel);
    leftLayout->addWidget(translationBtn);
    leftLayout->addWidget(grammarBtn);
    leftLayout->addStretch();

    exerciseWidget = new QWidget(mainWidget);
    exerciseLayout = new QVBoxLayout(exerciseWidget);
    exerciseLayout->setContentsMargins(20, 20, 20, 20);
    exerciseLayout->setSpacing(15);

    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(exerciseWidget, 2);

    exerciseWidget->setVisible(false);

    connect(translationBtn, &QPushButton::clicked, this, &LanguageApp::startTranslationExercise);
    connect(grammarBtn, &QPushButton::clicked, this, &LanguageApp::startGrammarExercise);

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%v/%m");
    statusBar()->addPermanentWidget(progressBar);

    scoreLabel = new QLabel("Баллы: 0", this);
    statusBar()->addPermanentWidget(scoreLabel);

    timeLabel = new QLabel("Время: --:--", this);
    statusBar()->addPermanentWidget(timeLabel);

    exerciseTimer = new QTimer(this);
    exerciseTimer->setInterval(1000);
    connect(exerciseTimer, &QTimer::timeout, this, [this]() {
        timeLeft--;
        updateTimeDisplay();

        if (timeLeft <= 0) {
            exerciseTimer->stop();
            QMessageBox::information(this, "Время вышло",
                "Время на выполнение задания истекло. Упражнение завершено.");
            finishExercise(false, true);
        }
    });

    setCentralWidget(mainWidget);
    setStyleSheet(R"(
    QMainWindow {
        background-color: #2b2b2b;
    }
    QWidget {
        background-color: #2b2b2b;
    }
    QLabel, QTextEdit, QRadioButton {
        color: white;
        font-size: 14px;
    }
    QTextEdit {
        background-color: #3c3c3c;
        border: 1px solid #555555;
    }
    QPushButton {
        background-color: green;
        color: white;
        border: none;
        padding: 8px 16px;
        font-size: 14px;
        border-radius: 4px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: darkgreen;
    }
    QProgressBar {
        background-color: #3c3c3c;
        border: 1px solid #555555;
        border-radius: 3px;
        text-align: center;
        color: gray;
    }
    QProgressBar::chunk {
        background-color: green;
    }
    )");
}
void LanguageApp::setupMenu() {
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *settingsMenu = menuBar->addMenu("Настройки");

    QAction *difficultyAction = new QAction("Уровень сложности", this);
    difficultyAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(difficultyAction, &QAction::triggered, this, &LanguageApp::changeDifficulty);

    QAction *helpAction = new QAction("Помощь", this);
    helpAction->setShortcut(QKeySequence("F1"));
    connect(helpAction, &QAction::triggered, this, [this]() {
        keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier));
    });

    settingsMenu->addAction(difficultyAction);
    settingsMenu->addAction(helpAction);

    setMenuBar(menuBar);
}
void LanguageApp::addTranslationExercise(const QString &english, const QString &russian, const QString &difficulty) {
    QSqlQuery query;
    query.prepare("INSERT INTO translation_exercises (english, russian, difficulty) "
                  "VALUES (:english, :russian, :difficulty)");
    query.bindValue(":english", english);
    query.bindValue(":russian", russian);
    query.bindValue(":difficulty", difficulty);
    query.exec();
}

void LanguageApp::addGrammarExercise(const QString &sentence, const QString &correctAnswer,
                                   const std::vector<QString> &options, const QString &difficulty) {
    QSqlQuery query;
    query.prepare("INSERT INTO grammar_exercises (sentence, correct_answer, option1, option2, option3, difficulty) "
                  "VALUES (:sentence, :correct_answer, :option1, :option2, :option3, :difficulty)");
    query.bindValue(":sentence", sentence);
    query.bindValue(":correct_answer", correctAnswer);
    query.bindValue(":option1", options[0]);
    query.bindValue(":option2", options[1]);
    query.bindValue(":option3", options[2]);
    query.bindValue(":difficulty", difficulty);
    query.exec();
}

void LanguageApp::setupExercises() {
    currentDifficulty = "Средний";
    score = 0;
    maxAttempts = 3;
    timeLimit = 120;
translationExercisesEasy.clear();
    translationExercisesMedium.clear();
    translationExercisesHard.clear();
    grammarExercisesEasy.clear();
    grammarExercisesMedium.clear();
    grammarExercisesHard.clear();
    translationExercisesEasy = {
        {"Hello", "Привет", "Легкий"},
        {"Goodbye", "До свидания", "Легкий"},
        {"Thank you", "Спасибо", "Легкий"},
        {"Please", "Пожалуйста", "Легкий"},
        {"I love you", "Я тебя люблю", "Легкий"}
    };

    translationExercisesMedium = {
        {"How are you?", "Как дела?", "Средний"},
        {"What is your name?", "Как тебя зовут?", "Средний"},
        {"Where are you from?", "Откуда ты?", "Средний"},
        {"I don't understand", "Я не понимаю", "Средний"},
        {"Could you help me?", "Не могли бы вы мне помочь?", "Средний"},
        {"What time is it?", "Который час?", "Средний"},
        {"How much does it cost?", "Сколько это стоит?", "Средний"},
        {"Where is the bathroom?", "Где туалет?", "Средний"}
    };

    translationExercisesHard = {
        {"I'm hungry", "Я голоден", "Сложный"},
        {"I'm thirsty", "Я хочу пить", "Сложный"},
        {"Excuse me", "Извините", "Сложный"},
        {"I'm sorry", "Мне жаль", "Сложный"},
        {"Congratulations", "Поздравляю", "Сложный"},
        {"Happy birthday", "С днем рождения", "Сложный"},
        {"Happy new year", "С новым годом", "Сложный"},
        {"Bon appetit", "Приятного аппетита", "Сложный"},
        {"Cheers", "Ваше здоровье", "Сложный"},
        {"I agree", "Я согласен", "Сложный"}
    };

    grammarExercisesEasy = {
        {"She ___ to school every day.", "goes", {"go", "going", "went"}, "Легкий"},
        {"I ___ a book yesterday.", "read", {"reads", "reading", "red"}, "Легкий"},
        {"They ___ playing football now.", "are", {"is", "am", "be"}, "Легкий"},
        {"We ___ never been to Paris.", "have", {"has", "had", "having"}, "Легкий"}
    };

    grammarExercisesMedium = {
        {"He ___ his homework yet.", "hasn't done", {"didn't do", "doesn't do", "won't do"}, "Средний"},
        {"If I ___ you, I would go home.", "were", {"was", "am", "be"}, "Средний"},
        {"By next year, she ___ here for five years.", "will have been", {"will be", "has been", "had been"}, "Средний"},
        {"The book ___ by Hemingway in 1926.", "was written", {"wrote", "has written", "had written"}, "Средний"},
        {"I wish I ___ how to swim.", "knew", {"know", "had known", "have known"}, "Средний"}
    };

    grammarExercisesHard = {
        {"By the time we arrived, the movie ___.", "had already started", {"already started", "has already started", "was already starting"}, "Сложный"},
        {"If it rains, we ___ cancel the picnic.", "will", {"would", "would have", "had"}, "Сложный"},
        {"She ___ her keys before leaving home.", "had lost", {"loses", "lost", "has lost"}, "Сложный"},
        {"This time tomorrow, I ___ on the beach.", "will be lying", {"will lie", "will have lied", "am lying"}, "Сложный"},
        {"The children ___ TV when I came home.", "were watching", {"watched", "had watched", "have watched"}, "Сложный"}
    };
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(translationExercisesEasy.begin(), translationExercisesEasy.end(), g);
    std::shuffle(translationExercisesMedium.begin(), translationExercisesMedium.end(), g);
    std::shuffle(translationExercisesHard.begin(), translationExercisesHard.end(), g);

    std::shuffle(grammarExercisesEasy.begin(), grammarExercisesEasy.end(), g);
    std::shuffle(grammarExercisesMedium.begin(), grammarExercisesMedium.end(), g);
    std::shuffle(grammarExercisesHard.begin(), grammarExercisesHard.end(), g);

    QSqlQuery query;
    query.exec("DELETE FROM translation_exercises");
    query.exec("DELETE FROM grammar_exercises");

    for (const auto& exercise : translationExercisesEasy) {
        addTranslationExercise(exercise.english, exercise.russian, exercise.difficulty);
    }
    for (const auto& exercise : translationExercisesMedium) {
        addTranslationExercise(exercise.english, exercise.russian, exercise.difficulty);
    }
    for (const auto& exercise : translationExercisesHard) {
        addTranslationExercise(exercise.english, exercise.russian, exercise.difficulty);
    }

    for (const auto& exercise : grammarExercisesEasy) {
        addGrammarExercise(exercise.sentence, exercise.correctAnswer, exercise.options, exercise.difficulty);
    }
    for (const auto& exercise : grammarExercisesMedium) {
        addGrammarExercise(exercise.sentence, exercise.correctAnswer, exercise.options, exercise.difficulty);
    }
    for (const auto& exercise : grammarExercisesHard) {
        addGrammarExercise(exercise.sentence, exercise.correctAnswer, exercise.options, exercise.difficulty);
    }
}

void LanguageApp::startTranslationExercise() {
    currentExerciseType = "translation";
    currentExerciseIndex = 0;
    currentTranslationExercises.clear();

    QSqlQuery query;
    query.prepare("SELECT english, russian FROM translation_exercises WHERE difficulty = :difficulty");
    query.bindValue(":difficulty", currentDifficulty);
    if (query.exec()) {
        while (query.next()) {
            TranslationExercise exercise;
            exercise.english = query.value(0).toString();
            exercise.russian = query.value(1).toString();
            exercise.difficulty = currentDifficulty;
            currentTranslationExercises.push_back(exercise);
        }
    } else {
        QMessageBox::critical(this, "Ошибка базы данных", "Не удалось загрузить упражнения для перевода: " + query.lastError().text());
        return;
    }

    if (currentTranslationExercises.empty()) {
        QMessageBox::warning(this, "Внимание", "Нет упражнений для перевода с выбранным уровнем сложности.");
        exerciseWidget->setVisible(false);
        return;
    }

    if (currentDifficulty == "Легкий") {
        timeLimit = 180;
        maxAttempts = 5;
    } else if (currentDifficulty == "Средний") {
        timeLimit = 120;
        maxAttempts = 3;
    } else {
        timeLimit = 60;
        maxAttempts = 2;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(currentTranslationExercises.begin(), currentTranslationExercises.end(), g);

    if (currentTranslationExercises.size() > 10) {
        currentTranslationExercises.resize(10);
    }

    attemptsLeft = maxAttempts;
    timeLeft = timeLimit;
    updateTimeDisplay();

    setupTranslationUI();
    progressBar->setRange(0, static_cast<int>(currentTranslationExercises.size()));
    progressBar->setValue(0);
    showNextTranslationExercise();
    exerciseWidget->setVisible(true);
    exerciseTimer->start();
}

void LanguageApp::startGrammarExercise() {
    currentExerciseType = "grammar";
    currentExerciseIndex = 0;
    currentGrammarExercises.clear();

    QSqlQuery query;
    query.prepare("SELECT sentence, correct_answer, option1, option2, option3 FROM grammar_exercises WHERE difficulty = :difficulty");
    query.bindValue(":difficulty", currentDifficulty);
    if (query.exec()) {
        while (query.next()) {
            GrammarExercise exercise;
            exercise.sentence = query.value(0).toString();
            exercise.correctAnswer = query.value(1).toString();
            exercise.options = {
                query.value(1).toString(),
                query.value(2).toString(),
                query.value(3).toString(),
                query.value(4).toString()
            };
            exercise.difficulty = currentDifficulty;
            currentGrammarExercises.push_back(exercise);
        }
    } else {
        QMessageBox::critical(this, "Ошибка базы данных", "Не удалось загрузить грамматические упражнения: " + query.lastError().text());
        return;
    }

    if (currentGrammarExercises.empty()) {
        QMessageBox::warning(this, "Внимание", "Нет грамматических упражнений с выбранным уровнем сложности.");
        exerciseWidget->setVisible(false);
        return;
    }

    if (currentDifficulty == "Легкий") {
        timeLimit = 180;
        maxAttempts = 5;
    } else if (currentDifficulty == "Средний") {
        timeLimit = 120;
        maxAttempts = 3;
    } else {
        timeLimit = 60;
        maxAttempts = 2;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(currentGrammarExercises.begin(), currentGrammarExercises.end(), g);

    if (currentGrammarExercises.size() > 10) {
        currentGrammarExercises.resize(10);
    }

    attemptsLeft = maxAttempts;
    timeLeft = timeLimit;
    updateTimeDisplay();

    setupGrammarUI();
    progressBar->setRange(0, static_cast<int>(currentGrammarExercises.size()));
    progressBar->setValue(0);
    showNextGrammarExercise();
    exerciseWidget->setVisible(true);
    exerciseTimer->start();
}
void LanguageApp::setupTranslationUI() {
    clearExerciseWidgets();

    exerciseLabel = new QLabel(exerciseWidget);
    exerciseLabel->setStyleSheet("QLabel { font-size: 16px; }");

    answerEdit = new QTextEdit(exerciseWidget);
    answerEdit->setMaximumHeight(100);

    submitBtn = new QPushButton("Submit", exerciseWidget);
    submitBtn->setMinimumSize(150, 40);

    QPushButton *exitBtn = new QPushButton("Exit Exercise", exerciseWidget);
    exitBtn->setMinimumSize(150, 40);
    exitBtn->setStyleSheet("QPushButton { background-color: #d9534f; } QPushButton:hover { background-color: #c9302c; }");

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(submitBtn);
    buttonLayout->addWidget(exitBtn);

    exerciseLayout->addWidget(exerciseLabel);
    exerciseLayout->addWidget(answerEdit);
    exerciseLayout->addLayout(buttonLayout);

    connect(submitBtn, &QPushButton::clicked, this, &LanguageApp::checkTranslationAnswer);
    connect(exitBtn, &QPushButton::clicked, this, [this]() {
        finishExercise(false, false, true);
    });

    progressBar->setRange(0, static_cast<int>(currentTranslationExercises.size()));
    progressBar->setValue(0);
    progressBar->setVisible(true);
}

void LanguageApp::setupGrammarUI() {
    clearExerciseWidgets();

    exerciseLabel = new QLabel(exerciseWidget);
    exerciseLabel->setStyleSheet("QLabel { font-size: 16px; }");

    radioGroup = new QButtonGroup(exerciseWidget);

    submitBtn = new QPushButton("Submit", exerciseWidget);
    submitBtn->setMinimumSize(150, 40);

    QPushButton *exitBtn = new QPushButton("Exit Exercise", exerciseWidget);
    exitBtn->setMinimumSize(150, 40);
    exitBtn->setStyleSheet("QPushButton { background-color: #d9534f; } QPushButton:hover { background-color: #c9302c; }");

    QVBoxLayout *optionsLayout = new QVBoxLayout();
    optionsLayout->setSpacing(10);

    for (int i = 0; i < 4; ++i) {
        QRadioButton *radioBtn = new QRadioButton(exerciseWidget);
        radioBtn->setStyleSheet("QRadioButton { font-size: 14px; }");
        radioGroup->addButton(radioBtn, i);
        optionsLayout->addWidget(radioBtn);
    }

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(submitBtn);
    buttonLayout->addWidget(exitBtn);

    exerciseLayout->addWidget(exerciseLabel);
    exerciseLayout->addLayout(optionsLayout);
    exerciseLayout->addLayout(buttonLayout);

    connect(submitBtn, &QPushButton::clicked, this, &LanguageApp::checkGrammarAnswer);
    connect(exitBtn, &QPushButton::clicked, this, [this]() {
        finishExercise(false, false, true);
    });

    progressBar->setRange(0, static_cast<int>(currentGrammarExercises.size()));
    progressBar->setValue(0);
    progressBar->setVisible(true);
}


void LanguageApp::showNextTranslationExercise() {
    if (currentExerciseIndex >= static_cast<int>(currentTranslationExercises.size())) {
        finishExercise(true);
        return;
    }

    auto exercise = currentTranslationExercises[currentExerciseIndex];
    exerciseLabel->setText("Переведите: <b>" + exercise.english + "</b>");
    answerEdit->clear();
    answerEdit->setFocus();
}


void LanguageApp::showNextGrammarExercise() {
    if (currentExerciseIndex >= static_cast<int>(currentGrammarExercises.size())) {
        finishExercise(true);
        return;
    }

    auto exercise = currentGrammarExercises[currentExerciseIndex];
    exerciseLabel->setText("<b>" + exercise.sentence + "</b>");

    std::vector<QString> options = exercise.options;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(options.begin(), options.end(), g);

    for (int i = 0; i < 4; ++i) {
        QAbstractButton *button = radioGroup->button(i);
        if (button) {
            button->setText(options[i]);
            button->setChecked(false);
        }
    }
}

void LanguageApp::checkTranslationAnswer() {
    QString userAnswer = answerEdit->toPlainText().trimmed();
    QString correctAnswer = currentTranslationExercises[currentExerciseIndex].russian;

    if (userAnswer.compare(correctAnswer, Qt::CaseInsensitive) == 0) {
        currentExerciseIndex++;
        progressBar->setValue(currentExerciseIndex);
        showNextTranslationExercise();
    } else {
        attemptsLeft--;
        if (attemptsLeft <= 0) {
            finishExercise(false);
        } else {
            QMessageBox::warning(this, "Неверно",
                QString("Неправильный ответ. Осталось попыток: %1").arg(attemptsLeft));
        }
    }
}

void LanguageApp::checkGrammarAnswer() {
    int selectedId = radioGroup->checkedId();
    if (selectedId == -1) {
        QMessageBox::warning(this, "Ошибка", "Выберите вариант ответа");
        return;
    }

    QString selectedAnswer = radioGroup->button(selectedId)->text();
    QString correctAnswer = currentGrammarExercises[currentExerciseIndex].correctAnswer;

    if (selectedAnswer == correctAnswer) {
        currentExerciseIndex++;
        progressBar->setValue(currentExerciseIndex);
        showNextGrammarExercise();
    } else {
        attemptsLeft--;
        if (attemptsLeft <= 0) {
            finishExercise(false);
        } else {
            QMessageBox::warning(this, "Неверно",
                QString("Неправильный ответ. Осталось попыток: %1").arg(attemptsLeft));
        }
    }
}

void LanguageApp::finishExercise(bool success, bool timeExpired, bool manualExit) {
    timeLabel->setText("Время: --:--");
    progressBar->setVisible(false);
    exerciseTimer->stop();

    if (success) {
        int pointsEarned = 0;
        if (currentDifficulty == "Легкий") {
            pointsEarned = 5;
        } else if (currentDifficulty == "Средний") {
            pointsEarned = 10;
        } else {
            pointsEarned = 15;
        }
        score += pointsEarned;
        scoreLabel->setText(QString("Баллы: %1").arg(score));

        QMessageBox::information(this, "Успех",
            QString("Поздравляем! Вы успешно завершили упражнение.\n"
                   "Уровень сложности: %1\n"
                   "Заработано баллов: %2\n"
                   "Общий счет: %3")
                .arg(currentDifficulty)
                .arg(pointsEarned)
                .arg(score));
    } else {
        if (manualExit) {
            QMessageBox::information(this, "Упражнение прервано",
                "Вы вышли из упражнения. Баллы не начислены.");
        } else if (!timeExpired) {
            QMessageBox::information(this, "Завершено",
                "Упражнение завершено.\n"
                "Вы превысили максимальное количество попыток.");
        }
    }

    exerciseWidget->setVisible(false);
}

void LanguageApp::changeDifficulty() {
    DifficultyDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        currentDifficulty = dialog.getDifficulty();

        if (currentDifficulty == "Легкий") {
            maxAttempts = 5;
            timeLimit = 180;
        } else if (currentDifficulty == "Средний") {
            maxAttempts = 3;
            timeLimit = 120;
        } else {
            maxAttempts = 2;
            timeLimit = 60;
        }

        QMessageBox::information(this, "Уровень сложности",
            QString("Установлен уровень: %1\n"
                   "Максимальное количество попыток: %2\n"
                   "Лимит времени: %3 секунд")
                .arg(currentDifficulty)
                .arg(maxAttempts)
                .arg(timeLimit));
    }
}

void LanguageApp::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_H) {
        QString helpMessage;
        if (currentExerciseType == "translation") {
            helpMessage = "Подсказка по переводу:\n"
                         "1. Обратите внимание на контекст предложения\n"
                         "2. Учитывайте временные формы глаголов\n"
                         "3. Помните об устойчивых выражениях";
        } else if (currentExerciseType == "grammar") {
            helpMessage = "Подсказка по грамматике:\n"
                         "1. Определите время предложения\n"
                         "2. Проверьте согласование подлежащего и сказуемого\n"
                         "3. Для условных предложений помните правила:\n"
                         "   - 0 тип: If + Present, Present\n"
                         "   - 1 тип: If + Present, Future\n"
                         "   - 2 тип: If + Past, would + Infinitive\n"
                         "   - 3 тип: If + Past Perfect, would have + Participle";
        } else {
            helpMessage = "Выберите упражнение для получения подсказки";
        }
        QMessageBox::information(this, "Подсказка", helpMessage);
    }
    QMainWindow::keyPressEvent(event);
}

void LanguageApp::clearExerciseWidgets() {
    QLayoutItem *item;
    while ((item = exerciseLayout->takeAt(0))) {
        if (item->widget()) {
            if (item->widget()->parent() == exerciseWidget) {
                delete item->widget();
            }
        }
        delete item;
    }

    if (radioGroup) {
        QList<QAbstractButton*> buttons = radioGroup->buttons();
        for (auto button : buttons) {
            radioGroup->removeButton(button);
            if (button->parent() == exerciseWidget) {
                delete button;
            }
        }
    }
}