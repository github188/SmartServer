#include "CDtransmmiter.h"
namespace hx_net{
	CDtransmmiter::CDtransmmiter(int subprotocol,int addresscode)
        :Transmmiter()
        ,m_subprotocol(subprotocol)
		,m_addresscode(addresscode)
	{

	}

	CDtransmmiter::~CDtransmmiter()
	{

	}

    int CDtransmmiter::check_msg_header( unsigned char *data,int nDataLen,CmdType cmdType,int number)
	{
		switch(m_subprotocol)
		{
		case CHENGDU_XINGUANG_247:
			{
				if(data[0]!=0x01 || data[1] != 0x03)
					return -1;
				return (((data[3]<<8)|data[4])+2);
			}
		default:
			return -1;
		}
		return -1;
	}

	bool CDtransmmiter::IsStandardCommand()
	{
		switch(m_subprotocol)
		{
		case CHENGDU_XINGUANG_247:
			return true;
		}
		return false;
	}

	void CDtransmmiter::GetSignalCommand( devCommdMsgPtr lpParam,CommandUnit &cmdUnit )
	{

	}

	int CDtransmmiter::decode_msg_body( unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen,int& runstate )
	{
		switch(m_subprotocol)
		{
		case CHENGDU_KAITENG:
			break;
		case CHENGDU_KAITENG_300W:
			break;
		case CHENGDU_CHENGGUANG:
			break;
		case CHENGDU_KT_DIG:
			break;
		case CHENGDU_KAITENG_TV10KW:
			break;
		case CHENGDU_XINGUANG:
			break;
		case CHENGDU_XINGUANG_247:
			{
				return OnXG_247Data(data,data_ptr,nDataLen,runstate);
			}
		}
		return -1;
	}

	void CDtransmmiter::GetAllCmd( CommandAttribute &cmdAll )
	{
		switch(m_subprotocol)
		{
		case CHENGDU_KAITENG:
			break;
		case CHENGDU_KAITENG_300W:
			break;
		case CHENGDU_CHENGGUANG:
			break;
		case CHENGDU_KT_DIG:
			break;
		case CHENGDU_KAITENG_TV10KW:
			break;
		case CHENGDU_XINGUANG:
		case CHENGDU_XINGUANG_247:
			{
				CommandUnit tmUnit;
				tmUnit.ackLen = 5;
				tmUnit.commandLen=0;
				vector<CommandUnit> vtUnit;
				vtUnit.push_back(tmUnit);
				cmdAll.mapCommand[MSG_DEVICE_QUERY] = vtUnit;
				//cmdAll.queryComm.push_back(tmUnit);
				tmUnit.commandId[0] = 0x01;
				tmUnit.commandId[1] = 0x03;
				tmUnit.commandId[2] = 0x04;
				tmUnit.commandId[3] = 0x00;
				tmUnit.commandId[4] = 0x41;
				tmUnit.commandId[5] = 0x00;
				tmUnit.commandId[6] = 0x01;
				tmUnit.commandId[7] = 0x01;	
				tmUnit.commandLen = 8;
				vector<CommandUnit> vtTurnOnUnit;
				vtTurnOnUnit.push_back(tmUnit);
				cmdAll.mapCommand[MSG_TRANSMITTER_TURNON_OPR] = vtTurnOnUnit;
			//	cmdAll.turnonComm.push_back(tmUnit);
				tmUnit.commandId[7] = 0x02;
				vector<CommandUnit> vtTurnOffUnit;
				vtTurnOffUnit.push_back(tmUnit);
				cmdAll.mapCommand[MSG_TRANSMITTER_TURNOFF_OPR] = vtTurnOffUnit;
			//	cmdAll.turnoffComm.push_back(tmUnit);
			}
			break;
		}
	}

