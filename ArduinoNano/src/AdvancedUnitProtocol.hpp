#pragma once
#include "Arduino.h"
#include "HardwareSerial.h"
#include "SoftwareSerial.h"

typedef enum ASCII
{
    STX = 0x02, //Start of Text
    ETX = 0x03, //End of Text
    ENQ = 0x05, //Enquiry
    ACK = 0x06, //Acknowledge
    CAN = 0x18, //Cancel
} ControlCodes;

class AdvancedUnitProtocol
{
private:
    HardwareSerial *_PORTH;
    SoftwareSerial *_PORTS;
    uint64_t _watchDogTime1, _watchDogTime2;
    long _baudPort, _timeoutUs, _arrayLenght, _incomingDataCounter;
    int communicatePhase, isSoftwareStream;
    uint8_t _buffer[500], _bufferCheckSum = 0;

    int checkENQorCAN();
    void sendACK();
    bool checkArrayLenght();
    bool saveToBuffer();
    bool confirmCheckSum();
    void applyData(uint8_t *);

    void sendCAN();
    void sendENQ();
    bool checkACK();
    void sendArrayLenght(int);
    void sendData(int, uint8_t *);
    void sendCheckSum(int, uint8_t *);

    bool waitData();

public:
    AdvancedUnitProtocol(int rx, int tx, long uartBaud);
    AdvancedUnitProtocol(HardwareSerial &port, long uartBaud);
    ~AdvancedUnitProtocol();

    int work(int, uint8_t *, uint8_t *);
    void setTimeout(int);
    void setCommunicationPhase(bool);
};

AdvancedUnitProtocol::AdvancedUnitProtocol(int rx, int tx, long uartBaud)
{
    _baudPort = uartBaud;
    _PORTS = new SoftwareSerial(rx, tx);
    _PORTS->begin(uartBaud);
    _timeoutUs = 5000; //set the timeout time to 5000Us
    communicatePhase = 0;
    isSoftwareStream = 1;
    _PORTS->listen();
}

AdvancedUnitProtocol::AdvancedUnitProtocol(HardwareSerial &port, long uartBaud)
{
    _baudPort = uartBaud;
    _PORTH = &port;
    _PORTH->begin(uartBaud);
    _timeoutUs = 5000; //set the timeout time to 5000Us
    communicatePhase = 0;
    isSoftwareStream = 0;
}

AdvancedUnitProtocol::~AdvancedUnitProtocol()
{
}

