/*
 *   This software is copyright protected and proprietary to
 *   LG electronics. LGE grants to you only those rights as
 *   set out in the license conditions. All other rights remain
 *   with LG electronics.
 * \author  Jong Kyung Byun
 * \date    2017.11.14
 * \attention Copyright (c) 2015 by LG electronics co, Ltd. All rights reserved.
 */

#define LOG_TAG "DiagInputManager"

#include <Log.h>

#include <binder/IServiceManager.h>

/*!! appmgr Inheritance CGA start-------------------------------------------------*/ 
#include <services/ApplicationManagerService/IApplicationManagerService.h>
#include <services/ApplicationManagerService/IApplicationManagerServiceType.h>
/*!! appmgr Inheritance CGA end-------------------------------------------------*/ 
/*!! vif Inheritance CGA start-------------------------------------------------*/		
#include <services/vifManagerService/IvifManagerService.h>
#include <services/vifManagerService/IvifManagerReceiver.h>
#include <services/vifManagerService/IvifManagerServiceType.h>
/*!! vif Inheritance CGA end-------------------------------------------------*/
/*!! hmi Inheritance CGA start-------------------------------------------------*/		
#include <services/HMIManagerService/IHMIManagerService.h>
#include <services/HMIManagerService/IHMIManagerServiceType.h>
/*!! hmi Inheritance CGA end-------------------------------------------------*/
/*!! Audio Inheritance CGA start-------------------------------------------------*/ 
#include <services/AudioManagerService/IAudioManagerService.h>
#include <services/AudioManagerService/IAudioManagerServiceType.h>
/*!! Audio Inheritance CGA end-------------------------------------------------*/

#include <utils/watchdog/watchdog_client.h>

#include "DiagInputManager.h"
#include "DiagManagerService.h"
#include "services/DiagManagerService/DiagCommand.h"
#include "services/DiagManagerService/OEM_Diag_Hal_Access.h"
#include "services/DiagManagerService/OEM_Diag_Defines.h"

#include "HMIType.h"

DiagInputManager::DiagInputManager(android::sp<DiagManagerService> diagMgrService)
		: mDiagMgrService(diagMgrService), each_packet_ptr(0), mDiagInputMgrTimer(NULL)
{

/*!! appmgr Inheritance CGA start-------------------------------------------------*/		
	mAppManager = NULL;
    mSystemPostReceiver = NULL;
/*!! appmgr Inheritance CGA end-------------------------------------------------*/ 
/*!! audio Inheritance CGA start-------------------------------------------------*/		
	mAudioManager = NULL;
    mAudioReceiver = NULL;
    mAudioPostReceiver = NULL;
/*!! audio Inheritance CGA end-------------------------------------------------*/
/*!! hmi Inheritance CGA start-------------------------------------------------*/		
	mHmiManager = NULL;
    mHmiReceiver = NULL;
    mHmiPostReceiver = NULL;
/*!! hmi Inheritance CGA end-------------------------------------------------*/
/*!! vif Inheritance CGA start-------------------------------------------------*/		
	mVifManager = NULL;
    mVifReceiver = NULL;
    mVifPostReceiver = NULL;
/*!! vif Inheritance CGA end-------------------------------------------------*/


    isLongBufferActive = false;
    memset(longDataBuffer, 0U, DIAGDATA_BUFSIZE);

}

DiagInputManager::~DiagInputManager()
{
    if (mDiagInputMgrTimer != NULL) {
        delete mDiagInputMgrTimer;
    }
}

