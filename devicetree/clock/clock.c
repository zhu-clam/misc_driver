#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/io.h>

#include "clock.h"

#define log_info(fmt, args...) printk(KERN_INFO fmt, ##args)

#define SYS_REF_CLK    24000000
#define TSM_REF_CLK    27000000
#define I2S_MCLK_EXTERNAL_PIN_CLK  0

#define REF_DIV(param) 		((param) & 0x3f)
#define MULTI_DIV(param)    ((param) >> 8) & 0xfff
#define DIV1(param)  		(((param) >> 20) & 0x7)
#define DIV2(param)			(((param) >> 24) & 0x7)
#define CAL_CLK_DIV(cfg, start_bit, bitnum)  (((cfg >> start_bit) & ((1 << (bitnum)) -1)) + 1)

#define MOD_NAME "driver/clk"

struct gv_clk{
	void __iomem * base;
	unsigned int clk_ref;
	unsigned int tsm_clk_ref;
	unsigned int i2s_extern_pin_clk;
};

static struct gv_clk clk_ctrl;

static struct proc_dir_entry * clk_proc = NULL;

static inline void clk_write(struct gv_clk * pclk, u32 offset, u32 val)
{
	iowrite32(val, pclk->base + offset);
}

static inline u32 clk_read(struct gv_clk * pclk, u32 offset)
{
	return ioread32(pclk->base + offset);
}

static inline unsigned int pll_clk_out(struct gv_clk * pclk, unsigned int offset)
{
	unsigned int div1 = 1, div2 = 1;
	unsigned int ref_div = 1;
	unsigned int multi_div = 1;
	unsigned int pll_param = 1;

	//PLL BYPASSED: 0->normal
	if((clk_read(pclk, offset + 0x4) & 0x1) == 0)
	{
		pll_param = clk_read(pclk, offset);
		ref_div = REF_DIV((pll_param));
		multi_div = MULTI_DIV(pll_param);
		div1 = DIV1(pll_param);
		div2 = DIV2(pll_param);
	}
	else{
		log_info("module offset: 0x%x, pll is not open\n", offset);
	}
	
	if(offset == TSM_PLL_PARAM)
		return (pclk->tsm_clk_ref / ref_div ) * multi_div / (div1 * div2);
	else
		return (pclk->clk_ref / ref_div ) * multi_div / (div1 * div2);
}

