#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <hi_comm_aio.h>
#include <mpi_ai.h>
#include <mpi_vb.h>
#include <mpi_sys.h>
#include <hi_comm_sys.h>
#include <hi_comm_vb.h>

#define AUDIO_IN_DEV 0

HI_S32 SYS_Init(VB_CONF_S* pstVbConf)
{
   	HI_S32 s32ret;
	MPP_SYS_CONF_S stSysConf = {0};

	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();

	if(NULL == pstVbConf)
	{
		printf("stVbConf is null,it is invaild\n");
		return HI_FAILURE;
	}

	s32ret = HI_MPI_VB_SetConf(pstVbConf);
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_VB_SetConf failed\n");
		return HI_FAILURE;
	}

	s32ret = HI_MPI_VB_Init();
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_VB_Init failed\n");
		return HI_FAILURE;
	}

	s32ret = HI_MPI_SYS_SetConf(&stSysConf);
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_SYS_SetConf failed\n");
		return HI_FAILURE;
	}

	s32ret = HI_MPI_SYS_Init();
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_SYS_Init failed\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS; 
}

HI_S32 AUDIO_StartAi(AUDIO_DEV aiDevId, AI_CHN aiChn, AIO_ATTR_S *pstAioAttr)
{
	HI_S32 s32ret;
	s32ret = HI_MPI_AI_SetPubAttr(aiDevId, pstAioAttr);
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_AI_SetPubAttr %d failed with 0x%x\n",aiDevId, s32ret );
		return s32ret;
	}

	s32ret = HI_MPI_AI_Enable(aiDevId);
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_AI_Enable %d failed whith 0x%x\n",aiDevId, s32ret );
		return s32ret;
	}

	s32ret = HI_MPI_AI_EnableChn(aiDevId, aiChn);
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_AI_EnableChn %d:%d failed with 0x%x\n",aiDevId,aiChn,s32ret);
		return s32ret;
	}

	return HI_SUCCESS;
}

HI_S32 AUDIO_StopAi(AUDIO_DEV aiDevId, AI_CHN aiChn)
{
	HI_S32 s32ret;

	s32ret = HI_MPI_AI_DisableChn(aiDevId,aiChn);
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_AI_DisableChn %d:%d failed with 0x%x\n", aiDevId, aiChn, s32ret);
		return s32ret;
	}

	s32ret = HI_MPI_AI_Disable(aiDevId);
	if(HI_SUCCESS != s32ret)
	{
		printf("HI_MPI_AI_Disable %d failed with 0x%x\n", aiDevId,s32ret);
		return s32ret;
	}

	return HI_SUCCESS;
}

int main()
{
    HI_S32 s32ret;
    AUDIO_DEV aiDev = AUDIO_IN_DEV;
    AI_CHN aiChn = 0;
    FILE *pfd = NULL;
    AIO_ATTR_S stAioAttr;

    stAioAttr.enSamplerate	= AUDIO_SAMPLE_RATE_16000;
	stAioAttr.enBitwidth	= 1;
	stAioAttr.enWorkmode	= AIO_MODE_I2S_MASTER;
	stAioAttr.enSoundmode	= AUDIO_SOUND_MODE_MONO;
	stAioAttr.u32EXFlag		= 0;
	stAioAttr.u32FrmNum		= 30;   //AI DEV need to save at least 30 frm
	stAioAttr.u32PtNumPerFrm= 160;  //must >=160
	stAioAttr.u32ChnCnt 	= 1;
	stAioAttr.u32ClkSel		= 0;

    //step0: init vb 
	VB_CONF_S stVbConf;
	memset(&stVbConf, 0, sizeof(VB_CONF_S));
	s32ret = SYS_Init(&stVbConf);
	if(HI_SUCCESS != s32ret)
	{
		printf("SYS_Init failed\n");
		return HI_FAILURE;
	}

    //setp1: start Ai
    s32ret = AUDIO_StartAi(aiDev, aiChn, &stAioAttr);
    if(HI_SUCCESS != s32ret)
    {
        printf("AUDIO_StartAi failed\n");
        return HI_FAILURE;
    }
    

    //step2: get Frame
    AUDIO_FRAME_S stAudioFrm;
    s32ret = HI_MPI_AI_GetFrame(aiDev, aiChn, &stAudioFrm, NULL, -1);
    if(HI_SUCCESS != s32ret)
    {
        printf("Audio_GetFrame failed\n");
        return HI_FAILURE;
    }

    pfd = fopen("/home/znr1995/mycode/audio","w+");
    fwrite(stAudioFrm.pVirAddr[0], 1, stAudioFrm.u32Len, pfd);
    fclose(pfd); 

    s32ret = HI_MPI_AI_ReleaseFrame(aiDev, aiChn, &stAudioFrm, NULL);
    if(HI_SUCCESS != s32ret)
    {
        printf("AUDIO_ReleaseFrame failed\n");
        return HI_FAILURE;
    }

    //step3: stop Ai
    s32ret = AUDIO_StopAi(aiDev, aiChn);
	if(HI_SUCCESS != s32ret)
	{
		printf("AUDIO_StopAi failed\n");
		return HI_FAILURE;
	}

    printf("The End!\n");
    return 0;

}
