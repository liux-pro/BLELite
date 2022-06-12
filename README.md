# BLELite
CH573或CH571蓝牙简单通信模块  
模块自身设定为蓝牙从设备，默认名字为BLELite，只有一个Service，内含一个Characteristic，两者uuid均为`0xFF`  
主机通过notify和write这个Characteristic，透传芯片上的串口。  
同时监听附近的蓝牙广播，如果收到厂商id为`0xBABE`的广播，把其内容也透传到串口。

# PIN
使用串口（115200）通讯

| PIN | CH571K或CH573K | 
| :----: | :----: | 
| TXD |  PA14| 
| RXD | PA15 | 

# 通讯协议
通过串口（UART）进行通讯，波特率115200  
实际数据包最后附带一个异或校验字节（BCC）  
模块输出数据也将以此规则输出。  
发送时，两个数据包之间间隔至少1ms  

例如发送AT+RUOK检查模块是否正常运行，模块返回AT+OK  
串口实际通讯为  
发送 41 54 2b 52 55 4f 4b 3d  
接收 41 54 2B 4F 4B 3A   
# AT指令
如果发送其他数据均会透传到notify
| 功能| 发送 | 回复 | 例 | 
| :----: | :----: | :----: | :----: | 
| 检查是否正常运行 | AT+RUOK |  AT+OK|  | 
| 修改蓝牙名字| AT+SNAME | AT+OK | 把蓝牙名字设置为abc：`AT+SNAMEabc` | 
| 设置蓝牙广播| AT+SADV | AT+OK |  | 
