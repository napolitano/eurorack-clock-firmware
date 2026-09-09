/**
 * @file HardwareTimer.h
 * @brief Host-test fake for the STM32duino HardwareTimer wrapper.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstdint>
#include "Arduino.h"
inline constexpr std::uint8_t HERTZ_FORMAT=1U;
class HardwareTimer {
public:
 using Callback=void (*)();
 explicit HardwareTimer(std::uint32_t instance):instance_(instance){lastInstance=this;}
 void pause(){lastInstance=this;paused=true;++pauseCount;} void setOverflow(std::uint32_t v,std::uint8_t format){lastInstance=this;overflow=v;overflowFormat=format;}
 void setInterruptPriority(std::uint32_t preempt,std::uint32_t sub){lastInstance=this;preemptPriority=preempt;subPriority=sub;}
 void attachInterrupt(Callback cb){lastInstance=this;callback=cb;} void resume(){lastInstance=this;paused=false;++resumeCount;}
 void fire(){if(callback)callback();}
 std::uint32_t instance_=0,overflow=0,preemptPriority=0xFFFFFFFFU,subPriority=0xFFFFFFFFU;std::uint8_t overflowFormat=0;Callback callback=nullptr;bool paused=true;unsigned pauseCount=0,resumeCount=0;
 inline static HardwareTimer* lastInstance=nullptr;
};
