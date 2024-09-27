/**
 * @file
 * @brief Handle Audio Codec 97
 * @author Denis Deryugin <deryugin.denis@gmail.com>
 * @version 0.1
 * @date 2015-12-03
 *
 * Look "AC'97 Component Specification Revision 2.3" for details
 */

/* Register offsets */
#define AC97_RESET		0x00
#define AC97_MASTER		0x02
#define AC97_HEADPHONE		0x04
#define AC97_MASTER_MONO	0x06
#define AC97_MASTER_TONE	0x08
#define AC97_PC_BEEP		0x0A
#define AC97_PHONE		0x0C
#define AC97_MIC		0x0E
#define AC97_LINE		0x10
#define AC97_CD			0x12
#define AC97_VIDEO		0x14
#define AC97_AUX		0x16
#define AC97_PCM		0x18
#define AC97_REC_SEL		0x1A
#define AC97_REC_GAIN		0x1C
#define AC97_REC_GAIN_MIC	0x1E
#define AC97_GENERAL_PURPOSE	0x20
#define AC97_3D_CONTROL		0x22
#define AC97_INT_PAGING		0x24
#define AC97_POWERDOWN		0x26
#define AC97_DAC_RATE		0x2C
#define AC97_ADC_RATE		0x34
#define AC97_DAC_SLOT_MAP	0x6C
#define AC97_ADC_SLOT_MAP	0x6E

enum ac97_slot {
	AC97_FRONT_DAC,
	AC97_MIC_ADC
};

extern int ac97_init(void);
extern int ac97_reset(void);
extern int ac97_set_vol(int vol);
extern int ac97_get_vol(void);
extern int ac97_set_rate(uint16_t rate, enum ac97_slot slot);
extern int ac97_get_rate(enum ac97_slot slot);

extern uint16_t ac97_reg_read(uint16_t reg);
extern void ac97_reg_write(uint16_t reg, uint16_t val);
