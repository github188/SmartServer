#pragma once
#include <map>
#include "device_message.h"
#include "../share_ptr_object_define.h"
#include "include.h"
#include "DataTypeDefine.h"
#include "device_session.h"
using namespace std;

namespace hx_net
{
	class session;
	class MsgHandleAgentImpl;
	class MsgHandleAgent
	{
	public:
        MsgHandleAgent(session_ptr conPtr,boost::asio::io_service& io_service,DeviceInfo &devInfo);
		~MsgHandleAgent(void);
	public:
        bool Init(pDevicePropertyExPtr devProperty);
        bool Finit();
		int start();
		int stop();
		bool is_auto_run();
		int  check_msg_header(unsigned char *data,int nDataLen);
		int  decode_msg_body(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen);
		void input_params(const vector<string> &vParam);
        int   PreHandleMsg();
		bool IsStandardCommand();
		void GetSignalCommand(devCommdMsgPtr lpParam,CommandUnit &cmdUnit);
        void GetAllCmd(CommandAttribute &cmdAll);
        int   getChannelCount();
		bool isBelongChannel(int nChnnel,int monitorItemId);
		bool isMonitorChannel(int nChnnel,DevMonitorDataPtr curDataPtr);
		bool ItemValueIsAlarm(DevMonitorDataPtr curDataPtr,int monitorItemId,dev_alarm_state &curState);

		bool isRegister();
		void getRegisterCommand(CommandUnit &cmdUnit);

        int   cur_dev_state();
        void exec_task_now(int icmdType,string sUser);
        void start_task_timeout_timer();
        //获得运行状态
        int get_run_state();
        //设置运行状态
        void reset_run_state();
	private:
		MsgHandleAgentImpl *m_msgImpl;
	};
	typedef boost::shared_ptr<MsgHandleAgent>        HMsgHandlePtr;
}
