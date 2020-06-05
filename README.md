# TPConeNATd
基于TPROXY的用户模式Cone NAT实现

![](https://raw.githubusercontent.com/claw6148/TPConeNATd/master/screenshot.png)

## 功能

- UDP NAT
- NAT 环回
- ICMP ALG （路由追踪）

## 用法

```
-h Help
-p TPROXY port
-i Minimum port (default: 10240)
-x Maximum port (default: 65535)
-s NAT ip (default: 0.0.0.0, depends on system)
-n NEW(no reply) timeout (default: 30)
-e ESTABLISHED timeout (default: 300)
-o Port limit per source ip (default: 65535, unlimited)
-t NAT type, 1. full-cone, 2. restricted-cone, 3. port-restricted-cone (default: 1)
-r Sender thread (default: 1)
-f PID file
-l Log level (default: 6. INFO)
-d Run as daemon, log to syslog
```

## 配置

### 基本配置

```
#!/bin/sh

NAT_IP=外部地址
INT_IF=内部接口
INT_NET=内部地址范围
TP_PORT=TPROXY端口

ip rule add fwmark 1 lookup 100
ip route add local 0/0 dev lo table 100

iptables -t mangle -A PREROUTING -i ${INT_IF} -s ${INT_NET} -p udp -j TPROXY --on-port ${TP_PORT} --tproxy-mark 1

TPConeNATd -p ${TP_PORT} -s ${NAT_IP} -d
```

### 进阶配置1 限制每个内部地址的最大端口数

为防止外部端口被单个内部地址耗尽，可追加参数`-o ?`以限制每个内部地址的最大端口数。

以端口范围[1024, 65535]，每个内部地址可**同时**使用1024个端口为例：

单个外部地址可为`(65535-1024+1)/1024=63`个内部地址提供Cone NAT服务。

### 进阶配置2 与原生NAT共存

对于DNS查询（目的端口53）之类不需要使用Cone NAT的业务可交由原生NAT处理，以节省端口资源。

划分端口范围：

- **目的端口**在[0, 1023]范围内的连接交由原生NAT处理，并限制NAT**源端口**在[1024, 4095]范围内
- **目的端口**在[1024, 65535]范围内的连接交由TPConeNATd处理，并限制NAT**源端口**在[4096, 65535]范围内

配置如下：

```
#!/bin/sh

EXT_IF=外部接口
NAT_IP=外部地址
INT_IF=内部接口
INT_NET=内部地址范围
TP_PORT=TPROXY端口

ip rule add fwmark 1 lookup 100
ip route add local 0/0 dev lo table 100

iptables -t mangle -A PREROUTING -i ${INT_IF} -s ${INT_NET} -p udp --dport 1024:65535 -j TPROXY --on-port ${TP_PORT} --tproxy-mark 1
iptables -t nat -I POSTROUTING -o ${EXT_IF} -s ${INT_NET} -p udp --dport 0:1023 -j SNAT --to ${NAT_IP}:1024-4095

TPConeNATd -p ${TP_PORT} -s ${NAT_IP} -i 4096 -x 65535 -d
```

## 性能

Xeon E5-2670 / Debian 9

- 单线程 `-r 1`

载荷长度 | SNAT Mbps | SNAT Kpps | DNAT Mbps | DNAT Kpps
-|-|-|-|-
12 | 6.51 | 67.83 | 6.95 | 72.37
730 | 404 | 69.18 | 417 | 71.40
1472 | 827 | 70.36 | 846 | 71.95

- 多线程 `-r 16`

载荷长度 | SNAT Mbps | SNAT Kpps | DNAT Mbps | DNAT Kpps
-|-|-|-|-
12 | 19.6 | 203.89 | 19.5 | 203.62
730 | 1180 | 202.25 | 1220 | 209.34
1472 | 2390 | 203.23 |  2440 | 207.73

## 建议

1. 调大最大fd数
2. 将NAT类型设为2可兼顾安全性和互通性。

&nbsp; | NAT-1 | NAT-2 | NAT-3 | NAT-4
-|-|-|-|-
**NAT-1** | &#10003; | &#10003; | &#10003; | &#10003;
**NAT-2** | &#10003; | &#10003; | &#10003; | &#10003;
**NAT-3** | &#10003; | &#10003; | &#10003; | &#10005;
**NAT-4** | &#10003; | &#10003; | &#10005; | &#10005;
