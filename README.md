# DeviceLink

基于 Qt 6 的多设备通信与实时数据监控平台，包含**设备模拟器**与**监控客户端**两个可执行程序。

模拟器驱动多台虚拟设备持续产生温度、压力、振动采样，通过 TCP 广播；客户端连接后解析二进制协议，实时展示设备状态与曲线，并将数据落盘到 SQLite。

## 已实现

**通信协议**

- 自定义二进制帧：同步字 `0xA5 0x5A` + 版本 + 类型 + 长度 + 载荷 + CRC-16/MODBUS，统一大端字节序
- 载荷仅使用定长类型，不依赖 Qt 序列化，可被非 Qt 客户端解析
- 流式帧解析器处理粘包、半包与畸形帧；帧头到齐后立刻校验版本、类型与长度是否匹配，不匹配按字节滑动重同步，不为假长度等待整帧

**模拟器**

- 5 台设备并行采样（随机游走模型），可整体或单台启停
- 支持注入异常数据与恢复正常，用于验证客户端报警逻辑
- 表格实时展示各设备状态
- 可选 LinkGate 网关模式：每台设备单独一条 TCP 连接，主动连到网关设备端口（默认 9100）

**客户端**

- 基于 `QAbstractTableModel` 的数据模型配合 `QTableView`，设备按 ID 动态上线建行
- Qt Charts 多设备多指标实时曲线，历史数据统一存于模型层，支持运行时切换观测指标
- 边沿触发的两级阈值报警（Warn / Critical），仅在状态跃迁时记录日志并高亮行
- SQLite 持久化：WAL 日志模式，满 200 条或超 2 秒批量提交
- 按设备与时间范围查询历史数据，并导出 UTF-8 CSV
- 应用层心跳（1 Hz）与看门狗（3 秒超时），断线后按 1→2→4→8→16 秒指数退避重连
- 存储层运行于独立工作线程，磁盘 I/O 不阻塞 UI

**测试**

- Qt Test 编写 40 个单元测试（5 个测试程序），接入 CTest，覆盖逐字节喂入、超长长度字段、类型与长度不匹配、未知类型、CRC 位翻转、前置垃圾数据等边界场景

**跨平台**

- Windows 与 Linux 均已编译通过、全部测试通过，并完成模拟器与客户端联调

| 平台 | Qt | 编译器 |
| --- | --- | --- |
| Windows 10 | 6.11.1 MinGW 64-bit | MinGW-w64 g++ 13.1 |
| CentOS Stream 10 | 6.10.1（系统软件源） | GCC 14.4 |

## 构建

需要 Qt 6.5 及以上、支持 C++17 的编译器、CMake 3.19 及以上，Qt 组件：Core、Gui、Widgets、Network、Charts、Sql、Test。

Linux（CentOS Stream 10）安装依赖：

```bash
sudo dnf install -y qt6-qtbase-devel qt6-qtcharts-devel
```

配置、编译与测试（Windows 上若 Qt 不在默认路径，需额外指定 `-DCMAKE_PREFIX_PATH=<Qt 安装目录>`）：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

产物位于 `build/bin/`，包括 `simulator`、`client` 与测试程序。先运行模拟器并开始监听（默认端口 9000），再运行客户端连接。Linux 下两个图形程序需在桌面会话的终端中运行。

客户端数据库位置：Windows 为 `%APPDATA%\client\devicelink.db`，Linux 为 `~/.local/share/client/devicelink.db`。

大规模接入时，设备与客户端都连接 LinkGate 网关，连接数从 N×M 降为 N+M。模拟器通过「连接网关」接入。

## 目录结构

```
common/     两端共用：数据结构、协议编解码、帧解析、报警判定（静态库）
simulator/  设备模拟器（含 LinkGate 上行）
client/     监控客户端
tests/      单元测试（40 个用例，5 个测试程序）
tools/      golden_dump：按当前协议导出参考帧
```
