#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class SyntaxHighlighter : public QSyntaxHighlighter {
public:
    SyntaxHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {
        setupFormats();
    }

protected:
    void highlightBlock(const QString &text) override {
        for (const HighlightingRule &rule : highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    void setupFormats() {
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(Qt::red);
        keywordFormat.setFontWeight(QFont::Bold);

        QTextCharFormat loopFormat;
        loopFormat.setForeground(Qt::darkCyan);

        QTextCharFormat dataTypeFormat;
        dataTypeFormat.setForeground(Qt::darkGreen);

        HighlightingRule rule;

        // Data types
        QStringList dataTypes = {"int", "char", "float", "double", "void"};
        for (const QString &dataType : dataTypes) {
            rule.pattern = QRegularExpression("\\b" + dataType + "\\b");
            rule.format = dataTypeFormat;
            highlightingRules.append(rule);
        }

        // Loops
        QStringList loopKeywords = {"for", "while", "do", "break", "continue"};
        for (const QString &loopKeyword : loopKeywords) {
            rule.pattern = QRegularExpression("\\b" + loopKeyword + "\\b");
            rule.format = loopFormat;
            highlightingRules.append(rule);
        }

        // Other keywords
        QStringList otherKeywords = {
            "if", "else", "return", "switch", "case", "default"
        };
        for (const QString &otherKeyword : otherKeywords) {
            rule.pattern = QRegularExpression("\\b" + otherKeyword + "\\b");
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }
    }
};

class CodeEditor : public QMainWindow {
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr) : QMainWindow(parent) {
        textEdit = new QTextEdit(this);
        setCentralWidget(textEdit);

        highlighter = new SyntaxHighlighter(textEdit->document());

        createActions();
        createMenus();

        setWindowTitle("Code Editor");
        resize(600, 400);
    }

private slots:
    void newFile() {
        if (maybeSave()) {
            textEdit->clear();
            setCurrentFile("");
        }
    }

    void open() {
        if (maybeSave()) {
            QString fileName = QFileDialog::getOpenFileName(this);
            if (!fileName.isEmpty())
                loadFile(fileName);
        }
    }

    bool save() {
        if (currentFile.isEmpty()) {
            return saveAs();
        } else {
            return saveFile(currentFile);
        }
    }

    bool saveAs() {
        QString fileName = QFileDialog::getSaveFileName(this);
        if (!fileName.isEmpty())
            return saveFile(fileName);
        return false;
    }

    void about() {
        QMessageBox::about(this, tr("About Code Editor"),
                           tr("Simple code editor written in C++ with Qt"));
    }

private:
    void createActions() {
        newAct = new QAction(tr("&New"), this);
        newAct->setShortcuts(QKeySequence::New);
        connect(newAct, &QAction::triggered, this, &CodeEditor::newFile);

        openAct = new QAction(tr("&Open..."), this);
        openAct->setShortcuts(QKeySequence::Open);
        connect(openAct, &QAction::triggered, this, &CodeEditor::open);

        saveAct = new QAction(tr("&Save"), this);
        saveAct->setShortcuts(QKeySequence::Save);
        connect(saveAct, &QAction::triggered, this, &CodeEditor::save);

        saveAsAct = new QAction(tr("Save &As..."), this);
        connect(saveAsAct, &QAction::triggered, this, &CodeEditor::saveAs);

        exitAct = new QAction(tr("E&xit"), this);
        exitAct->setShortcuts(QKeySequence::Quit);
        connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);

        aboutAct = new QAction(tr("&About"), this);
        connect(aboutAct, &QAction::triggered, this, &CodeEditor::about);
    }

    void createMenus() {
        fileMenu = menuBar()->addMenu(tr("&File"));
        fileMenu->addAction(newAct);
        fileMenu->addAction(openAct);
        fileMenu->addAction(saveAct);
        fileMenu->addAction(saveAsAct);
        fileMenu->addSeparator();
        fileMenu->addAction(exitAct);

        helpMenu = menuBar()->addMenu(tr("&Help"));
        helpMenu->addAction(aboutAct);
    }

    bool maybeSave() {
        if (textEdit->document()->isModified()) {
            QMessageBox::StandardButton ret;
            ret = QMessageBox::warning(this, tr("Application"),
                                       tr("The document has been modified.\n"
                                          "Do you want to save your changes?"),
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            if (ret == QMessageBox::Save)
                return save();
            else if (ret == QMessageBox::Cancel)
                return false;
        }
        return true;
    }

    void loadFile(const QString &fileName) {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("Application"),
                                 tr("Cannot read file %1:\n%2.")
                                     .arg(QDir::toNativeSeparators(fileName), file.errorString()));
            return;
        }

        QTextStream in(&file);
        textEdit->setPlainText(in.readAll());

        setCurrentFile(fileName);
    }

    bool saveFile(const QString &fileName) {
        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("Application"),
                                 tr("Cannot write file %1:\n%2.")
                                     .arg(QDir::toNativeSeparators(fileName), file.errorString()));
            return false;
        }

        QTextStream out(&file);
        out << textEdit->toPlainText();

        setCurrentFile(fileName);
        return true;
    }

    void setCurrentFile(const QString &fileName) {
        currentFile = fileName;
        textEdit->document()->setModified(false);
        setWindowModified(false);

        QString shownName = currentFile;
        if (currentFile.isEmpty())
            shownName = "untitled.txt";
        setWindowFilePath(shownName);
    }

    QTextEdit *textEdit;
    QString currentFile;

    QMenu *fileMenu;
    QMenu *helpMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *aboutAct;

    SyntaxHighlighter *highlighter;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    CodeEditor editor;
    editor.show();
    return app.exec();
}

#include "main.moc"
