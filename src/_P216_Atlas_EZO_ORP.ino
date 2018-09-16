#ifdef USES_P216
//########################################################################
//################## Plugin 216 : Atlas Scientific EZO ORP sensor  ########
//########################################################################

// datasheet at https://www.atlas-scientific.com/_files/_datasheets/_circuit/ORP_EZO_datasheet.pdf
// works only in i2c mode

#define PLUGIN_216
#define PLUGIN_ID_216 216
#define PLUGIN_NAME_216       "Environment - Atlas Scientific ORP EZO"
#define PLUGIN_VALUENAME1_216 "ORP"

boolean Plugin_216_init = false;

boolean Plugin_216(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_216;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_216);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_216));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        #define ATLASEZO_ORP_I2C_NB_OPTIONS 4
        byte I2Cchoice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        int optionValues[ATLASEZO_ORP_I2C_NB_OPTIONS] = { 0x62, 0x61, 0x63, 0x64 };
        addFormSelectorI2C(F("Plugin_216_i2c"), ATLASEZO_ORP_I2C_NB_OPTIONS, optionValues, I2Cchoice);

        addFormSubHeader(F("General"));

        char sensordata[32];
        bool status;
        status = _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0],"i",sensordata);

        if (status) {
          String boardInfo(sensordata);

          addHtml(F("<TR><TD>Board type : </TD><TD>"));
          int pos1 = boardInfo.indexOf(',');
          int pos2 = boardInfo.lastIndexOf(',');
          addHtml(boardInfo.substring(pos1+1,pos2));
          if (boardInfo.substring(pos1+1,pos2) != "ORP"){
            addHtml(F("<span style='color:red'>  WARNING : Board type should be 'ORP', check your i2c Address ? </span>"));
          }
          addHtml(F("</TD></TR><TR><TD>Board version :</TD><TD>"));
          addHtml(boardInfo.substring(pos2+1));
          addHtml(F("</TD></TR>\n"));

          addHtml(F("<input type='hidden' name='Plugin_216_sensorVersion' value='"));
          addHtml(boardInfo.substring(pos2+1));
          addHtml(F("'>"));

          status = _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0],"i",sensordata);
          boardInfo = sensordata;
          addHtml(F("<TR><TD>Sensor type : </TD><TD>K="));
          pos2 = boardInfo.lastIndexOf(',');
          addHtml(boardInfo.substring(pos2+1));
          addHtml(F("</TD></TR>\n"));

        } else {
          addHtml(F("<span style='color:red;'>Unable to send command to device</span>"));
          success = false;
          break;
        }

        addFormCheckBox(F("Status LED"), F("Plugin_216_status_led"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);

        addFormSubHeader(F("Calibration"));

        addRowLabel(F("<strong>Single point calibration</strong> "));
        addFormCheckBox(F("Enable"),F("Plugin_216_enable_cal_single"), false);
        addHtml(F("\n<script type='text/javascript'>document.getElementById(\"Plugin_216_enable_cal_single\").onclick=function() {};</script>"));
        addFormNumericBox(F("Ref value"),F("Plugin_216_ref_cal_single"), Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        addUnit("mV");

        status = _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0], "Cal,?",sensordata);
        if (status){
          switch (sensordata[5]) {
            case '0' :
              addFormNote(F("<span style='color:red'>Calibration needed</span>"));
            break;
            case '1' :
              addFormNote(F("<span style='color:green'>Single point calibration ok</span>"));
            break;
          }
        }

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("Plugin_216_i2c"));

        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] = getFormItemFloat(F("Plugin_216_sensorVersion"));

        char sensordata[32];
        if (isFormItemChecked(F("Plugin_216_status_led"))) {
          _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0],"L,1",sensordata);
        } else {
          _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0],"L,0",sensordata);
        }
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = isFormItemChecked(F("Plugin_216_status_led"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("Plugin_216_ref_cal_single"));

        String cmd ("Cal,");
        bool triggerCalibrate = false;
        if (isFormItemChecked("Plugin_216_enable_cal_single")){
          cmd += Settings.TaskDevicePluginConfig[event->TaskIndex][2];
          triggerCalibrate = true;
        }
        if (triggerCalibrate){
          char sensordata[32];
          _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0],cmd.c_str(),sensordata);
        }

        Plugin_216_init = false;
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_216_init = true;
      }

    case PLUGIN_READ:
      {
        char sensordata[32];

        //ok, now we can read the ORP value
        bool status = _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0],"r",sensordata);

        if (status){
          String sensorString(sensordata);
          UserVar[event->BaseVarIndex] = sensorString.toFloat();
        }
        else {
          UserVar[event->BaseVarIndex] = -1;
        }

        //go to sleep
        // status = _P216_send_I2C_command(Settings.TaskDevicePluginConfig[event->TaskIndex][0],"Sleep",sensordata);

        success = true;
        break;
      }
      case PLUGIN_WRITE:
        {
          //TODO : do something more usefull ...

        //   String tmpString  = string;
        //   int argIndex = tmpString.indexOf(',');
        //   if (argIndex)
        //     tmpString = tmpString.substring(0, argIndex);
        //   if (tmpString.equalsIgnoreCase(F("ATLASCMD")))
        //   {
        //     success = true;
        //     argIndex = string.lastIndexOf(',');
        //     tmpString = string.substring(argIndex + 1);
        //     if (tmpString.equalsIgnoreCase(F("CalMid"))){
        //       String log("Asking for Mid calibration ");
        //       addLog(LOG_LEVEL_INFO, log);
        //     }
        //     else if (tmpString.equalsIgnoreCase(F("CalLow"))){
        //       String log("Asking for Low calibration ");
        //       addLog(LOG_LEVEL_INFO, log);
        //     }
        //     else if (tmpString.equalsIgnoreCase(F("CalHigh"))){
        //       String log("Asking for High calibration ");
        //       addLog(LOG_LEVEL_INFO, log);
        //     }
        //   }
          break;
        }
  }
  return success;
}