error_t DiagInputManager::init()
{
    error_t result = E_OK;
    mServiceDeathRecipient = new ServiceDeathRecipient(*this);
    mProcessDataManager = new ProcessDataManager(this);

    result = mProcessDataManager->init();
    mMyHandler = new DiagHandler(mDiagMgrService->looper(), *this);

/*!! appmgr Inheritance CGA start-------------------------------------------------*/		
    connectToAppMgr();
/*!! appmgr Inheritance CGA end-------------------------------------------------*/ 
/*!! audio Inheritance CGA start-------------------------------------------------*/		
    connectToAudioMgr();
/*!! audio Inheritance CGA end-------------------------------------------------*/
/*!! hmi Inheritance CGA start-------------------------------------------------*/		
    connectToHmiMgr();
/*!! hmi Inheritance CGA end-------------------------------------------------*/
/*!! vif Inheritance CGA start-------------------------------------------------*/		
    connectToVifMgr();
/*!! vif Inheritance CGA end-------------------------------------------------*/


    mDiagInputMgrTimer = new DiagInputMgrTimer (mMyHandler);
    m_WatcdogTimer = new Timer (mDiagInputMgrTimer, DiagInputMgrTimer::DIAG_WATCHDOG_TIMER);
    mDIDStartTimer = new Timer (mDiagInputMgrTimer, DiagInputMgrTimer::BOOT_COMPLETE_TIME_OUT);

    m_WatcdogTimer->setDuration(WATCHDOG_START_DURATION, WATCHDOG_TIME_OUT);
    m_WatcdogTimer->start();
    HeartBeat_Ready();

    return result;
}

void DiagInputManager::TimerStart()
{
    LOGV("TimerStart");
    mDIDStartTimer->setDuration(DIAG_BOOTING_WAIT_TIME, DIAG_TIME_OUT);
	mDIDStartTimer->start();
}

void DiagInputManager::TimerStop()
{
    LOGV("TimerStop");
    mDIDStartTimer->stop();
    return mProcessDataManager->boot_completed_StartDID();
}

void DiagInputManager::DiagHandler::handleMessage(const android::sp<sl::Message>& msg)
{
    const int32_t what = msg->what;

    android::sp<Buffer> buf = new Buffer();
    if (msg->buffer.size() > 0) {
        buf->setTo(msg->buffer.data(), msg->buffer.size());
    }

    switch (what) {

/*!! appmgr Inheritance CGA start-------------------------------------------------*/		
	case MSG_CONNECT_TO_APPMGR:
        LOGV("handleMessage MSG_CONNECT_TO_APPMGR");
        mDiagInputMgr.connectToAppMgr();
        break;

    case MSG_RECEIVE_BOOT_COMPLETE:
	    LOGV("handleMessage MSG_RECEIVE_BOOT_COMPLETE");
	    mDiagInputMgr.TimerStart();
	    break;	
/*!! appmgr Inheritance CGA end-------------------------------------------------*/ 
/*!! audio Inheritance CGA start-------------------------------------------------*/		
    case MSG_CONNECT_TO_AUDIOMGR:
        LOGV("handleMessage MSG_CONNECT_TO_AUDIOMGR");
        mDiagInputMgr.connectToAudioMgr();
        break;
		
	case MSG_RECEIVE_FROM_AUDIO:
		LOGV("handleMessage MSG_RECEIVE_FROM_AUDIO sigID : %x", msg->arg1);
		mDiagInputMgr.messagefromAudio((uint16_t)msg->arg1, buf);
		break;
/*!! audio Inheritance CGA end-------------------------------------------------*/
/*!! hmi Inheritance CGA start-------------------------------------------------*/		
    case MSG_CONNECT_TO_HMIMGR:
        LOGV("handleMessage MSG_CONNECT_TO_HMIMGR");
        mDiagInputMgr.connectToHmiMgr();
        break;
		
	case MSG_RECEIVE_FROM_HMI:
		LOGV("handleMessage MSG_RECEIVE_FROM_HMI sigID : %x", msg->arg1);
		mDiagInputMgr.messagefromHmi((uint16_t)msg->arg1, buf);
		break;
/*!! hmi Inheritance CGA end-------------------------------------------------*/
/*!! vif Inheritance CGA start-------------------------------------------------*/		
    case MSG_CONNECT_TO_VIFMGR:
        LOGV("handleMessage MSG_CONNECT_TO_VIFMGR");
        mDiagInputMgr.connectToVifMgr();
        break;
		
	case MSG_RECEIVE_FROM_VIF:
		LOGV("handleMessage MSG_RECEIVE_FROM_VIF sigID : %x", msg->arg1);
		mDiagInputMgr.messagefromVif((uint16_t)msg->arg1, buf);
		break;
/*!! vif Inheritance CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_? case CGA start-------------------------------------------------*/
    //case MSG_DID_WORK_FOR_DEFINE_1:
        //LOGV("handleMessage MSG_DID_WORK_FOR_DEFINE_1");
        //mDiagInputMgr.DID_WORK_FOR_DEFINE(msg->arg1);
        //break;
    case DID_WORK_FOR_DEFINE_0:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_0");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_0(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_1:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_1");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_1(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_2:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_2");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_2(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_3:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_3");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_3(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_4:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_4");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_4(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_5:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_5");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_5(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_6:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_6");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_6(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_7:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_7");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_7(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_8:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_8");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_8(msg->arg1);
        break;
    case DID_WORK_FOR_DEFINE_9:
        LOGV("handleMessage DID_WORK_FOR_DEFINE_9");
        mDiagInputMgr.DID_WORK_FOR_DEFINE_9(msg->arg1);
        break;
/*!! DID_WORK_FOR_DEFINE_? case CGA end-------------------------------------------------*/

    case MSG_DIAGDATA_QUEUE:
        LOGV("handleMessage MSG_DIAGDATA_QUEUE");
        // mDiagInputMgr.sendingData(buf);
        break;
		
	case MSG_RECEIVE_WATCH_DOG:
		LOGV("handleMessage MSG_RECEIVE_WATCH_DOG");
		HeartBeat();
		break;
	
	case MSG_BOOT_COMPLETE_DID_START:
		LOGV("handleMessage MSG_BOOT_COMPLETE_DID_START");
		// mDiagInputMgr.TimerStop();
		break;

    default:
        LOGV("Wrong Message received.[%8x]", what);
        break;
    }
}

