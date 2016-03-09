#include "StationConfig.h"
#include "LocalConfig.h"
#include "./net/config.h"
StationConfig::StationConfig(void)
{

}

StationConfig::~StationConfig(void)
{
	if(mapTransmitterInfo.size()>0)
		mapTransmitterInfo.clear();
}

//加载某台站配置信息
bool StationConfig::load_station_config()
{
	mapTransmitterInfo.clear();
	mapAntennaInfo.clear();
	mapAssociateInfo.clear();
	mapModleInfo.clear();
	vSmsInfo.clear();
	vecUploadDevInfo.clear();
	mapTransmitAgentInfo.clear();
	vLinkageInfo.clear();
	vecMediaDevInfo.clear();

	cur_station_id = GetInst(LocalConfig).local_station_id();
	cur_devSvc_id = GetInst(LocalConfig).local_dev_server_number();

    if(!GetInst(DbManager).GetTransmitters(cur_station_id,cur_devSvc_id,mapTransmitterInfo))
		return false;
    if(!GetInst(DbManager).GetAntennaInfo(cur_station_id,cur_devSvc_id,mapAntennaInfo))
		return false;
    if(!GetInst(DbManager).GetAssociateInfo(cur_station_id,cur_devSvc_id,mapAssociateInfo))
		return false;
    if(!GetInst(DbManager).GetModleInfo(cur_station_id,cur_devSvc_id,mapModleInfo))
		return false;
    if(!GetInst(DbManager).GetThransmitterAgentInfo(cur_station_id,cur_devSvc_id,mapTransmitAgentInfo))
		return false;
    if(!GetInst(DbManager).GetUploadDevInfo(cur_station_id,cur_devSvc_id,vecUploadDevInfo))
		return false;
    if(!GetInst(DbManager).GetTelInfoByStation(cur_station_id,vSmsInfo))
		return false;
    if(!GetInst(DbManager).GetAllLinkageRoleNumber(cur_station_id,vLinkageInfo))
		return false;

    if(!GetInst(DbManager).GetConnectMediaDevInfo(cur_station_id,cur_devSvc_id,vecMediaDevInfo))
		return false;

	return true;
}

//获取某台站的发射机信息
vector<TransmitterInformation>& StationConfig::get_transmitter()
{
	return mapTransmitterInfo;
}

//获得所有天线信息
vector<AntennaInformation>& StationConfig::get_antenna()
{
	return mapAntennaInfo;
}

AntennaInformation* StationConfig::get_antenna_ptr_by_id(string sAntennaNumber)
{
	vector<AntennaInformation>::iterator iter=mapAntennaInfo.begin();
	for(;iter!=mapAntennaInfo.end();++iter)
	{
		if((*iter).sNumber == sAntennaNumber)
			return &(*iter);
	}
	return 0;
}

//获得协议转换器
vector<ModleInfo>& StationConfig::get_Modle()
{
	return mapModleInfo;
}

//获得发射机代理设备
vector<ModleInfo>& StationConfig::get_transmitter_agent()
{
	return mapTransmitAgentInfo;
}

//获得发射机代理ip
bool StationConfig::get_transmitter_agent_endpoint_by_id(string sTransmitterId,string &sIp,int &nPort)
{
	for(int i=0;i<mapTransmitAgentInfo.size();++i)
	{
		if(mapTransmitAgentInfo[i].mapDevInfo.find(sTransmitterId)!=
			mapTransmitAgentInfo[i].mapDevInfo.end())
		{
			sIp = mapTransmitAgentInfo[i].sModIP;
			nPort = mapTransmitAgentInfo[i].nModPort;
			return true;
		}
	}

	return false;
}


//获得该等级的用户电话号码
vector<SendMSInfo>& StationConfig::get_sms_user_info()
{
	return vSmsInfo;
}

//获得所有上传设备
vector<MediaDeviceParamInfo>& StationConfig::get_upload_dev_info()
{
	return vecUploadDevInfo;
}
//获得指定上传设备
bool StationConfig::get_upload_dev_info(int ndevAddr,MediaDeviceParamInfo& uploadDevInfo)
{
	vector<MediaDeviceParamInfo>::iterator iter=vecUploadDevInfo.begin();
	for(;iter!=vecUploadDevInfo.end();++iter)
	{
		if((*iter).nDevAddr == ndevAddr)
		{
			uploadDevInfo = (*iter);
			return true;
		}
	}
	return false;
}