// Call this function with two char arrays, one containing the command
// The other containing an allocatted char array for answer
// Returns true on success, false otherwise

bool _P216_send_I2C_command(uint8_t I2Caddress,const char * cmd, char* sensordata) {
    uint16_t sensor_bytes_received = 0;

    byte error;
    byte i2c_response_code = 0;
    byte in_char = 0;

    Serial.println(cmd);
    Wire.beginTransmission(I2Caddress);
    Wire.write(cmd);
    error = Wire.endTransmission();

    if (error != 0) {
      return false;
    }

    //don't read answer if we want to go to sleep
    if (strncmp(cmd,"Sleep",5) == 0) {
      return true;
    }

    i2c_response_code = 254;
    while (i2c_response_code == 254) {      // in case the cammand takes longer to process, we keep looping here until we get a success or an error

      if  (
             (  (cmd[0] == 'r' || cmd[0] == 'R') && cmd[1] == '\0'  )
             ||
             (  ( strncmp(cmd,"cal",3) || strncmp(cmd,"Cal",3) ) && !strncmp(cmd,"Cal,?",5) )
           )
      {
        delay(600);
      }
      else {
        delay(300);
      }

      Wire.requestFrom(I2Caddress, (uint8_t) 32);    //call the circuit and request 32 bytes (this is more then we need).
      i2c_response_code = Wire.read();      //read response code

      while (Wire.available()) {            //read response
        in_char = Wire.read();

        if (in_char == 0) {                 //if we receive a null caracter, we're done
          while (Wire.available()) {  //purge the data line if needed
            Wire.read();
          }

          break;                            //exit the while loop.
        }
        else {
          sensordata[sensor_bytes_received] = in_char;        //load this byte into our array.
          sensor_bytes_received++;
        }
      }
      sensordata[sensor_bytes_received] = '\0';

      switch (i2c_response_code) {
        case 1:
          Serial.print( F("< success, answer = "));
          Serial.println(sensordata);
          break;

        case 2:
          Serial.println( F("< command failed"));
          return false;

        case 254:
          Serial.println( F("< command pending"));
          break;

        case 255:
          Serial.println( F("< no data"));
          return false;
      }
    }

    Serial.println(sensordata);
    return true;
}

#endif