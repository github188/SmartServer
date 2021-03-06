#pragma once
#include "MsgHandleAgentImpl.h"
#include "./dev_message/base_message.h"

namespace hx_net
{
	class MsgHandleAgentImpl
	{
	public:
        MsgHandleAgentImpl(session_ptr pSession,boost::asio::io_service& io_service,DeviceInfo &devInfo);
		~MsgHandleAgentImpl(void);
	public:
        bool Init(pDevicePropertyExPtr devProperty);
		bool Finit();
		int start();
		int stop();
		bool is_auto_run();
		int PreHandleMsg();
        int  check_msg_header(unsigned char *data,int nDataLen,CmdType cmdType,int number);
		int  decode_msg_body(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen);
        int  decode_msg_body(Snmp *snmp,DevMonitorDataPtr data_ptr,CTarget *target);
		void input_params(const vector<string> &vParam);
		bool IsStandardCommand();
        void GetAllCmd(CommandAttribute &cmdAll);
		void GetSignalCommand(devCommdMsgPtr lpParam,CommandUnit &cmdUnit);
		int getChannelCount();
		bool isBelongChannel(int nChnnel,int monitorItemId);
		bool isMonitorChannel(int nChnnel,DevMonitorDataPtr curDataPtr);
		bool ItemValueIsAlarm(DevMonitorDataPtr curDataPtr,int monitorItemId,dev_alarm_state &curState);

		bool isRegister();
		void getRegisterCommand(CommandUnit &cmdUnit);
        int  cur_dev_state();
        void exec_task_now(int icmdType,string sUser,e_ErrorCode &eErrCode);
        void start_task_timeout_timer();
        //获得运行状态
        int  get_run_state();
         void reset_run_state();
         //添加新告警
         bool  add_new_alarm(string sPrgName,int alarmId,int nState,time_t  startTime);
         bool  add_new_data(string sIp,int nChannel,DevMonitorDataPtr &mapData);
    public:
		base_message *m_pbaseMsg;
        session_ptr m_pSessionPtr;
		boost::asio::io_service    &m_io_service;
        DeviceInfo     &m_devInfo;
	};
}
