#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <3ds.h>
#include "math.h"
#include "gs.h"
#include "world.h"
#include "player.h"
#include "test_vsh_shbin.h"
#include "terrain_bin.h"
// #include "mdl.h"

DVLB_s* shader;
u32* texData;

player_s player;
world_s world;
worldCluster_s wcl;
gsVbo_s modelVbo;

float angle=0.0f;
float angleZ=0.0f;
float tx, ty, tz;

u32* gpuOut=(u32*)0x1F119400;
u32* gpuDOut=(u32*)0x1F370800;

// topscreen
void doFrame1()
{
	//general setup
		GPU_SetViewport((u32*)osConvertVirtToPhys((u32)gpuDOut),(u32*)osConvertVirtToPhys((u32)gpuOut),0,0,240*2,400);

		GPU_DepthRange(-1.0f, 0.0f);

		GPU_SetFaceCulling(GPU_CULL_BACK_CCW);
		GPU_SetStencilTest(false, GPU_ALWAYS, 0x00);
		GPU_SetDepthTest(true, GPU_GREATER, 0x1F);

	// ?
		GPUCMD_AddSingleParam(0x00010062, 0x00000000); //param always 0x0 according to code
		GPUCMD_AddSingleParam(0x000F0118, 0x00000000);

	//setup shader
		SHDR_UseProgram(shader, 0);

	//?
		GPUCMD_AddSingleParam(0x000F0100, 0x00E40100);
		GPUCMD_AddSingleParam(0x000F0101, 0x01010000);
		GPUCMD_AddSingleParam(0x000F0104, 0x00000010);
	
	//texturing stuff
		GPUCMD_AddSingleParam(0x0002006F, 0x00000100);
		GPUCMD_AddSingleParam(0x000F0080, 0x00011001); //enables/disables texturing
	
	//texenv
		GPU_SetTexEnv(3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000);
		GPU_SetTexEnv(4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000);
		GPU_SetTexEnv(5, GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR), GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
			GPU_TEVOPERANDS(0,0,0), GPU_TEVOPERANDS(0,0,0), GPU_MODULATE, GPU_MODULATE, 0xFFFFFFFF);

	//texturing stuff
		GPU_SetTexture((u32*)osConvertVirtToPhys((u32)texData),256,256,0x6,GPU_RGBA8);

	gsMatrixMode(GS_MODELVIEW);

	//draw first model
		gsPushMatrix();
			gsTranslate(tx, ty, tz);
			gsRotateX(angle);
			gsRotateZ(angleZ);

			// gsVboDraw(&modelVbo);
			drawWorldCluster(&wcl);
		gsPopMatrix();

	// //finalize stuff ?
	// 	GPUCMD_AddSingleParam(0x000F0111, 0x00000001);
	// 	GPUCMD_AddSingleParam(0x000F0110, 0x00000001);
	// 	GPUCMD_AddSingleParam(0x0008025E, 0x00000000);
}

void demoControls(void)
{
	hidScanInput();
	u32 PAD=hidKeysHeld();

	if(PAD&KEY_UP)tx+=0.1f;
	if(PAD&KEY_DOWN)tx-=0.1f;

	if(PAD&KEY_LEFT)ty+=0.1f;
	if(PAD&KEY_RIGHT)ty-=0.1f;

	if(PAD&KEY_R)tz+=0.1f;
	if(PAD&KEY_L)tz-=0.1f;

	if(PAD&KEY_A)angle+=0.1f;
	if(PAD&KEY_Y)angle-=0.1f;

	if(PAD&KEY_X)angleZ+=0.1f;
	if(PAD&KEY_B)angleZ-=0.1f;

	controlsPlayer(&player);
}

extern u32* gxCmdBuf;

int main()
{
	srvInit();	
	aptInit();
	gfxInit();
	hidInit(NULL);
	aptSetupEventHandler();
	
	GPU_Init(NULL);

	gsInit();

	u32 gpuCmdSize=0x40000;
	u32* gpuCmd=(u32*)linearAlloc(gpuCmdSize*4);

	GPU_Reset(gxCmdBuf, gpuCmd, gpuCmdSize);

	texData=(u32*)linearAlloc(terrain_bin_size);
	memcpy(texData, terrain_bin, terrain_bin_size);
	GSPGPU_FlushDataCache(NULL, (u8*)texData, terrain_bin_size);

	// gsVboCreate(&modelVbo, sizeof(mdlData));
	// gsVboAddData(&modelVbo, mdlData, sizeof(mdlData));

	initWorldCluster(&wcl, vect3Di(0,8,0));
	generateWorldClusterData(&wcl);
	generateWorldClusterGeometry(&wcl);

	gsMatrixMode(GS_PROJECTION);
	gsProjectionMatrix(1.3962634f, 240.0f/400.0f, 0.01f, 10.0f);
	
	initPlayer(&player);

	tx=ty=0.0f; tz=-0.1f;
	shader=SHDR_ParseSHBIN((u32*)test_vsh_shbin,test_vsh_shbin_size);

	GX_SetMemoryFill(gxCmdBuf, (u32*)gpuOut, 0x404040FF, (u32*)&gpuOut[0x2EE00], 0x201, (u32*)gpuDOut, 0x00000000, (u32*)&gpuDOut[0x2EE00], 0x201);
	gfxSwapBuffersGpu();

	APP_STATUS status;
	while((status=aptGetStatus())!=APP_EXITING)
	{
		if(status==APP_RUNNING)
		{
			demoControls();
			updatePlayer(&player);

			gfxSwapBuffersGpu();

			GPUCMD_SetBuffer(gpuCmd, gpuCmdSize, 0);
			doFrame1();
			GPUCMD_Finalize();
			gspWaitForVBlank(); //TEMP
			GPUCMD_Run(gxCmdBuf);

			gspWaitForVBlank();

			GX_SetDisplayTransfer(gxCmdBuf, (u32*)gpuOut, 0x019001E0, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), 0x019001E0, 0x01001000);
			svcSleepThread(100000); //TEMP
			GX_SetMemoryFill(gxCmdBuf, (u32*)gpuOut, 0x404040FF, (u32*)&gpuOut[0x2EE00], 0x201, (u32*)gpuDOut, 0x00000000, (u32*)&gpuDOut[0x2EE00], 0x201);
			svcSleepThread(100000); //TEMP
		}
	}

	gsExit();
	hidExit();
	gfxExit();
	aptExit();
	srvExit();
	return 0;
}
