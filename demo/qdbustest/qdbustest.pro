# 根项目使用 subdirs 模板，用于管理子项目
TEMPLATE = subdirs

# 声明子项目（按顺序编译，server 先于 client 编译）
SUBDIRS += \
    server \
    client \
    test

# 可选：指定子项目的输出目录（统一编译到根目录的 build 文件夹）
# 若不指定，默认在各自子目录的 build 中生成可执行文件
server.target = server
server.depends =
client.target = client
client.depends = server  # 客户端依赖服务端（确保服务端先编译）

# 统一输出目录配置（可选，推荐）
# 所有子项目的编译产物（.o、可执行文件等）都放在根目录的 build 下
DESTDIR = $$PWD/build  # 可执行文件输出目录
MOC_DIR = $$DESTDIR/moc  # moc 生成文件目录
OBJECTS_DIR = $$DESTDIR/obj  # 目标文件（.o）目录
UI_DIR = $$DESTDIR/ui  # UI 生成文件目录（此处无用，可省略）

# 将上述目录配置传递给所有子项目
server_DESTDIR = $$DESTDIR
server_MOC_DIR = $$MOC_DIR
server_OBJECTS_DIR = $$OBJECTS_DIR

client_DESTDIR = $$DESTDIR
client_MOC_DIR = $$MOC_DIR
client_OBJECTS_DIR = $$OBJECTS_DIR