static int gvclk_list(struct seq_file *m, void *v)
{
	unsigned int sys_pll_clk;
	unsigned int ck_pll_clk;
	unsigned int ddr_pll_clk;
	unsigned int video0_pll_clk;
	unsigned int video1_pll_clk;
	unsigned int dsp_pll_clk;
	unsigned int tsm_pll_clk;
	unsigned int gmac_pll_clk;
	unsigned int pixel_pll_clk;
	unsigned int audio_pll_clk;
	unsigned int sensor_pll_clk;

	unsigned int clk_cfg;
	unsigned int tmp_clk_out;

	unsigned int sys_aclk_h;
	unsigned int sys_aclk_l;
	unsigned int sys_hclk;
	unsigned int sys_pclk;
	
	struct gv_clk *pclk = (struct gv_clk *)m->private;
	
	log_info("current system input clk: %u\n", pclk->clk_ref);
	sys_pll_clk = pll_clk_out(pclk, SYS_PLL_PARAM);
	ck_pll_clk = pll_clk_out(pclk, CK_PLL_PARAM);
	ddr_pll_clk = pll_clk_out(pclk, DDR_PLL_PARAM);
	video0_pll_clk = pll_clk_out(pclk, VIDEO0_PLL_PARAM);
	video1_pll_clk = pll_clk_out(pclk, VIDEO1_PLL_PARAM);
	dsp_pll_clk = pll_clk_out(pclk, DSP_PLL_PARAM);
	tsm_pll_clk = pll_clk_out(pclk, TSM_PLL_PARAM);
	gmac_pll_clk = pll_clk_out(pclk, GMAC_PLL_PARAM);
	pixel_pll_clk = pll_clk_out(pclk, PIXEL_PLL_PARAM);
	audio_pll_clk = pll_clk_out(pclk, AUDIO_PLL_PARAM);
	sensor_pll_clk = pll_clk_out(pclk, SENSOR_PLL_PARAM);
	
	log_info("*************system pll clk****************\n");
	log_info("sys_pll_clk				%u\n", sys_pll_clk);
	log_info("ck_pll_clk				%u\n", ck_pll_clk);
	log_info("ddr_pll_clk				%u\n", ddr_pll_clk);
	log_info("video0_pll_clk				%u\n", video0_pll_clk);
	log_info("video1_pll_clk				%u\n", video1_pll_clk);
	log_info("dsp_pll_clk				%u\n", dsp_pll_clk);
	log_info("tsm_pll_clk				%u\n", tsm_pll_clk);
	log_info("gmac_pll_clk				%u\n", gmac_pll_clk);
	log_info("pixel_pll_clk				%u\n", pixel_pll_clk);
	log_info("audio_pll_clk				%u\n", audio_pll_clk);
	log_info("sensor_pll_clk				%u\n", sensor_pll_clk);

	log_info("************module clock ouput*************\n");
	clk_cfg = clk_read(pclk, CK860_CLK_CFG);
	log_info("ck860_aclk				%u\n", ck_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 3));
	
	clk_cfg = clk_read(pclk, SYS_CLK_CFG);
	sys_aclk_h = sys_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 3);
	sys_aclk_l = sys_aclk_h / CAL_CLK_DIV(clk_cfg, 4, 3);
	sys_hclk = sys_aclk_l / CAL_CLK_DIV(clk_cfg, 8, 3);
	sys_pclk = sys_aclk_l / CAL_CLK_DIV(clk_cfg, 12, 3);
	log_info("sys_aclk_h				%u\n", sys_aclk_h);
	log_info("sys_aclk_l				%u\n", sys_aclk_l);
	log_info("sys_hclk					%u\n", sys_hclk);
	log_info("sys_pclk					%u\n", sys_pclk);

	clk_cfg = clk_read(pclk, XDMA_CLK_CFG);
	log_info("axi dma aclk				%u\n", sys_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, GMAC_CLK_CFG);
	log_info("gmac					%u\n", gmac_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, SDIO_CLK_CFG);
	log_info("SD CLK CFG:0x%x\n", clk_cfg);
	if(clk_cfg & 0x10000){
		log_info("SD0 CLK					%u\n", gmac_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 6));
		log_info("SD1 CLK					%u\n", gmac_pll_clk / CAL_CLK_DIV(clk_cfg, 8, 6));
	}else{
		log_info("SD0 CLK					%u\n", sys_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 6));
		log_info("SD1 CLK					%u\n", sys_pll_clk / CAL_CLK_DIV(clk_cfg, 8, 6));
	}

	clk_cfg = clk_read(pclk, PCIE_CLK_CFG);
	log_info("PCIE CLK					%u\n", gmac_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, I2S_CLK_CFG);
	if(clk_cfg & 0x10000){
		//external clock
		tmp_clk_out = pclk->i2s_extern_pin_clk / CAL_CLK_DIV(clk_cfg, 0, 12);
		log_info("I2S MASTER CLK				%u\n", tmp_clk_out);
		log_info("I2S SLAVE CLK					%u\n",  tmp_clk_out / CAL_CLK_DIV(clk_cfg, 0, 12));
	}else{
		tmp_clk_out = audio_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 12);
		log_info("I2S MASTER CLK				%u\n", tmp_clk_out);
		log_info("I2S SLAVE CLK				%u\n",  tmp_clk_out / CAL_CLK_DIV(clk_cfg, 12, 6));
	}

	clk_cfg = clk_read(pclk, VIN_CLK_CFG);
	if(clk_cfg & 0x10)
		log_info("VIN CLK					%u\n",  pixel_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));
	else
		log_info("VIN CLK					%u\n",  video0_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, VDEC_CLK_CFG);
	if(clk_cfg & 0x10)
		log_info("VDEC CLK					%u\n",  pixel_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));
	else
		log_info("VDEC CLK					%u\n",  video0_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, VENC_CLK_CFG);
	log_info("VENC CLK					%u\n", video0_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, JPEG_CLK_CFG);
	log_info("JPEG CLK					%u\n", video0_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, ISP_CLK_CFG);
	if(clk_cfg & 0x10)
		log_info("ISP CLK					%u\n",  gmac_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));
	else
		log_info("ISP CLK					%u\n",  video0_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, GPU_CLK_CFG);
	if(clk_cfg & 0x10)
		log_info("GPU CLK					%u\n",  pixel_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));
	else
		log_info("GPU CLK					%u\n",  video1_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, VOUT_CLK_CFG);
	if(clk_cfg & 0x10)
		log_info("VOUT CLK					%u\n",  pixel_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));
	else
		log_info("VOUT CLK					%u\n",  video0_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, AVS2_CLK_CFG);
	if(clk_cfg & 0x1000)
		log_info("CK610 CLK				%u\n",  pixel_pll_clk / CAL_CLK_DIV(clk_cfg, 8, 4));
	else
		log_info("CK610 CLK				%u\n",  video1_pll_clk / CAL_CLK_DIV(clk_cfg, 8, 4));
	log_info("AVSP CLK					%u\n",  video1_pll_clk / CAL_CLK_DIV(clk_cfg, 4, 4));
	log_info("AVS2 CLK 				%u\n",	video1_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, TS_CLK_CFG);
	if(clk_cfg & 0x10000)
		log_info("STC 27M CLK				%u\n",	pixel_pll_clk / CAL_CLK_DIV(clk_cfg, 8, 8));
	else
		log_info("STC 27M CLK				27000000\n");
	tmp_clk_out = tsm_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4);
	log_info("TSM PSI CLK 				%u\n",	tmp_clk_out);
	log_info("TSM PSI 27M CLK 				%u\n",	tmp_clk_out / CAL_CLK_DIV(clk_cfg, 4, 4));
	
	clk_cfg = clk_read(pclk, C5_CLK_CFG);
	if(clk_cfg & 0x10)
		tmp_clk_out = video1_pll_clk;
	else
		tmp_clk_out = dsp_pll_clk;
	log_info("C5 CLK					%u\n",	tmp_clk_out / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, P6_CLK_CFG);
	log_info("P6 CLK					%u\n",	dsp_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, CDVS_CLK_CFG);
	if(clk_cfg & 0x10)
		tmp_clk_out = 0; //uni pll clock output;
	else
		tmp_clk_out = video0_pll_clk;
	tmp_clk_out = tmp_clk_out / CAL_CLK_DIV(clk_cfg, 0, 4);
	log_info("CDVS MASTER CLK 				%u\n",	tmp_clk_out);
	log_info("CDVS SLAVE CLK 				%u\n",	tmp_clk_out / CAL_CLK_DIV(clk_cfg, 8, 4));
	
	clk_cfg = clk_read(pclk, VIP_CLK_CFG);
	switch((clk_cfg >> 4) & 0x3){
		case 0:
			tmp_clk_out = video0_pll_clk;
			break;
		case 1:
			tmp_clk_out = tsm_pll_clk;
			break;
		case 2:
			tmp_clk_out = dsp_pll_clk;
			break;
		case 3:
		default:
			tmp_clk_out = 0;
	}
	log_info("VIP CLK					%u\n",	tmp_clk_out / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, CAMB_CLK_CFG);
	log_info("CAMB CLK					%u\n",	tsm_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));

	clk_cfg = clk_read(pclk, SCI_CLK_CFG);
	log_info("SCI CLK					%u\n",	sys_pclk / CAL_CLK_DIV(clk_cfg, 0, 8));

	clk_cfg = clk_read(pclk, GPIO_DBCLK_CFG);
	log_info("GPIO CLK 				%u\n",	sys_pclk / (clk_cfg + 1));

	clk_cfg = clk_read(pclk, TIMER_CLK_CFG);
	log_info("TIMER CLK				%u\n",	sys_pclk / CAL_CLK_DIV(clk_cfg, 0, 16));
	
	clk_cfg = clk_read(pclk, SENSOR0_CLK_CFG);
	log_info("SENSOR0 CLK				%u\n",	sensor_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 12));

	clk_cfg = clk_read(pclk, SENSOR1_CLK_CFG);
	if(clk_cfg & 0x10000)
		tmp_clk_out = pclk->clk_ref;
	else if(clk_cfg & 0x1000)
		tmp_clk_out = gmac_pll_clk;
	else
		tmp_clk_out = pixel_pll_clk;
	log_info("SENSOR1 CLK				%u\n",	tmp_clk_out / CAL_CLK_DIV(clk_cfg, 0, 12));

	clk_cfg = clk_read(pclk, DISPLAY_CLK_CFG);
	log_info("DISPLAY CLK				%u\n",  pixel_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 8));

	clk_cfg = clk_read(pclk, MIPI_EXT_CLK_CFG);
	if(clk_cfg & 0x10)
		log_info("MIPI CLK					%u\n",  video1_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));
	else
		log_info("MIPI CLK					%u\n",  video0_pll_clk / CAL_CLK_DIV(clk_cfg, 0, 4));
	
	return 0;
}
static int gvclk_proc_open(struct inode *inode, struct file *file)
{
	struct gv_clk *pclk = &clk_ctrl;

	pclk->base = ioremap(CRM_BASE, CRM_SIZE);
	if(pclk->base == NULL){
		printk("Failed to ioreamp CRM base, addr:0x%x, size:0x%x\n", CRM_BASE, CRM_SIZE);
		return -1;
	}
	pclk->clk_ref = SYS_REF_CLK;
	pclk->tsm_clk_ref = TSM_REF_CLK;
	pclk->i2s_extern_pin_clk = I2S_MCLK_EXTERNAL_PIN_CLK;
	
	return single_open(file, gvclk_list, pclk);
}

