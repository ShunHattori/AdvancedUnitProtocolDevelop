#include <mbed.h>
#include "AdvancedUnitProtocol.hpp"

#define TX PA_9
#define RX PA_10

//#define IsControllerTransmitter

//Serial com(TX, RX, 9600);
Serial PC(USBTX, USBRX, 256000);

AdvancedUnitProtocol cont(TX, RX, 256000);

int main()
{

  /*
  *  TransmitterCodes below
  */
#ifdef IsControllerTransmitter
  {
    static uint8_t Tdata[256], Rdata[256];
    Tdata[0] = 32;
    Tdata[1] = 85;
    Tdata[2] = 109;
    Tdata[3] = 75;
    Tdata[4] = 49;
    cont.setCommunicationPhase(0);
    while (1)
    {
      Tdata[0]++;
      PC.printf("%d\t%d\t%d\t%d\t%d\t%d\r\n",
                cont.work(5, Tdata, Rdata),
                Rdata[0], Rdata[1], Rdata[2],
                Rdata[3], Rdata[4]);
      //wait_ms(5);
    }
  }
#endif

  /*
  *  ReceiverCodes below
  */
  {
    static uint8_t Tdata[256], Rdata[256];
    Tdata[0] = 84;
    Tdata[1] = 205;
    Tdata[2] = 48;
    Tdata[3] = 59;
    Tdata[4] = 12;
    cont.setCommunicationPhase(1);
    while (1)
    {
      Tdata[0]++;
      PC.printf("%d\t%d\t%d\t%d\t%d\t%d\r\n",
                cont.work(5, Tdata, Rdata),
                Rdata[0], Rdata[1], Rdata[2],
                Rdata[3], Rdata[4]);
      //wait_ms(5);
    }
  }
}
