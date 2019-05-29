#ifndef COMMANDS_H
#define COMMANDS_H

#define RESPONSE_CODE_RX_TIMEOUT 0xaa
#define RESPONSE_CODE_CMD_INTERRUPTED 0xbb
#define RESPONSE_CODE_SUCCESS 0xdd
#define RESPONSE_CODE_PARAM_ERROR 0x11
#define RESPONSE_CODE_UNKNOWN_COMMAND 0x22

enum CommandCode {
  CmdGetState         = 0x01,
  CmdGetVersion       = 0x02,
  CmdGetPacket        = 0x03,
  CmdSendPacket       = 0x04,
  CmdSendAndListen    = 0x05,
  CmdUpdateRegister   = 0x06,
  CmdReset            = 0x07,
  CmdLED              = 0x08,
  CmdReadRegister     = 0x09,
  CmdSetModeRegisters = 0x0a,
  CmdSetSWEncoding    = 0x0b,
  CmdSetPreamble      = 0x0c,
  CmdResetRadioConfig = 0x0d,
  CmdGetStatistics    = 0x0e
};

enum RegisterMode {
  RegisterModeTx = 0x01,
  RegisterModeRx = 0x02
  /* maybe idle in future? */
};

void get_command();

// This is set when a command is received while processing a long running
// command, like get_packet
extern uint8_t interrupting_cmd;

#endif
