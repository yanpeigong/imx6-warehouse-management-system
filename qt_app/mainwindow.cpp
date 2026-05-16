#include "mainwindow.h"
#include "serialreader.h"
#include "stockindialog.h"
#include "stockoutdialog.h"
#include "inventorydialog.h"
#include "logdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStatusBar>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>

// ====== 修改这里：填入你和队友的真实姓名学号 ======
static const QString TEAM_INFO =
    "组员：龚焱培 2023211640   李宏恩 2023211644";

// ====== 串口设备名：你的扫码枪是 USB CDC-ACM 设备 ======
static const QString SERIAL_PORT_NAME = "/dev/ttyACM0";
static const int     SERIAL_BAUD      = 9600;

MainWindow::MainWindow(const QString &username, const QString &role, QWidget *parent)
    : QMainWindow(parent), username_(username), role_(role) {
    setWindowTitle("仓库管理系统");
    resize(800, 480);

    // ===== 顶部信息栏（必须显示组员学号姓名）=====
    QLabel *teamLbl = new QLabel(TEAM_INFO);
    teamLbl->setStyleSheet(
        "background-color:#1976D2; color:white; padding:10px; font-size:14px; font-weight:bold;");
    teamLbl->setAlignment(Qt::AlignCenter);

    QLabel *titleLbl = new QLabel("基于条码扫描的仓库管理系统");
    QFont tf = titleLbl->font();
    tf.setPointSize(20); tf.setBold(true);
    titleLbl->setFont(tf);
    titleLbl->setAlignment(Qt::AlignCenter);
    titleLbl->setStyleSheet("padding:18px; color:#0D47A1;");

    QLabel *userLbl = new QLabel(QString("当前用户：%1（%2）")
                                     .arg(username_)
                                     .arg(role_ == "admin" ? "管理员" : "员工"));
    userLbl->setAlignment(Qt::AlignCenter);
    userLbl->setStyleSheet("color:#555; font-size:12px;");

    // ===== 4 个功能按钮（2x2 网格）=====
    auto makeBtn = [](const QString &emoji, const QString &text, const QString &color){
        QPushButton *b = new QPushButton(emoji + "\n" + text);
        b->setMinimumSize(220, 110);
        b->setStyleSheet(QString(
            "QPushButton{font-size:18px; font-weight:bold; color:white;"
            "background-color:%1; border-radius:10px; padding:8px;}"
            "QPushButton:hover{background-color:%1; opacity:0.85;}"
            "QPushButton:pressed{padding-top:10px;}").arg(color));
        return b;
    };
    QPushButton *btnIn   = makeBtn("Stock Out", "入库", "#1976D2");
    QPushButton *btnOut  = makeBtn("Stock In", "出库", "#D32F2F");
    QPushButton *btnInv  = makeBtn("Stock Inquiry", "库存查询", "#388E3C");
    QPushButton *btnLog  = makeBtn("Operation Query", "操作日志", "#7B1FA2");

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(btnIn,  0, 0);
    grid->addWidget(btnOut, 0, 1);
    grid->addWidget(btnInv, 1, 0);
    grid->addWidget(btnLog, 1, 1);
    grid->setSpacing(20);

    QPushButton *btnExit = new QPushButton("退出系统");
    btnExit->setMinimumHeight(36);
    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->addStretch(); bottom->addWidget(btnExit);

    QWidget *central = new QWidget;
    QVBoxLayout *vb = new QVBoxLayout(central);
    vb->addWidget(teamLbl);
    vb->addWidget(titleLbl);
    vb->addWidget(userLbl);
    vb->addSpacing(20);
    vb->addLayout(grid);
    vb->addStretch();
    vb->addLayout(bottom);
    setCentralWidget(central);

    // ===== 状态栏：显示串口状态 =====
    statusBar()->showMessage("正在连接串口...");

    // ===== 串口 =====
    reader_ = new SerialReader(this);
    connect(reader_, &SerialReader::errorOccurred,
            this,    &MainWindow::onSerialError);
    if (reader_->open(SERIAL_PORT_NAME, SERIAL_BAUD)) {
        statusBar()->showMessage(QString("串口已连接：%1 @ %2bps")
                                     .arg(SERIAL_PORT_NAME).arg(SERIAL_BAUD));
    } else {
        statusBar()->showMessage("⚠ 串口未连接，可使用 [手动输入条码] 进行测试");
    }

    // ===== 信号槽 =====
    connect(btnIn,   &QPushButton::clicked, this, &MainWindow::onStockIn);
    connect(btnOut,  &QPushButton::clicked, this, &MainWindow::onStockOut);
    connect(btnInv,  &QPushButton::clicked, this, &MainWindow::onInventory);
    connect(btnLog,  &QPushButton::clicked, this, &MainWindow::onLog);
    connect(btnExit, &QPushButton::clicked, qApp, &QApplication::quit);
}

MainWindow::~MainWindow() {}

void MainWindow::onStockIn() {
    StockInDialog dlg(reader_, username_, this);
    dlg.exec();
}

void MainWindow::onStockOut() {
    StockOutDialog dlg(reader_, username_, this);
    dlg.exec();
}

void MainWindow::onInventory() {
    InventoryDialog dlg(this);
    dlg.exec();
}

void MainWindow::onLog() {
    LogDialog dlg(this);
    dlg.exec();
}

void MainWindow::onSerialError(const QString &msg) {
    statusBar()->showMessage("串口错误: " + msg);
}
