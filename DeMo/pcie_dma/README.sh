# PCIe DMA Qt Demo 项目说明
## 1. 项目介绍
基于Qt5.9 + C++11 + qmake开发的PCIe DMA交互工具，实现图形化的PCIe/FPGA设备DMA读写操作

## 2. 环境要求
- 操作系统：Linux（Ubuntu/CentOS，仅Linux支持PCIe DMA操作）
- Qt版本：5.9.x（需安装Qt5.9开发包）
- 编译工具：gcc/g++ 4.8+（支持C++11）
- 依赖：libqt5widgets5、libqt5core5a、libqt5gui5

## 3. 编译步骤
```bash
# 1. 安装Qt5.9依赖（以Ubuntu为例）
sudo apt-get install qt5-default qttools5-dev-tools

# 2. 进入项目目录
cd pcie_dma_qt_demo

# 3. 生成Makefile
qmake

# 4. 编译
make -j4

# 5. 运行
./PCIeDMADemo
