/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <string.h>
#include "types.h"
#include "coreinit.h"
#include "pad.h"
#include "../global.h"

static unsigned int getButtonsDown(unsigned int padscore_handle, unsigned int vpad_handle);

#define BUS_SPEED                       248625000
#define SECS_TO_TICKS(sec)              (((unsigned long long)(sec)) * (BUS_SPEED/4))
#define MILLISECS_TO_TICKS(msec)        (SECS_TO_TICKS(msec) / 1000)
#define MICROSECS_TO_TICKS(usec)        (SECS_TO_TICKS(usec) / 1000000)

#define usleep(usecs)                   OSSleepTicks(MICROSECS_TO_TICKS(usecs))
#define sleep(secs)                     OSSleepTicks(SECS_TO_TICKS(secs))

#define FORCE_SYSMENU (VPAD_BUTTON_ZL | VPAD_BUTTON_ZR | VPAD_BUTTON_L | VPAD_BUTTON_R)
#define FORCE_LDN (VPAD_BUTTON_A)
#define FORCE_HBL (VPAD_BUTTON_B)
#define FORCE_FTP (VPAD_BUTTON_X)
#define FORCE_APP (VPAD_BUTTON_Y)
#define SD_HBL_PATH "/vol/external01/wiiu/apps/homebrew_launcher/homebrew_launcher.elf"
#define SD_LDN_PATH "/vol/external01/wiiu/apps/loadiine_gx2/loadiine_gx2.elf"
#define SD_FTP_PATH "/vol/external01/wiiu/apps/ftpiiu/ftpiiu.elf"
#define SD_APP_PATH "/vol/external01/wiiu/apps/appstore/hbas.elf"
#define SD_MOCHA_PATH "/vol/external01/wiiu/apps/mocha/mocha.elf"

static const char *verChar = "CBHC Mod v1.5u1 (FIX94)";

#define DEFAULT_DISABLED 0
#define DEFAULT_SYSMENU 1
#define DEFAULT_MOCHA 3
#define DEFAULT_CFW_IMG 4
#define DEFAULT_HBL 5
#define DEFAULT_LDN 6
#define DEFAULT_MAX 7

static const char *defOpts[DEFAULT_MAX] = {
	"DEFAULT_DISABLED",
	"DEFAULT_SYSMENU",
	"DEFAULT_MOCHA",
	"DEFAULT_CFW_IMG",
	"DEFAULT_HBL",
	"DEFAULT_LDN",
	"DEFAULT_FTP",
};

static const char *bootOpts[DEFAULT_MAX] = {
	"Disabled",
	"System Menu",
	"Mocha CFW",
	"IosuHax CFW",
	"Homebrew Launcher",
	"Loadiine",
	"FTPiiU",
};

#define OSScreenEnable(enable) OSScreenEnableEx(0, enable); OSScreenEnableEx(1, enable);
#define OSScreenClearBuffer(tmp) OSScreenClearBufferEx(0, tmp); OSScreenClearBufferEx(1, tmp);
#define OSScreenPutFont(x, y, buf) OSScreenPutFontEx(0, x, y, buf); OSScreenPutFontEx(1, x, y, buf);
#define OSScreenFlipBuffers() OSScreenFlipBuffersEx(0); OSScreenFlipBuffersEx(1);

