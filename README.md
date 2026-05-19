# SystemStatusTool

基于 ROS2 和 Qt5 的实时系统监控工具，通过自定义 ROS2 消息采集并可视化主机 CPU、内存、网络等关键运行状态。

![界面截图](./image/system_status_tool.png)

## 功能特性

- 实时采集：独立 ROS2 节点定期读取 `/proc/stat`、`/proc/meminfo`、`/proc/net/dev` 等系统文件，发布状态消息
- 自定义消息：`SystemStatus.msg` 包含时间戳、主机名、CPU/内存使用率、内存总量/可用、网络收发累计字节数
- Qt 图形界面：订阅状态消息，动态显示数值、进度条，支持深色主题，UI 更新在主线程中安全执行
- 非侵入式：基于 ROS2 话题通信，不影响被监控系统的原有逻辑
- 低延迟：采集频率可调（默认 20 Hz），端到端延迟 < 50 ms

## 消息定义

`msg/SystemStatus.msg`

```msg
builtin_interfaces/Time timestamp
string host_name
float32 cpu_percent
float32 memory_percent
float32 memory_total      # 单位 MB
float32 memory_available  # 单位 MB
float64 net_sent          # 累计发送字节数
float64 net_recv          # 累计接收字节数