# 基于条码扫描的仓库管理系统

《计算系统设计与实现》综合实验

## 一、项目简介

本系统是一个嵌入式仓库管理应用，由两端组成：

- **嵌入式端**：QT5 程序运行在 IMX6 实验箱 LCD 上，通过串口扫码枪录入商品条码，实现入库、出库、库存查询、操作日志四个功能。
- **Web 端**：在主机浏览器访问实验箱的 HTTP 服务，查看库存总览、出入库流水、可视化报表，并可远程管理商品。

**核心架构**：QT 程序在 LCD 显示界面的同时，**内置一个轻量 HTTP 服务器**（基于 QTcpServer），主机浏览器直接访问这个端口即可。

```
┌─────────────────────────────────────────┐
│  实验箱 IMX6 (一个 WarehouseQt 进程)     │
│   ┌──────────────────────────────────┐  │
│   │  QT GUI (LCD 界面)                │  │
│   │  串口监听 (/dev/ttyACM0)           │  │
│   │  SQLite 读写                      │  │
│   │  HTTP 服务 (端口 8080) ◄─────HTTP │  │
│   └──────────────────────────────────┘  │
│          ↕                              │
│    warehouse.db                         │
└─────────────────────────────────────────┘
              ▲
              │ HTTP
              │
        [主机浏览器]
```

满足综合实验四大硬性要求：

| 要求 | 实现位置 |
|------|----------|
| 硬件使用（驱动/接口编程） | 串口扫码枪 → `qt_app/serialreader.cpp` 通过 QSerialPort 读取 |
| QT 在 LCD 生成窗口界面    | `qt_app/` 工程，5 个对话框 + 1 个主窗口 |
| 嵌入式 SQLite 数据库      | `db/warehouse.db`，QT 通过 SQL 模块读写 |
| Web 服务器生成网页界面    | `qt_app/httpserver.cpp` 内置 HTTP，serve `web/index.html` |

附加要求：

- 全汉化界面
- 主窗口顶部固定显示组员姓名学号

## 二、目录结构

```
warehouse/
├── db/
│   ├── init_db.sql               # 数据库初始化脚本
│   └── warehouse.db              # 生成的 SQLite 数据库
├── qt_app/                       # QT 工程
│   ├── WarehouseQt.pro
│   ├── main.cpp                  # 入口（数据库 + HTTP + GUI）
│   ├── mainwindow.{h,cpp}        # 主窗口（顶部组员姓名学号）
│   ├── logindialog.{h,cpp}       # 登录
│   ├── serialreader.{h,cpp}      # 串口监听
│   ├── dbmanager.{h,cpp}         # SQLite 单例封装
│   ├── httpserver.{h,cpp}        # 内置 HTTP 服务器（核心改造点）
│   ├── stockindialog.{h,cpp}     # 入库
│   ├── stockoutdialog.{h,cpp}    # 出库
│   ├── inventorydialog.{h,cpp}   # 库存查询
│   └── logdialog.{h,cpp}         # 操作日志
├── web/
│   └── index.html                # Web 前端（被 QT HTTP 服务器 serve）
└── docs/
    └── README.md
```

## 三、部署步骤

### 步骤 1：检查/补全实验箱运行库

minicom 中：

```sh
ls /usr/lib/libQt5SerialPort.so*
ls /usr/lib/libQt5Network.so*
ls /usr/lib/libsqlite3.so*
ls /usr/lib/qt5/plugins/sqldrivers/libqsqlite.so
```

缺哪个就从主机交叉编译目录拷过去：

```sh
# 主机
cd /opt/fsl-imx-wayland/4.9.88-2.0.0/sysroots/cortexa9hf-neon-poky-linux-gnueabi/usr/lib/
cp -d libQt5SerialPort.so* libsqlite3.so* /home/uptech/

# 实验箱（minicom）
cd /mnt/warehouse                              # NFS 挂载点
cp -d libQt5SerialPort.so* libsqlite3.so* /usr/lib/
ldconfig
ldconfig -p | grep -E "Qt5SerialPort|sqlite3"
```

### 步骤 2：在主机准备数据库

```sh
cd db
sqlite3 warehouse.db < init_db.sql
# 输出：用户数 3 / 商品数 10 / 流水数 4
```

### 步骤 3：交叉编译 QT 程序（主机）

```sh
sudo -s
source /opt/fsl-imx-wayland/4.9.88-2.0.0/environment-setup-cortexa9hf-neon-poky-linux-gnueabi
cd /home/uptech/warehouse/qt_app
qmake WarehouseQt.pro
make
```

产物 `WarehouseQt`。

### 步骤 4：通过 NFS 拷到实验箱

```sh
# 主机：把所有运行时文件放到 NFS 共享目录
cd /home/uptech/warehouse
mkdir -p /home/uptech/warehouse_run/web
cp qt_app/WarehouseQt           /home/uptech/warehouse_run/
cp db/warehouse.db              /home/uptech/warehouse_run/
cp web/index.html               /home/uptech/warehouse_run/web/

# 实验箱（minicom）
cp -r /mnt/warehouse_run /opt/warehouse
chmod +x /opt/warehouse/WarehouseQt
chmod 666 /opt/warehouse/warehouse.db
```