uint32_t __main(void)
{
	unsigned int coreinit_handle;
	OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle);

	void (*DCStoreRange)(const void *addr, uint32_t length);
	OSDynLoad_FindExport(coreinit_handle, 0, "DCStoreRange", &DCStoreRange);

	void (*OSExitThread)(int);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSExitThread", &OSExitThread);

	unsigned int sysapp_handle;
	OSDynLoad_Acquire("sysapp.rpl", &sysapp_handle);

	unsigned long long(*_SYSGetSystemApplicationTitleId)(int sysApp);
	OSDynLoad_FindExport(sysapp_handle,0,"_SYSGetSystemApplicationTitleId",&_SYSGetSystemApplicationTitleId);
	unsigned long long sysmenu = _SYSGetSystemApplicationTitleId(0);

	unsigned int vpad_handle;
	OSDynLoad_Acquire("vpad.rpl", &vpad_handle);

	int(*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
	OSDynLoad_FindExport(vpad_handle, 0, "VPADRead", &VPADRead);

	int vpadError = -1;
	VPADData vpad;
	VPADRead(0, &vpad, 1, &vpadError);
	if(vpadError == 0)
	{
		if(((vpad.btns_d|vpad.btns_h) & FORCE_SYSMENU) == FORCE_SYSMENU)
		{
			// iosuhax-less menu launch backup code
			int(*_SYSLaunchTitleWithStdArgsInNoSplash)(unsigned long long tid, void *ptr);
			OSDynLoad_FindExport(sysapp_handle,0,"_SYSLaunchTitleWithStdArgsInNoSplash",&_SYSLaunchTitleWithStdArgsInNoSplash);
			_SYSLaunchTitleWithStdArgsInNoSplash(sysmenu, 0);
			OSExitThread(0);
			return 0;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_HBL) == FORCE_HBL)
		{
			// original hbl loader payload
			strcpy((void*)0xF5E70000,SD_HBL_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_LDN) == FORCE_LDN)
		{
			// ldn loader payload
			strcpy((void*)0xF5E70000,SD_LDN_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_FTP) == FORCE_FTP)
		{
			// ftp loader payload
			strcpy((void*)0xF5E70000,SD_FTP_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_APP) == FORCE_APP)
		{
			// app loader payload
			strcpy((void*)0xF5E70000,SD_APP_PATH);
			return 0x01800000;
		}
	}

	unsigned int *pMEMAllocFromDefaultHeapEx;
	unsigned int *pMEMFreeToDefaultHeap;
	OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeapEx", &pMEMAllocFromDefaultHeapEx);
	OSDynLoad_FindExport(coreinit_handle, 1, "MEMFreeToDefaultHeap", &pMEMFreeToDefaultHeap);
	void*(*MEMAllocFromDefaultHeapEx)(int size, int align) = (void*)(*pMEMAllocFromDefaultHeapEx);
	void(*MEMFreeToDefaultHeap)(void *ptr) = (void*)(*pMEMFreeToDefaultHeap);

	void *pClient = MEMAllocFromDefaultHeapEx(0x1700,4);
	void *pCmd = MEMAllocFromDefaultHeapEx(0xA80,4);

	int(*FSInit)(void);
	void(*FSShutdown)(void);
	int(*FSAddClient)(void *pClient, int errHandling);
	int(*FSDelClient)(void *pClient);
	void(*FSInitCmdBlock)(void *pCmd);

	int(*FSWriteFile)(void *pClient, void *pCmd, const void *buffer, int size, int count, int fd, int flag, int errHandling);
	int(*FSCloseFile)(void *pClient, void *pCmd, int fd, int errHandling);

	OSDynLoad_FindExport(coreinit_handle, 0, "FSInit", &FSInit);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSShutdown", &FSShutdown);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSInitCmdBlock", &FSInitCmdBlock);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSAddClient", &FSAddClient);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSDelClient", &FSDelClient);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSWriteFile", &FSWriteFile);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSCloseFile", &FSCloseFile);

	unsigned int act_handle;
	OSDynLoad_Acquire("nn_act.rpl", &act_handle);

	unsigned int save_handle;
	OSDynLoad_Acquire("nn_save.rpl", &save_handle);
	void(*SAVEInit)(void);
	void(*SAVEShutdown)(void);
	void(*SAVEInitSaveDir)(unsigned char user);
	int(*SAVEOpenFile)(void *pClient, void *pCmd, unsigned char user, const char *path, const char *mode, int *fd, int errHandling);
	int(*SAVEFlushQuota)(void *pClient, void *pCmd, unsigned char user, int errHandling);
	void(*SAVERename)(void *pClient, void *pCmd, unsigned char user, const char *oldpath, const char *newpath, int errHandling);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInit",&SAVEInit);
	OSDynLoad_FindExport(save_handle, 0, "SAVEShutdown",&SAVEShutdown);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInitSaveDir",&SAVEInitSaveDir);
	OSDynLoad_FindExport(save_handle, 0, "SAVEOpenFile", &SAVEOpenFile);
	OSDynLoad_FindExport(save_handle, 0, "SAVEFlushQuota", &SAVEFlushQuota);
	OSDynLoad_FindExport(save_handle, 0, "SAVERename", &SAVERename);

	void(*nn_act_initialize)(void);
	unsigned char(*nn_act_getslotno)(void);
	unsigned char(*nn_act_getdefaultaccount)(void);
	void(*nn_act_finalize)(void);
	OSDynLoad_FindExport(act_handle, 0, "Initialize__Q2_2nn3actFv", &nn_act_initialize);
	OSDynLoad_FindExport(act_handle, 0, "GetSlotNo__Q2_2nn3actFv", &nn_act_getslotno);
	OSDynLoad_FindExport(act_handle, 0, "GetDefaultAccount__Q2_2nn3actFv", &nn_act_getdefaultaccount);
	OSDynLoad_FindExport(act_handle, 0, "Finalize__Q2_2nn3actFv", &nn_act_finalize);

	FSInit();
	nn_act_initialize();
	unsigned char slot = nn_act_getslotno();
	unsigned char defaultSlot = nn_act_getdefaultaccount();
	SAVEInit();
	SAVEInitSaveDir(slot);
	FSAddClient(pClient, -1);
	FSInitCmdBlock(pCmd);

	int autoboot = -1;
	int iFd = -1;
	int i;
	for(i = 0; i < DEFAULT_MAX; i++)
	{
		SAVEOpenFile(pClient, pCmd, slot, defOpts[i], "r", &iFd, -1);
		if (iFd >= 0)
		{
			autoboot = i;
			FSCloseFile(pClient, pCmd, iFd, -1);
			break;
		}
	}
	if(autoboot < 0)
	{
		autoboot = DEFAULT_DISABLED;
		SAVEOpenFile(pClient, pCmd, slot, defOpts[DEFAULT_DISABLED], "w", &iFd, -1);
		if (iFd >= 0)
			FSCloseFile(pClient, pCmd, iFd, -1);
	}
	int launchmode = (autoboot > 0) ? autoboot : LAUNCH_SYSMENU;
	int cur_autoboot = autoboot;

	void(*OSScreenInit)();
	void(*OSScreenEnableEx)(unsigned int bufferNum, int enable);
	unsigned int(*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
	unsigned int(*OSScreenSetBufferEx)(unsigned int bufferNum, void * addr);

	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenInit", &OSScreenInit);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenEnableEx", &OSScreenEnableEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenSetBufferEx", &OSScreenSetBufferEx);

	unsigned int(*OSScreenClearBufferEx)(unsigned int bufferNum, unsigned int temp);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenClearBufferEx", &OSScreenClearBufferEx);

	unsigned int(*OSScreenPutFontEx)(unsigned int bufferNum, unsigned int posX, unsigned int posY, const char * buffer);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenPutFontEx", &OSScreenPutFontEx);

	unsigned int(*OSScreenFlipBuffersEx)(unsigned int bufferNum);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenFlipBuffersEx", &OSScreenFlipBuffersEx);

	OSScreenInit();
	int screen_buf0_size = OSScreenGetBufferSizeEx(0);
	OSScreenSetBufferEx(0, (void*)(0xF4000000));
	OSScreenSetBufferEx(1, (void*)(0xF4000000 + screen_buf0_size));
	OSScreenEnable(1);

	unsigned long long(*OSGetTitleID)();
	OSDynLoad_FindExport(coreinit_handle, 0, "OSGetTitleID", &OSGetTitleID);
	unsigned int dsvcid = (unsigned int)(OSGetTitleID(0) & 0xFFFFFFFF);

	char verInfStr[64];
	__os_snprintf(verInfStr,64,"%s (DS Title %08X)", verChar, dsvcid);

	unsigned int padscore_handle;
	OSDynLoad_Acquire("padscore.rpl", &padscore_handle);

	void(*OSSleepTicks)(unsigned long long ticks);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSSleepTicks",&OSSleepTicks);

	void(*WPADEnableURCC)(int enable);
	void(*KPADSetConnectCallback)(int chan, void *ptr);
	void*(*WPADSetSyncDeviceCallback)(void *ptr);
	void(*KPADShutdown)(void);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADEnableURCC", &WPADEnableURCC);
	OSDynLoad_FindExport(padscore_handle, 0, "KPADSetConnectCallback", &KPADSetConnectCallback);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADSetSyncDeviceCallback", &WPADSetSyncDeviceCallback);
	OSDynLoad_FindExport(padscore_handle, 0, "KPADShutdown",&KPADShutdown);
	//easly allows us callback without execute permission on other cores
	char(*WPADGetSpeakerVolume)(void);
	void(*WPADSetSpeakerVolume)(char);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADGetSpeakerVolume", &WPADGetSpeakerVolume);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADSetSpeakerVolume", &WPADSetSpeakerVolume);
	//enable wiiu pro controller connection
	WPADEnableURCC(1);
	//hachihachi instantly disconnects wiimotes normally
	KPADSetConnectCallback(0,NULL);
	KPADSetConnectCallback(1,NULL);
	KPADSetConnectCallback(2,NULL);
	KPADSetConnectCallback(3,NULL);
	char oriVol = WPADGetSpeakerVolume();
	//WPAD_SYNC_EVT=0 is button pressed
	WPADSetSpeakerVolume(1);
	WPADSetSyncDeviceCallback(WPADSetSpeakerVolume);

	if(autoboot == DEFAULT_DISABLED)
		goto cbhc_menu;

	OSScreenClearBuffer(0);
	OSScreenPutFont(0, 0, verInfStr);
	OSScreenPutFont(0, 1, "Autobooting...");
	OSScreenFlipBuffers();

	//garbage read
	getButtonsDown(padscore_handle, vpad_handle);
	//see if menu is requested
	int loadMenu = 0;
	int waitCnt = 40;
	while(waitCnt--)
	{
		unsigned int btnDown = getButtonsDown(padscore_handle, vpad_handle);

		if((btnDown & VPAD_BUTTON_HOME) || WPADGetSpeakerVolume() == 0)
		{
			WPADSetSpeakerVolume(1);
			loadMenu = 1;
			break;
		}
		usleep(50000);
	}

	if(loadMenu == 0)
		goto doIOSUexploit;

	OSScreenClearBuffer(0);
	OSScreenPutFont(0, 0, verInfStr);
	OSScreenPutFont(0, 1, "Entering Menu...");
	OSScreenFlipBuffers();
	waitCnt = 30;
	while(waitCnt--)
	{
		getButtonsDown(padscore_handle, vpad_handle);
		usleep(50000);
	}