//获得所有主动连接媒体设备
vector<MediaDeviceParamInfo>& StationConfig::get_media_dev_info()
{
	return vecMediaDevInfo;
}

//获得指定上传设备
bool StationConfig::get_media_dev_info(int ndevAddr,MediaDeviceParamInfo& mediaDevInfo)
{
	vector<MediaDeviceParamInfo>::iterator iter=vecMediaDevInfo.begin();
	for(;iter!=vecMediaDevInfo.end();++iter)
	{
		if((*iter).nDevAddr == ndevAddr)
		{
			mediaDevInfo = (*iter);
			return true;
		}
	}
	return false;
}

//发射机是否使用代理
bool StationConfig::tranmitter_is_use(string sTransmitterNumber)
{
	vector<TransmitterInformation>::iterator iter = mapTransmitterInfo.begin();
	for(;iter!=mapTransmitterInfo.end();++iter)
	{
		if((*iter).sNumber == sTransmitterNumber)
		{
			return (*iter).IsUsed;
		}
	}
	return false;
}

//由主机获得发射机对应的天线信息
AntennaInformation* StationConfig::get_antenna_by_host_transmitter(string sTransmitterNumber)
{
	vector<AssociateInfo>::iterator iterAss = mapAssociateInfo.begin();
	for(;iterAss!=mapAssociateInfo.end();++iterAss)
	{
		if((*iterAss).sHostNumber == sTransmitterNumber)
		{
			vector<AntennaInformation>::iterator iter = mapAntennaInfo.begin();
			for(;iter!=mapAntennaInfo.end();++iter)
			{
				if((*iter).sNumber == (*iterAss).sHostNumber)
					return &(*iter);
			}
		}
	}

	return 0;

}

//由天线获得天线关联信息
AssociateInfo* StationConfig::get_antenna_associate_info(string sAntennaNumber)
{
	vector<AssociateInfo>::iterator iter = mapAssociateInfo.begin();
	for(;iter!=mapAssociateInfo.end();++iter)
	{
		if((*iter).sAntennaNumber == sAntennaNumber)
			return &(*iter);
	}
	return 0;
}

//由主机获得天线关联信息
AssociateInfo* StationConfig::get_antenna_associate_info_by_host(string sHostTransmitterNumber)
{
	vector<AssociateInfo>::iterator iter = mapAssociateInfo.begin();
	for(;iter!=mapAssociateInfo.end();++iter)
	{
		if((*iter).sHostNumber == sHostTransmitterNumber)
			return &(*iter);
	}

	return 0;
}

//由备机获得天线关联信息
AssociateInfo* StationConfig::get_antenna_associate_info_by_backup(string sBackupTransmitterNumber)
{
	vector<AssociateInfo>::iterator iter = mapAssociateInfo.begin();
	for(;iter!=mapAssociateInfo.end();++iter)
	{
		if((*iter).sBackupNumber == sBackupTransmitterNumber)
			return &(*iter);
	}

	return 0;
}

//查找other设备对应的串口服务器id
string StationConfig::get_modle_id_by_devid(string sStationId,string sDevId)
{
	for(int i=0;i<mapModleInfo.size();++i)
	{
		if(mapModleInfo[i].mapDevInfo.find(sDevId)!=mapModleInfo[i].mapDevInfo.end())
			return mapModleInfo[i].sModleNumber;
	}
	return "";
}

//查找发射机代理设备管理器对应的串口服务器id
string StationConfig::get_transmitter_id_by_agent(string sStationId,string sDevId)
{
	for(int i=0;i<mapTransmitAgentInfo.size();++i)
	{
		if(mapTransmitAgentInfo[i].mapDevInfo.find(sDevId)!=mapTransmitAgentInfo[i].mapDevInfo.end())
			return mapTransmitAgentInfo[i].sModleNumber;
	}
	return "";
}


//获得设备基本信息
bool StationConfig::get_dev_base_info(string sStationId,string sDevId,DevBaseInfo &devBaseInfo)
{
	return false;
}

