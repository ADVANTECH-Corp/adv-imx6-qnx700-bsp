
#include <audio_driver.h>
#include <string.h>
#include <hw/i2c.h>
#include "pca954x.h"

#define PCA954X_MAX_NCHANS 8

enum pca_type {
	pca_9545,
};

struct i2c_adapter {
	int i2c_fd;
};

struct pca954x {
	enum pca_type type;
	struct i2c_adapter *virt_adaps[PCA954X_MAX_NCHANS];
	unsigned char last_chan;		/* last register value */
};

struct chip_desc {
	unsigned char nchans;
	unsigned char enable;	/* used for muxes only */
	enum muxtype {
		pca954x_ismux = 0,
		pca954x_isswi
	} muxtype;
};

/* Provide specs for the PCA954x types we know about */
static const struct chip_desc chips[] = {
	[pca_9545] = {
		.nchans = 4,
		.muxtype = pca954x_isswi,
	},
};

static struct pca954x *data = NULL;

static int i2c_read_byte (struct i2c_adapter *adapr)
{ 
    struct {
        i2c_recv_t      hdr;
        //uint16_t        val;
        uint8_t        val;
    } msgval;
    int rbytes;

    msgval.hdr.slave.addr = 0x70;
    msgval.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msgval.hdr.len        = 1;
    msgval.hdr.stop       = 1;

    if (devctl(adapr->i2c_fd, DCMD_I2C_RECV, &msgval, 
                        sizeof(msgval), &rbytes)) {
        printf("%s(),L:%d: i2c_fd = %d, i2c_read_byte DATA failed\n", __FUNCTION__, __LINE__, adapr->i2c_fd);
	return (-1);
    }

    //printf("%s(), L:%d: Read = 0x%02x, rbytes = %d\n", __FUNCTION__, __LINE__, msgval.val, rbytes);

    return (msgval.val);
}

static int i2c_write_byte(struct i2c_adapter *adapr, uint8_t val)
{
    struct {
        i2c_send_t      hdr;
        uint8_t        val;
    } msg;

    msg.hdr.slave.addr = 0x70;
    msg.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msg.hdr.len        = 1;
    msg.hdr.stop       = 1;
    msg.val            = val;

    if (devctl (adapr->i2c_fd, DCMD_I2C_SEND, &msg, sizeof(msg), NULL))
    {
        //printf("%s(), L:%d: i2c_fd = %d, i2c_write_byte failed\n", __FUNCTION__, __LINE__, adapr->i2c_fd);
	return -1;
    }
    else
    {
        //printf("%s(), L:%d: Wrote = 0x%02x\n", __FUNCTION__, __LINE__, val);
    }

    return (EOK);
}

/* Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
   for this as they will try to lock adapter a second time */
static int pca954x_reg_write(struct i2c_adapter *adap,  unsigned char val)
{
	int ret = -ENODEV;
	ret = i2c_write_byte(adap, val);
	return ret;
}

static int pca954x_reg_read(struct i2c_adapter *adap)
{
	int ret = -ENODEV;
	ret = i2c_read_byte(adap);
	return ret;
}

static int pca954x_select_chan(struct i2c_adapter *adap, unsigned int chan)
{
	//const struct chip_desc *chip = &chips[data->type];
	unsigned char regval, read_regval;
	int ret = 0;

	/* we make switches look like muxes, not sure how to be smarter */
#if 0
	if (chip->muxtype == pca954x_ismux) {
		regval = chan | chip->enable;
		printf("%s(), L:%d revgal = chan | chip->enable = %d\n", __FUNCTION__, __LINE__, regval);
	}
	else 
#endif
	{
		regval = 1 << chan;
		//printf("%s(), L:%d revgal = 1<< chan = %d\n", __FUNCTION__, __LINE__, regval);
	}

	/* Only select the channel if its different from the last channel */
	if (data->last_chan != regval) {
		ret = pca954x_reg_write(adap, regval);
		data->last_chan = regval;
		//printf("%s(), L:%d\n", __FUNCTION__, __LINE__);
	}


	read_regval = pca954x_reg_read(adap) & 0xf; 
	if (read_regval == regval)
		printf("%s() change to chan %d success\n", __FUNCTION__, read_regval);
	else
		printf("%s() change to chan %d %d fail (regval != read_regval)\n", __FUNCTION__, regval, read_regval);

	return ret;
}

#if 0
static int pca954x_deselect_mux(struct i2c_adapter *adap, unsigned int chan)
{
	/* Deselect active channel */
	data->last_chan = 0;
	return pca954x_reg_write(adap, data->last_chan);
}
#endif

/*
 * I2C init/probing/exit functions
 */
static int pca954x_probe(struct i2c_adapter *adap)
{
	//int num, force;
	int ret = -ENODEV;

	data = (struct pca954x*) malloc(sizeof(struct pca954x));
	if (!data) {
		ret = -ENOMEM;
		goto err;
	}

	/* Write the mux register at addr to verify
	 * that the mux is in fact present. This also
	 * initializes the mux to disconnected state.
	 */
	if (i2c_write_byte(adap, 0) < 0) {
		printf("%s(), L:%d, probe failed\n", __FUNCTION__, __LINE__);
		goto exit_free;
	}

	data->last_chan = 0;		   /* force the first selection */

	return 0;

exit_free:
	free(data);
err:
	return ret;
}

int pca954x_init(int i2c_fd, int channel)
{
	int ret;
	struct i2c_adapter pca954x_adap;
	pca954x_adap.i2c_fd = i2c_fd;
	ret = pca954x_probe(&pca954x_adap);

	if (ret == 0) 
		ret = pca954x_select_chan(&pca954x_adap, channel);

	return ret;
}

void pca954x_exit(void)
{
	if (data) 
		free(data);
}