static int gvclk_proc_release(struct inode *inode, struct file *file)
{
	struct seq_file *m = (struct seq_file *)file->private_data;
	struct gv_clk *pclk = (struct gv_clk *)m->private;

	if(pclk->base != NULL){
		iounmap(pclk->base);
		pclk->base = NULL;
	}
	
	return seq_release(inode, file);
}

static const struct file_operations gvclk_proc_operation = {
	.owner		= THIS_MODULE,
	.open		= gvclk_proc_open,
	.read		= seq_read,
	.llseek 	= seq_lseek,
	.release	= gvclk_proc_release,
};

void gvclock_proc_init(void)
{
	clk_proc = proc_create(MOD_NAME, 0400, NULL, &gvclk_proc_operation);
}

void gvclock_proc_exit(void)
{
	if(clk_proc != NULL){
		proc_remove(clk_proc);
		clk_proc = NULL;
	}
}

static int __init gv_clock_init(void)
{
    gvclock_proc_init();
	
    return 0;
}

static void __exit gv_clock_exit(void)
{   
	gvclock_proc_exit();
}

module_init(gv_clock_init);
module_exit(gv_clock_exit);

MODULE_AUTHOR("yongliang.tu@byavs.com");
MODULE_DESCRIPTION("Byavs Clock Driver");
MODULE_LICENSE("GPL");

