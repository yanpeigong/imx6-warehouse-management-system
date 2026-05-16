# 基于条码扫描的仓库管理系统

《计算系统设计与实现》综合实验

## 一、项目简介

本系统是一个嵌入式仓库管理应用，由两端组成：

- **嵌入式端**：QT5 程序运行在 IMX6 实验箱 LCD 上，通过 USB 扫码枪（CDC-ACM 设备）录入商品条码，实现入库、出库、库存查询、操作日志四个功能。
- **Web 端**：在主机浏览器访问实验箱的 HTTP 服务，查看库存总览、出入库流水、可视化报表，并可远程管理商品。

**核心架构**：QT 程序在 LCD 显示界面的同时，**内置一个轻量 HTTP 服务器**（基于 QTcpServer），主机浏览器直接访问这个端口即可。**职责分离设计**：商品的录入由 Web 端管理者完成，LCD 端只负责扫码出入库等高频操作。

```
┌─────────────────────────────────────────┐
│  实验箱 IMX6 (一个 WarehouseQt 进程)     │
│   ┌──────────────────────────────────┐  │
│   │  QT GUI (LCD 界面)                │  │
│   │  串口监听 (/dev/ttyACM0)           │  │
│   │  SQLite 直接读写 (sqlite3 C API)  │  │
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
| 硬件使用（驱动/接口编程） | USB 扫码枪 → `qt_app/serialreader.cpp` 通过 QSerialPort 读取 |
| QT 在 LCD 生成窗口界面    | `qt_app/` 工程，5 个对话框 + 1 个主窗口 |
| 嵌入式 SQLite 数据库      | `db/warehouse.db`，通过 sqlite3 C API 读写 |
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
│   ├── WarehouseQt.pro           # 注：链接 -lsqlite3 不依赖 Qt SQL
│   ├── main.cpp                  # 入口（数据库 + HTTP + GUI）
│   ├── mainwindow.{h,cpp}        # 主窗口（顶部组员姓名学号）
│   ├── logindialog.{h,cpp}       # 登录
│   ├── serialreader.{h,cpp}      # 串口监听
│   ├── dbmanager.{h,cpp}         # SQLite 单例封装（用 sqlite3 C API）
│   ├── httpserver.{h,cpp}        # 内置 HTTP 服务器
│   ├── stockindialog.{h,cpp}     # 入库（未登记商品引导到 Web 端）
│   ├── stockoutdialog.{h,cpp}    # 出库（库存三重校验）
│   ├── inventorydialog.{h,cpp}   # 库存查询
│   └── logdialog.{h,cpp}         # 操作日志
├── web/                          # Web 前端（被 QT HTTP 服务器 serve）
│   ├── index.html                # 单页应用
│   ├── bootstrap.min.css         # 本地化的 CDN 文件
│   ├── bootstrap.bundle.min.js
│   └── chart.umd.min.js
└── docs/
    └── README.md
```

## 三、部署步骤

> 假设你的 NFS 共享目录是 `/home/uptech`（实验箱挂载点 `/mnt`）。如果不是，请按实际路径替换。

### 步骤 1：补全实验箱 Qt5 运行时

实验箱出厂可能自带的是 Qt 5.3.2，而本工程用 Qt 5.9.4 编译。**必须把整套 Qt5 库替换成 5.9.4 版本**，否则运行时会报：

```
Cannot mix incompatible Qt library (version 0x50302) with this library (version 0x50904)
```

**主机上**：

```sh
cd /opt/fsl-imx-wayland/4.9.88-2.0.0/sysroots/cortexa9hf-neon-poky-linux-gnueabi/usr/lib/

# 把所有 Qt5 相关的 so 文件都拷到 NFS 共享目录
# (含 Core/Gui/Widgets/Sql/Network/SerialPort/Wayland 等等)
cp -d libQt5*.so* /home/uptech/

# 把 sqlite3 库也带上（实验箱默认没装）
cp -d libsqlite3.so* /home/uptech/
```

**实验箱（minicom）**：

```sh
cd /mnt
# 备份原来的（保险起见）
mkdir -p /usr/lib/qt5_old_backup
mv /usr/lib/libQt5*.so* /usr/lib/qt5_old_backup/ 2>/dev/null

# 拷新版库
cp -d libQt5*.so* /usr/lib/
cp -d libsqlite3.so* /usr/lib/
ldconfig

# 验证版本
strings /usr/lib/libQt5Core.so.5 | grep "Qt 5\." | head -1
# 应该输出 Qt 5.9.4 ...

ldconfig -p | grep -E "Qt5Core|sqlite3"
```

### 步骤 2：在主机准备数据库

```sh
cd /home/uptech/warehouse/db
sqlite3 warehouse.db < init_db.sql
# 输出：用户数 3 / 商品数 10 / 流水数 4
```

> 注：**主机上必须装 `sqlite3` 命令**（`apt install sqlite3`），实验箱上不需要——本程序通过 `libsqlite3.so` 库直接调用 C API，不依赖命令行工具。