/*!! appmgr Inheritance CGA start-------------------------------------------------*/ 
void DiagInputManager::connectToAppMgr(void)
{
    mAppManager = android::interface_cast<IApplicationManagerService>
                (android::defaultServiceManager()->getService(android::String16(APPLICATION_SRV_NAME)));

    if (mAppManager != NULL) {
        if (mAppManager->getBootCompleted()) {
            mMyHandler->sendMessageDelayed(mMyHandler->obtainMessage(DiagHandler::MSG_RECEIVE_BOOT_COMPLETE),
                                            DiagHandler::TIME_SEND_RETRY_DELAY_MS);
        } else {
            android::IInterface::asBinder(mAppManager)->linkToDeath(mServiceDeathRecipient);
            LOGV("connectToAppMgr: AppManager OK...");
        }
    } else {
        LOGE("appManager is NULL retry in 500ms");
        if(mMyHandler != NULL) {
            mMyHandler->sendMessageDelayed(mMyHandler->obtainMessage(DiagHandler::MSG_CONNECT_TO_APPMGR),
                                            DiagHandler::TIME_SEND_RETRY_DELAY_MS);
        } else {
            LOGE("connectToAppMgr: mMyHandler is null");
        }
        return;
    }

    if (mSystemPostReceiver == NULL) {
        mSystemPostReceiver = new DiagSystemPostReceiver(mMyHandler);
    } else {
        LOGV("Already mSystemPostReceiver was created.");
    }

    mAppManager->registerSystemPostReceiver(mSystemPostReceiver, SYS_POST_BOOT_COMPLETED);
}
/*!! appmgr Inheritance CGA end-------------------------------------------------*/ 
/*!! audio Inheritance CGA start-------------------------------------------------*/ 
void DiagInputManager::connectToAudioMgr(void)
{
    mAudioManager = android::interface_cast<IaudioManagerService>
                (android::defaultServiceManager()->getService(android::String16(audio_SRV_NAME)));

    if (mAudioManager != NULL) {
                android::IInterface::asBinder(mAudioManager)->linkToDeath(mServiceDeathRecipient);
                LOGV("connectToAudioMgr: audioManagerService OK...");
    } else {
        LOGE("mAudioManager is NULL retry in 500ms");
        if (mMyHandler != NULL) {
            mMyHandler->sendMessageDelayed(mMyHandler->obtainMessage(DiagHandler::MSG_CONNECT_TO_audioMGR),
                                            DiagHandler::TIME_SEND_RETRY_DELAY_MS);
        } else {
            LOGE("connectToAudioMgr: mMyHandler is null");
        }
        return;
    }

    if (mAudioReceiver == NULL) {
        mAudioReceiver = new AudioReceiver(mMyHandler);
    } else {
        LOGV("Already mAudioReceiver was created.");
    }

// CGA_VARIANT:DiagInputManager.cpp:DiagInputManager:connectToAudioMgr(void):variant START

    /*  
     * Write your own code 
    */

// CGA_VARIANT:DiagInputManager.cpp:DiagInputManager:connectToAudioMgr(void):variant END
}
/*!! audio Inheritance CGA end-------------------------------------------------*/
/*!! hmi Inheritance CGA start-------------------------------------------------*/ 
void DiagInputManager::connectToHmiMgr(void)
{
    mHmiManager = android::interface_cast<IhmiManagerService>
                (android::defaultServiceManager()->getService(android::String16(hmi_SRV_NAME)));

    if (mHmiManager != NULL) {
                android::IInterface::asBinder(mHmiManager)->linkToDeath(mServiceDeathRecipient);
                LOGV("connectToHmiMgr: hmiManagerService OK...");
    } else {
        LOGE("mHmiManager is NULL retry in 500ms");
        if (mMyHandler != NULL) {
            mMyHandler->sendMessageDelayed(mMyHandler->obtainMessage(DiagHandler::MSG_CONNECT_TO_hmiMGR),
                                            DiagHandler::TIME_SEND_RETRY_DELAY_MS);
        } else {
            LOGE("connectToHmiMgr: mMyHandler is null");
        }
        return;
    }

    if (mHmiReceiver == NULL) {
        mHmiReceiver = new HmiReceiver(mMyHandler);
    } else {
        LOGV("Already mHmiReceiver was created.");
    }

// CGA_VARIANT:DiagInputManager.cpp:DiagInputManager:connectToHmiMgr(void):variant START

    /*  
     * Write your own code 
    */

// CGA_VARIANT:DiagInputManager.cpp:DiagInputManager:connectToHmiMgr(void):variant END
}
/*!! hmi Inheritance CGA end-------------------------------------------------*/
/*!! vif Inheritance CGA start-------------------------------------------------*/ 
void DiagInputManager::connectToVifMgr(void)
{
    mVifManager = android::interface_cast<IvifManagerService>
                (android::defaultServiceManager()->getService(android::String16(vif_SRV_NAME)));

    if (mVifManager != NULL) {
                android::IInterface::asBinder(mVifManager)->linkToDeath(mServiceDeathRecipient);
                LOGV("connectToVifMgr: vifManagerService OK...");
    } else {
        LOGE("mVifManager is NULL retry in 500ms");
        if (mMyHandler != NULL) {
            mMyHandler->sendMessageDelayed(mMyHandler->obtainMessage(DiagHandler::MSG_CONNECT_TO_vifMGR),
                                            DiagHandler::TIME_SEND_RETRY_DELAY_MS);
        } else {
            LOGE("connectToVifMgr: mMyHandler is null");
        }
        return;
    }

    if (mVifReceiver == NULL) {
        mVifReceiver = new VifReceiver(mMyHandler);
    } else {
        LOGV("Already mVifReceiver was created.");
    }

// CGA_VARIANT:DiagInputManager.cpp:DiagInputManager:connectToVifMgr(void):variant START

    /*  
     * Write your own code 
    */

// CGA_VARIANT:DiagInputManager.cpp:DiagInputManager:connectToVifMgr(void):variant END
}
/*!! vif Inheritance CGA end-------------------------------------------------*/

