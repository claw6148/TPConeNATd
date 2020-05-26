# TPConeNATd
基于TPROXY的用户模式Full-cone NAT实现

## 用法

```
-h Help
-p TPROXY port
-i Minimum port (default: 10240)
-x Maximum port (default: 65535)
-s NAT ip (default: 0.0.0.0, depends on system)
-n NEW timeout (default: 30)
-e ESTABLISHED timeout (default: 300)
-o Session limit per source ip (default: 65535, unlimited)
-t NAT type, 1. full-cone, 2. restricted-cone, 3. port-restricted-cone (default: 1)
-c Clean interval (default: 10)
-f PID file
-d Run as daemon
```

## 配置示例

### 基本配置

```
#!/bin/sh

NAT_IP=外部地址
INT_IF=内部接口
TP_PORT=TPROXY端口

ip rule add fwmark 1 lookup 100
ip route add local 0/0 dev lo table 100

iptables -t mangle -A PREROUTING -i ${INT_IF} ! -d ${NAT_IP} -p udp -j TPROXY --on-port ${TP_PORT} --tproxy-mark 1

tpconenatd -p ${TP_PORT} -s ${NAT_IP}
```

### 进阶配置1 限制每个内部地址的最大端口数

为防止外部端口被单个内部地址耗尽，可追加参数`-o ?`以限制每个内部地址的最大端口数。

以端口范围[1024, 65535]，每个内部地址可**同时**使用1024个端口为例：

单个外部地址可为(65535-1024+1)/1024=63个内部地址提供Full-cone NAT服务。

### 进阶配置2 与原生NAT共存

对于DNS查询（目的端口53）之类不需要使用Full-cone NAT的业务可交由原生NAT处理，以节省Full-cone NAT端口资源。

划分端口范围：

- **目的端口**在[1, 1023]范围内的连接交由原生NAT处理，并限制NAT**源端口**在[1024, 10239]范围内
- **目的端口**在[1024, 65535]范围内的连接交由TPConeNATd处理，并限制NAT**源端口**在[10240, 65535]范围内

配置如下：

```
#!/bin/sh

NAT_IP=外部地址
INT_IF=内部接口
EXT_IF=外部接口
TP_PORT=TPROXY端口

ip rule add fwmark 1 lookup 100
ip route add local 0/0 dev lo table 100

iptables -t mangle -A PREROUTING -i ${INT_IF} ! -d ${NAT_IP} -p udp --dport 1024:65535 -j TPROXY --on-port ${TP_PORT} --tproxy-mark 1
iptables -t nat -A POSTROUTING -o ${EXT_IF} -p udp -j SNAT --to ${NAT_IP}:1024-10239

tpconenatd -p ${TP_PORT} -s ${NAT_IP} -i 10240 -x 65535
```

## TODO

- 透传ToS, TTL