### 步骤 3：交叉编译 QT 程序（主机）

```sh
sudo -s
source /opt/fsl-imx-wayland/4.9.88-2.0.0/environment-setup-cortexa9hf-neon-poky-linux-gnueabi
cd /home/uptech/warehouse/qt_app
qmake WarehouseQt.pro
make
```

产物 `WarehouseQt`（ARM 32-bit ELF）。

> 验证编译产物正确：`file WarehouseQt` 应该输出 `ELF 32-bit ... ARM`。

### 步骤 4：通过 NFS 拷到实验箱

**主机**：

```sh
cd /home/uptech/warehouse
mkdir -p /home/uptech/warehouse_run/web
cp qt_app/WarehouseQt    /home/uptech/warehouse_run/
cp db/warehouse.db       /home/uptech/warehouse_run/
cp web/*                 /home/uptech/warehouse_run/web/      # 注：web/* 含 4 个文件
```

**实验箱（minicom）**：

```sh
cp -r /mnt/warehouse_run /opt/warehouse
chmod +x /opt/warehouse/WarehouseQt
chmod 666 /opt/warehouse/warehouse.db
```

> **路径关键**：QT 程序里 `DB_PATH = "warehouse.db"` 和 `WEB_ROOT = "./web"` 是**相对当前目录**，所以下面启动时一定要先 `cd /opt/warehouse`。

### 步骤 5：创建启动脚本

直接 `./WarehouseQt` 不会工作——需要先设置 wayland 显示相关的环境变量。

**实验箱**：

```sh
cat > /opt/warehouse/run.sh << 'EOF'
#!/bin/sh
export XDG_RUNTIME_DIR=/run/user/0
export WAYLAND_DISPLAY=wayland-0
export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/qt5/plugins/platforms
export QT_QPA_PLATFORM=wayland-egl
cd /opt/warehouse
./WarehouseQt
EOF
chmod +x /opt/warehouse/run.sh
```

### 步骤 6：插好扫码枪 + 启动程序

```sh
# 实验箱：先确认扫码枪
ls /dev/ttyACM0     # 应该存在
# 试一下：cat /dev/ttyACM0 然后扫一个条码，应该有输出（按 Ctrl+C 退出）

# 启动
/opt/warehouse/run.sh
```

启动成功的标志（控制台输出）：

```
数据库已连接: "warehouse.db"
HttpServer 已监听端口: 8080
===== Web 端访问地址 =====
"  http://192.168.x.x:8080/"
Using Wayland-EGL
Using the 'xdg-shell-v6' shell integration
串口已打开: "/dev/ttyACM0"  波特率: 9600
```

记下打印的 URL 备用。LCD 上会弹出登录窗口。

### 步骤 7：登录使用

LCD 弹出登录窗口：
- `admin / 123456`（管理员）
- `zhang / 111`（员工）

### 步骤 8：主机浏览器访问 Web 端

主机浏览器输入步骤 6 打印的 URL，例如 `http://192.168.1.100:8080/`。

## 四、运行验证清单

| # | 操作 | 期望结果 |
|---|------|----------|
| 1 | LCD 启动 → admin/123456 → 主窗口 | 顶部显示组员姓名学号；状态栏显示串口已连接 |
| 2 | 控制台输出 | `HttpServer 已监听端口: 8080` |
| 3 | 入库 → 扫已存在条码（6901234567890） | 自动显示"康师傅红烧牛肉面" |
| 4 | 输入数量 5 → 确认 | 提示"✓ 入库成功" |
| 5 | **入库扫一个未登记的新条码** | **弹"未登记，请联系管理员在 Web 端录入"提示** |
| 6 | 出库扫库存 0 的商品 | 红色，按钮禁用 |
| 7 | 出库数量超过库存 | 弹"库存不足" |
| 8 | 库存查询 → 搜"可乐" | 表格只显示可口可乐 |
| 9 | 操作日志 | 入库绿色，出库红色 |
| 10 | 主机浏览器 `http://实验箱IP:8080/` | Web 端正常加载（蓝色顶栏 + 卡片 + Tab） |
| 11 | LCD 入库一笔 → Web 等 5 秒 | 仪表盘自动更新 |
| 12 | **Web 端商品管理 → 录入步骤 5 那个未知条码** | 提示成功，库存能看到 |
| 13 | **回到 LCD 入库 → 再次扫该条码** | 这次能正常识别 → 入库成功（演示前后端协同） |
| 14 | Web 数据报表 | 库存柱状图 + 7 日趋势 |

## 五、常见问题（基于真实部署中遇到的坑）

**Q1: 启动报 `Cannot mix incompatible Qt library`**
版本不匹配。按步骤 1 重新替换 `/usr/lib/libQt5*.so*` 整套，然后 `ldconfig`。

**Q2: 启动报 `QSqlDatabase: QSQLITE driver not loaded`**
你跑的是**老二进制**——新版用 sqlite3 C API 不依赖 Qt SQL plugin。重新交叉编译并拷到实验箱。
验证方法：`ldd /opt/warehouse/WarehouseQt | grep Qt5Sql` 应该**没有输出**（新版不链接 Qt5Sql）。