void DiagInputManager::onServiceBinderDied(const android::wp<android::IBinder>& who)
{
/*!! appmgr Inheritance CGA start-------------------------------------------------*/ 		
    if (mAppManager != NULL && android::IInterface::asBinder(mAppManager) == who) {
        LOGW("killed AppManager!!");
        connectToAppMgr();
/*!! appmgr Inheritance CGA end-------------------------------------------------*/ 
/*!! audio Inheritance CGA start-------------------------------------------------*/ 
    } else if (mAudioManager != NULL && android::IInterface::asBinder(mAudioManager) == who) {
        LOGW("killed mAudioManager!!");
        connectToAudioMgr();
/*!! audio Inheritance CGA end-------------------------------------------------*/
/*!! hmi Inheritance CGA start-------------------------------------------------*/ 
    } else if (mHmiManager != NULL && android::IInterface::asBinder(mHmiManager) == who) {
        LOGW("killed mHmiManager!!");
        connectToHmiMgr();
/*!! hmi Inheritance CGA end-------------------------------------------------*/
/*!! vif Inheritance CGA start-------------------------------------------------*/ 
    } else if (mVifManager != NULL && android::IInterface::asBinder(mVifManager) == who) {
        LOGW("killed mVifManager!!");
        connectToVifMgr();
/*!! vif Inheritance CGA end-------------------------------------------------*/
    } else {
        LOGE("nothing!! onServiceBinderDied()");
    }
}

