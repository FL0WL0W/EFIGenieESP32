#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "EmbeddedIOServiceCollection.h"
#include "Esp32IdfDigitalService.h"
#include "Esp32IdfTimerService.h"
#include "Esp32IdfAnalogService.h"
#include "Esp32IdfPwmService.h"
#include "EngineMain.h"
#include "Variable.h"
#include "CallBack.h"

using namespace OperationArchitecture;
using namespace EmbeddedIOServices;
using namespace Esp32;
using namespace Engine;

char _config[16384]; //ESP chips have gobs of ram. just load the config into ram. this will make live tuning easier later on this platform

extern "C"
{
    uint32_t Commands[32] = {0};
    uint8_t CommandReadPointer = 0;
    bool secondCommand = false;
    EmbeddedIOServiceCollection _embeddedIOServiceCollection;
    EngineMain *_engineMain;
    Variable *loopTime;
    uint32_t prev;

    bool val = true;
    void test() {
    _embeddedIOServiceCollection.DigitalService->WritePin(2, val);
        val = !val;
    }
    Task t = Task(&test);

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

        printf( "Initializing EmbeddedIOServices\n\r");
        _embeddedIOServiceCollection.DigitalService = new Esp32IdfDigitalService();
        // _embeddedIOServiceCollection.AnalogService = new Esp32IdfAnalogService();
        _embeddedIOServiceCollection.TimerService = new Esp32IdfTimerService(TIMER_GROUP_0, TIMER_0);
        // _embeddedIOServiceCollection.PwmService = new Esp32IdfPwmService();
        printf( "EmbeddedIOServices Initialized\n\r");

        _embeddedIOServiceCollection.DigitalService->InitPin(2, Out);
        _embeddedIOServiceCollection.TimerService->ScheduleTask(&t, _embeddedIOServiceCollection.TimerService->GetTick() + _embeddedIOServiceCollection.TimerService->GetTicksPerSecond());

        // printf( "Initializing EngineMain\n\r");
        // unsigned int _configSize = 0;
        // _engineMain = new EngineMain(reinterpret_cast<void*>(&_config), _configSize, &_embeddedIOServiceCollection);
        // printf( "EngineMain Initialized\n\r");
        // test();

        // printf( "Setting Up EngineMain\n\r");
        // _engineMain->Setup();
        // printf( "EngineMain Setup\n\r");
        // loopTime = _engineMain->SystemBus->GetOrCreateVariable(250);
    }
    void Loop() 
    {
        if(!t.Scheduled)
            _embeddedIOServiceCollection.TimerService->ScheduleTask(&t, t.ScheduledTick + _embeddedIOServiceCollection.TimerService->GetTicksPerSecond());

        if(Commands[CommandReadPointer] != 0)
        {
            std::map<uint32_t, Variable*>::iterator it = _engineMain->SystemBus->Variables.find(Commands[CommandReadPointer]);
            if (it != _engineMain->SystemBus->Variables.end())
            {
                if(it->second->Type == POINTER || it->second->Type == BIGOTHER)
                {
                    if(Commands[CommandReadPointer + 1] != 0)
                    {
                        fwrite(((uint8_t *)((uint64_t*)it->second->Value + (Commands[CommandReadPointer + 1] - 1))), 1, sizeof(uint64_t), stdout);
                        Commands[CommandReadPointer] = 0;
                        CommandReadPointer++;
                        CommandReadPointer++;
                        CommandReadPointer %= 32;
                        secondCommand = false;
                    }
                    else if(!secondCommand)
                    {
                        fwrite((uint8_t*)&it->second->Type, 1, sizeof(VariableType), stdout);
                        secondCommand = true;
                    }
                }
                else
                {
                    fwrite((uint8_t*)it->second, 1, sizeof(Variable), stdout);
                    Commands[CommandReadPointer] = 0;
                    CommandReadPointer++;
                    CommandReadPointer %= 32;
                }
            }
        }
        const tick_t now = _embeddedIOServiceCollection.TimerService->GetTick();
        // loopTime->Set((float)(now-prev) / _embeddedIOServiceCollection.TimerService->GetTicksPerSecond());
        prev = now;
        _engineMain->Loop();
        vTaskDelay(1);
    }
    void app_main(void)
    {
        Setup();
        while (1)
        {
            Loop();
        }
    }
}