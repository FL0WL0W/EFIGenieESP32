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
// #include "nvs.h"
// #include "nvs_flash.h"

using namespace OperationArchitecture;
using namespace EmbeddedIOServices;
using namespace Esp32;
using namespace Engine;

char *_config = 0; //ESP chips have gobs of ram. just load the config into ram. this will make live tuning easier later on this platform_config
extern const uint8_t tune_start[] asm("_binary_tune_bin_start");
extern const uint8_t tune_end[] asm("_binary_tune_bin_end");
const unsigned int tune_bytes = tune_end - tune_start;

// esp_err_t loadConfig()
// {
//     if(_config != 0)
//     {
//         free(_config);
//         _config = 0;
//     }

//     nvs_handle_t my_handle;
//     esp_err_t err;

//     // Open
//     err = nvs_open("nvs", NVS_READWRITE, &my_handle);
//     if (err != ESP_OK) return err;

//     // Read the size of memory space required for blob
//     size_t required_size = 0;  // value will default to 0, if not set yet in NVS
//     err = nvs_get_blob(my_handle, "config", NULL, &required_size);
//     if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

//     // Read previously saved blob if available
//     _config = reinterpret_cast<char *>(malloc(required_size + sizeof(uint32_t)));
//     if (required_size > 0) {
//         err = nvs_get_blob(my_handle, "config", _config, &required_size);
//         if (err != ESP_OK) {
//             free(_config);
//             _config = 0;
//             return err;
//         }
//     }

//     return ESP_OK;
// }

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
        printf( "UINT8 ALIGN  %d\n\r", uint8_align);
        printf( "UINT16 ALIGN %d\n\r", uint16_align);
        printf( "UINT32 ALIGN %d\n\r", uint32_align);
        printf( "UINT64 ALIGN %d\n\r", uint64_align);
        printf( "UINT64 BOOL  %d\n\r", bool_align);
        printf( "INT8 ALIGN   %d\n\r", int8_align);
        printf( "INT16 ALIGN  %d\n\r", int16_align);
        printf( "INT32 ALIGN  %d\n\r", int32_align);
        printf( "INT64 ALIGN  %d\n\r", int64_align);
        printf( "FLOAT ALIGN  %d\n\r", float_align);
        printf( "DOUBLE ALIGN %d\n\r", double_align);

        printf( "Initializing EmbeddedIOServices\n\r");
        _embeddedIOServiceCollection.DigitalService = new Esp32IdfDigitalService();
        _embeddedIOServiceCollection.AnalogService = new Esp32IdfAnalogService(ADC_ATTEN_DB_11);
        _embeddedIOServiceCollection.TimerService = new Esp32IdfTimerService(TIMER_GROUP_1, TIMER_0);
        // _embeddedIOServiceCollection.PwmService = new Esp32IdfPwmService();
        printf( "EmbeddedIOServices Initialized\n\r");
        _embeddedIOServiceCollection.DigitalService->InitPin(5, Out);

        _embeddedIOServiceCollection.DigitalService->InitPin(2, Out);
        _embeddedIOServiceCollection.TimerService->ScheduleTask(&t, _embeddedIOServiceCollection.TimerService->GetTick() + _embeddedIOServiceCollection.TimerService->GetTicksPerSecond());

        printf( "Initializing EngineMain\n\r");
        unsigned int _configSize = 0;
        _engineMain = new EngineMain(reinterpret_cast<const void*>(_config), _configSize, &_embeddedIOServiceCollection);
        printf( "EngineMain Initialized\n\r");
        test();

        printf( "Setting Up EngineMain\n\r");
        _engineMain->Setup();
        printf( "EngineMain Setup\n\r");
        loopTime = _engineMain->SystemBus->GetOrCreateVariable(250);
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
        loopTime->Set((float)(now-prev) / _embeddedIOServiceCollection.TimerService->GetTicksPerSecond());
        prev = now;
        _engineMain->Loop();
        std::map<uint32_t, Variable*>::iterator it = _engineMain->SystemBus->Variables.find(11);
        printf("%.6f\r\n", it->second->To<float>());
        // vTaskDelay(1);
    }
    void app_main(void)
    {
        // esp_err_t err = nvs_flash_init();
        // if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        //     // NVS partition was truncated and needs to be erased
        //     // Retry nvs_flash_init
        //     ESP_ERROR_CHECK(nvs_flash_erase());
        //     err = nvs_flash_init();
        // }
        // ESP_ERROR_CHECK( err );
        // loadConfig();

        _config = reinterpret_cast<char *>(malloc(tune_bytes));
        std::memcpy(_config, tune_start, tune_bytes);

        Setup();
        while (1)
        {
            Loop();
        }
    }
}