cbhc_menu:	;
	int redraw = 1;
	int PosX = 1;
	int ListMax = 8;
	int clickT = 0;
	while(1)
	{
		unsigned int btnDown = getButtonsDown(padscore_handle, vpad_handle);

		if(WPADGetSpeakerVolume() == 0)
		{
			if(clickT == 0)
				clickT = 8;
			else
			{
				btnDown |= VPAD_BUTTON_A;
				clickT = 0;
			}
			WPADSetSpeakerVolume(1);
		}
		else if(clickT)
		{
			clickT--;
			if(clickT == 0)
				btnDown |= VPAD_BUTTON_DOWN;
		}

		if( btnDown & VPAD_BUTTON_DOWN )
		{
			if(PosX+1 == ListMax)
				PosX = 0;
			else
				PosX++;
			redraw = 1;
		}

		if( btnDown & VPAD_BUTTON_UP )
		{
			if( PosX <= 1 )
				PosX = (ListMax-1);
			else
				PosX--;
			redraw = 1;
		}

		if( btnDown & VPAD_BUTTON_A )
		{
			if(PosX == 7)
			{
				cur_autoboot++;
				if(cur_autoboot == DEFAULT_MAX)
					cur_autoboot = DEFAULT_DISABLED;
				redraw = 1;
			}
			else
			{
				launchmode = PosX;
				break;
			}
		}

		if(redraw)
		{
			OSScreenClearBuffer(0);
			OSScreenPutFont(0, 0, verInfStr);
			char printStr[64];
			__os_snprintf(printStr,64,"%c Boot System Menu", 1 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 2, printStr);
			__os_snprintf(printStr,64,"%c Boot Mocha CFW", 2 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 3, printStr);
			__os_snprintf(printStr,64,"%c Boot IosuHax CFW", 3 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 4, printStr);
			__os_snprintf(printStr,64,"%c Boot Homebrew Launcher", 4 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 5, printStr);
			__os_snprintf(printStr,64,"%c Boot Loadiine", 5 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 6, printStr);
			__os_snprintf(printStr,64,"%c Boot FTPiiU", 6 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 7, printStr);
			__os_snprintf(printStr,64,"%c Autoboot: %s", 7 == PosX ? '>' : ' ', bootOpts[cur_autoboot]);
			OSScreenPutFont(0, 8, printStr);

			OSScreenFlipBuffers();
			redraw = 0;
		}
		usleep(50000);
	}
	OSScreenClearBuffer(0);
	OSScreenFlipBuffers();
	usleep(50000);