error_t DiagInputManager::messagefrom_SOMEIP(uint16_t sigId, android::sp<Buffer>& buf)
{
	error_t result = E_OK;
	return result;
}

error_t DiagInputManager::messagefromVIF(uint16_t sigId, android::sp<Buffer>& buf)
{
	error_t result = E_OK;
	return result;
}

/*!! DID_WORK_FOR_DEFINE_0 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_0(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_0 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_1 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_1(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_1 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_2 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_2(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_2 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_3 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_3(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_3 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_4 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_4(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_4 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_5 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_5(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_5 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_6 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_6(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_6 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_7 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_7(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_7 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_8 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_8(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_8 func body CGA end-------------------------------------------------*/

/*!! DID_WORK_FOR_DEFINE_9 func body CGA start-------------------------------------------------*/
error_t DiagInputManager::DID_WORK_FOR_DEFINE_9(android::sp<DiagData>& mdiagData)
{
	error_t result = E_OK;
	return result;
}
/*!! DID_WORK_FOR_DEFINE_9 func body CGA end-------------------------------------------------*/


error_t DiagInputManager::sendToQueue(android::sp<DiagData>& mdiagData)
{
    LOGV("sendToQueue start");

    uint8_t payload[DIAGDATA_BUFSIZE] = {0,};
    android::sp<Buffer> buf = new Buffer();
    did_data_transfer sendData;
    uint16_t sigID = SIGNAL_INTERNAL_DIAG_BASE;
    android::sp<sl::Message> message = mMyHandler->obtainMessage(DiagHandler::MSG_DIAGDATA_QUEUE, sigID);
    error_t result = E_OK;

    LOGV("sendToQueue start 1");

    sendData.DID = mdiagData->getDid();
    sendData.length = mdiagData->getLen();
    sendData.attribute = mdiagData->getAtt();
    sendData.pkgIndex = 0U;

    if (mdiagData->getData() == NULL) {
        LOGE("sendToQueue data buffer is null");
        result = E_BUFFER_EMPTY;
        return result;
    }

    if (mdiagData->getLen() <= 0) {
        LOGE("sendToQueue data size is 0");
        result = E_INPUT_EMPTY;
        return result;
    }
    LOGV("sendToQueue start 2");

    memcpy(payload, &sendData, DIAGDATA_HEAD);
    LOGV("sendToQueue start 3");

    memcpy((payload + DIAGDATA_HEAD), mdiagData->getData(), (int)sendData.length);
    LOGV("sendToQueue start 4");

    buf->setTo(payload, (int)sendData.length + DIAGDATA_HEAD);

    message->buffer.setTo(buf->data(), buf->size());
    message->sendToTarget();
    result = E_OK;

    return result;
}

void DiagInputManager::sendingData(android::sp<Buffer>& buf)
{
    LOGV("sendingData");

    did_data_transfer sendData;
    uint8_t payload[DIAGDATA_BUFSIZE] = {0U,};
    android::sp<DiagData> newDiagData = new DiagData();

    if (buf == NULL || buf->data() == NULL) {
        LOGE("sendingData: buf or buf data is null");
        return;
    }

    memcpy(&sendData, buf->data(), DIAGDATA_HEAD);
    memcpy(payload,(buf->data() + DIAGDATA_HEAD),  (int)sendData.length);

    if (sendData.length <= 0U) {
        LOGV("sendingData buf data size is 0.");
    return;
    }

    newDiagData->setData(sendData.DID, sendData.length, sendData.attribute, payload);
    newDiagData->toString();

    sendToVif(newDiagData);
}

error_t DiagInputManager::sendTo_SOMEIP(android::sp<DiagData>& mdiagData) {
	error_t result = E_OK;
	return result;
}

error_t DiagInputManager::DiagOutputManager(uint16_t id,uint16_t size, uint8_t mAtt_type, uint8_t* data )
{
    error_t result = E_OK;

    android::sp<DiagData> newDiagData = new DiagData();

    if (newDiagData == NULL) {
        result = E_NOT_ENOUGH_MEMORY;
    } else {
        newDiagData->setData((uint16_t)id, (uint16_t)size, (uint8_t)mAtt_type, (uint8_t*)data);
        result = sendToQueue(newDiagData);
    }

    return result;
}

