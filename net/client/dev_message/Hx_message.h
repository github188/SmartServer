#pragma once
#include "base_message.h"
using namespace std;

namespace net
{
	typedef struct tagDataMgr
	{
          tagDataMgr():antenaS(dev_unknown){}
		string         sHostId;
		string         sBackupId;
		string         sAntennaId;
		string         sStationId;
		dev_run_state  antenaS;
	}HxDataMgrParam;
	typedef boost::shared_ptr<HxDataMgrParam> hxDataMgrParamPtr;
	class Hx_message:public base_message
	{
	public:
		Hx_message(session *pSession=NULL);
		~Hx_message(void);
	public:
		void SetSubPro(int subprotocol);
		int  check_msg_header(unsigned char *data,int nDataLen);
		int  decode_msg_body(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen);
		int  PreHandleMsg();
		void input_params(const vector<string> &vParam);
		bool IsStandardCommand();
		void GetSignalCommand(devCommdMsgPtr lpParam,CommandUnit &cmdUnit);
		void SetAddr(int addr){m_DevAddr = addr;};
		int  getChannelCount();
		//该监控量是否属于该通道
		bool isBelongChannel(int nChnnel,int monitorItemId);
		//判断监测量是否报警
		bool ItemValueIsAlarm(DevMonitorDataPtr curDataPtr,int monitorItemId,dev_alarm_state &curState);
		//设备监测通道是否打开
		bool isMonitorChannel(int nChnnel,DevMonitorDataPtr curDataPtr);
	protected:
		//解析汇鑫数据管理器数据
		int parse_DATA_MGR_data(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen);
		int parse_761_data(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen);
		int parse_730P_data(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen);
		int parse_740P_data(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen);
        //int On761Data( DevMonitorDataPtr dataBuf,unsigned char lpBuffer,DWORD dwCount);
		bool itemIsAlarm(int nAlarmS,int itemId,dev_alarm_state &alarm_state);
	private:
		HxSubPrototcol m_Subprotocol;
		session *m_pSession;
		hxDataMgrParamPtr  m_ParamPtr; 
		int           m_DevAddr;//设备地址
		map<int,int>  m_mapAlarm;//当前报警
        map<int,std::pair<int,unsigned int> > m_mapItemAlarmRecord;
	};
}
