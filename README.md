# twemproxy (nutcracker) [![Build Status](https://github.com/twitter/twemproxy/actions/workflows/main.yml/badge.svg?branch=master)](https://github.com/twitter/twemproxy/actions/workflows/main.yml?query=branch%3Amaster)

### 背景 Background information
鉴于官方twemproxy已经很久没更新了,而随着redis版本的不断迭代,新增了越来越多的命令和特性支持,twemproxy不支持的命令和特性越来越多。
因此本人开启这个仓库,用于一些实用命令的支持。

Since the official twemproxy has not been updated for a long time, and with the continuous iteration of the redis version, more and more commands and feature support have been added, more and more commands and features are not supported by twemproxy.
Therefore, I opened this repository and started to support some useful commands of redis.

**能力有限,代码质量不敢保证。**

### 新增已支持命令 NEW SUPPORTED COMMANDS
- [script](https://redis.io/commands/script-load/) :script load/script exists/script flush
- [scan](https://redis.io/commands/scan/)