error_t DiagInputManager::transferDatabyVIF(android::sp<DiagData>& mdiagData)
{
    error_t result = E_OK;
    LOGV("transferDatabyVIF  ");

    result = mDiagMgrService->queryReceiverById((int32_t)(mdiagData->getDid()), mdiagData);

    return result;
}


/* sample code send to vif
error_t DiagInputManager::sendToVif(android::sp<DiagData>& mdiagData)
{
    uint16_t sigID = SIGNAL_INTERNAL_DIAG_BASE;
    uint8_t SendingvifData[DIAGDATA_BUFSIZE] = {0U,};

    int index = 0;
    int out_each_packet_ptr = 0;
    int dataSize = (int)mdiagData->getLen();
    int SendingvifDataSize = (int)mdiagData->getLen() + DIAGDATA_HEAD;

    LOGV("DiagInputManager::sendToVif  start");

    if (mVifManager == NULL) {
        LOGE("sendToVif: mVifManager is null");
        return E_ERROR;
    }

    did_data_transfer sendData;
    sendData.DID = mdiagData->getDid();
    sendData.length = mdiagData->getLen();
    sendData.attribute = mdiagData->getAtt();
    sendData.pkgIndex = 0U;

    mdiagData->toString();

    memset(SendingvifData, 0x00, SendingvifDataSize);

    memcpy(SendingvifData, mdiagData->getData(), dataSize);

    if (SendingvifDataSize > PAYLOAD_SIZE) {

        LOGV("DiagInputManager::sendToVif Multi-Packet Start");	
        for (index ; index < ((dataSize-1)/PURE_PAYLOAD_SIZE)+1; index++) {
            payload_data payload;
            memset(&payload, 0x00, sizeof(payload));

            payload.group_id = 0U;
            payload.data_id = sigID;
            payload.data_len = (uint16_t)SendingvifDataSize;
            sendData.pkgIndex = index;

            memcpy(payload.data, (uint8_t*)&sendData, DIAGDATA_HEAD);
            memcpy((payload.data+DIAGDATA_HEAD), SendingvifData+out_each_packet_ptr , PURE_PAYLOAD_SIZE);

            android::sp<vifCANContainer> vifContainer = new vifCANContainer();
            vifContainer->setData((uint32_t)(CH_RPMSG << 24) | sigID, (uint16_t)sizeof(payload), (uint8_t *)(&payload));
            mVifManager->send_dataToVif(1U, vifContainer);

            out_each_packet_ptr += PURE_PAYLOAD_SIZE;
        }
        LOGV("DiagInputManager::sendToVif Multi-Packet End");

    }else {
        LOGV("DiagInputManager::sendToVif Single-Packet Start");

        payload_data payload;
        memset(&payload, 0x00, sizeof(payload));

        payload.group_id = 0;
        payload.data_id = sigID;
        payload.data_len = (uint16_t)SendingvifDataSize;

        memcpy(payload.data, (uint8_t*)&sendData, DIAGDATA_HEAD);
        memcpy((payload.data+DIAGDATA_HEAD), SendingvifData, PURE_PAYLOAD_SIZE);

        android::sp<vifCANContainer> vifContainer = new vifCANContainer();
        vifContainer->setData((uint32_t)(CH_RPMSG << 24) | sigID, (uint16_t)sizeof(payload), (uint8_t *)(&payload));
        mVifManager->send_dataToVif(1U, vifContainer);
        LOGV("DiagInputManager::sendToVif Single-Packet End");
    }

    return E_OK;
}
*/


/*  
-------------------------------------------------------

DTC같이 이벤트가 발생했을 시, processdatamanager 에게 보내는 예제 
void DiagInputManager::antennaStatus (void)
{
    android::sp<DiagData> newDiagData = new DiagData();
    android::sp<mindroid::String> newdata = new mindroid::String("1");

    LOGV("antennaStatus");

    newDiagData->setData(0U, 0U, 0U, (uint8_t*)newdata->c_str());
    (void)mProcessDataManager->toServicefromIO(DiagService::ANTENNA_UPDATE, newDiagData);
}
*/