int AdvancedUnitProtocol::work(int arrayLenght, uint8_t *DataPacket, uint8_t *variableToStore)
{
    if ((millis() - _watchDogTime2) > 10)
    {
        communicatePhase = !communicatePhase;
        _watchDogTime2 = millis();
    }
    switch (communicatePhase)
    {
    case 0: //receive
        uint8_t comState;
        comState = checkENQorCAN();
        if (!comState) //ENQorCAN到着確認
        {
            return 0;
        }
        if (comState == 2) //相手側マイコン転送キャンセル信号検知
        {
            communicatePhase = 1;
            return 8;
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
        _watchDogTime2 = millis();
        return 4;

    case 1:               //transmit
        if (!arrayLenght) //no data send
        {
            sendCAN();
            communicatePhase = 0;
            return 9;
        }
        sendENQ();
        if (!checkACK())
        {
            return 5;
        }
        sendArrayLenght(arrayLenght);
        sendData(arrayLenght, DataPacket);
        sendCheckSum(arrayLenght, DataPacket);
        communicatePhase = 0;
        _watchDogTime2 = millis();
        return 6;
    }
    return 7;
}

bool AdvancedUnitProtocol::waitData()
{
    if (isSoftwareStream)
    {
        _PORTS->listen();
        _watchDogTime1 = micros();
        while (!_PORTS->available())
        {
            if ((micros() - _watchDogTime1) > _timeoutUs) //タイムアウト
            {
                return 0;
            }
        }
        return 1;
    }

    _watchDogTime1 = micros();
    while (!_PORTH->available())
    {
        if ((micros() - _watchDogTime1) > _timeoutUs) //タイムアウト
        {
            return 0;
        }
    }
    return 1;
}

int AdvancedUnitProtocol::checkENQorCAN()
{
    if (isSoftwareStream)
    {
        if (!waitData())
        {
            return 0;
        }
        uint8_t data = _PORTS->read();
        if (data != ENQ && data != ControlCodes::CAN)
        {
            return 0;
        }
        if (data == ControlCodes::CAN)
        {
            return 2;
        }
        return 1;
    }

    if (!waitData())
    {
        return 0;
    }
    uint8_t data = _PORTH->read();
    if (data != ENQ && data != ControlCodes::CAN)
    {
        return 0;
    }
    if (data == ControlCodes::CAN)
    {
        return 2;
    }
    return 1;
}

bool AdvancedUnitProtocol::checkACK()
{
    if (isSoftwareStream)
    {
        if (!waitData())
        {
            return 0;
        }
        if (_PORTS->read() != ACK)
        {
            return 0;
        }
        return 1;
    }

    if (!waitData())
    {
        return 0;
    }
    if (_PORTH->read() != ACK)
    {
        return 0;
    }
    return 1;
}

void AdvancedUnitProtocol::sendACK()
{
    if (isSoftwareStream)
        _PORTS->write(ACK);

    else
        _PORTH->write(ACK);
}

void AdvancedUnitProtocol::sendCAN()
{
    if (isSoftwareStream)
        _PORTS->write(ControlCodes::CAN);
    else
        _PORTH->write(ControlCodes::CAN);
}

void AdvancedUnitProtocol::sendENQ()
{
    if (isSoftwareStream)
        _PORTS->write(ENQ);
    else
        _PORTH->write(ENQ);
}

void AdvancedUnitProtocol::sendArrayLenght(int _lenght)
{
    if (isSoftwareStream)
        _PORTS->write(_lenght);
    else
        _PORTH->write(_lenght);
}

void AdvancedUnitProtocol::sendData(int _lenght, uint8_t *_sendData)
{
    for (int i = 0; i < _lenght; i++)
    {
        if (isSoftwareStream)
            _PORTS->write(_sendData[i]);
        else
            _PORTH->write(_sendData[i]);
    }
}

void AdvancedUnitProtocol::sendCheckSum(int _lenght, uint8_t *_sendData)
{
    _bufferCheckSum = 0;
    for (int i = 0; i < _lenght; i++)
    {
        //printf("TRANSMITTED DATA [%d], [%d]\r\n", i, _sendData[i]);
        _bufferCheckSum ^= _sendData[i];
    }
    //printf("TRANSMITTED CHECKSUM [%d]\r\n", _bufferCheckSum);

    if (isSoftwareStream)
        _PORTS->write(_bufferCheckSum);
    else
        _PORTH->write(_bufferCheckSum);
}

bool AdvancedUnitProtocol::checkArrayLenght()
{
    if (!waitData())
    {
        return 0;
    }
    if (isSoftwareStream)
        _arrayLenght = _PORTS->read(); //変数長取得
    else
        _arrayLenght = _PORTH->read(); //変数長取得
    return 1;
}

bool AdvancedUnitProtocol::saveToBuffer()
{
    _incomingDataCounter = 0;
    for (int i = 0; i < _arrayLenght; i++)
    {
        if (!waitData())
        {
            return 0;
        }
        if (isSoftwareStream)
            _buffer[_incomingDataCounter++] = _PORTS->read();
        else
            _buffer[_incomingDataCounter++] = _PORTH->read();
    }
    return 1;
}

bool AdvancedUnitProtocol::confirmCheckSum()
{
    if (_arrayLenght != _incomingDataCounter)
    {
        //printf("LENGHT ERROR [%d], [%d]", _arrayLenght, _incomingDataCounter);
        return 0;
    }
    //printf("GOT PACKET LENGHT [%d]\r\n", _arrayLenght);
    _bufferCheckSum = 0;
    for (int i = 0; i < _arrayLenght; i++)
    {
        //printf("RECEIVED DATA [%d], [%d]\r\n", i, _buffer[i]);
        _bufferCheckSum ^= _buffer[i];
    }
    if (!waitData())
    {
        //printf("TIMEOUT WHILE DATA RECEIVING\r\n");
        return 0;
    }
    uint8_t check;
    if (isSoftwareStream)
        check = _PORTS->read();
    else
        check = _PORTH->read();
    if (check != _bufferCheckSum) //チェックサム比較
    {
        //printf("CHECKSUM INCORRECT [%d], [%d]\r\n", check, _bufferCheckSum);
        return 0;
    }
    //printf("CHECKSUM CORRENT [%d], [%d]\r\n", check, _bufferCheckSum);
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
    _timeoutUs = timeoutTimeInUs < 5000 ? 5000 : timeoutTimeInUs; //minimum = 5ms
}

void AdvancedUnitProtocol::setCommunicationPhase(bool phase)
{
    communicatePhase = phase;
}