	int CDtransmmiter::OnXG_247Data( unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen,int& runstate )
	{
		if(data[2]!=0x03)
		{
			return 0;
		}
		int curpos=5;
		int iparamtype=0;
		int iparamlen = 0;
		
		while((curpos+2)<nDataLen)
		{
			iparamtype = ((data[curpos]<<8)|data[curpos+1]);
			iparamlen = ((data[curpos+2]<<8)|data[curpos+3]);
			int ivalue=0;
			for(int i=0;i<iparamlen;++i)
			{
				ivalue = ((ivalue<<8)|data[curpos+4+i]);
			}
			curpos = curpos+4+iparamlen;

			DataInfo dainfo;
			switch (iparamtype)
			{
            case 0x0041:{
					if(ivalue==0x01)
                        runstate = 0;
					else
						runstate = 1;
				}
				break;
            case 0x0501:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[3] = dainfo;
				}
				break;
            case 0x0502:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[4] = dainfo;
				}
				break;
			case 0x0503:
				data_ptr->mValues[5] = dainfo;
				break;
            case 0x0504:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[6] = dainfo;
				}
				break;
            case 0x0505:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[7] = dainfo;
				}
				break;
            case 0x0506:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[8] = dainfo;
				}
				break;
            case 0x0507:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[9] = dainfo;
				}
				break;
            case 0x0508:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[10] = dainfo;
				}
				break;

            case 0x0509:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[11] = dainfo;
				}
				break;
            case 0x050A:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[12] = dainfo;
				}
				break;
            case 0x050B:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[13] = dainfo;
				}
				break;
            case 0x050C:{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[14] = dainfo;
				}
				break;
			case 0x050D:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[15] = dainfo;
				}
				break;
			case 0x050E:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[16] = dainfo;
				}
				break;
			case 0x050F:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[17] = dainfo;
				}
				break;
			case 0x0510:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[18] = dainfo;
				}
				break;
			case 0x0511:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[19] = dainfo;
				}
				break;
			case 0x0512:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[20] = dainfo;
				}
				break;
			case 0x0513:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[21] = dainfo;
				}
				break;
			case 0x0514:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[22] = dainfo;
				}
				break;
			case 0x0515:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[23] = dainfo;
				}
				break;
			case 0x0516:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[24] = dainfo;
				}
				break;
			case 0x0517:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[25] = dainfo;
				}
				break;
			case 0x0518:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[26] = dainfo;
				}
				break;
			case 0x0519:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[27] = dainfo;
				}
				break;
			case 0x051A:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[28] = dainfo;
				}
				break;
			case 0x051B:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[29] = dainfo;
				}
				break;
			case 0x051C:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[30] = dainfo;
				}
				break;
			case 0x051D:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[31] = dainfo;
				}
				break;
			case 0x051E:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[32] = dainfo;
				}
				break;
			case 0x051F:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[33] = dainfo;
				}
				break;
			case 0x0520:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[34] = dainfo;
				}
				break;
			case 0x0521:
				{
					dainfo.bType = false;
					dainfo.fValue = float(ivalue*0.001);
					data_ptr->mValues[0] = dainfo;
				}
				break;
			case 0x0522:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[1] = dainfo;
				}
				break;
			case 0x0523:
				{
					dainfo.bType = false;
					dainfo.fValue = float(ivalue*0.1);
					data_ptr->mValues[35] = dainfo;
				}
				break;
			case 0x0524:
				{
					dainfo.bType = false;
					dainfo.fValue = float(ivalue*0.1);
					data_ptr->mValues[36] = dainfo;
				}
				break;
			case 0x0525:
				{
					dainfo.bType = false;
					dainfo.fValue = float(ivalue*0.1);
					data_ptr->mValues[37] = dainfo;
				}
				break;
			case 0x0526:
				{
					dainfo.bType = false;
					dainfo.fValue = ivalue;
					data_ptr->mValues[38] = dainfo;
				}
				break;
			default:break;
			}
		}
		map<int,DataInfo>::iterator iterPW = data_ptr->mValues.find(0);
		map<int,DataInfo>::iterator iterRF = data_ptr->mValues.find(1);
		DataInfo zbb;
		zbb.bType = false;
		zbb.fValue = 1.0;
		if(iterPW!=data_ptr->mValues.end() && iterRF!=data_ptr->mValues.end())
		{
			zbb.fValue = ((*iterPW).second.fValue*1000+(*iterRF).second.fValue)/((*iterPW).second.fValue*1000-(*iterRF).second.fValue);
		}
		data_ptr->mValues[2] = zbb;
		return 0;
	}
}
