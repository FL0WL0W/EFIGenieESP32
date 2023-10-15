#include "EmbeddedIOServiceCollection.h"
#include "Esp32IdfDigitalService.h"
#include "Esp32IdfTimerService.h"
#include "Esp32IdfAnalogService.h"
#include "Esp32IdfPwmService.h"
#include "EFIGenieMain.h"
#include "Variable.h"
#include "CallBack.h"
#include "Config.h"
#include "CommunicationHandler_Prefix.h"
#include "CommunicationHandlers/CommunicationHandler_GetVariable.h"

using namespace OperationArchitecture;
using namespace EmbeddedIOServices;
using namespace EmbeddedIOOperations;
using namespace Esp32;
using namespace EFIGenie;

char *_config = 0; //ESP chips have gobs of ram. just load the config into ram. this will make live tuning easier later on this platform_config
extern const uint8_t tune_start[] asm("_binary_tune_bin_start");
extern const uint8_t tune_end[] asm("_binary_tune_bin_end");
const unsigned int tune_bytes = tune_end - tune_start;

extern "C"
{
  const char ack[1] = {6};
  const char nack[1] = {21};
  GeneratorMap<Variable> *_variableMap;
  uint8_t VariableBuff[9];
  uint32_t Commands[32];
  uint8_t CommandReadPointer = 0;
  const char doneResponseText[10] = " (Done)\n\r";
  const void *_metadata;
  bool secondCommand = false;
  EmbeddedIOServiceCollection _embeddedIOServiceCollection;
//   STM32HalCommunicationService_CDC *_cdcService;
  CommunicationHandler_GetVariable *_getVariableHandler;
  CommunicationHandler_Prefix *_prefixHandler;
  EFIGenieMain *_engineMain;
  Variable *loopTime;
  uint32_t prev;

  void Setup() 
  {
    //ignore the warnings. these are here so we can check the alignment so we can put it in the editor
    volatile size_t uint8_align = alignof(uint8_t);
    volatile size_t uint16_align = alignof(uint16_t);
    volatile size_t uint32_align = alignof(uint32_t);
    volatile size_t uint64_align = alignof(uint64_t);
    volatile size_t bool_align = alignof(bool);
    volatile size_t int8_align = alignof(int8_t);
    volatile size_t int16_align = alignof(int16_t);
    volatile size_t int32_align = alignof(int32_t);
    volatile size_t int64_align = alignof(int64_t);
    volatile size_t float_align = alignof(float);
    volatile size_t double_align = alignof(double);

    _variableMap = new GeneratorMap<Variable>();
    _prefixHandler = new CommunicationHandler_Prefix();
    // _cdcService = new STM32HalCommunicationService_CDC();
    // _cdcService->RegisterHandler();

    _embeddedIOServiceCollection.DigitalService = new Esp32IdfDigitalService();
    _embeddedIOServiceCollection.AnalogService = new Esp32IdfAnalogService(ADC_ATTEN_DB_11);
    // _embeddedIOServiceCollection.TimerService = new Esp32IdfTimerService(TIMER_GROUP_1, TIMER_0);
    // _embeddedIOServiceCollection.PwmService = new Esp32IdfPwmService();

		size_t _configSize = 0;
    _engineMain = new EFIGenieMain(reinterpret_cast<void*>(&_config), _configSize, &_embeddedIOServiceCollection, _variableMap);

    _metadata = Config::OffsetConfig(&_config, *reinterpret_cast<const uint32_t *>(&_config) + 8);
    _getVariableHandler = new CommunicationHandler_GetVariable(_variableMap);
    _prefixHandler->RegisterHandler(_getVariableHandler, "g", 1);
    _prefixHandler->RegisterReceiveCallBack(new communication_receive_callback_t([](communication_send_callback_t send, void *data, size_t length)
    { 
      const size_t minDataSize = sizeof(void *) + sizeof(size_t);
      if(length < minDataSize)
        return static_cast<size_t>(0);

      void *readData = *reinterpret_cast<void **>(data);
      size_t readDataLength = *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(data) + sizeof(void *));
      send(readData, readDataLength);

      return minDataSize;
    }), "r", 1);
    _prefixHandler->RegisterReceiveCallBack(new communication_receive_callback_t([](communication_send_callback_t send, void *data, size_t length)
    { 
      const size_t minDataSize = sizeof(void *) + sizeof(size_t);
      if(length < minDataSize)
        return static_cast<size_t>(0);

      void *writeDataDest = *reinterpret_cast<void **>(data);
      size_t writeDataLength = *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(data) + sizeof(void *));
      void *writeData = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(data) + sizeof(void *) + sizeof(size_t));

      if(length < minDataSize + writeDataLength)
        return static_cast<size_t>(0);

      if(reinterpret_cast<size_t>(writeDataDest) >= 0x20000000 && reinterpret_cast<size_t>(writeDataDest) <= 0x2000FA00)
      {
        std::memcpy(writeDataDest, writeData, writeDataLength);
      }
      else if(reinterpret_cast<size_t>(writeDataDest) >= 0x8004000 && reinterpret_cast<size_t>(writeDataDest) <= 0x8008000)
      {
        //TODO Write to flash
      }

      send(ack, sizeof(ack));

      return minDataSize + writeDataLength;
    }), "w", 1);
    _prefixHandler->RegisterReceiveCallBack(new communication_receive_callback_t([](communication_send_callback_t send, void *data, size_t length)
    { 
      if(_engineMain != 0)
      {
        delete _engineMain;
        _engineMain = 0;
      }
      send(ack, sizeof(ack));
      return static_cast<size_t>(0);
    }), "q", 1, false);
    _prefixHandler->RegisterReceiveCallBack(new communication_receive_callback_t([](communication_send_callback_t send, void *data, size_t length)
    { 
      if(_engineMain == 0)
      {
        size_t configSize = 0;
        _engineMain = new EFIGenieMain(&_config, configSize, &_embeddedIOServiceCollection, _variableMap);
        _metadata = Config::OffsetConfig(&_config, *reinterpret_cast<const uint32_t *>(&_config) + 8);

        _engineMain->Setup();
      }
      send(ack, sizeof(ack));
      return static_cast<size_t>(0);
    }), "s", 1, false);
    _prefixHandler->RegisterReceiveCallBack(new communication_receive_callback_t([](communication_send_callback_t send, void *data, size_t length)
    { 
      if(length < sizeof(uint32_t))//make sure there are enough bytes to process a request
        return static_cast<size_t>(0);
      uint8_t offset = *reinterpret_cast<uint32_t *>(data); //grab offset from data

      send(reinterpret_cast<const uint8_t *>(_metadata) + offset * 64, 64);

      return static_cast<size_t>(sizeof(uint32_t));//return number of bytes handled
    }), "m", 1);
    _prefixHandler->RegisterReceiveCallBack(new communication_receive_callback_t([](communication_send_callback_t send, void *data, size_t length)
    { 
      size_t configLocation[1] = { reinterpret_cast<size_t>(&_config) };
      send(configLocation, sizeof(configLocation));
      
      return static_cast<size_t>(0);
    }), "c", 1, false);

    _engineMain->Setup();
    loopTime = _variableMap->GenerateValue(250);
  }
  void Loop() 
  {
    const tick_t now = _embeddedIOServiceCollection.TimerService->GetTick();
    loopTime->Set((float)(now-prev) / _embeddedIOServiceCollection.TimerService->GetTicksPerSecond());
    prev = now;
    // _cdcService->Flush();

    if(_engineMain != 0)
      _engineMain->Loop();
  }
    void esp_startup_start_app_other_cores(void)
    {
        _config = reinterpret_cast<char *>(malloc(tune_bytes));
        std::memcpy(_config, tune_start, tune_bytes);

        Setup();
        while (1)
        {
            Loop();
        }
    }
    void app_main()
    {
      //rtos app
    }
}