**Q3: 启动报 `could not find or load the Qt platform plugin`**
缺 wayland-egl 的 plugin 依赖。检查 `ldd /usr/lib/qt5/plugins/platforms/libqwayland-egl.so | grep "not found"`，把缺的 `libQt5Wayland*.so` 从主机拷过来。步骤 1 已包含这步（`cp -d libQt5*.so*` 会带上 Wayland 相关库）。

**Q4: 启动报"数据库文件不存在"**
要 `cd /opt/warehouse` 后再运行，DB_PATH 是相对路径。**用 `run.sh` 启动可避免此问题**。

**Q5: 串口未连接**
`ls /dev/ttyACM*` 确认设备名。本工程默认 `/dev/ttyACM0`。如果你的扫码枪是其它编号，改 `mainwindow.cpp` 顶部的 `SERIAL_PORT_NAME` 后重新编译。

**Q6: 扫码后 LCD 没反应**
先用 `cat /dev/ttyACM0` 测扫码枪。本程序按 `\r` 或 `\n` 切分条码——绝大多数扫码枪默认带回车结束符。

**Q7: 主机浏览器访问超时**
1. 实验箱 `ifconfig eth0 | grep inet` 看 IP；
2. 主机 `ping 实验箱IP` 看通不通；
3. 实验箱 `netstat -anp | grep 8080` 看端口是否在 LISTEN。

**Q8: Web 端样式丢失（裸 HTML 黑白页面）**
浏览器拉不到 Bootstrap/Chart.js 的 CDN。解决：本工程的 `web/` 目录已经把这 3 个文件**本地化**了（`bootstrap.min.css`、`bootstrap.bundle.min.js`、`chart.umd.min.js`），HTML 引用相对路径。只要按步骤 4 完整拷贝 `web/*`，就不会有这个问题。

**Q9: 启动时控制台有 `could not load cursor 'dnd-move'` / `xkbcommon: ERROR couldn't find a Compose file`**
这是 Wayland 找不到鼠标主题/键盘 compose 文件的**警告**，**不影响程序运行**。可以忽略。

## 六、附加分扩展方向

1. 加微型打印机：出库后自动打印小票
2. 库存预警蜂鸣器（库存 < N 时报警）
3. CSV 报表导出
4. 多操作员并发登录

## 七、组员分工

- **同学 A**：环境搭建、串口驱动、QT 6 个界面、SQLite C API 接入
- **同学 B**：HTTP 服务器、HTML/JS 前端、Chart.js 图表、美化

## 八、技术栈

- **嵌入式**：IMX6Q + Linux 4.9.88 + QT5 (Widgets + SerialPort + Network + Wayland)
- **数据库**：SQLite 3（直接 C API，不走 Qt SQL plugin）
- **Web**：HTML5 + Bootstrap 5 + Chart.js 4
- **通信**：扫码枪 → USB CDC-ACM (`/dev/ttyACM0`) → QT；浏览器 → HTTP/8080 → QTcpServer → SQLite

## 九、架构设计要点（写报告用）

### 9.1 为什么 QT 内置 HTTP 服务器，而不是 boa + Python CGI？

1. **依赖最小化**：实验箱 Yocto 镜像的 Python 2.7 没有 sqlite3 模块；走 CGI 路线需要交叉编译 ARM 版的 sqlite3 命令行或 Python 扩展，工作量大且容易出错。
2. **架构更简单**：HTTP 服务器和数据库读写在**同一进程**，无文件锁/并发竞争问题。
3. **更"嵌入式"**：实验箱真正成为独立的服务端设备。
4. **代码可控**：自己实现的 HTTP 解析器约 250 行 C++，无黑盒。

### 9.2 为什么用 sqlite3 C API 而不是 Qt SQL 模块？

实验箱镜像中的 `libqsqlite.so` plugin 是**老版本（Qt 5.3.2）**，而本工程用 Qt 5.9.4 编译，运行时报"Cannot mix incompatible Qt library"。
解决方案是绕开 Qt SQL plugin，直接调用 `libsqlite3.so` 的 C API（`sqlite3_open`、`sqlite3_prepare_v2`、`sqlite3_step` 等）。这样：

- 不再依赖 `libqsqlite.so` 这个 plugin
- 只需要 `libsqlite3.so.0` 一个共享库（已随 Qt 交叉编译工具链提供）
- 代码更直接，性能略好

### 9.3 LCD 与 Web 的职责分离

- **LCD 端**：仓库现场员工使用，只负责高频的扫码出入库操作。**不允许新增商品**，因为触屏不适合输入中文（商品名/规格通常含中文）。
- **Web 端**：管理者使用，负责商品上架（新增/修改/删除）、库存盘点、报表分析。

这种设计来源于真实仓库管理系统（如 SAP WM、用友 U8）的标准模式：现场员工和管理人员的工作场景、设备、需求不同，应分而治之。