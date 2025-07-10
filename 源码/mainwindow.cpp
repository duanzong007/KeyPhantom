#include "mainwindow.h"
#include "ui_mainwindow.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <QMessageBox>

/* =================================================================== */
/*                         KeySenderThread                              */
/* =================================================================== */
void KeySenderThread::run()
{
    int batch{}, charInt{}, batchInt{};
    switch (speed_) {
    case Slow :  batch=1;  charInt=20; batchInt=300; break;
    case Normal: batch=5;  charInt=10; batchInt= 50; break;
    case Fast :  batch=20; charInt= 2; batchInt= 20; break;
    case Ultra:  batch=50; charInt= 0; batchInt= 10; break;
    }

    int cnt = 0;
    for (QChar ch : text_)
    {
        if (stop_.load()) { emit finished(true); return; }

        // ---------- 新增分支 ----------
        if (ch == u'\n' || ch == u'\r') {
            sendVk(VK_RETURN);                     // 真实回车
        }
        else if (ch == u'\t') {
            sendVk(VK_TAB);                        // Tab
        }
        else {
            sendUnicode(ch);                       // 其他仍走 Unicode
        }
        // --------------------------------

        msleep(charInt);
        if (++cnt >= batch) { cnt = 0; msleep(batchInt); }
    }

    emit finished(false);
}

void KeySenderThread::sendUnicode(QChar ch)
{
    INPUT ip[2]{};
    ip[0].type = INPUT_KEYBOARD;
    ip[0].ki.wScan = ch.unicode();
    ip[0].ki.dwFlags = KEYEVENTF_UNICODE;        // down
    ip[1] = ip[0];
    ip[1].ki.dwFlags |= KEYEVENTF_KEYUP;         // up
    SendInput(2, ip, sizeof(INPUT));
}

void KeySenderThread::sendVk(unsigned short vk)
{
    INPUT ip[2]{};
    ip[0].type       = INPUT_KEYBOARD;
    ip[0].ki.wVk     = vk;               // 虚拟键
    ip[0].ki.dwFlags = 0;                // down

    ip[1]            = ip[0];
    ip[1].ki.dwFlags = KEYEVENTF_KEYUP;  // up

    SendInput(2, ip, sizeof(INPUT));
}

/* =================================================================== */
/*                            MainWindow                                */
/* =================================================================== */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox->addItems({u8"慢速", u8"正常", u8"快速", u8"超高速"});
    ui->comboBox->setCurrentIndex(1);

    /* 单一按钮：粘贴 / 停止 */
    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::onMainBtnClicked);

    /* 3 秒单次定时器 */
    delayTimer_.setSingleShot(true);
    connect(&delayTimer_, &QTimer::timeout,
            this, &MainWindow::beginAfterDelay);

    /* 线程结束 */
    connect(&sender_, &KeySenderThread::finished,
            this, &MainWindow::onSendFinished);
}

MainWindow::~MainWindow()
{
    sender_.requestStop();
    sender_.quit();
    sender_.wait();
    delete ui;
}

/* ----------------------------- 槽函数 ----------------------------- */
void MainWindow::onMainBtnClicked()
{
    if (state_ == Idle) {                           // 准备开始
        if (ui->plainTextEdit->toPlainText().isEmpty()) {
            QMessageBox::information(this, u8"提示", u8"文本为空，无法发送！");
            return;
        }
        state_ = Waiting;
        ui->pushButton->setEnabled(false);
        ui->pushButton->setText(u8"3 秒后开始…");
        delayTimer_.start(3000);
    }
    else if (state_ == Sending) {                   // 停止
        ui->pushButton->setEnabled(false);
        sender_.requestStop();
    }
    /* Waiting 状态下按钮是 disabled，不会点击进来 */
}

void MainWindow::beginAfterDelay()
{
    sender_.configure(ui->plainTextEdit->toPlainText(), currentSpeed());
    sender_.start();

    state_ = Sending;
    ui->pushButton->setText(u8"停止");
    ui->pushButton->setEnabled(true);
}

void MainWindow::onSendFinished(bool byUserStop)
{
    state_ = Idle;
    ui->pushButton->setText(u8"粘贴");
    ui->pushButton->setEnabled(true);

    QMessageBox::information(this, u8"完成",
                             byUserStop ? u8"已手动停止输入！" : u8"全部字符已发送完毕！");
}


/* ----------------------------- 工具 ------------------------------ */
KeySenderThread::Speed MainWindow::currentSpeed() const
{
    return static_cast<KeySenderThread::Speed>(ui->comboBox->currentIndex());
}