doIOSUexploit:
	WPADSetSpeakerVolume(oriVol);
	KPADShutdown();

	if(cur_autoboot != autoboot)
		SAVERename(pClient, pCmd, slot, defOpts[autoboot], defOpts[cur_autoboot], -1);

	SAVEFlushQuota(pClient, pCmd, slot, -1);
	FSDelClient(pClient);
	SAVEShutdown();
	nn_act_finalize();
	FSShutdown();

	MEMFreeToDefaultHeap(pClient);
	MEMFreeToDefaultHeap(pCmd);

	OSScreenClearBuffer(0);
	OSScreenFlipBuffers();

	int (*OSForceFullRelaunch)(void);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSForceFullRelaunch", &OSForceFullRelaunch);

	//for patched menu launch
	void (*SYSLaunchMenu)(void);
	OSDynLoad_FindExport(sysapp_handle, 0,"SYSLaunchMenu", &SYSLaunchMenu);
	void(*_SYSLaunchMenuWithCheckingAccount)(unsigned char slot);
	OSDynLoad_FindExport(sysapp_handle,0,"_SYSLaunchMenuWithCheckingAccount",&_SYSLaunchMenuWithCheckingAccount);

	//store path to sd fw.img for arm_kernel
	if(launchmode == LAUNCH_CFW_IMG)
	{
		strcpy((void*)0xF5E70000,"/vol/sdcard");
		DCStoreRange((void*)0xF5E70000,0x100);
	}

	//do iosu patches
	void (*patch_iosu)(unsigned int coreinit_handle, unsigned int sysapp_handle, int launchmode, int from_cbhc) = (void*)0x01804000;
	patch_iosu(coreinit_handle, sysapp_handle, launchmode, 1);

	if(launchmode == LAUNCH_HBL)
	{
		strcpy((void*)0xF5E70000,SD_HBL_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_MOCHA)
	{
		strcpy((void*)0xF5E70000,SD_MOCHA_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_LDN)
	{
		strcpy((void*)0xF5E70000,SD_LDN_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_FTP)
	{
		strcpy((void*)0xF5E70000,SD_FTP_PATH);
		return 0x01800000;
	}

	//sysmenu or cfw
	if(launchmode == LAUNCH_CFW_IMG)
	{
		OSForceFullRelaunch();
		SYSLaunchMenu();
	}
	else
	{
		if(defaultSlot) //normal menu boot
			SYSLaunchMenu();
		else //show mii select
			_SYSLaunchMenuWithCheckingAccount(slot);
	}
	OSExitThread(0);
	return 0;
}

/* General Input Code */

static unsigned int wpadToVpad(unsigned int buttons)
{
	unsigned int conv_buttons = 0;

	if(buttons & WPAD_BUTTON_LEFT)
		conv_buttons |= VPAD_BUTTON_LEFT;

	if(buttons & WPAD_BUTTON_RIGHT)
		conv_buttons |= VPAD_BUTTON_RIGHT;

	if(buttons & WPAD_BUTTON_DOWN)
		conv_buttons |= VPAD_BUTTON_DOWN;

	if(buttons & WPAD_BUTTON_UP)
		conv_buttons |= VPAD_BUTTON_UP;

	if(buttons & WPAD_BUTTON_PLUS)
		conv_buttons |= VPAD_BUTTON_PLUS;

	if(buttons & WPAD_BUTTON_2)
		conv_buttons |= VPAD_BUTTON_X;

	if(buttons & WPAD_BUTTON_1)
		conv_buttons |= VPAD_BUTTON_Y;

	if(buttons & WPAD_BUTTON_B)
		conv_buttons |= VPAD_BUTTON_B;

	if(buttons & WPAD_BUTTON_A)
		conv_buttons |= VPAD_BUTTON_A;

	if(buttons & WPAD_BUTTON_MINUS)
		conv_buttons |= VPAD_BUTTON_MINUS;

	if(buttons & WPAD_BUTTON_HOME)
		conv_buttons |= VPAD_BUTTON_HOME;

	return conv_buttons;
}

static unsigned int wpadClassicToVpad(unsigned int buttons)
{
	unsigned int conv_buttons = 0;

	if(buttons & WPAD_CLASSIC_BUTTON_LEFT)
		conv_buttons |= VPAD_BUTTON_LEFT;

	if(buttons & WPAD_CLASSIC_BUTTON_RIGHT)
		conv_buttons |= VPAD_BUTTON_RIGHT;

	if(buttons & WPAD_CLASSIC_BUTTON_DOWN)
		conv_buttons |= VPAD_BUTTON_DOWN;

	if(buttons & WPAD_CLASSIC_BUTTON_UP)
		conv_buttons |= VPAD_BUTTON_UP;

	if(buttons & WPAD_CLASSIC_BUTTON_PLUS)
		conv_buttons |= VPAD_BUTTON_PLUS;

	if(buttons & WPAD_CLASSIC_BUTTON_X)
		conv_buttons |= VPAD_BUTTON_X;

	if(buttons & WPAD_CLASSIC_BUTTON_Y)
		conv_buttons |= VPAD_BUTTON_Y;

	if(buttons & WPAD_CLASSIC_BUTTON_B)
		conv_buttons |= VPAD_BUTTON_B;

	if(buttons & WPAD_CLASSIC_BUTTON_A)
		conv_buttons |= VPAD_BUTTON_A;

	if(buttons & WPAD_CLASSIC_BUTTON_MINUS)
		conv_buttons |= VPAD_BUTTON_MINUS;

	if(buttons & WPAD_CLASSIC_BUTTON_HOME)
		conv_buttons |= VPAD_BUTTON_HOME;

	if(buttons & WPAD_CLASSIC_BUTTON_ZR)
		conv_buttons |= VPAD_BUTTON_ZR;

	if(buttons & WPAD_CLASSIC_BUTTON_ZL)
		conv_buttons |= VPAD_BUTTON_ZL;

	if(buttons & WPAD_CLASSIC_BUTTON_R)
		conv_buttons |= VPAD_BUTTON_R;

	if(buttons & WPAD_CLASSIC_BUTTON_L)
		conv_buttons |= VPAD_BUTTON_L;

	return conv_buttons;
}

static unsigned int getButtonsDown(unsigned int padscore_handle, unsigned int vpad_handle)
{
	int(*WPADProbe)(int chan, int * pad_type);
	int(*KPADRead)(int chan, void * data, int size);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADProbe",&WPADProbe);
	OSDynLoad_FindExport(padscore_handle, 0, "KPADRead",&KPADRead);

	unsigned int btnDown = 0;

	int(*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
	OSDynLoad_FindExport(vpad_handle, 0, "VPADRead", &VPADRead);

	int vpadError = -1;
	VPADData vpad;
	VPADRead(0, &vpad, 1, &vpadError);
	if(vpadError == 0)
		btnDown |= vpad.btns_d;

	int i;
	for(i = 0; i < 4; i++)
	{
		int controller_type;
		if(WPADProbe(i, &controller_type) != 0)
			continue;
		KPADData kpadData;
		KPADRead(i, &kpadData, 1);
		if(kpadData.device_type <= 1)
			btnDown |= wpadToVpad(kpadData.btns_d);
		else
			btnDown |= wpadClassicToVpad(kpadData.classic.btns_d);
	}

	return btnDown;
}
