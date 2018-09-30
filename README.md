SIM_COM
====

使用SIMCOM公司通信模块已经好几年了，周末抽空把底层重新写了，将底层的通信与应用完全进行了分离，便于移植。
-------

* SIMCOM.h //定义了相关的结构体与类型。
* SIMCOM_AT.c//定义了底层的AT接口
* SIMCOM_GSM.c//需要的模块GSM相关命令
* SIMCOM_GPRS.c//上网相关-未移植
* SIMCOM_SMS.c//短信收发相关-未移植
* SIMCOM_USER.c//用户最终接口
* 需要自己实现数据收发相关接口，DCD,DTR,PWRKEY,STATUS相关IO接口，需要一个ms延时支持


---------------------

本文来自 cp1300 的CSDN 博客 ，全文地址请点击：[SIM800/SIM900/SIM7000/SIM7600底层操作接口_句柄方式完全分离通信底层](https://blog.csdn.net/cp1300/article/details/79686460?utm_source=copy) 