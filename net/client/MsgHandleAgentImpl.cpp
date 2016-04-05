#include "MsgHandleAgentImpl.h"
#include "./dev_message/Envir_message.h"
//#include "./dev_message/Hx_message.h"
#include "./dev_message/Tsmt_message.h"
#include "./dev_message/Electric_message.h"

namespace hx_net
{
    MsgHandleAgentImpl::MsgHandleAgentImpl(net_session *pSession,boost::asio::io_service& io_service,DeviceInfo &devInfo)
		:m_pSessionPtr(pSession)
		,m_pbaseMsg(0)
		,m_io_service(io_service)
        ,m_devInfo(devInfo)
	{
	}

	MsgHandleAgentImpl::~MsgHandleAgentImpl(void)
	{
	}

    bool MsgHandleAgentImpl::Init()
	{
        switch(m_devInfo.iDevType)
		{
        case DEVICE_TRANSMITTER:{
                m_pbaseMsg = new Tsmt_message(m_pSessionPtr,m_io_service,m_devInfo);
             }
            break;
        case DEVICE_TEMP:
        case DEVICE_SMOKE:
        case DEVICE_WATER:
        case DEVICE_AIR:{
                m_pbaseMsg = new Envir_message(m_pSessionPtr,m_devInfo);
			}
			break;
        case HUIXIN:	{
                //m_pbaseMsg = new Hx_message(m_pSessionPtr);
			}
			break;
        case DEVICE_ELEC:{
                m_pbaseMsg = new Electric_message(m_pSessionPtr,m_io_service,m_devInfo);
			}
			break;
		default:
			return false;
		}
		return true;
	}

	bool MsgHandleAgentImpl::Finit()
	{
		if(m_pbaseMsg != NULL)
		{
			delete m_pbaseMsg;
			m_pbaseMsg=NULL;
		}
		return true;
	}

	int MsgHandleAgentImpl::start()
	{
		if(m_pbaseMsg==NULL)
			return -1;
		return m_pbaseMsg->start();
	}
	int MsgHandleAgentImpl::stop()
	{
		if(m_pbaseMsg==NULL)
			return -1;
		return m_pbaseMsg->stop();
	}

	bool MsgHandleAgentImpl::is_auto_run()
	{
		if(m_pbaseMsg==NULL)
			return false;
		return m_pbaseMsg->is_auto_run();
	}

	int  MsgHandleAgentImpl::check_msg_header(unsigned char *data,int nDataLen)
	{
		if(m_pbaseMsg==NULL)
			return -1;
		return m_pbaseMsg->check_msg_header(data,nDataLen);
	}
	int  MsgHandleAgentImpl::decode_msg_body(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen)
	{
		if(m_pbaseMsg==NULL)
			return -1;
		return m_pbaseMsg->decode_msg_body(data,data_ptr,nDataLen);
	}
    /*HDevAgentPtr MsgHandleAgentImpl::DevAgent()
	{
		if(m_pbaseMsg==NULL)
			return HDevAgentPtr();
		return m_pbaseMsg->Agent();
    }*/

	void MsgHandleAgentImpl::input_params(const vector<string> &vParam)
	{
		if(m_pbaseMsg==NULL)
			return;
		return m_pbaseMsg->input_params(vParam);
	}

	int MsgHandleAgentImpl::PreHandleMsg()
	{
		if(m_pbaseMsg==NULL)
			return -1;
		return m_pbaseMsg->PreHandleMsg();
	}

	bool MsgHandleAgentImpl::IsStandardCommand()
	{
		if(m_pbaseMsg==NULL)
			return false;
		return m_pbaseMsg->IsStandardCommand();
	}

	void MsgHandleAgentImpl::GetSignalCommand(devCommdMsgPtr lpParam,CommandUnit &cmdUnit)
	{
		if(m_pbaseMsg==NULL)
			return;
		return m_pbaseMsg->GetSignalCommand(lpParam,cmdUnit);
	}

	int MsgHandleAgentImpl::getChannelCount()
	{
		if(m_pbaseMsg==NULL)
			return -1;
		return m_pbaseMsg->getChannelCount();
	}

	bool MsgHandleAgentImpl::isBelongChannel(int nChnnel,int monitorItemId)
	{
		if(m_pbaseMsg==NULL)
			return false;
		return m_pbaseMsg->isBelongChannel(nChnnel,monitorItemId);
	}

	bool MsgHandleAgentImpl::ItemValueIsAlarm(DevMonitorDataPtr curDataPtr,
		                                      int monitorItemId,dev_alarm_state &curState)
	{
		if(m_pbaseMsg==NULL)
			return false;
		return m_pbaseMsg->ItemValueIsAlarm(curDataPtr,monitorItemId,curState);
	}

	bool MsgHandleAgentImpl::isMonitorChannel(int nChnnel,DevMonitorDataPtr curDataPtr)
	{
		if(m_pbaseMsg==NULL)
			return false;
		return m_pbaseMsg->isMonitorChannel(nChnnel,curDataPtr);
	}

	bool MsgHandleAgentImpl::isRegister()
	{
		if(m_pbaseMsg==NULL)
			return false;
		return m_pbaseMsg->isRegister();
	}
	void MsgHandleAgentImpl::getRegisterCommand(CommandUnit &cmdUnit)
	{
		if(m_pbaseMsg==NULL)
			return ;
		return m_pbaseMsg->getRegisterCommand(cmdUnit);
	}

    int  MsgHandleAgentImpl::cur_dev_state()
    {
        if(m_pbaseMsg==NULL)
            return -1;
        return m_pbaseMsg->cur_dev_state();
    }

    void MsgHandleAgentImpl::exec_task_now(int icmdType,int nResult)
    {
        if(m_pbaseMsg==NULL)
            return ;
        return m_pbaseMsg->exec_task_now(icmdType,nResult);
    }
}

