-- ============================================================
-- 仓库管理系统 数据库初始化脚本
-- 使用方法：sqlite3 warehouse.db < init_db.sql
-- ============================================================

-- 删除旧表（重复执行也不出错）
DROP TABLE IF EXISTS operations;
DROP TABLE IF EXISTS products;
DROP TABLE IF EXISTS users;

-- ----------------------------------------
-- 1. 用户表：登录用，简单两个角色
-- ----------------------------------------
CREATE TABLE users (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    username  TEXT    NOT NULL UNIQUE,
    password  TEXT    NOT NULL,
    role      TEXT    NOT NULL DEFAULT 'staff',   -- 'admin' / 'staff'
    realname  TEXT
);

-- ----------------------------------------
-- 2. 商品表：条码作为业务主键
-- ----------------------------------------
CREATE TABLE products (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    barcode     TEXT    NOT NULL UNIQUE,         -- 条码字符串
    name        TEXT    NOT NULL,                -- 商品名称
    spec        TEXT,                            -- 规格（如 500ml、12寸）
    price       REAL    NOT NULL DEFAULT 0,      -- 单价
    stock       INTEGER NOT NULL DEFAULT 0,      -- 当前库存
    created_at  TEXT    DEFAULT (datetime('now','localtime'))
);

-- ----------------------------------------
-- 3. 出入库流水表
-- ----------------------------------------
CREATE TABLE operations (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    op_type     TEXT    NOT NULL,                -- 'in' 入库 / 'out' 出库
    product_id  INTEGER NOT NULL,
    barcode     TEXT    NOT NULL,                -- 冗余存一份，方便查询
    quantity    INTEGER NOT NULL,
    operator    TEXT,                            -- 操作员用户名
    op_time     TEXT    DEFAULT (datetime('now','localtime')),
    remark      TEXT,
    FOREIGN KEY (product_id) REFERENCES products(id)
);

-- 索引：按时间倒序查日志会很快
CREATE INDEX idx_op_time ON operations(op_time DESC);
CREATE INDEX idx_op_barcode ON operations(barcode);

-- ============================================================
-- 初始测试数据
-- ============================================================

-- 用户：一个管理员、两个员工（密码就用明文了，嵌入式实验场景够用）
INSERT INTO users (username, password, role, realname) VALUES
    ('admin',   '123456', 'admin', '管理员'),
    ('zhang',   '111',    'staff', '张三'),
    ('li',      '111',    'staff', '李四');

-- 商品：10个常见商品，条码用真实EAN-13格式，方便手机生成测试码
INSERT INTO products (barcode, name, spec, price, stock) VALUES
    ('6901234567890', '康师傅红烧牛肉面',  '袋装120g',  3.5,  100),
    ('6901234567891', '农夫山泉饮用水',    '550ml',     2.0,  200),
    ('6901234567892', '可口可乐',          '听装330ml', 3.0,  150),
    ('6901234567893', '伊利纯牛奶',        '盒装250ml', 4.5,  80),
    ('6901234567894', '奥利奥饼干',        '原味116g',  8.5,  60),
    ('6901234567895', '老干妈风味豆豉',    '瓶装280g',  12.0, 50),
    ('6901234567896', '维达抽纸',          '3层120抽',  9.9,  40),
    ('6901234567897', '海飞丝洗发水',      '400ml',     38.5, 30),
    ('6901234567898', '中华牙膏',          '180g',      15.5, 70),
    ('6901234567899', '雕牌洗衣液',        '2kg',       29.9, 25);

-- 几条历史流水（方便Web端首次打开有内容看）
INSERT INTO operations (op_type, product_id, barcode, quantity, operator, remark) VALUES
    ('in',  1, '6901234567890', 50, 'admin', '初始入库'),
    ('in',  2, '6901234567891', 100,'admin', '初始入库'),
    ('out', 1, '6901234567890', 10, 'zhang', '门店调拨'),
    ('out', 2, '6901234567891', 20, 'li',    '损耗');

-- 显示初始化结果
SELECT '=== 用户数 ==='  AS info;  SELECT COUNT(*) FROM users;
SELECT '=== 商品数 ==='  AS info;  SELECT COUNT(*) FROM products;
SELECT '=== 流水数 ==='  AS info;  SELECT COUNT(*) FROM operations;
