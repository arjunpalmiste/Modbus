#ifndef MODBUS_H
#define MODBUS_H

#include <QtGlobal>
#include <cstring>
#include <QListWidget>

#define MB_REQ_MAX 1024
#define MB_DATA_MAX 1024

typedef enum MBError
{
    MB_NORMAL = 0,
    MB_ERR = -1
} MBError;


class modbus
{
public:
    modbus(int maxnum);
    ~modbus();

    char mb_req_msg[MB_REQ_MAX];
    char mb_rsp_msg[MB_DATA_MAX];

    uint16_t mb_req_len;
    uint16_t mb_number;
    uint16_t mb_begin_addr;
    uint16_t max_coil_num;
    int16_t *m_Value;
    int m_base_num;

    int Create_Req_Msg(uint8_t func_code, uint16_t start_addr = 0, uint16_t num = 1, int16_t *value = NULL);
    int Param_Response(QListWidget *readValue);
private:
    int Create_MBAPHead(uint16_t pdu_len);
    int mb_exception_rsp(uint8_t err_code);
};


//uint16_t get_buf_len(char *buf, uint16_t size);
//void Print_Msg(uint8_t *buf, uint16_t len);



#endif // MODBUS_H
