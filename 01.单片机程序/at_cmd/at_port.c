#include "at_port.h"
#include "at_cmd.h"
#include "at.h"
#include "sx1278.h"
#define at_cmdLenMax 256

at_stateType  at_state = at_statIdle;
AtMode_t at_mode ;

 uint8_t at_cmdLine[at_cmdLenMax];

 
 
 uint8_t Transport_counter = 0;
 uint32_t Transport_last_time = 0;
 uint32_t Transport_sending_note_flag = 0;
 uint8_t Transport_exit_flag_pre = 0;
 
 
void at_recv_event(char temp)
{
    static uint8_t atHead[3];
    static uint8_t *pCmdLine;
    static uint16_t count = 0;
    switch(at_state)
    {
    case at_statIdle:
        atHead[0] = atHead[1];
        atHead[1] = temp;
        if((memcmp(atHead, "AT", 2) == 0) || (memcmp(atHead, "at", 2) == 0))
        {
            at_state = at_statRecving;
            pCmdLine = at_cmdLine;
            atHead[1] = 0x00;
        }
      break;
    case at_statRecving: //push receive data to cmd line
      *pCmdLine = temp;
      if(temp == '\n')
      {
        pCmdLine++;
        *pCmdLine = '\0';
        at_state = at_statProcess;

      }
      else if(pCmdLine >= &at_cmdLine[at_cmdLenMax - 1])
      {
        at_state = at_statIdle;
      }
      pCmdLine++;
      break;      
    case at_statProcess: //process data
      if(temp == '\n')
      {
//      system_os_post(at_busyTaskPrio, 0, 1);
        at_back(AT_ERR_CPU_BUSY_ID);
      }
      break;
      
      
    case at_statTranInit://no break;
        pCmdLine = at_cmdLine;
        at_state = at_statIpTraning;
    case at_statIpTraning:
      if(count  < (LoRaPacket.len - 4) )
      {
        *pCmdLine = temp;
        pCmdLine++;
        count++;
      }
      
      if(count >= (LoRaPacket.len - 4))
      {
            LoRaPacket.source.val = LoRaAddr;
            LoRaPacket.destination.val = DestAddr;
            LoRaPacket.data = (uint8_t *)at_cmdLine;
            SX1278SetTxPacket(&LoRaPacket);
            at_state = at_statIdle;
            count = 0;
            uart1_write_string("AT,SENDING\r\n");
      }
      break;
    case at_statTransportIdle://no break,进入透传后的初始化
        pCmdLine = at_cmdLine;
        at_state = at_statTransportRecv;
        Transport_counter = 0;
        Transport_sending_note_flag = 0;
    case at_statTransportRecv:
            *pCmdLine++ = temp;
            Transport_counter++;
            if(Transport_counter < 3)
            {
              atHead[0] = atHead[1];
              atHead[1] = atHead[2];
              atHead[2] = temp;
              Transport_last_time = millis();  
              if(memcmp(atHead, "+++", 3) == 0)
              {

                Transport_exit_flag_pre = 1;
                atHead[2] = 0x00;
             }          
            }
      break;

    case at_statTransportSending: 
        if(Transport_sending_note_flag == 0)
        {
          uart1_write_string("AT,lora sending...\r\n");
          Transport_sending_note_flag = 1;
        }
        break;
      /*
    case at_statIpSended: //send data
    if(temp == '\n')
    {
        //system_os_post(at_busyTaskPrio, 0, 2);
        uart1_write_string("busy s...\r\n");
    }
      break;*/     
    }
}
void at_process_loop()
{
    if(at_state == at_statProcess)
    {
        at_cmdProcess(at_cmdLine);
    }
    else if(at_state == at_statTransportRecv)
    {
      if(millis() - Transport_last_time > 5)
      {
            if(Transport_counter == 3)
            {
                at_state = at_statIdle;
                at_mode = AtModeCMD;
                uart1_write_string("EXIT\r\n");
                return ;
            }

            LoRaPacket.len = Transport_counter + 4;
            LoRaPacket.source.val = LoRaAddr;
            LoRaPacket.destination.val = DestAddr;
            LoRaPacket.data = (uint8_t *)at_cmdLine;
            SX1278SetTxPacket(&LoRaPacket);
            at_state = at_statTransportSending;
            Transport_counter = 0;
            
      }
       // SX1278SetTxPacket(at_dataLine,123);//UartDev.rcv_buff.pRcvMsgBuff);
    }
    else if(at_state == at_statTransportSending)
    {
        //at_ipDataSendNow();//UartDev.rcv_buff.pRcvMsgBuff);
    }
}