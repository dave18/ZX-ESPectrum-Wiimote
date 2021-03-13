/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2020 Fabrizio Di Vittorio.
  All rights reserved.

  This file is part of FabGL Library.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once



/**
 * @file
 *
 * @brief This file contains fabgl::PS2Controller definition.
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "fabutils.h"
#include "fabglconf.h"


namespace fabgl {


/** \ingroup Enumerations
 * @brief This enum defines what is connected to PS/2 ports
 */
enum class PS2Preset {
  KeyboardPort0_MousePort1,   /**< Keyboard on Port 0 and Mouse on Port 1 */
  KeyboardPort0,              /**< Keyboard on Port 0 (no mouse) */
  MousePort0,                 /**< Mouse on port 0 (no keyboard) */
};


/** \ingroup Enumerations
 * @brief This enum defines how handle keyboard virtual keys
 */
enum class KbdMode {
  NoVirtualKeys,           /**< No virtual keys are generated */
  GenerateVirtualKeys,     /**< Virtual keys are generated. You can call Keyboard.isVKDown() only. */
  CreateVirtualKeysQueue,  /**< Virtual keys are generated and put on a queue. You can call Keyboard.isVKDown(), Keyboard.virtualKeyAvailable() and Keyboard.getNextVirtualKey() */
};


class Keyboard;
class Mouse;


/**
 * @brief The PS2 device controller class.
 *
 * The PS2 controller uses ULP coprocessor and RTC slow memory to communicate with up to two PS2 devices.<br>
 * The ULP coprocessor continuously monitor CLK and DATA lines for incoming data. Optionally can send commands to the PS2 devices.
 */
class PS2Controller {

public:

  PS2Controller();

  // unwanted methods
  PS2Controller(PS2Controller const&)   = delete;
  void operator=(PS2Controller const&)  = delete;

  /**
  * @brief Initializes PS2 device controller.
  *
  * Initializes the PS2 controller assigning GPIOs to DAT and CLK lines.
  *
  * Because PS/2 ports are handled by the ULP processor, just few GPIO ports are actually usable. They are:
  * GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_12 (with some limitations), GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32 and GPIO_NUM_33.
  *
  * @param port0_clkGPIO The GPIO number of Clock line for PS/2 port 0.
  * @param port0_datGPIO The GPIO number of Data line for PS/2 port 0.
  * @param port1_clkGPIO The GPIO number of Clock line for PS/2 port 1 (GPIO_UNUSED to disable).
  * @param port1_datGPIO The GPIO number of Data line for PS/2 port 1 (GPIO_UNUSED to disable).
  */
  void begin(gpio_num_t port0_clkGPIO, gpio_num_t port0_datGPIO, gpio_num_t port1_clkGPIO = GPIO_UNUSED, gpio_num_t port1_datGPIO = GPIO_UNUSED);

  /**
  * @brief Initializes PS2 device controller using default GPIOs.
  *
  * Initializes the PS2 controller assigning:
  *   - GPIO_NUM_33 (CLK) and GPIO_NUM_32 (DATA) as PS/2 Port 0
  *   - GPIO_NUM_26 (CLK) and GPIO_NUM_27 (DATA) as PS/2 Port 1
  *
  * @param preset Specifies what is connected to PS/2 ports (mouse, keyboard or boths).
  * @param keyboardMode Specifies how handle keyboard virtual keys.
  *
  * Example:
  *
  *     // Keyboard connected to port 0 and mouse to port1
  *     PSController.begin(PS2Preset::KeyboardPort0_MousePort1);
  */
  void begin(PS2Preset preset = PS2Preset::KeyboardPort0_MousePort1, KbdMode keyboardMode = KbdMode::CreateVirtualKeysQueue);

  /**
   * @brief Determines if one byte has been received from the specified port
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   *
   * @return True if one byte is available
   */
  bool dataAvailable(int PS2Port);

  /**
   * @brief Gets a scancode from the queue.
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   *
   * @return The first scancode of the queue (-1 if no data is available).
   */
  int getData(int PS2Port, int timeOutMS);

  /**
   * @brief Sends a command to the device.
   *
   * @param data Byte to send to the PS2 device.
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   */
  void sendData(uint8_t data, int PS2Port);

  /**
   * @brief Disables inputs from PS/2 port driving the CLK line Low
   *
   * Use enableRX() to release CLK line.
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   */
  void disableRX(int PS2Port);

  /**
   * @brief Enables inputs from PS/2 port releasing CLK line
   *
   * Use disableRX() to disable inputs from PS/2 port.
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   */
  void enableRX(int PSPort);

  /**
   * @brief Returns the instance of Keyboard object automatically created by PS2Controller.
   *
   * @return A pointer to a Keyboard object
   */
  Keyboard * keyboard() { return m_keyboard; }

  void setKeyboard(Keyboard * value) { m_keyboard = value; }

  /**
   * @brief Returns the instance of Mouse object automatically created by PS2Controller.
   *
   * @return A pointer to a Mouse object
   */
  Mouse * mouse() { return m_mouse; }

  void setMouse(Mouse * value) { m_mouse = value; }

  /**
   * @brief Returns the singleton instance of PS2Controller class
   *
   * @return A pointer to PS2Controller singleton object
   */
  static PS2Controller * instance()  { return s_instance; }

  bool parityError(int PS2Port)      { return m_parityError[PS2Port]; }

  bool syncError(int PS2Port)        { return m_syncError[PS2Port]; }

  bool CLKTimeOutError(int PS2Port)  { return m_CLKTimeOutError[PS2Port]; }

private:


  static void IRAM_ATTR ULPWakeISR(void * arg);

  static PS2Controller * s_instance;

  // Keyboard and Mouse instances can be created by PS2Controller in one of the begin() calls, or can be
  // set using setKeyboard() and setMouse() calls.
  Keyboard *            m_keyboard;
  Mouse *               m_mouse;

  bool                  m_portEnabled[2];

  intr_handle_t         m_ULPWakeISRHandle;

  // true if last call to getData() had a parity, sync error (start or stop missing bits) or CLK timeout
  bool                  m_parityError[2];
  bool                  m_syncError[2];
  bool                  m_CLKTimeOutError[2];

  // one word queue (contains just the last received word)
  QueueHandle_t         m_dataIn[2];

};





} // end of namespace






