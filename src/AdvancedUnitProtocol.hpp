#pragma once
#include "mbed.h"

typedef enum ASCII
{
    STX = 0x02, //Start of Text
    ETX = 0x03, //End of Text
    ENQ = 0x05, //Enquiry
    ACK = 0x06, //Acknowledge
} ControlCodes;

class AdvancedUnitProtocol
{
private:
    Serial *_PORT;
    Timer *_watchDogTimer;
    int _baudPort, _timeoutUs, _arrayLenght, _incomingDataCounter;
    bool communicatePhase;
    char _buffer[500], _bufferCheckSum = 0;

    bool checkENQ();
    void sendACK();
    bool checkArrayLenght();
    bool saveToBuffer();
    bool confirmCheckSum();
    void applyData(uint8_t *);

    void sendENQ();
    bool checkACK();
    void sendArrayLenght(int);
    void sendData(int, uint8_t *);
    void sendCheckSum(int, uint8_t *);

    bool waitData();

public:
    AdvancedUnitProtocol(PinName uartTX, PinName uartRX, int uartBaud);
    ~AdvancedUnitProtocol();

    int work(int, uint8_t *, uint8_t *);
    void setTimeout(int);
    void setCommunicationPhase(bool);
};

AdvancedUnitProtocol::AdvancedUnitProtocol(PinName uartTX, PinName uartRX, int uartBaud)
{
    _baudPort = uartBaud;
    _PORT = new Serial(uartTX, uartRX, _baudPort);
    _watchDogTimer = new Timer();
    _watchDogTimer->start();
    _timeoutUs = 5000; //set the timeout time to 10000Us
    communicatePhase = 0;
}

AdvancedUnitProtocol::~AdvancedUnitProtocol()
{
}

int AdvancedUnitProtocol::work(int arrayLenght, uint8_t *DataPacket, uint8_t *variableToStore)
{
    switch (communicatePhase)
    {
    case 0:              //receive
        if (!checkENQ()) //ENQ到着確認
        {
            return 0;
        }
        sendACK();               //ACKリスポンス送信
        if (!checkArrayLenght()) //タイムアウトするまで変数長取得
        {
            return 1;
        }
        if (!saveToBuffer()) //バッファにデータを一時保存
        {
            return 2;
        }
        if (!confirmCheckSum())
        {
            return 3;
        }
        applyData(variableToStore);
        communicatePhase = 1;
        return 4;

    case 1: //transmit
        sendENQ();
        if (!checkACK())
        {
            return 5;
        }
        sendArrayLenght(arrayLenght);
        sendData(arrayLenght, DataPacket);
        sendCheckSum(arrayLenght, DataPacket);
        communicatePhase = 0;
        return 6;
    }
    return 7;
}

bool AdvancedUnitProtocol::waitData()
{
    _watchDogTimer->reset();
    while (!_PORT->readable())
    {
        if (_watchDogTimer->read_us() > _timeoutUs) //タイムアウト
        {
            return 0;
        }
    }
    return 1;
}

bool AdvancedUnitProtocol::checkENQ()
{
    if (!waitData())
    {
        return 0;
    }
    if (_PORT->getc() != ENQ)
    {
        return 0;
    }
    return 1;
}

bool AdvancedUnitProtocol::checkACK()
{
    if (!waitData())
    {
        return 0;
    }
    if (_PORT->getc() != ACK)
    {
        return 0;
    }
    return 1;
}

void AdvancedUnitProtocol::sendACK()
{
    _PORT->putc(ACK);
}

void AdvancedUnitProtocol::sendENQ()
{
    _PORT->putc(ENQ);
}

void AdvancedUnitProtocol::sendArrayLenght(int _lenght)
{
    _PORT->putc(_lenght);
}

void AdvancedUnitProtocol::sendData(int _lenght, uint8_t *_sendData)
{
    for (int i = 0; i < _lenght; i++)
    {
        _PORT->putc(_sendData[i]);
    }
}

void AdvancedUnitProtocol::sendCheckSum(int _lenght, uint8_t *_sendData)
{
    for (int i = 0; i < _lenght; i++)
    {
        _bufferCheckSum ^= _sendData[i];
    }
    _PORT->putc(_bufferCheckSum);
}

bool AdvancedUnitProtocol::checkArrayLenght()
{
    if (!waitData())
    {
        return 0;
    }
    _arrayLenght = _PORT->getc(); //変数長取得
    return 1;
}

bool AdvancedUnitProtocol::saveToBuffer()
{
    for (int i = 0; i < _arrayLenght; i++)
    {
        if (!waitData())
        {
            return 0;
        }
        _buffer[_incomingDataCounter++] = _PORT->getc();
    }
    return 1;
}

bool AdvancedUnitProtocol::confirmCheckSum()
{
    if (_arrayLenght != _incomingDataCounter)
    {
        return 0;
    }
    _incomingDataCounter = 0;
    for (int i = 0; i < _arrayLenght; i++)
    {
        _bufferCheckSum ^= _buffer[i];
    }
    if (!waitData())
    {
        return 0;
    }
    if (_PORT->getc() != _bufferCheckSum) //チェックサム比較
    {
        return 0;
    }
    return 1;
}

void AdvancedUnitProtocol::applyData(uint8_t *__variableToStore)
{
    for (int i = 0; i < _arrayLenght; i++)
    {
        __variableToStore[i] = _buffer[i];
    }
}

void AdvancedUnitProtocol::setTimeout(int timeoutTimeInUs)
{
    _timeoutUs = timeoutTimeInUs < 500 ? 500 : timeoutTimeInUs; //minimum = 10ms
}

void AdvancedUnitProtocol::setCommunicationPhase(bool phase)
{
    communicatePhase = phase;
}