> **路径关键**：QT 程序里 `DB_PATH = "warehouse.db"` 和 `WEB_ROOT = "./web"` 是**相对当前目录**，所以要先 `cd /opt/warehouse`。

### 步骤 5：插好扫码枪 + 启动程序

```sh
# 实验箱
ls /dev/ttyACM0     # 应该存在
cd /opt/warehouse
./WarehouseQt
```

控制台会打印 Web 端访问地址：

```
HttpServer 已监听端口: 8080
===== Web 端访问地址 =====
  http://192.168.1.100:8080/
```

记住这个 URL。

### 步骤 6：登录使用

LCD 弹出登录窗口：
- `admin / 123456`（管理员）
- `zhang / 111`（员工）

### 步骤 7：主机浏览器访问 Web 端

主机浏览器输入步骤 5 打印的 URL，例如 `http://192.168.1.100:8080/`。

## 四、运行验证清单

| # | 操作 | 期望结果 |
|---|------|----------|
| 1 | LCD 启动 → admin/123456 → 主窗口 | 顶部显示组员姓名学号；状态栏显示串口已连接 |
| 2 | 控制台输出 | `HttpServer 已监听端口: 8080` |
| 3 | 入库 → 扫已存在条码（6901234567890） | 自动显示"康师傅红烧牛肉面" |
| 4 | 输入数量 5 → 确认 | 提示"✓ 入库成功" |
| 5 | 入库扫新条码 | 弹"是否新增" → 填写 → 入库 |
| 6 | 出库扫库存 0 的商品 | 红色，按钮禁用 |
| 7 | 出库数量超过库存 | 弹"库存不足" |
| 8 | 库存查询 → 搜"可乐" | 表格只显示可口可乐 |
| 9 | 操作日志 | 入库绿色，出库红色 |
| 10 | 主机浏览器 `http://实验箱IP:8080/` | Web 端正常加载 |
| 11 | LCD 入库一笔 → Web 等 5 秒 | 仪表盘自动更新 |
| 12 | Web 商品管理 → 新增 | 提示成功，库存能看到 |
| 13 | Web 数据报表 | 库存柱状图 + 7 日趋势 |

## 五、常见问题

**Q1: QT 启动报"数据库文件不存在"**
要 `cd /opt/warehouse` 后再运行，DB_PATH 是相对路径。

**Q2: QT 启动报 SQLite 驱动加载失败**
检查 `/usr/lib/qt5/plugins/sqldrivers/libqsqlite.so` 和 `/usr/lib/libsqlite3.so.0` 都在。

**Q3: 串口未连接**
`ls /dev/ttyACM*` 确认设备名；不同则改 `mainwindow.cpp` 里的 `SERIAL_PORT_NAME`。

**Q4: 扫码后没反应**
`cat /dev/ttyACM0` 测扫码枪是否输出。程序按 `\r` 或 `\n` 切分条码。

**Q5: HTTP 启动失败**
`netstat -an | grep 8080` 检查端口；占用就改 `main.cpp` 的 `HTTP_PORT`。

**Q6: 主机浏览器访问超时**
1) `ifconfig` 看实验箱 eth0 IP；2) 主机和实验箱同网段；3) ping 通否。

**Q7: Web 端 Bootstrap/Chart.js 加载不出（无外网）**
主机下载文件到 `/opt/warehouse/web/`，改 HTML 里 CDN 链接为相对路径。HTTP 服务器会自动 serve。

## 六、附加分扩展方向

1. 加微型打印机：出库后自动打印小票
2. 库存预警蜂鸣器
3. CSV 报表导出
4. 多操作员并发登录

## 七、组员分工

- **同学 A**：环境搭建、串口驱动、QT 6 个界面、SQLite 接入
- **同学 B**：HTTP 服务器、HTML/JS 前端、Chart.js 图表、美化

## 八、技术栈

- **嵌入式**：IMX6Q + Linux 4.9.88 + QT5（Widgets + Sql + SerialPort + **Network**）+ SQLite3
- **Web**：HTML5 + Bootstrap 5 + Chart.js 4
- **通信**：扫码枪 → 串口 (USB-CDC-ACM) → QT；浏览器 → HTTP/8080 → QTcpServer → SQLite

## 九、架构设计要点（写报告用）

**为什么 QT 内置 HTTP 服务器，而不是 boa + Python CGI？**

1. **依赖最小化**：实验箱 Yocto 镜像的 Python 2.7 没有 sqlite3 模块；走 CGI 路线需要交叉编译 ARM 版的 sqlite3 命令行或 Python 扩展，工作量大且容易出错。
2. **架构更简单**：HTTP 服务器和数据库读写在**同一进程**，无文件锁/并发竞争问题。
3. **更"嵌入式"**：实验箱真正成为独立的服务端设备，符合"物联网网关"角色。
4. **代码可控**：自己实现的 HTTP 解析器约 250 行 C++，无黑盒。

**为什么用 QTcpServer 而不是引第三方库？**

QT 自带 QTcpServer 是基于事件循环的异步 IO，无需多线程，无需额外依赖。手写 HTTP 解析器对本项目场景够用，避免引入 boost/cpp-httplib 等需要单独交叉编译的库。