/* Sample Code message from vif module
------------------------------------------
error_t DiagInputManager::messagefromVIF(uint16_t sigId, android::sp<Buffer>& buf)
{
    error_t result = E_OK;
    did_data_transfer sendData;
    uint8_t payload[DIAGDATA_BUFSIZE] = {0U,};
    android::sp<DiagData> newDiagData = new DiagData();

    LOGV("messagefromVIF Start");

    if (buf == NULL || buf->data() == NULL) {
        LOGE("messagefromVIF data buffer is null");
        result = E_BUFFER_EMPTY;
        return result;
    }

    memcpy(&sendData,(buf->data()+VIF_HEAD),DIAGDATA_HEAD);

        if (!(sendData.attribute == DID_READ_REQUEST || sendData.attribute == DID_READ_RESPONSE ||
            sendData.attribute == DID_WRITE_REQUEST || sendData.attribute == DID_WRITE_RESPONSE)) {
            LOGV("messagefromVIF Data Wrong attribute");
            memset(longDataBuffer, 0U, DIAGDATA_BUFSIZE);
            result = E_INVALID_PARAM;
            return result;
        }

    if (sendData.length < 0) {
        result = E_INPUT_EMPTY;
        return result;
    }

    if(sendData.length > (PAYLOAD_SIZE - DIAGDATA_HEAD)){
        longData.DID = sendData.DID;
        longData.length = sendData.length;
        longData.attribute = sendData.attribute;
        longData.pkgIndex = sendData.pkgIndex;

        if((sendData.length-1) / (PAYLOAD_SIZE - DIAGDATA_HEAD) == sendData.pkgIndex) {
            //end of long data
            if (longDataBuffer != NULL) {
                memcpy(longDataBuffer + each_packet_ptr, (buf->data() + VIF_HEAD + DIAGDATA_HEAD), (longData.length - each_packet_ptr));
            }

            newDiagData->setData(longData.DID, longData.length, longData.attribute, longDataBuffer);
            newDiagData->toString("long Data");

            if (mDiagMgrService->isApplicationExecuted(longData.DID)) {
                LOGV("messagefromVIF ApplicationExecuted");
                result = transferDatabyVIF(newDiagData);
            }
            else {
                LOGV("messagefromVIF REQUEST_PROCESS_DATA_ID");
                result = mProcessDataManager->toServicefromIO(DiagService::REQUEST_PROCESS_DATA_ID, newDiagData);
            }

            longData.DID = 0U;
            longData.length = 0U;
            longData.attribute = 0U;
            longData.pkgIndex = 0U;
            each_packet_ptr = 0;
            memset(longDataBuffer, 0U, DIAGDATA_BUFSIZE);
            LOGV("messagefromVIF Multi-Packet End");
            return result;
        } else {
            //continue long data
            if (longDataBuffer != NULL) {
                memcpy(longDataBuffer + each_packet_ptr, (buf->data() + VIF_HEAD + DIAGDATA_HEAD), PURE_PAYLOAD_SIZE);
            }
            each_packet_ptr += PURE_PAYLOAD_SIZE;
            return result;
        }
    } else {
        //short data
        if (payload != NULL) {
            LOGV("messagefromVIF Single-Packet start");
            memcpy(payload, (buf->data() + VIF_HEAD + DIAGDATA_HEAD),  (int)sendData.length);
        }
        newDiagData->setData(sendData.DID, sendData.length, sendData.attribute, payload);
        newDiagData->toString();

        if (mDiagMgrService->isApplicationExecuted(sendData.DID)) {
            LOGV("messagefromVIF ApplicationExecuted");
            result = transferDatabyVIF(newDiagData);
        } else {
            LOGV("messagefromVIF REQUEST_PROCESS_DATA_ID");
            result = mProcessDataManager->toServicefromIO(DiagService::REQUEST_PROCESS_DATA_ID, newDiagData);
        }
        longData.DID = 0U;
        longData.length = 0U;
        longData.attribute = 0U;
        longData.pkgIndex = 0U;
        each_packet_ptr = 0;
        memset(longDataBuffer, 0U, DIAGDATA_BUFSIZE);
        LOGV("messagefromVIF  Single-Packet End");
        return result;
    }

    return result;
}
*/

