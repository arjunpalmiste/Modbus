#include "modbus.h"
#include <QDebug>

modbus::modbus(int maxnum)
{
    mb_req_len = 0;
    mb_number = 0;
    mb_begin_addr = 0;
    max_coil_num = 0;
    m_base_num = 10;
    m_Value = new int16_t[maxnum]();
    memset(mb_req_msg, 0, MB_REQ_MAX);
    memset(mb_rsp_msg, 0, MB_DATA_MAX);
}
modbus::~modbus()
{
    if(m_Value)
        delete [] m_Value;
}


int modbus::Create_MBAPHead(uint16_t pdu_len)
{
    // 事务标识符，每个请求唯一
    mb_req_msg[0] = mb_number / 256;
    mb_req_msg[1] = mb_number % 256;
    // 协议标识符 0x0000表示modbus
    mb_req_msg[2] = 0x00;
    mb_req_msg[3] = 0x00;
    // 协议后面的字节长度
    mb_req_msg[4] = pdu_len / 256;
    mb_req_msg[5] = pdu_len % 256;
    // 单元标识符，在TCP中无用，一般设为0xFF
    mb_req_msg[6] = 0xff;

    mb_number++;

    return MB_NORMAL;
}

int modbus::Create_Req_Msg(uint8_t func_code, uint16_t start_addr, uint16_t num, int16_t *value)
{
    int ret;

    uint16_t pdu_len = 6;
    mb_begin_addr = start_addr;
    // 功能码
    mb_req_msg[7] = func_code;
    // 起始地址
    mb_req_msg[8] = mb_begin_addr / 256;
    mb_req_msg[9] = mb_begin_addr % 256;

    switch (func_code)
    {
    // 写单个线圈
    case 0x05:
    {
        if (*value == 0)
        {
            *value = 0x00;
        }
        else
        {
            *value = 0xFF00;
        }
        mb_req_msg[10] = *value / 256;
        mb_req_msg[11] = *value % 256;
        break;
    }
    // 写单个寄存器
    case 0x06:
    {
        mb_req_msg[10] = (*value) >> 8;
        mb_req_msg[11] = (*value);
        break;
    }
    // 写多个线圈
    case 0x0F:
    {
        mb_req_msg[10] = num / 256;
        mb_req_msg[11] = num % 256;
        if (num % 8 == 0)
            mb_req_msg[12] = num / 8;
        else
            mb_req_msg[12] = (num / 8) + 1;
        for (uint16_t i = 0; i < mb_req_msg[12]; i += 2)
        {
            mb_req_msg[13 + i] = value[i / 2];
            mb_req_msg[14 + i] = value[i / 2] >> 8;
        }
        pdu_len = 7 + mb_req_msg[12];
        break;
    }
    // 写多个寄存器
    case 0x10:
    {
        mb_req_msg[10] = num / 256;
        mb_req_msg[11] = num % 256;
        mb_req_msg[12] = num * 2;
        for (uint16_t i = 0; i < num * 2; i += 2)
        {
            mb_req_msg[13 + i] = value[i / 2] >> 8;
            mb_req_msg[14 + i] = value[i / 2];
        }
        pdu_len = 7 + num * 2;
        break;
    }
    default:
        // 寄存器数量
        mb_req_msg[10] = num / 256;
        mb_req_msg[11] = num % 256;
        max_coil_num = start_addr + num;
        break;
    }

    ret = Create_MBAPHead(pdu_len);

    mb_req_len = pdu_len + 6;
    return ret;
}

int modbus::mb_exception_rsp(uint8_t err_code)
{
    switch (err_code)
    {
    case 0x01:
        return 1;
//        printf("非法功能码");
    case 0x02:
//        printf("非法数据地址");
        return 2;
    case 0x03:
//        printf("非法数据值");
        return 3;
    case 0x04:
//        printf("服务器故障");
        return 4;
    default:
        break;
    }
    return MB_NORMAL;
}

/*
00 00 00 00 00 06 ff 03 00 00 00 01
00 00 00 00 00 05 ff 03 02 00 0a
*/
int modbus::Param_Response(QListWidget *readValue)
{
    uint8_t func_code = mb_rsp_msg[7];
    uint8_t len, i;
    uint16_t addr;
    if (func_code > 0x80)
    {
        uint8_t err_code = mb_rsp_msg[8];
        return mb_exception_rsp(err_code);
    }
    if(m_base_num == 10)
        readValue->addItem(QString("Addr\tValue"));
    else if(m_base_num == 16)
        readValue->addItem(QString("Addr(hex)\tValue(hex)"));

    switch (func_code)
    {
    // 读线圈
    case 0x01:
    // 读离散量输入
    case 0x02:
    {
        len = mb_rsp_msg[8];
        addr = mb_begin_addr;

        for (i = 0; i < len; i++)
        {
            uint8_t data = mb_rsp_msg[9 + i];
            uint8_t flag = 0x01;
            while (flag)
            {
                if (data & flag)
//                    readValue->addItem(QString("地址：0x%1\t值：1").arg(addr, 2, 16, QLatin1Char('0')));
                    readValue->addItem(QString("%1\t1").arg(addr, 4, m_base_num, QLatin1Char('0')));
                else
//                    readValue->addItem(QString("地址：0x%1\t值：0").arg(addr, 2, 16, QLatin1Char('0')));
                    readValue->addItem(QString("%1\t0").arg(addr, 4, m_base_num, QLatin1Char('0')));
                flag = flag << 1;
                addr++;
                if (addr >= max_coil_num)
                {
                    break;
                }
            }
        }

        break;
    }
    // 读保持寄存器
    case 0x03:
    // 读输入寄存器
    case 0x04:
    {
        len = mb_rsp_msg[8];
        addr = mb_begin_addr;
        int16_t data = 0;

        for (i = 0; i < len; i += 2)
        {
            int16_t data = ((uint8_t)mb_rsp_msg[9 + i] << 8) + (uint8_t)mb_rsp_msg[10 + i];
//            readValue->addItem(QString("地址：0x%1\t值：%2").arg(addr, 2, 16, QLatin1Char('0')).arg(data));
            readValue->addItem(QString("%1\t%2").arg(addr, 4, m_base_num, QLatin1Char('0')).arg(data, 0, m_base_num));
            addr++;
        }
        break;
    }
    // 写单个线圈
    case 0x05:
    {
        addr = mb_rsp_msg[8] * 256 + mb_rsp_msg[9];
        uint16_t data = mb_rsp_msg[10] * 256 + mb_rsp_msg[11];

        if (data == 0xFF00)
        {
//            readValue->addItem(QString("地址：0x%1\t值：1").arg(addr, 2, 16, QLatin1Char('0')));
            readValue->addItem(QString("%1\t1").arg(addr, 4, m_base_num, QLatin1Char('0')));
        }
        else if (data == 0x0000)
        {
//            readValue->addItem(QString("地址：0x%1\t值：0").arg(addr, 2, 16, QLatin1Char('0')));
            readValue->addItem(QString("%1\t0").arg(addr, 4, m_base_num, QLatin1Char('0')));
        }
        break;
    }
    // 写单个寄存器
    case 0x06:
    {
        addr = mb_rsp_msg[8] * 256 + mb_rsp_msg[9] + 1;
        int16_t data = 0;
        data = ((uint8_t)mb_rsp_msg[10] << 8) + (uint8_t)mb_rsp_msg[11];

//        readValue->addItem(QString("地址：0x%1\t值：%2").arg(addr, 2, 16, QLatin1Char('0')).arg(data));
        readValue->addItem(QString("%1\t%2").arg(addr, 4, m_base_num, QLatin1Char('0')).arg(data, 0, m_base_num));
//        printf("地址: 0x%04x 数据: %d\r\n", addr, data);
        break;
    }
    // 写多个线圈
    case 0x0F:
    {
        addr = mb_rsp_msg[8] * 256 + mb_rsp_msg[9];
        uint16_t num = mb_rsp_msg[10] * 256 + mb_rsp_msg[11];
        uint16_t data;
        uint16_t flag = 0x0001;
        uint16_t cnt = 0;

        for (i = 0; i < num; ++i) {
            if (i % 16 == 8)
                flag = 0x0100;
            else if(i != 0 && i % 16 == 0)
            {
                flag = 0x0001;
                cnt++;
            }
            data = m_Value[cnt];
            if (data & flag)
//                readValue->addItem(QString("地址：0x%1\t值：1").arg(addr, 2, 16, QLatin1Char('0')));
                readValue->addItem(QString("%1\t1").arg(addr, 4, m_base_num, QLatin1Char('0')));
            else
//                readValue->addItem(QString("地址：0x%1\t值：0").arg(addr, 2, 16, QLatin1Char('0')));
                readValue->addItem(QString("%1\t0").arg(addr, 4, m_base_num, QLatin1Char('0')));
            flag = flag << 1;
            addr++;
        }
        break;
    }
    // 写多个寄存器
    case 0x10:
    {
        addr = mb_rsp_msg[8] * 256 + mb_rsp_msg[9];
        uint16_t num = mb_rsp_msg[10] * 256 + mb_rsp_msg[11];
        for (i = 0; i < num; ++i) {
//            readValue->addItem(QString("地址：0x%1\t值：%2").arg(addr, 2, 16, QLatin1Char('0')).arg(m_Value[i]));
            readValue->addItem(QString("%1\t%2").arg(addr, 4, m_base_num, QLatin1Char('0')).arg(m_Value[i], 0, m_base_num));
            addr++;
        }
        break;
    }

    default:
        readValue->clear();
//        printf("暂时没有此响应，等待更新....\r\n");
        return MB_ERR;
    }

    return MB_NORMAL;
}
